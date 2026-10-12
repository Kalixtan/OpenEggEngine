#include "renderer.h"
#include "draw_context.h"
#include "../core/engine_context.h"
#include <GL/glu.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <limits>

// Desktop compatibility-OpenGL backend for the Sound Shapes draw commands.
// The Flowers of Error desktop binary is historical reference material only;
// its 0x00420xxx bodies are not executable code in the Sound Shapes PS3 EBOOT
// and do not establish retail rendering parity (see ENGINE_AUDIT.md).
//
// Texture upload orientation is chosen to match the desktop asset loaders.
// Its equivalence to the PS3 retail RSX texture convention has not been
// established; do not infer it from the unrelated desktop reference backend.
//
// Power-of-two padding and logical-extent UV scaling are implemented for the
// desktop texture path. The PS3 sampler's wrap/filter semantics remain a
// separate unresolved render-config question.



namespace {

constexpr float kPi = 3.1415927f; // legacy desktop-reference constant

// PPC fctiwz truncates toward zero and produces the indefinite signed-int
// result (0x80000000) for NaN, infinity, or overflow. Use the same result
// instead of C++'s undefined out-of-range float-to-int conversion.
int RetailFctiwz(float a_value) {
    if (!std::isfinite(a_value) || a_value >= 2147483648.0f ||
        a_value < -2147483648.0f)
        return INT32_MIN;
    return static_cast<int>(a_value);
}

// FUN_0004e6b8 uses PPC subf + srwi to center the adjusted viewport. The
// subtraction and offset addition wrap as 32-bit integers; srwi is logical.
int RetailViewportOrigin(int a_deviceSize, int a_viewSize, int a_offset) {
    uint32_t difference = static_cast<uint32_t>(a_deviceSize) -
                          static_cast<uint32_t>(a_viewSize);
    uint32_t origin = (difference >> 1) + static_cast<uint32_t>(a_offset);
    int result;
    std::memcpy(&result, &origin, sizeof(result));
    return result;
}

// Legacy Flowers of Error desktop-reference transform (not Sound Shapes
// retail evidence; this helper is currently unused):
//
//   glRotatef(pitch, 1, 0, 0);
//   angleRad = (pitch - 90.0f) * PI / 180.0f;
//   s = sinf(angleRad);  if (s == 0.0f) s = 0.001f;   // guard, 0x3a83126f
//   yOff = fabsf(camZ) * cosf(angleRad) / s;
//   glTranslatef(-camX, yOff - camY, -camZ);
//
// Currently unreferenced. It is retained only as a decoded reference helper;
// the Sound Shapes world-projection path is a separate PS3 command routine.
float CameraYOffset(float a_pitchDeg, float a_camZ) {
    const float angleRad = (a_pitchDeg - 90.0f) * kPi / 180.0f;
    float s = std::sin(angleRad);
    if (s == 0.0f) s = 0.001f;
    return std::fabs(a_camZ) * std::cos(angleRad) / s;
}

// Retail FUN_000548b8 applies descriptor blend state when its flag changes:
// SRC_ALPHA/ONE_MINUS_SRC_ALPHA and FUNC_ADD for RGB and alpha. Flag 1 enables
// blending; flag 0 disables it. Both paths disable logic-op and line/polygon
// smoothing. The flags are recovered from the render-config descriptors.
void ApplyRenderConfigBlendState(bool a_translucent) {
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                        GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (a_translucent) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    glDisable(GL_COLOR_LOGIC_OP);
    glDisable(GL_LINE_SMOOTH);
    glDisable(GL_POLYGON_SMOOTH);
}

// FUN_00302f30 uses vrefp followed by one Newton refinement to build its
// orthographic coefficients. A host divide is used only as the initial
// estimate; PPC's vrefp seed is not reproduced here, so last-bit parity is
// still open. The refinement and subsequent coefficient operations preserve
// the traced float operation boundaries.
float RetailRefinedReciprocal(float a_value) {
    const float estimate = 1.0f / a_value;
    const float residual = std::fma(-a_value, estimate, 1.0f);
    return std::fma(residual, estimate, estimate);
}

void LoadRetailOrthographicMatrix(float a_left, float a_right,
                                  float a_bottom, float a_top,
                                  float a_near, float a_far) {
    const float width = a_right - a_left;
    const float height = a_top - a_bottom;
    const float depth = a_far - a_near;
    const float reciprocalWidth = RetailRefinedReciprocal(width);
    const float reciprocalHeight = RetailRefinedReciprocal(height);
    const float reciprocalDepth = RetailRefinedReciprocal(depth);
    const float horizontalCenter = a_right + a_left;
    const float verticalCenter = a_top + a_bottom;
    const float depthCenter = a_far + a_near;

    // Column-major OpenGL layout of FUN_00302f30's standard ortho matrix.
    // The native path doubles the X/Y/Z reciprocal for scale and uses the
    // signed reciprocal against the summed bounds for translation.
    const GLfloat matrix[16] = {
        reciprocalWidth + reciprocalWidth, 0.0f, 0.0f, 0.0f,
        0.0f, reciprocalHeight + reciprocalHeight, 0.0f, 0.0f,
        0.0f, 0.0f, -(reciprocalDepth + reciprocalDepth), 0.0f,
        std::fma(-horizontalCenter, reciprocalWidth, 0.0f),
        std::fma(-verticalCenter, reciprocalHeight, 0.0f),
        std::fma(-depthCenter, reciprocalDepth, 0.0f), 1.0f};
    glLoadMatrixf(matrix);
}

} // namespace



// Verification: desktop implementation guided by retail shader microcode and
// Cg reflection; frontend OpenGL setup and unresolved uniform inputs are approximate.
bool Renderer::Init(PlatformWindow& a_window, int a_width, int a_height) {
    m_window = &a_window;
    if (!m_window->MakeContextCurrent()) {
        std::fprintf(stderr, "Renderer: frontend could not make the OpenGL context current\n");
        m_window = nullptr;
        return false;
    }

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(glewErr));
        return false;
    }
    glGetError(); // glew often leaves a spurious GL_INVALID_ENUM on init

    // Sound Shapes note textures use grayscale distance fields. The fragment
    // Math and discard-level mapping follow the retail TextureDist microcode.
    // Native descriptor row 1 maps aColor to category 5, uploaded by
    // FUN_000548b8 from drawContext +0x1a10 (current draw color). Native rows
    // 2/3 are aDiscardLevel/aSmoothLevel; Cg paramno is a separate index space.
    // FUN_0003e040's distance-texture payloads now map to those uniforms in
    // desktop state. Nested replay's row-4 aSize write follows a texture-state
    // route and does not override smooth (see TextureDist_Uniform_Analysis.md).
    static const char* kDistanceTextureVS = R"GLSL(#version 120
varying vec2 v_uv;
void main() {
    gl_Position = ftransform();
    v_uv = gl_MultiTexCoord0.st;
}
)GLSL";
    static const char* kDistanceTextureFS = R"GLSL(#version 120
uniform sampler2D u_texture;
uniform float u_discardLevel;
uniform float u_smoothLevel;
uniform vec4 u_color;
varying vec2 v_uv;
void main() {
    float distanceValue = texture2D(u_texture, v_uv).a;
    // Retail TextureDist microcode first kills samples at or below its
    // discard level, then performs a signed divide, clamp, and the cubic
    // smooth-coverage polynomial.  GLSL smoothstep is undefined when the
    // edges are equal or reversed, while the retail instruction sequence
    // executes its signed divide and clamp directly for those inputs.
    if (distanceValue <= u_discardLevel) discard;
    float edgeSpan = u_smoothLevel - u_discardLevel;
    float signedSpan = max(abs(edgeSpan), 1.0e-15) * sign(edgeSpan);
    float t = clamp((distanceValue - u_discardLevel) / signedSpan, 0.0, 1.0);
    float coverage = t * t * (3.0 - 2.0 * t);
    gl_FragColor = vec4(u_color.rgb, u_color.a * coverage);
}
)GLSL";
    m_distanceTextureShaderReady = m_distanceTextureShader.Compile(
        kDistanceTextureVS, kDistanceTextureFS);
    if (!m_distanceTextureShaderReady)
        std::fprintf(stderr, "Renderer: distance-field texture shader unavailable; note edges will use raw texture alpha\n");

    // Verification: retail's selector table maps shader type 0 to
    // TextureTrans (`FUN_00055bb4`). Its static descriptor points directly to
    // the vertex CgBinary at EBOOT VA 0x430198 and fragment CgBinary at VA
    // 0x42ffa8. Reflection names aColor as a vertex uniform and vColor as the
    // varying consumed by the fragment program. Descriptor 0x43bcc0 points
    // to binding table 0x41f8d8, where aColor maps to native category 5;
    // FUN_000548b8 uploads that category from drawContext +0x1a10. The decoded
    // fragment samples the complete texel, discards when texel alpha <= 0,
    // and multiplies the texel by vColor.
    static const char* kTextureTransFS = R"GLSL(#version 120
uniform sampler2D u_texture;
uniform vec4 u_color;
varying vec2 v_uv;
void main() {
    vec4 texel = texture2D(u_texture, v_uv);
    if (texel.a <= 0.0) discard;
    gl_FragColor = texel * u_color;
}
)GLSL";
    m_textureTransShaderReady = m_textureTransShader.Compile(
        kDistanceTextureVS, kTextureTransFS);
    if (!m_textureTransShaderReady)
        std::fprintf(stderr, "Renderer: TextureTrans shader unavailable; textured shapes use generic texture modulation\n");

    // Verification: TextureTransFast and TextureTransFastNB share the
    // CgBinary fragment at EBOOT VA 0x430528 (profile 0x1b5c), which
    // decompiles to one fetch from aTexture at vTexCord. It has no
    // vColor/aColor parameter, so the sample ignores current draw color.
    // Fast enables translucent blending; FastNB's native flag is zero and
    // disables blending. Alpha/depth parity remains open.
    static const char* kTextureTransFastFS = R"GLSL(#version 120
uniform sampler2D u_texture;
varying vec2 v_uv;
void main() {
    gl_FragColor = texture2D(u_texture, v_uv);
}
)GLSL";
    m_textureTransFastShaderReady = m_textureTransFastShader.Compile(
        kDistanceTextureVS, kTextureTransFastFS);
    if (!m_textureTransFastShaderReady)
        std::fprintf(stderr, "Renderer: TextureTransFast shader unavailable; textured shapes use generic texture modulation\n");

    // FontHelper.gm's atlas is a grayscale distance field stored in the
    // alpha channel by Texture::LoadFromFile (single-channel sources become
    // white RGB + source coverage alpha). Glyph rendering uses the same
    // smooth coverage operation as the note distance-texture path.
    m_glyphMaskShaderReady = m_glyphMaskShader.Compile(
        kDistanceTextureVS, kDistanceTextureFS);
    if (!m_glyphMaskShaderReady)
        std::fprintf(stderr, "Renderer: glyph distance shader unavailable; text uses raw atlas coverage\n");

    // Retail Palette4bOverlayTrans samples the base and attached animation
    // through their generated palettes, then computes
    // back.rgb + front.a * (front.rgb - back.rgb). The desktop animation
    // textures already contain those palette lookup results, so this shader
    // preserves the retail sampling and compositing operation.
static const char* kPaletteOverlayVS = R"GLSL(#version 120
varying vec2 v_backUV;
varying vec2 v_frontUV;
varying float v_drawAlpha;
void main() {
    gl_Position = ftransform();
    v_backUV = gl_MultiTexCoord0.st;
    v_frontUV = gl_MultiTexCoord1.st;
    v_drawAlpha = gl_Color.a;
}
)GLSL";
    static const char* kPaletteOverlayFS = R"GLSL(#version 120
uniform sampler2D u_backTexture;
uniform sampler2D u_frontTexture;
varying vec2 v_backUV;
varying vec2 v_frontUV;
varying float v_drawAlpha;
void main() {
    vec4 back = texture2D(u_backTexture, v_backUV);
    vec4 front = texture2D(u_frontTexture, v_frontUV);
    if (back.a == 0.0 && front.a == 0.0) discard;
    gl_FragColor = vec4(back.rgb + front.a * (front.rgb - back.rgb),
                        back.a * v_drawAlpha);
}
)GLSL";
    m_paletteOverlayShaderReady = m_paletteOverlayShader.Compile(
        kPaletteOverlayVS, kPaletteOverlayFS);
    if (!m_paletteOverlayShaderReady)
        std::fprintf(stderr, "Renderer: paired animation shader unavailable\n");

    // FireGroundShader2 RSX microcode computes aPosition.xy + aData.xy*aAmp,
    // then transforms that point at aZ and forwards aColour. The desktop
    // vertex shader mirrors that operation. aData's retail attribute-8 source
    // is unresolved; see FIRE_GROUND_SHADER_AUDIT.md before treating this as
    // a complete port.
    static const char* kFireGroundVS = R"GLSL(#version 120
uniform float u_amp;
uniform float u_z;
varying vec4 v_color;
void main() {
    vec2 displaced = gl_Vertex.xy + gl_MultiTexCoord0.xy * u_amp;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(displaced, u_z, 1.0);
    v_color = gl_Color;
}
)GLSL";
    static const char* kFireGroundFS = R"GLSL(#version 120
varying vec4 v_color;
void main() { gl_FragColor = v_color; }
)GLSL";
    m_fireGroundShaderReady = m_fireGroundShader.Compile(kFireGroundVS, kFireGroundFS);
    if (!m_fireGroundShaderReady)
        std::fprintf(stderr, "Renderer: FireGroundShader2 desktop shader unavailable; danger fill will use undisplaced geometry\n");

    m_rawWidth = a_width;
    m_rawHeight = a_height;
    m_width = a_width;
    m_height = a_height;
    UpdateLetterboxBars();

    // Desktop render defaults. Sound Shapes is a PS3 title and does not
    // import OpenGL, so this is not a literal retail call sequence.
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    // Retail's zero-argument Video.setLineWidth default is 2.0
    // (DAT_000774dc); FUN_00052550 stores the selected value at +0x8210.
    SetLineWidth(kDefaultLineWidth);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // Retail's device init clears colour AND depth once, glClear(0x4100).
    // Per-frame clears are colour-only (see BeginFrame).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Retail dispatcher FUN_00044d4c uses comparison-table slot 2 for
    // enabled depth and slot 7 for disabled depth. DAT_00431684 maps them
    // to 0x203 (LEQUAL) and 0x207 (ALWAYS). Same-depth sprite layers need
    // equality: GL_LESS discarded every layer after the first Goomba head.
    glDepthFunc(GL_LEQUAL);

    // Fixed-function texturing modulates the sampled texel by the current
    // glColor4f, which is how retail tints its textured quads. Note retail
    // imports no glTexEnvi at all -- it relies on GL_MODULATE being the
    // spec default for GL_TEXTURE_ENV_MODE. Stated explicitly here rather
    // than relied on implicitly; the value is identical either way.

    return true;
}

void Renderer::Resize(int a_width, int a_height) {
    if (a_width <= 0 || a_height <= 0) return;
    m_rawWidth = a_width;
    m_rawHeight = a_height;
    UpdateLetterboxBars();
    if (m_window) m_window->Resize(a_width, a_height);
}

bool Renderer::SetFullscreen(bool a_fullscreen) {
    if (!m_window || !m_window->SetFullscreen(a_fullscreen)) return false;
    m_isFullscreen = m_window->IsFullscreen();
    int width = 0, height = 0;
    m_window->GetSize(width, height);
    OnWindowResized(width, height);
    return true;
}

void Renderer::OnWindowResized(int a_width, int a_height) {
    if (a_width <= 0 || a_height <= 0) return;
    m_rawWidth = a_width;
    m_rawHeight = a_height;
    UpdateLetterboxBars();
}

// Verification: bytecode-matched to FUN_0004e6b8. Retail compares the
// physical aspect passed by FUN_000574a0 with device +0x81bc, rounds the
// adjusted width or height using +0.5 then fctiwz, stores +0x81a4/+0x81a8,
// and centers the viewport origin in +0x8178/+0x817c. The frontend supplies the
// desktop physical dimensions; OpenGL's bottom-left viewport origin is the
// platform adaptation.
void Renderer::UpdateLetterboxBars() {
    if (m_rawWidth <= 0 || m_rawHeight <= 0) return;
    const VideoState& video = GetEngineContext().video;
    const float targetAspect = video.aspectRatio;
    const float rawAspect = (float)m_rawWidth / (float)m_rawHeight;

    int logicalW, logicalH;
    if (rawAspect > targetAspect) {
        logicalH = m_rawHeight;
        logicalW = RetailFctiwz(m_rawHeight * targetAspect + 0.5f);
    } else {
        logicalW = m_rawWidth;
        logicalH = RetailFctiwz(m_rawWidth / targetAspect + 0.5f);
    }

    m_width = logicalW;
    m_height = logicalH;
    m_barX = RetailViewportOrigin(m_rawWidth, logicalW, video.viewportOffsetX);
    m_barY = RetailViewportOrigin(m_rawHeight, logicalH, video.viewportOffsetY);
}

// Verification: black regions correspond to the adjusted retail viewport
// rectangle; the GL quad submission is a desktop adaptation. FUN_0004e6b8
// uses a floored half-difference for the origin and retains the independently
// rounded viewport size, so odd leftover pixels belong to the far edge.
void Renderer::DrawLetterboxBars() {
    const int rightBar = m_rawWidth - (m_barX + m_width);
    const int topBar = m_rawHeight - (m_barY + m_height);
    if (m_barX == 0 && rightBar == 0 && m_barY == 0 && topBar == 0) return;

    glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT);
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, m_rawWidth, m_rawHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)(m_rawWidth - 1), 0.0, (double)(m_rawHeight - 1));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.0f, 0.0f, 0.0f);

    const float w = (float)m_rawWidth;
    const float h = (float)m_rawHeight;
    glBegin(GL_QUADS);
    if (m_barY > 0 || topBar > 0) {
        // bottom bar
        glVertex2f(0.0f, 0.0f);  glVertex2f(w, 0.0f);  glVertex2f(w, (float)m_barY); glVertex2f(0.0f, (float)m_barY);
        // top bar
        glVertex2f(0.0f, (float)(m_barY + m_height)); glVertex2f(w, (float)(m_barY + m_height)); glVertex2f(w, h); glVertex2f(0.0f, h);
    }
    if (m_barX > 0 || rightBar > 0) {
        // left bar
        glVertex2f(0.0f, 0.0f);  glVertex2f((float)m_barX, 0.0f); glVertex2f((float)m_barX, h); glVertex2f(0.0f, h);
        // right bar
        glVertex2f((float)(m_barX + m_width), 0.0f); glVertex2f(w, 0.0f); glVertex2f(w, h); glVertex2f((float)(m_barX + m_width), h);
    }
    glEnd();

    glPopAttrib();
}

void Renderer::Shutdown() {
    m_distanceTextureShader.Destroy();
    m_distanceTextureShaderReady = false;
    m_textureTransShader.Destroy();
    m_textureTransShaderReady = false;
    m_textureTransFastShader.Destroy();
    m_textureTransFastShaderReady = false;
    m_paletteOverlayShader.Destroy();
    m_paletteOverlayShaderReady = false;
    m_glyphMaskShader.Destroy();
    m_glyphMaskShaderReady = false;
    m_fireGroundShader.Destroy();
    m_fireGroundShaderReady = false;
    m_window = nullptr;
}

void Renderer::BeginFrame() {
    if (m_window && !m_window->MakeContextCurrent()) return;
    UpdateLetterboxBars();
    // Verification: desktop frame setup; no equivalent PS3 RSX frame-start
    // command sequence has been established for this viewport/clear policy.
    // The adjusted dimensions are stored separately from the centered
    // origin in retail (+0x81a4/+0x81a8 vs +0x8178/+0x817c). Using raw size
    // minus twice the floored origin grows the viewport by one pixel whenever
    // the leftover margin is odd, so pass the rounded logical size verbatim.
    glViewport(m_barX, m_barY, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // This clear policy is a desktop choice; do not treat the old FoE GL
    // clear-function addresses as evidence for Sound Shapes' RSX frame loop.
    glClear(GL_COLOR_BUFFER_BIT);
    // The desktop port clears these buffers to prevent stale contents. Retail
    // clear cadence remains open and must be recovered from RSX commands.
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // Desktop recovery from unbalanced script state. Retail's per-frame
    // colour-mask reset behavior has not been proven.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // Depth-test comparison mapping is separately grounded in Sound Shapes
    // opcode/table evidence. Whether other depth state resets at frame start
    // remains unresolved.
}

void Renderer::EndFrame() {
    // Bars go on last, over the finished scene -- retail draws them as
    // geometry rather than reserving space for them.
    DrawLetterboxBars();
    if (m_window) m_window->Present();
}

// Only meaningful in WorldPerspective mode -- the other three modes have
// fixed extents that do not depend on the camera.
float Renderer::GetWorldHalfHeight() const {
    // FUN_00052568 returns full width = cameraZ * 0.44721359 * 4 and
    // full height = width / (deviceWidth / deviceHeight); the matrix path
    // halves the extents to build camera-centred bounds.
    const float fullWidth = static_cast<float>(
        static_cast<double>(m_cameraZ) * kWorldViewExtentBase * 4.0);
    return (fullWidth / GetAspect()) * 0.5f;
}

float Renderer::GetWorldHalfWidth() const {
    const float fullWidth = static_cast<float>(
        static_cast<double>(m_cameraZ) * kWorldViewExtentBase * 4.0);
    return fullWidth * 0.5f;
}

// Debug-only: dumps the just-rendered frame as a PPM (P6). Reads the full
// raw window, since the viewport is no longer narrowed -- the letterbox bars
// are part of the rendered image now, exactly as they are in retail.
void Renderer::SaveScreenshot(const char* a_path) const {
    std::vector<unsigned char> pixels((size_t)m_rawWidth * m_rawHeight * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, m_rawWidth, m_rawHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    FILE* f = fopen(a_path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", m_rawWidth, m_rawHeight);
    // glReadPixels rows are bottom-to-top; PPM wants top-to-bottom.
    for (int y = m_rawHeight - 1; y >= 0; --y) {
        fwrite(pixels.data() + (size_t)y * m_rawWidth * 3, 1, (size_t)m_rawWidth * 3, f);
    }
    fclose(f);
}

unsigned int Renderer::LoadTexture(const std::string& a_path) {
    return LoadTexture(a_path, a_path);
}

unsigned int Renderer::LoadTexture(const std::string& a_path, const std::string& a_cacheKey) {
    auto it = m_texturesByPath.find(a_cacheKey);
    if (it != m_texturesByPath.end()) return it->second;

    Texture tex;
    if (!Texture::LoadFromFile(a_path, tex)) return 0;

    m_texturesByPath[a_cacheKey] = tex.id;
    m_texturesByHandle[tex.id] = tex;
    if (std::getenv("GE3_TRACE_UI_QUADS"))
        std::fprintf(stderr, "[ui-texture] handle=%u %dx%d path=%s\n", tex.id, tex.width, tex.height, a_path.c_str());
    return tex.id;
}

unsigned int Renderer::LoadTextureFromMemory(const unsigned char* a_data, std::size_t a_size,
                                              const std::string& a_cacheKey) {
    auto it = m_texturesByPath.find(a_cacheKey);
    if (it != m_texturesByPath.end()) return it->second;

    Texture tex;
    if (!Texture::LoadFromEncodedMemory(a_data, a_size, tex)) return 0;
    m_texturesByPath[a_cacheKey] = tex.id;
    m_texturesByHandle[tex.id] = tex;
    return tex.id;
}

unsigned int Renderer::CreateTextureFromRGBA(const unsigned char* a_rgba, int a_width, int a_height,
                                              bool a_nearestFilter) {
    Texture tex;
    if (!Texture::LoadFromMemoryRGBA(a_rgba, a_width, a_height, tex, a_nearestFilter)) return 0;
    m_texturesByHandle[tex.id] = tex; // not path-cached -- caller owns the handle
    return tex.id;
}

void Renderer::UnloadTexture(unsigned int a_handle) {
    auto it = m_texturesByHandle.find(a_handle);
    if (it == m_texturesByHandle.end()) return;
    GLuint id = it->second.id;
    glDeleteTextures(1, &id);
    for (auto pit = m_texturesByPath.begin(); pit != m_texturesByPath.end();) {
        if (pit->second == a_handle) pit = m_texturesByPath.erase(pit);
        else ++pit;
    }
    m_texturesByHandle.erase(it);
}

bool Renderer::DetachTexturePath(unsigned int a_handle) {
    bool detached = false;
    for (auto it = m_texturesByPath.begin(); it != m_texturesByPath.end();) {
        if (it->second == a_handle) {
            it = m_texturesByPath.erase(it);
            detached = true;
        } else {
            ++it;
        }
    }
    return detached;
}

void Renderer::ReleaseAllTextures() {
    for (const auto& entry : m_texturesByHandle) {
        GLuint id = entry.second.id;
        if (id != 0) glDeleteTextures(1, &id);
    }
    m_texturesByHandle.clear();
    m_texturesByPath.clear();
}

// RETAIL BYTECODE VERIFIED: FUN_0004f23c returns pool object +0x20
// (capacity) minus +0x00 (used bytes). Its owner initializes +0x20 to
// 0x0aa00000 (170 MiB) at EBOOT 0x173c68. The desktop texture backend uploads
// each resident texture as RGBA8, so account those allocations against the
// same engine-owned pool budget instead of reporting unrelated driver-wide
// free VRAM.
int Renderer::GetTextureMemoryFreeBytes() const {
    constexpr uint64_t kRetailTexturePoolBytes = 0x0AA00000ull;
    uint64_t residentBytes = 0;
    for (const auto& entry : m_texturesByHandle) {
        const Texture& texture = entry.second;
        if (texture.potWidth <= 0 || texture.potHeight <= 0) continue;
        const uint64_t textureBytes = static_cast<uint64_t>(texture.potWidth) *
                                      static_cast<uint64_t>(texture.potHeight) * 4ull;
        if (textureBytes >= kRetailTexturePoolBytes -
                            std::min(residentBytes, kRetailTexturePoolBytes))
            return 0;
        residentBytes += textureBytes;
    }
    return static_cast<int>(kRetailTexturePoolBytes - residentBytes);
}

void Renderer::SetTextureGeneratedTint(unsigned int a_handle, float a_fillR, float a_fillG, float a_fillB,
                                        float a_outlineR, float a_outlineG, float a_outlineB) {
    auto it = m_texturesByHandle.find(a_handle);
    if (it == m_texturesByHandle.end()) return;
    Texture& tex = it->second;
    const bool unchanged = tex.hasGeneratedTint &&
        tex.fillTintR == a_fillR && tex.fillTintG == a_fillG && tex.fillTintB == a_fillB &&
        tex.outlineTintR == a_outlineR && tex.outlineTintG == a_outlineG && tex.outlineTintB == a_outlineB;
    tex.hasGeneratedTint = true;
    tex.fillTintR = a_fillR; tex.fillTintG = a_fillG; tex.fillTintB = a_fillB;
    tex.outlineTintR = a_outlineR; tex.outlineTintG = a_outlineG; tex.outlineTintB = a_outlineB;
    if (unchanged || !tex.isChromaKeyMask || tex.paletteTemplateRgba.empty()) return;

    // Red and blue encode the two palette weights; green is the transparent
    // key. Reconstruct alpha from total mask coverage so anti-aliased edges
    // remain smooth after the script replaces either palette colour.
    std::vector<unsigned char> recoloured = tex.paletteTemplateRgba;
    for (size_t i = 0; i < recoloured.size(); i += 4) {
        const float fillWeight = recoloured[i + 0] / 255.0f;
        const float outlineWeight = recoloured[i + 2] / 255.0f;
        const float sum = fillWeight + outlineWeight;
        const float coverage = std::min(1.0f, sum);
        if (coverage <= 0.0f) {
            recoloured[i + 0] = recoloured[i + 1] = recoloured[i + 2] = recoloured[i + 3] = 0;
            continue;
        }
        const float r = (a_fillR * fillWeight + a_outlineR * outlineWeight) / sum;
        const float g = (a_fillG * fillWeight + a_outlineG * outlineWeight) / sum;
        const float b = (a_fillB * fillWeight + a_outlineB * outlineWeight) / sum;
        recoloured[i + 0] = (unsigned char)std::clamp(r * 255.0f, 0.0f, 255.0f);
        recoloured[i + 1] = (unsigned char)std::clamp(g * 255.0f, 0.0f, 255.0f);
        recoloured[i + 2] = (unsigned char)std::clamp(b * 255.0f, 0.0f, 255.0f);
        recoloured[i + 3] = (unsigned char)std::clamp(recoloured[i + 3] * coverage, 0.0f, 255.0f);
    }
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.potWidth, tex.potHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, recoloured.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Renderer::UsesGeneratedTexturePalette(unsigned int a_handle) const {
    const auto it = m_texturesByHandle.find(a_handle);
    if (it == m_texturesByHandle.end()) return false;
    const Texture& tex = it->second;
    return tex.hasGeneratedTint ||
        (tex.subTexturePalette && tex.subTexturePalette->hasPendingTint);
}

bool Renderer::SetTexturePalette(unsigned int a_handle, const unsigned char* a_paletteAbgr, size_t a_size) {
    auto it = m_texturesByHandle.find(a_handle);
    if (it == m_texturesByHandle.end() || !a_paletteAbgr || a_size < 256 * 4) return false;
    Texture& tex = it->second;
    if (!tex.hasIndexedPixels || tex.indexedPixels.size() != (size_t)tex.width * tex.height) return false;

    // Retail palette storage is [A,B,G,R] per entry. Rebuild the RGBA
    // upload from preserved PNG indices so palette edits affect the source
    // texels (including alpha) instead of heuristically tinting expanded RGB.
    std::vector<unsigned char> rgba((size_t)tex.potWidth * tex.potHeight * 4, 0);
    const unsigned char keyR = (unsigned char)(tex.keyR * 255.0f + 0.5f);
    const unsigned char keyG = (unsigned char)(tex.keyG * 255.0f + 0.5f);
    const unsigned char keyB = (unsigned char)(tex.keyB * 255.0f + 0.5f);
    for (int y = 0; y < tex.height; ++y) {
        for (int x = 0; x < tex.width; ++x) {
            const size_t index = tex.indexedPixels[(size_t)y * tex.width + x];
            const unsigned char* src = a_paletteAbgr + index * 4;
            unsigned char* dst = rgba.data() + ((size_t)y * tex.potWidth + x) * 4;
            dst[0] = src[3]; dst[1] = src[2]; dst[2] = src[1]; dst[3] = src[0];

            // Some retail sprite PNGs are indexed RGB masks with a solid
            // green background instead of a PNG tRNS alpha channel (for
            // example sway_tree_idle.png). LoadFromFile converts that exact
            // source key to transparent alpha. Palette rebuilds must retain
            // the same mask: rebuilding from the new palette alone would
            // make the background opaque again, drawing a rotated square
            // around the sprite. The key is a property of the source image,
            // so test its original PLTE entry rather than the replacement
            // palette colour.
            if (tex.isChromaKeyMask && tex.sourcePaletteRgba.size() >= (size_t)(index + 1) * 4) {
                const unsigned char* source = tex.sourcePaletteRgba.data() + index * 4;
                if (source[0] == keyR && source[1] == keyG && source[2] == keyB)
                    dst[3] = 0;
            }
        }
    }
    glBindTexture(GL_TEXTURE_2D, tex.id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.potWidth, tex.potHeight,
                    GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    tex.paletteDirty = false;
    if (tex.subTexturePalette) tex.subTexturePalette->dirty = false;
    return true;
}

bool Renderer::GetTextureSourcePalette(unsigned int a_handle, std::vector<unsigned char>& a_outRgba) const {
    const auto it = m_texturesByHandle.find(a_handle);
    if (it == m_texturesByHandle.end() || !it->second.hasIndexedPixels ||
        it->second.sourcePaletteRgba.size() != 256 * 4) return false;
    a_outRgba = it->second.sourcePaletteRgba;
    return true;
}

void Renderer::ResetTexturePalette(unsigned int a_handle) {
    std::vector<unsigned char> sourceRgba;
    if (!GetTextureSourcePalette(a_handle, sourceRgba)) return;
    std::vector<unsigned char> sourceAbgr(256 * 4);
    for (size_t i = 0; i < 256; ++i) {
        const size_t p = i * 4;
        sourceAbgr[p + 0] = sourceRgba[p + 3];
        sourceAbgr[p + 1] = sourceRgba[p + 2];
        sourceAbgr[p + 2] = sourceRgba[p + 1];
        sourceAbgr[p + 3] = sourceRgba[p + 0];
    }
    SetTexturePalette(a_handle, sourceAbgr.data(), sourceAbgr.size());
}

std::shared_ptr<PaletteData> Renderer::GetTextureSubPalette(unsigned int a_handle) const {
    const auto it = m_texturesByHandle.find(a_handle);
    return it == m_texturesByHandle.end() ? nullptr : it->second.subTexturePalette;
}

void Renderer::BindTextureSubPalette(unsigned int a_handle, std::shared_ptr<PaletteData> a_palette) {
    const auto it = m_texturesByHandle.find(a_handle);
    if (it != m_texturesByHandle.end()) {
        it->second.subTexturePalette = std::move(a_palette);
        it->second.paletteDirty = true;
    }
}

void Renderer::GetTextureDimensions(unsigned int a_handle, int& a_outWidth, int& a_outHeight) const {
    auto it = m_texturesByHandle.find(a_handle);
    if (it == m_texturesByHandle.end()) {
        a_outWidth = 0;
        a_outHeight = 0;
        return;
    }
    a_outWidth = it->second.width;
    a_outHeight = it->second.height;
}

int Renderer::GetTextureSourceBitsPerPixel(unsigned int a_handle) const {
    const auto it = m_texturesByHandle.find(a_handle);
    return it == m_texturesByHandle.end() ? 0 : it->second.sourceBitsPerPixel;
}

void Renderer::GetTextureUVExtent(unsigned int a_handle, float& a_uMax, float& a_vMax) const {
    a_uMax = 1.0f;
    a_vMax = 1.0f;
    auto it = m_texturesByHandle.find(a_handle);
    if (it != m_texturesByHandle.end()) {
        a_uMax = it->second.uMax;
        a_vMax = it->second.vMax;
    }
}

void Renderer::SetTextureShaderType(unsigned int a_handle, int a_shaderType) {
    auto it = m_texturesByHandle.find(a_handle);
    if (it != m_texturesByHandle.end()) it->second.shaderType = a_shaderType;
}

int Renderer::GetTextureShaderType(unsigned int a_handle) const {
    auto it = m_texturesByHandle.find(a_handle);
    return it == m_texturesByHandle.end() ? 0 : it->second.shaderType;
}

void Renderer::MarkTexturePaletteDirty(unsigned int a_handle) {
    const auto it = m_texturesByHandle.find(a_handle);
    if (it != m_texturesByHandle.end()) it->second.paletteDirty = true;
}

void Renderer::BindTextureWithUVScale(unsigned int a_handle, float& a_uMax, float& a_vMax) {
    GetTextureUVExtent(a_handle, a_uMax, a_vMax);
    auto it = m_texturesByHandle.find(a_handle);
    if (it != m_texturesByHandle.end()) {
        Texture& tex = it->second;
        // Retail palette writes set TEXTURE +0x78 and the renderer consumes
        // that flag when preparing the image for drawing. Palette mutations
        // may happen after the draw-list's setTextureSubImg opcode, so replay
        // alone is not enough to keep the uploaded image current.
        const std::shared_ptr<PaletteData> palette = tex.subTexturePalette;
        if (palette && (tex.paletteDirty || palette->dirty)) {
            // A draw list can attach one palette object to more than one
            // image. Flush each indexed image which shares the changed
            // object before consuming the palette's shared dirty flag.
            for (auto& entry : m_texturesByHandle) {
                Texture& candidate = entry.second;
                if (candidate.subTexturePalette != palette || !candidate.hasIndexedPixels)
                    continue;
                SetTexturePalette(entry.first, palette->rgba.data(), palette->rgba.size());
            }
            palette->dirty = false;
        }
    }
    glBindTexture(GL_TEXTURE_2D, a_handle);
}

// Verification: retail bytecode confirms opcode 0x1c stores 2 for enabled
// and 7 for disabled, then
// FUN_00044d4c indexes DAT_00431684. The actual table entries are
// 0x203 (LEQUAL) and 0x207 (ALWAYS). Mapping these RSX compare modes to GL
// LEQUAL/ALWAYS is supported by enum meaning; whole render-state parity is
// not established.
void Renderer::SetDepthTest(bool a_enabled) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(a_enabled ? GL_LEQUAL : GL_ALWAYS);
}

void Renderer::SetDepthTestCompareIndex(int a_index) {
    static constexpr GLenum kCompareModes[] = {
        GL_NEVER, GL_LESS, GL_LEQUAL, GL_GREATER,
        GL_NOTEQUAL, GL_EQUAL, GL_GEQUAL, GL_ALWAYS,
    };
    if (a_index < 0 || a_index >= (int)(sizeof(kCompareModes) / sizeof(kCompareModes[0])))
        a_index = 7;
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(kCompareModes[a_index]);
}

void Renderer::SetDepthWrite(bool a_enabled) {
    glDepthMask(a_enabled ? GL_TRUE : GL_FALSE);
}

// Sound Shapes retail Video.setLineWidth stores the value at graphics-state
// offset +0x8210 (FUN_00052550). The former deviceWidth/800 rule came from a
// different game's PC binary and has no Sound Shapes evidence.
void Renderer::SetLineWidth(float a_width) {
    glLineWidth(a_width);
}

// --- Sound Shapes-only state (retail FoE imports none of these; see
// --- renderer.h's note above these declarations). Kept working so the
// --- ported_native/ Sound Shapes path does not regress.

// drawList.setColourMask(enabled) -- the argument is "is the colour MASK
// enabled", i.e. true means colour writes are BLOCKED, so it maps to
// glColorMask(GL_FALSE...). This was inverted, which blanked the entire
// game: WorldMapProcess.gm/TouchEditProcessBase.gm both use the mask to
// bracket stencil-only geometry --
//
//     drawList.setColourMask(true);                       // colour OFF
//     drawList.setAttrStencil(VIDEOPARAMS_ALWAYS, VIDEOPARAMS_REPLACE, 10);
//     drawList.drawShape(this.clearScreenShape);          // stencil only
//     ...
//     drawList.setColourMask(false);                      // colour back ON
//
// -- and those shapes are pure stencil geometry that must not appear on
// screen. With the old polarity, `setColourMask(true)` enabled colour
// writes and the trailing `setColourMask(false)` disabled them for
// everything drawn afterwards. Nothing per-frame resets glColorMask, so
// one pass through a world-map/edit draw left colour writes off forever:
// every later frame still submitted its full ~50 draw calls (6 textured
// quads, 13 gradient polys, 31 text glyphs, verified by instrumenting the
// submission path) with a valid GL context and no GL error, and every
// captured frame came out pure black.
void Renderer::SetColourMask(bool a_maskEnabled) {
    const GLboolean writeColour = a_maskEnabled ? GL_FALSE : GL_TRUE;
    glColorMask(writeColour, writeColour, writeColour, writeColour);
}

void Renderer::SetCulling(int a_mode) {
    if (a_mode == 0) {
        glDisable(GL_CULL_FACE);
        return;
    }
    glEnable(GL_CULL_FACE);
    if (a_mode == 1) {
        // The retail call-site describes mode 1 as culling the clockwise
        // back faces. GL's front-face selector names the *surviving* face,
        // so CCW must be front when CW is the back face to discard.
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
    } else {
        glFrontFace(GL_CCW);
        glCullFace(GL_FRONT);
    }
}

namespace {
// VIDEOPARAMS_* -> real GL enum, see constants_bindings.cpp.
GLenum VideoParamsToGLFunc(int a_func) {
    switch (a_func) {
        case 0: return GL_NEVER;
        case 1: return GL_LESS;
        case 2: return GL_EQUAL;
        case 3: return GL_LEQUAL;
        case 4: return GL_GREATER;
        case 5: return GL_NOTEQUAL;
        case 6: return GL_GEQUAL;
        default: return GL_ALWAYS;
    }
}
GLenum VideoParamsToGLOp(int a_op) {
    switch (a_op) {
        case 101: return GL_REPLACE;
        case 102: return GL_INCR;
        case 103: return GL_DECR;
        case 104: return GL_INVERT;
        case 105: return GL_INCR_WRAP;
        case 106: return GL_DECR_WRAP;
        case 107: return GL_ZERO;
        default: return GL_KEEP;
    }
}
} // namespace

void Renderer::SetStencilFunc(int a_func, int a_ref, unsigned int a_mask) {
    constexpr int kDisable = 8; // VIDEOPARAMS_DISABLE
    if (a_func == kDisable) {
        glDisable(GL_STENCIL_TEST);
        return;
    }
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(VideoParamsToGLFunc(a_func), a_ref, a_mask);
}

void Renderer::SetStencilOp(int a_sfail, int a_dpfail, int a_dppass) {
    glStencilOp(VideoParamsToGLOp(a_sfail), VideoParamsToGLOp(a_dpfail), VideoParamsToGLOp(a_dppass));
}

void Renderer::SetDefaultStencil() {
    glDisable(GL_STENCIL_TEST);
}

void Renderer::FillStencilFullscreen(float a_z) {
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    DrawFullscreenOverlay(a_z);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

// --- end Sound Shapes-only state ---

void Renderer::SetColour(float a_r, float a_g, float a_b, float a_a) {
    m_colour[0] = a_r; m_colour[1] = a_g; m_colour[2] = a_b; m_colour[3] = a_a;
}

void Renderer::SetCameraPos(float a_x, float a_y, float a_z) {
    m_cameraX = a_x;
    m_cameraY = a_y;
    m_cameraZ = a_z;
}

// Verification: Sound Shapes bytecode/decompilation confirms the unit and
// aspect-ratio orthographic setup helpers. WorldPerspective remains an
// approximation: retail FUN_0004cea0's VMX matrix path is decoded but not yet
// fully lifted. GL state and matrix conversion are also desktop adaptations.
void Renderer::ApplyProjection() const {
    const float viewportAspect = GetAspect();

    // FUN_000590c0 builds the frame base as diag(screenViewScale,
    // screenViewScale, 1, 1). FUN_000548b8 left-multiplies the selected view
    // matrix by that base before uploading aViewWorldProjection. In column
    // vector convention this scales the clip-space X/Y rows of the complete
    // projection matrix. Apply it after constructing the view projection so
    // all retail view modes receive the same frame-level transform.
    const auto applyRetailFrameViewScale = [] {
        const float scale = GetEngineContext().video.screenViewScale;
        if (scale == 1.0f) return;
        GLfloat projection[16] = {};
        glGetFloatv(GL_PROJECTION_MATRIX, projection);
        for (int column = 0; column < 4; ++column) {
            projection[column * 4 + 0] *= scale;
            projection[column * 4 + 1] *= scale;
        }
        glLoadMatrixf(projection);
    };

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    switch (m_viewMode) {
        case ViewMode::WorldPerspective: {
            // Retail FUN_0004cea0's false/default branch calls
            // FUN_00052568, which computes full width as
            // cameraZ * 0.44721359 * 4 and full height as width divided by
            // device width/height. It then builds camera-centred bounds from
            // half of those extents. The same branch supplies fixed world
            // near/far bounds -1023/+1023 to the matrix construction.
            const float fullWidth = static_cast<float>(
                static_cast<double>(m_cameraZ) * kWorldViewExtentBase * 4.0);
            const float fullHeight = fullWidth / viewportAspect;
            const float halfW = fullWidth * 0.5f;
            const float halfH = fullHeight * 0.5f;
            // FUN_0004cea0's false/default branch forms the near/far bound
            // vectors from the binary's -1023 and +1023 constants. The
            // orthographic builder uses their difference for the Z scale.
            LoadRetailOrthographicMatrix(
                m_cameraX - halfW, m_cameraX + halfW,
                m_cameraY - halfH, m_cameraY + halfH,
                -kLayerDepthRange, kLayerDepthRange);
            applyRetailFrameViewScale();
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // In this PPC branch cameraZ sets XY extents only. Z bounds are
            // fixed at -1023/+1023, with zero Z translation.
            // Inverse camera axis-angle rotation. Retail Video.setCameraRotation
            // stores radians and an axis at +0x81fc/+0x8200..+0x8208;
            // CameraHelper's billboard path applies the negative camera angle
            // to cancel this world rotation. Rotate about the camera position,
            // which remains the center of the orthographic bounds above.
            const float axisLengthSquared =
                m_cameraRotationX * m_cameraRotationX +
                m_cameraRotationY * m_cameraRotationY +
                m_cameraRotationZ * m_cameraRotationZ;
            if (m_worldViewRotationEnabled && m_cameraRotationAngle != 0.0f &&
                axisLengthSquared > 0.0f) {
                constexpr float kRadiansToDegrees = 57.29577951308232f;
                glTranslatef(m_cameraX, m_cameraY, m_cameraZ);
                glRotatef(-m_cameraRotationAngle * kRadiansToDegrees,
                          m_cameraRotationX, m_cameraRotationY,
                          m_cameraRotationZ);
                glTranslatef(-m_cameraX, -m_cameraY, -m_cameraZ);
            }
            // Pitch is retail's own camera field (+0x64) fed straight into
            // glRotatef; every piece of content in this tree leaves it at 0,
            // where this is the identity.
            glRotatef(m_cameraPitch, 1.0f, 0.0f, 0.0f);
            return;
        }
        case ViewMode::WorldOrtho: {
            // Sound Shapes GameBoard bounds; this projection is a desktop
            // interpretation of the retail world-ortho draw-list command.
            gluOrtho2D((double)kGameBoardMinX, (double)kGameBoardMaxX,
                       (double)kGameBoardMinY, (double)kGameBoardMaxY);
            break;
        }
        case ViewMode::UnitOrtho: {
            // Sound Shapes' own setupOrthoView with no arguments is
            // gluOrtho2D(0, 1, 0, 1) -- CONFIRMED against its binary, not
            // rather than inherited from FoE's FUN_00421640: the decompiled body
            // helper as FUN_0003e220(DAT_0008132c, DAT_00081330,
            // DAT_0008132c, DAT_00081330, list), and those two constants
            // read 0x00000000 and 0x3f800000 -- 0.0f and 1.0f -- straight
            // out of EBOOT.elf. Same (left, right, bottom, top) order as
            // the 4-argument form below.
            gluOrtho2D(0.0, 1.0, 0.0, 1.0);
            break;
        }
        case ViewMode::CustomOrtho: {
            // setupOrthoView(left, right, bottom, top) -- the 4-argument
            // form, confirmed real in the same decompiled body: with an
            // argument count of exactly 4 it reads four float-or-int
            // arguments and passes them to the same helper the 0-arg form
            // calls with (0,1,0,1). See DC_gmf_setupOrthoView.
            gluOrtho2D((double)m_orthoLeft, (double)m_orthoRight,
                       (double)m_orthoBottom, (double)m_orthoTop);
            break;
        }
        // RETAIL PPC: DrawList.setupAspectRatioOrthoView (0x814b0) calls
        // FUN_0003e4a0. It loads constants (-0.5,+0.5,-1,+1), multiplies
        // the first pair by Video+0x81bc, then passes six float arguments to
        // FUN_00302f30: horizontal bounds, vertical bounds (-0.5,+0.5), and
        case ViewMode::AspectRatioOrtho:
        default: {
            // Translation of the retail orthographic bounds to GL.
            const float aspect = m_hasViewAspectOverride
                ? m_viewAspectOverride : viewportAspect;
            float halfH = kUIOrthoHalfHeight;
            float halfW = 0.5f * aspect;
            // Retail consumes +0x81ac in a VMX render
            // matrix transform (0x59684..0x59700). The frame-level scale is
            // applied to the completed projection matrix below, matching
            // base * selectedView instead of approximating it in the bounds.
            gluOrtho2D(-(double)halfW, (double)halfW, -(double)halfH, (double)halfH);
            break;
        }
    }

    applyRetailFrameViewScale();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Verification: desktop immediate-quad implementation. Retail drawTexture
// records through FUN_0003f3dc and RSX render-config submission; matching its
// queue state, coverage, blend, and depth behavior remains open.
void Renderer::DrawTexturedQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                                 float a_x2, float a_y2, float a_x3, float a_y3,
                                 unsigned int a_textureHandle, float a_z,
                                 bool a_distanceTexture, float a_distanceDiscardLevel,
                                 float a_distanceSmoothLevel, int a_textureShaderType,
                                 float a_uvScaleU, float a_uvScaleV,
                                 unsigned int a_primitiveMode) {
    // RETAIL PPC: FUN_0003e6f0 returns before adding any vertices or command
    // when its retained texture pointer is null. Desktop texture lookup uses
    // handle 0 for that state, so do not let fixed-function GL's unbound
    // texture render as a solid white quad.
    if (a_textureHandle == 0) return;

    ApplyProjection();

    // The retail TextureDist descriptor binds aTexture to native row 0,
    // which the desktop shader maps to sampler unit 0. Other renderer paths
    // can temporarily leave a different unit active, so bind and toggle the
    // sampled texture on unit 0 explicitly, then restore the caller's unit.
    GLint previousActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);

    // Opt-in frame diagnosis for retail UI parity work. Logs actual quad
    // bounds/material after deferred replay, which distinguishes a stretched
    // loading-bar quad from a texture upload/UV issue without changing draw
    // state or requiring script edits.
    if (std::getenv("GE3_TRACE_UI_QUADS")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 160) {
            int tw = 0, th = 0;
            GetTextureDimensions(a_textureHandle, tw, th);
            GLint viewport[4] = {0, 0, 0, 0};
            GLfloat projection[16] = {};
            GLfloat modelview[16] = {};
            glGetIntegerv(GL_VIEWPORT, viewport);
            glGetFloatv(GL_PROJECTION_MATRIX, projection);
            glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
            std::fprintf(stderr,
                "[ui-quad] tex=%u %dx%d shader=%d dist=%d thresholds=(%.6f,%.6f) view=%d aspect=%.5f scale=%.5f viewport=(%d,%d,%d,%d) projection=(%.6f,%.6f,%.6f,%.6f) modelview=(%.6f,%.6f,%.6f) bounds=(%.4f,%.4f)-(%.4f,%.4f) z=%.6f camz=%.6f depth=%d/%d stencil=%d:%d color=(%.3f,%.3f,%.3f,%.3f)\n",
                a_textureHandle, tw, th, a_textureShaderType, a_distanceTexture,
                a_distanceDiscardLevel, a_distanceSmoothLevel,
                static_cast<int>(m_viewMode), GetAspect(),
                GetEngineContext().video.screenViewScale,
                viewport[0], viewport[1], viewport[2], viewport[3],
                projection[0], projection[5], projection[10], projection[14],
                modelview[12], modelview[13], modelview[14],
                std::min(std::min(a_x0, a_x1), std::min(a_x2, a_x3)),
                std::min(std::min(a_y0, a_y1), std::min(a_y2, a_y3)),
                std::max(std::max(a_x0, a_x1), std::max(a_x2, a_x3)),
                std::max(std::max(a_y0, a_y1), std::max(a_y2, a_y3)),
                a_z, m_cameraZ, GetDrawContext().depthTest, GetDrawContext().depthWrite,
                GetDrawContext().stencilFunc, GetDrawContext().stencilRef,
                m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
        }
    }

    float uMax, vMax;
    glEnable(GL_TEXTURE_2D);
    // Blending alone does not stop a zero-alpha texel from writing depth.
    // Discard it so the transparent area of a sprite quad cannot occlude
    // a later layer (for example the player behind a flower head).
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.0f);
    BindTextureWithUVScale(a_textureHandle, uMax, vMax);
    uMax *= a_uvScaleU;
    vMax *= a_uvScaleV;
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    GLint previousProgram = 0;
    const bool useDistanceShader = a_distanceTexture && m_distanceTextureShaderReady;
    // Retail texture selectors 0, 1 and 2 map to TextureTrans,
    // TextureTransFast and TextureTransFastNB. Keep unsupported palette/grey
    // selectors on the fixed-function fallback until their programs exist.
    const bool useTextureTransShader = !useDistanceShader && a_textureShaderType == 0 &&
                                       m_textureTransShaderReady;
    const bool useTextureTransFastShader = !useDistanceShader &&
        (a_textureShaderType == 1 || a_textureShaderType == 2) &&
        m_textureTransFastShaderReady;
    const bool knownQuadConfig = a_distanceTexture || a_textureShaderType == 0 ||
                                 a_textureShaderType == 1 || a_textureShaderType == 2;
    const bool hasShaderProgram = useDistanceShader || useTextureTransShader ||
                                  useTextureTransFastShader;
    if (knownQuadConfig) {
        ApplyRenderConfigBlendState(a_textureShaderType != 2);
    }
    if (hasShaderProgram) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        if (useDistanceShader) {
            m_distanceTextureShader.Use();
            glUniform1i(m_distanceTextureShader.Uniform("u_texture"), 0);
            glUniform1f(m_distanceTextureShader.Uniform("u_discardLevel"), a_distanceDiscardLevel);
            glUniform1f(m_distanceTextureShader.Uniform("u_smoothLevel"), a_distanceSmoothLevel);
            glUniform4fv(m_distanceTextureShader.Uniform("u_color"), 1, m_colour);
        } else if (useTextureTransShader) {
            m_textureTransShader.Use();
            glUniform1i(m_textureTransShader.Uniform("u_texture"), 0);
            glUniform4fv(m_textureTransShader.Uniform("u_color"), 1, m_colour);
        } else {
            m_textureTransFastShader.Use();
            glUniform1i(m_textureTransFastShader.Uniform("u_texture"), 0);
        }
    }

    if (a_primitiveMode == GL_TRIANGLE_STRIP) {
        glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2f(0.0f, 0.0f);  glVertex3f(a_x0, a_y0, a_z);
        glTexCoord2f(uMax, 0.0f);  glVertex3f(a_x1, a_y1, a_z);
        glTexCoord2f(0.0f, vMax);  glVertex3f(a_x3, a_y3, a_z);
        glTexCoord2f(uMax, vMax);  glVertex3f(a_x2, a_y2, a_z);
    } else if (a_primitiveMode == GL_TRIANGLES) {
        glBegin(GL_TRIANGLES);
        glTexCoord2f(0.0f, 0.0f);  glVertex3f(a_x0, a_y0, a_z);
        glTexCoord2f(uMax, 0.0f);  glVertex3f(a_x1, a_y1, a_z);
        glTexCoord2f(uMax, vMax);  glVertex3f(a_x2, a_y2, a_z);
        glTexCoord2f(0.0f, 0.0f);  glVertex3f(a_x0, a_y0, a_z);
        glTexCoord2f(uMax, vMax);  glVertex3f(a_x2, a_y2, a_z);
        glTexCoord2f(0.0f, vMax);  glVertex3f(a_x3, a_y3, a_z);
    } else {
        glBegin(a_primitiveMode ? a_primitiveMode : GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);  glVertex3f(a_x0, a_y0, a_z);
        glTexCoord2f(uMax, 0.0f);  glVertex3f(a_x1, a_y1, a_z);
        glTexCoord2f(uMax, vMax);  glVertex3f(a_x2, a_y2, a_z);
        glTexCoord2f(0.0f, vMax);  glVertex3f(a_x3, a_y3, a_z);
    }
    glEnd();

    if (hasShaderProgram) glUseProgram((GLuint)previousProgram);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST);
    glActiveTexture((GLenum)previousActiveTexture);
}

void Renderer::DrawPolygon(unsigned int a_glPrimitiveMode, const float* a_xy, int a_count, float a_z) {
    if (a_count <= 0) return;

    if (std::getenv("GE3_TRACE_UI_QUADS")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 160) {
            float minX = a_xy[0], maxX = a_xy[0], minY = a_xy[1], maxY = a_xy[1];
            for (int i = 1; i < a_count; ++i) {
                minX = std::min(minX, a_xy[i * 2]); maxX = std::max(maxX, a_xy[i * 2]);
                minY = std::min(minY, a_xy[i * 2 + 1]); maxY = std::max(maxY, a_xy[i * 2 + 1]);
            }
            std::fprintf(stderr,
                "[ui-poly] n=%d z=%.3f bounds=(%.4f,%.4f)-(%.4f,%.4f) color=(%.3f,%.3f,%.3f,%.3f)\n",
                a_count, a_z, minX, minY, maxX, maxY,
                m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
        }
    }

    ApplyProjection();
    glDisable(GL_TEXTURE_2D);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    glBegin(a_glPrimitiveMode);
    for (int i = 0; i < a_count; ++i) {
        glVertex3f(a_xy[i * 2 + 0], a_xy[i * 2 + 1], a_z);
    }
    glEnd();
}

void Renderer::DrawFireGroundStrip(const float* a_xy, const float* a_data, int a_count,
                                   float a_amp, float a_z) {
    if (!a_xy || a_count <= 0) return;
    if (!m_fireGroundShaderReady || !a_data) {
        DrawPolygon(GL_TRIANGLE_STRIP, a_xy, a_count, a_z);
        return;
    }

    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    ApplyProjection();
    glDisable(GL_TEXTURE_2D);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
    glUseProgram(m_fireGroundShader.Program());
    glUniform1f(m_fireGroundShader.Uniform("u_amp"), a_amp);
    glUniform1f(m_fireGroundShader.Uniform("u_z"), a_z);
    glBegin(GL_TRIANGLE_STRIP);
    for (int i = 0; i < a_count; ++i) {
        glMultiTexCoord2f(GL_TEXTURE0, a_data[i * 2], a_data[i * 2 + 1]);
        glVertex3f(a_xy[i * 2], a_xy[i * 2 + 1], a_z);
    }
    glEnd();
    glUseProgram((GLuint)previousProgram);
}

void Renderer::DrawPolygonGradient(unsigned int a_glPrimitiveMode, const float* a_xy, const float* a_rgba,
                                    const float* a_uv, int a_count, unsigned int a_textureHandle, float a_z,
                                    bool a_uvAlreadyIncludesTextureExtent,
                                    bool a_distanceTexture, float a_distanceDiscardLevel,
                                    float a_distanceSmoothLevel, int a_textureShaderType) {
    if (a_count <= 0) return;

    if (std::getenv("GE3_TRACE_UI_QUADS")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 10000) {
            float minX = a_xy[0], maxX = a_xy[0], minY = a_xy[1], maxY = a_xy[1];
            for (int i = 1; i < a_count; ++i) {
                minX = std::min(minX, a_xy[i * 2]); maxX = std::max(maxX, a_xy[i * 2]);
                minY = std::min(minY, a_xy[i * 2 + 1]); maxY = std::max(maxY, a_xy[i * 2 + 1]);
            }
            std::fprintf(stderr,
                "[ui-gradient] n=%d tex=%u shader=%d z=%.3f bounds=(%.4f,%.4f)-(%.4f,%.4f) rgba0=(%.3f,%.3f,%.3f,%.3f) mod=(%.3f,%.3f,%.3f,%.3f)\n",
                a_count, a_textureHandle, a_textureShaderType, a_z, minX, minY, maxX, maxY,
                a_rgba ? a_rgba[0] : m_colour[0], a_rgba ? a_rgba[1] : m_colour[1],
                a_rgba ? a_rgba[2] : m_colour[2], a_rgba ? a_rgba[3] : m_colour[3],
                m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
        }
    }

    ApplyProjection();

    float uMax = 1.0f, vMax = 1.0f;
    const bool textured = (a_textureHandle != 0);
    GLint previousActiveTexture = GL_TEXTURE0;
    if (textured) {
        glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.0f);
        BindTextureWithUVScale(a_textureHandle, uMax, vMax);
    } else {
        glDisable(GL_TEXTURE_2D);
    }

    // Verification: TextureDist fragment math is bytecode-confirmed; its
    // aColor source is confirmed by the native category-5 descriptor and
    // FUN_000548b8. TextureTrans's bytecode-confirmed fragment multiplies the
    // texel by vColor; its table maps aColor to category 5 at 0x41f8d8.
    // TextureTransFast and TextureTransFastNB share a fragment with only
    // aTexture/vTexCord and return its sampled texel unchanged. Selector 1
    // enables the native translucent state; selector 2 disables blending per
    // its native zero flag. Other specialized configs are not mapped here yet.
    // Grouped native/desktop draw-pass correspondence and depth behavior
    // remain open. Blend state is set from recovered descriptor values below.
    GLint previousProgram = 0;
    const bool useDistanceShader = textured && a_distanceTexture &&
                                   m_distanceTextureShaderReady;
    const bool useTextureTransShader = textured && !useDistanceShader &&
                                       a_textureShaderType == 0 &&
                                       m_textureTransShaderReady;
    const bool useTextureTransFastShader = textured && !useDistanceShader &&
                                           (a_textureShaderType == 1 ||
                                            a_textureShaderType == 2) &&
                                           m_textureTransFastShaderReady;
    if (useDistanceShader || useTextureTransShader || useTextureTransFastShader) {
        ApplyRenderConfigBlendState(a_textureShaderType != 2);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        if (useDistanceShader) {
            m_distanceTextureShader.Use();
            glUniform1i(m_distanceTextureShader.Uniform("u_texture"), 0);
            glUniform1f(m_distanceTextureShader.Uniform("u_discardLevel"),
                        a_distanceDiscardLevel);
            glUniform1f(m_distanceTextureShader.Uniform("u_smoothLevel"),
                        a_distanceSmoothLevel);
            glUniform4fv(m_distanceTextureShader.Uniform("u_color"), 1, m_colour);
        } else if (useTextureTransShader) {
            m_textureTransShader.Use();
            glUniform1i(m_textureTransShader.Uniform("u_texture"), 0);
            glUniform4fv(m_textureTransShader.Uniform("u_color"), 1, m_colour);
        } else {
            m_textureTransFastShader.Use();
            glUniform1i(m_textureTransFastShader.Uniform("u_texture"), 0);
        }
    }

    // UV fallback when the shape carries a colour gradient but no real
    // per-vertex UVs: remap xy into the shape's own bounding box.
    float minX = 0.0f, minY = 0.0f, spanX = 1.0f, spanY = 1.0f;
    if (!a_uv) {
        float maxX = a_xy[0], maxY = a_xy[1];
        minX = a_xy[0]; minY = a_xy[1];
        for (int i = 0; i < a_count; ++i) {
            minX = std::min(minX, a_xy[i * 2 + 0]);
            maxX = std::max(maxX, a_xy[i * 2 + 0]);
            minY = std::min(minY, a_xy[i * 2 + 1]);
            maxY = std::max(maxY, a_xy[i * 2 + 1]);
        }
        spanX = (maxX - minX); if (spanX < 0.0001f) spanX = 1.0f;
        spanY = (maxY - minY); if (spanY < 0.0001f) spanY = 1.0f;
    }

    glBegin(a_glPrimitiveMode);
    for (int i = 0; i < a_count; ++i) {
        const float x = a_xy[i * 2 + 0];
        const float y = a_xy[i * 2 + 1];
        if (textured) {
            // Video.beginTexture's normalized UVs use logical-image space
            // and scale by uMax/vMax, including out-of-range tiling. A
            // texture-configured shape can instead carry coordinates already
            // scaled by the retail texture fields; its caller selects that
            // path with a_uvAlreadyIncludesTextureExtent.
            const float u = a_uv ? a_uv[i * 2 + 0] : (x - minX) / spanX;
            const float v = a_uv ? a_uv[i * 2 + 1] : (y - minY) / spanY;
            // RETAIL BYTECODE MATCH: CreateTextBubble's PPC helper writes
            // UVs after applying texture +0x34/+0x38. Video.getTextureExtX/Y
            // expose those same scale values. Those authored shape UVs are
            // final texture coordinates and must not receive the padding
            // scale a second time.
            glTexCoord2f(a_uvAlreadyIncludesTextureExtent ? u : u * uMax,
                         a_uvAlreadyIncludesTextureExtent ? v : v * vMax);
        }
        // In the generic path the native draw colour and the mesh's
        // per-vertex colour are separate inputs: the draw-list's current
        // colour tints a shape, while plotVertex's stored RGBA supplies its
        // vertex gradient. The menu tabs expose this distinction clearly:
        // they are recorded white in the vertex stream, then drawList.setColour
        // assigns four different Cory palette colours. Passing only a_rgba
        // made all four shapes white (and erased the script's tint). The
        // specialized TextureTrans/TextureDist programs consume m_colour via
        // their uniforms, while TextureTransFast variants intentionally do
        // not; only the generic fixed-function fallback needs this product.
        float r = a_rgba[i * 4 + 0];
        float g = a_rgba[i * 4 + 1];
        float b = a_rgba[i * 4 + 2];
        float alpha = a_rgba[i * 4 + 3];
        if (!(useDistanceShader || useTextureTransShader || useTextureTransFastShader)) {
            r *= m_colour[0]; g *= m_colour[1]; b *= m_colour[2]; alpha *= m_colour[3];
        }
        glColor4f(r, g, b, alpha);
        glVertex3f(x, y, a_z);
    }
    glEnd();

    if (useDistanceShader || useTextureTransShader || useTextureTransFastShader)
        glUseProgram((GLuint)previousProgram);

    if (textured) {
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_ALPHA_TEST);
        glActiveTexture((GLenum)previousActiveTexture);
    }
}

bool Renderer::DrawPolygonPaletteOverlay(unsigned int a_glPrimitiveMode, const float* a_xy,
                                         const float* a_uv, int a_count,
                                         unsigned int a_backTexture, unsigned int a_frontTexture,
                                         float a_frontScaleX, float a_frontScaleY,
                                         float a_alpha, float a_z) {
    if (!m_paletteOverlayShaderReady || !a_xy || a_count <= 0 ||
        !a_backTexture || !a_frontTexture) return false;

    ApplyProjection();

    // The retail pair shader has two independent texture-coordinate outputs:
    // the base receives the shape UV and the attached frame receives that UV
    // multiplied by primarySize / attachedSize. Account for desktop POT
    // padding separately for each texture.
    GLint oldActive = GL_TEXTURE0;
    GLint oldProgram = 0;
    GLint oldBinding0 = 0, oldBinding1 = 0;
    GLboolean oldTexture0 = GL_FALSE, oldTexture1 = GL_FALSE;
    GLboolean oldAlphaTest = glIsEnabled(GL_ALPHA_TEST);
    GLfloat oldColour[4]{};
    glGetIntegerv(GL_ACTIVE_TEXTURE, &oldActive);
    glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    glGetFloatv(GL_CURRENT_COLOR, oldColour);

    glActiveTexture(GL_TEXTURE0);
    oldTexture0 = glIsEnabled(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBinding0);
    float backUMax, backVMax;
    BindTextureWithUVScale(a_backTexture, backUMax, backVMax);

    glActiveTexture(GL_TEXTURE1);
    oldTexture1 = glIsEnabled(GL_TEXTURE_2D);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &oldBinding1);
    float frontUMax, frontVMax;
    BindTextureWithUVScale(a_frontTexture, frontUMax, frontVMax);

    glEnable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_ALPHA_TEST); // the retail fragment shader performs the paired alpha kill
    m_paletteOverlayShader.Use();
    glUniform1i(m_paletteOverlayShader.Uniform("u_backTexture"), 0);
    glUniform1i(m_paletteOverlayShader.Uniform("u_frontTexture"), 1);
    glColor4f(1.0f, 1.0f, 1.0f, a_alpha);

    float minX = 0.0f, minY = 0.0f, spanX = 1.0f, spanY = 1.0f;
    if (!a_uv) {
        float maxX = a_xy[0], maxY = a_xy[1];
        minX = a_xy[0];
        minY = a_xy[1];
        for (int i = 1; i < a_count; ++i) {
            minX = std::min(minX, a_xy[i * 2]);
            maxX = std::max(maxX, a_xy[i * 2]);
            minY = std::min(minY, a_xy[i * 2 + 1]);
            maxY = std::max(maxY, a_xy[i * 2 + 1]);
        }
        spanX = maxX - minX; if (spanX < 0.0001f) spanX = 1.0f;
        spanY = maxY - minY; if (spanY < 0.0001f) spanY = 1.0f;
    }

    glBegin(a_glPrimitiveMode);
    for (int i = 0; i < a_count; ++i) {
        const float x = a_xy[i * 2];
        const float y = a_xy[i * 2 + 1];
        const float u = a_uv ? a_uv[i * 2] : (x - minX) / spanX;
        const float v = a_uv ? a_uv[i * 2 + 1] : (y - minY) / spanY;
        glMultiTexCoord2f(GL_TEXTURE0, u * backUMax, v * backVMax);
        glMultiTexCoord2f(GL_TEXTURE1, u * a_frontScaleX * frontUMax,
                          v * a_frontScaleY * frontVMax);
        glVertex3f(x, y, a_z);
    }
    glEnd();

    glUseProgram((GLuint)oldProgram);
    glColor4fv(oldColour);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, (GLuint)oldBinding0);
    if (oldTexture0) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, (GLuint)oldBinding1);
    if (oldTexture1) glEnable(GL_TEXTURE_2D); else glDisable(GL_TEXTURE_2D);
    if (oldAlphaTest) glEnable(GL_ALPHA_TEST); else glDisable(GL_ALPHA_TEST);
    glActiveTexture((GLenum)oldActive);
    return true;
}

void Renderer::DrawFullscreenOverlay(float a_z, unsigned int a_mode) {
    // Retail DrawList.clear appends opcode 4, whose dispatcher submits the
    // prebuilt oversized clear-screen triangle through FUN_0030a278. Its
    // depth and stencil effects depend on mode. This full-window triangle is
    // a desktop geometry approximation; preserve matrices and GL state.
    (void)a_z; // retail's clear shape uses fixed clip-space positions.

    GLint previousMatrixMode = GL_MODELVIEW;
    GLint previousProgram = 0;
    GLint previousTextureBinding = 0;
    GLint previousDepthFunction = GL_LEQUAL;
    GLint previousStencilFunction = GL_ALWAYS;
    GLint previousStencilReference = 0;
    GLint previousStencilValueMask = 0xff;
    GLint previousStencilFail = GL_KEEP;
    GLint previousStencilDepthFail = GL_KEEP;
    GLint previousStencilDepthPass = GL_KEEP;
    GLint previousCullFace = GL_BACK;
    GLboolean previousDepthWrite = GL_TRUE;
    GLboolean previousColourMask[4] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    GLboolean previousTexture2d = GL_FALSE;
    GLboolean previousCullEnabled = GL_FALSE;
    GLint previousActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTextureBinding);
    previousTexture2d = glIsEnabled(GL_TEXTURE_2D);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthWrite);
    glGetBooleanv(GL_COLOR_WRITEMASK, previousColourMask);
    glGetIntegerv(GL_STENCIL_FUNC, &previousStencilFunction);
    glGetIntegerv(GL_STENCIL_REF, &previousStencilReference);
    glGetIntegerv(GL_STENCIL_VALUE_MASK, &previousStencilValueMask);
    glGetIntegerv(GL_STENCIL_FAIL, &previousStencilFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &previousStencilDepthFail);
    glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &previousStencilDepthPass);
    previousCullEnabled = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // PPC at 0x30a144 maps both depth descriptors to ALWAYS; bit 1 controls
    // only the depth write flag. At 0x30a02c, bit 2 selects stencil ZERO
    // versus KEEP for all outcomes. The color-mask helper resolves both
    // opcode-4 branches to all channels enabled. Bit 3 selects one of two
    // retail render-config objects; their shared fragment program and blend
    // flag are traced in DRAWLIST_CLEAR_AUDIT.md.
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask((a_mode & 2u) ? GL_TRUE : GL_FALSE);
    glEnable(GL_STENCIL_TEST);
    // Retail sets the stencil comparison mask from the descriptor but leaves
    // the stencil write mask untouched on this non-null descriptor path.
    glStencilFunc(GL_ALWAYS, 0, 0xffu);
    const GLenum stencilOp = (a_mode & 4u) ? GL_ZERO : GL_KEEP;
    glStencilOp(stencilOp, stencilOp, stencilOp);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    if (a_mode & 8u) glEnable(GL_BLEND);
    else glDisable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    // Retail's prebuilt shape stores these clip-space vertices directly.
    glBegin(GL_TRIANGLES);
    glVertex3f(-1.0f, -1.0f, 1.0f);
    glVertex3f(3.0f, -1.0f, 1.0f);
    glVertex3f(-1.0f, 3.0f, 1.0f);
    glEnd();

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    // FUN_0030a414 restores the saved retail tuple after opcode 4. It enables
    // depth and stencil testing, reapplies the tuple's compare/write/color and
    // stencil state, and forces CCW front faces. It does not restore blending,
    // draw colour, or texture state. Avoid GL attrib-stack restoration here:
    // that would roll back more state than retail does.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(static_cast<GLenum>(previousDepthFunction));
    glDepthMask(previousDepthWrite);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(static_cast<GLenum>(previousStencilFunction),
                  previousStencilReference,
                  static_cast<GLuint>(previousStencilValueMask));
    glStencilOp(static_cast<GLenum>(previousStencilFail),
                static_cast<GLenum>(previousStencilDepthFail),
                static_cast<GLenum>(previousStencilDepthPass));
    glColorMask(previousColourMask[0], previousColourMask[1],
                previousColourMask[2], previousColourMask[3]);
    glFrontFace(GL_CCW);
    if (!previousCullEnabled) {
        glCullFace(GL_BACK);
        glDisable(GL_CULL_FACE);
    } else if (previousCullFace == GL_BACK || previousCullFace == GL_FRONT) {
        glCullFace(static_cast<GLenum>(previousCullFace));
        glEnable(GL_CULL_FACE);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTextureBinding));
    if (previousTexture2d) glEnable(GL_TEXTURE_2D);
    else glDisable(GL_TEXTURE_2D);
    glUseProgram((GLuint)previousProgram);
    glActiveTexture((GLenum)previousActiveTexture);
    glMatrixMode((GLenum)previousMatrixMode);
}

void Renderer::DrawGlyphQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                              float a_x2, float a_y2, float a_x3, float a_y3,
                              unsigned int a_textureHandle, float a_u0, float a_v0, float a_u1, float a_v1,
                              float a_edge, float a_width, float a_z) {
    ApplyProjection();

    float uMax, vMax;
    GLint previousActiveTexture = GL_TEXTURE0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    BindTextureWithUVScale(a_textureHandle, uMax, vMax);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    GLint previousProgram = 0;
    if (m_glyphMaskShaderReady) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        m_glyphMaskShader.Use();
        glUniform1i(m_glyphMaskShader.Uniform("u_texture"), 0);
        // Retail's TextureDist program consumes the two font attributes in
        // their recorded order. In particular, do not sort them here: the
        // native shader performs a signed divide when smooth < discard.
        glUniform1f(m_glyphMaskShader.Uniform("u_discardLevel"), a_edge);
        glUniform1f(m_glyphMaskShader.Uniform("u_smoothLevel"), a_width);
        glUniform4fv(m_glyphMaskShader.Uniform("u_color"), 1, m_colour);
    }

    // The atlas UVs bitmap_font computes are fractions of the logical atlas
    // image. The uploaded dimensions match that image, so no extra scale is
    // needed for retail's +0x34/+0x38 extent values (1.0, 1.0).
    const float u0 = a_u0 * uMax, u1 = a_u1 * uMax;
    const float v0 = a_v0 * vMax, v1 = a_v1 * vMax;

    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);  glVertex3f(a_x0, a_y0, a_z);
    glTexCoord2f(u1, v0);  glVertex3f(a_x1, a_y1, a_z);
    glTexCoord2f(u1, v1);  glVertex3f(a_x2, a_y2, a_z);
    glTexCoord2f(u0, v1);  glVertex3f(a_x3, a_y3, a_z);
    glEnd();

    if (m_glyphMaskShaderReady) glUseProgram((GLuint)previousProgram);

    glDisable(GL_TEXTURE_2D);
    glActiveTexture((GLenum)previousActiveTexture);
}
