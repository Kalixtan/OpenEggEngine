#include "renderer.h"
#include "draw_context.h"
#include "../core/engine_context.h"
#include "../core/system_config.h"
#include "../bindings/colour_bindings.h"
#include <cstdio>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <limits>




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

int RetailViewportOrigin(int a_deviceSize, int a_viewSize, int a_offset) {
    uint32_t difference = static_cast<uint32_t>(a_deviceSize) -
                          static_cast<uint32_t>(a_viewSize);
    uint32_t origin = (difference >> 1) + static_cast<uint32_t>(a_offset);
    int result;
    std::memcpy(&result, &origin, sizeof(result));
    return result;
}

float CameraYOffset(float a_pitchDeg, float a_camZ) {
    const float angleRad = (a_pitchDeg - 90.0f) * kPi / 180.0f;
    float s = std::sin(angleRad);
    if (s == 0.0f) s = 0.001f;
    return std::fabs(a_camZ) * std::cos(angleRad) / s;
}

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

    const GLfloat matrix[16] = {
        reciprocalWidth + reciprocalWidth, 0.0f, 0.0f, 0.0f,
        0.0f, reciprocalHeight + reciprocalHeight, 0.0f, 0.0f,
        0.0f, 0.0f, -(reciprocalDepth + reciprocalDepth), 0.0f,
        std::fma(-horizontalCenter, reciprocalWidth, 0.0f),
        std::fma(-verticalCenter, reciprocalHeight, 0.0f),
        std::fma(-depthCenter, reciprocalDepth, 0.0f), 1.0f};
    glLoadMatrixf(matrix);
}

// Keep the desktop NOTE_BASS silhouette calibration attached to the texture
// asset rather than to one renderer entry point. A TextureDist asset can
// reach the same fragment program through a normal textured quad or a
// textured VMSHAPE/polygon submission; both must see identical uniforms.
float NoteTextureDistanceBias(const std::string& a_sourcePath,
                              int a_textureShaderType) {
    if (a_textureShaderType != 8) return 0.0f;
    std::string normalizedPath = a_sourcePath;
    std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
    const bool noteDistanceFieldTexture =
        normalizedPath.find("/images/NOTE_") != std::string::npos ||
        normalizedPath.find("/images/note_loop") != std::string::npos;
    return noteDistanceFieldTexture ? 0.15f : 0.0f;
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

    GLint sampleBuffers = 0;
    GLint samples = 0;
    glGetIntegerv(GL_SAMPLE_BUFFERS, &sampleBuffers);
    glGetIntegerv(GL_SAMPLES, &samples);
    if (sampleBuffers > 0 && samples > 0) {
        glEnable(GL_MULTISAMPLE);
        std::fprintf(stderr, "Renderer: multisample antialiasing enabled (%d samples)\n",
                     samples);
    } else {
        glDisable(GL_MULTISAMPLE);
        std::fprintf(stderr, "Renderer: multisample antialiasing unavailable; using line smoothing\n");
    }

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

    // Glyphs are textured quads, so polygon MSAA does not smooth their
    // outlines. Keep their style thresholds, but widen sub-pixel transitions
    // to the sampled distance field's screen-space footprint.
    static const char* kGlyphDistanceFS = R"GLSL(#version 120
uniform sampler2D u_texture;
uniform float u_discardLevel;
uniform float u_smoothLevel;
uniform vec4 u_color;
varying vec2 v_uv;
void main() {
    float distanceValue = texture2D(u_texture, v_uv).a;
    float low = min(u_discardLevel, u_smoothLevel);
    float high = max(u_discardLevel, u_smoothLevel);
    float center = 0.5 * (low + high);
    float transition = max(high - low, fwidth(distanceValue));
    // Do not retain TextureDist's hard discard at `low`: normal FontHelper
    // settings produce equal thresholds (0.5, 0.5), and that discard clips
    // half of the derivative-sized fringe, causing the visible stair steps.
    if (distanceValue <= center - 0.5 * transition) discard;
    float t = clamp(0.5 + (distanceValue - center) / transition, 0.0, 1.0);
    float coverage = t * t * (3.0 - 2.0 * t);
    gl_FragColor = vec4(u_color.rgb, u_color.a * coverage);
}
)GLSL";

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

    static const char* kSelectionFS = R"GLSL(#version 120
uniform vec4 u_selectColour;
uniform float u_pulsePerc;
uniform float u_alphaPerc;
void main() {
    gl_FragColor = vec4(mix(gl_Color.rgb, u_selectColour.rgb, u_pulsePerc),
                        gl_Color.a * u_alphaPerc);
}
)GLSL";
    static const char* kSelectionVS = R"GLSL(#version 120
varying vec2 v_uv;
void main() {
    gl_Position = ftransform();
    gl_FrontColor = gl_Color;
    v_uv = gl_MultiTexCoord0.st;
}
)GLSL";
    m_selectionShaderReady = m_selectionShader.Compile(kSelectionVS, kSelectionFS);
    if (!m_selectionShaderReady)
        std::fprintf(stderr, "Renderer: Selection_f shader unavailable; selected geometry uses its base config\n");

    // Static RSX decode of TextureSelection_f: discard source texels with
    // alpha <= 0, multiply output alpha by alphaPerc, and pulse RGB toward
    // selectColour. Unlike TextureTrans, this selected variant has no vertex
    // colour modulation in the decoded fragment program.
    static const char* kTextureSelectionFS = R"GLSL(#version 120
uniform sampler2D u_texture;
uniform vec4 u_selectColour;
uniform float u_pulsePerc;
uniform float u_alphaPerc;
varying vec2 v_uv;
void main() {
    vec4 texel = texture2D(u_texture, v_uv);
    if (texel.a <= 0.0) discard;
    gl_FragColor = vec4(mix(texel.rgb, u_selectColour.rgb, u_pulsePerc),
                        texel.a * u_alphaPerc);
}
)GLSL";
    m_textureSelectionShaderReady = m_textureSelectionShader.Compile(
        kSelectionVS, kTextureSelectionFS);
    if (!m_textureSelectionShaderReady)
        std::fprintf(stderr, "Renderer: TextureSelection_f shader unavailable; selected textures use their base config\n");

    static const char* kTextureDistSelectionFS = R"GLSL(#version 120
uniform sampler2D u_texture;
uniform vec2 u_size;
uniform vec4 u_color;
uniform float u_discardLevel;
uniform float u_smoothLevel;
uniform vec4 u_selectColour;
uniform float u_pulsePerc;
uniform float u_alphaPerc;
varying vec2 v_uv;
void main() {
    float edgeWidth = abs(dFdx(dot(v_uv, u_size))) * u_discardLevel;
    float distanceValue = texture2D(u_texture, v_uv).a;
    float edgeStart = u_smoothLevel - edgeWidth * 0.25;
    if (distanceValue <= edgeStart) discard;
    float edgeSpan = edgeWidth * 0.5;
    float signedSpan = max(abs(edgeSpan), 1.0e-15) * sign(edgeSpan);
    float t = clamp((distanceValue - edgeStart) / signedSpan, 0.0, 1.0);
    float coverage = t * t * (3.0 - 2.0 * t);
    vec3 rgb = mix(u_color.rgb, u_selectColour.rgb, u_pulsePerc);
    gl_FragColor = vec4(rgb, u_color.a * coverage * u_alphaPerc);
}
)GLSL";
    m_textureDistSelectionShaderReady = m_textureDistSelectionShader.Compile(
        kSelectionVS, kTextureDistSelectionFS);
    if (!m_textureDistSelectionShaderReady)
        std::fprintf(stderr, "Renderer: TextureDistSelection_f unavailable; selected distance fields use their base shader\n");

    m_glyphMaskShaderReady = m_glyphMaskShader.Compile(
        kDistanceTextureVS, kGlyphDistanceFS);
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

    // Port the decoded DefaultTrans vertex/fragment operations for retained
    // player shapes: position.y*matrix[1], then x*matrix[0], z*matrix[2],
    // matrix[3], with current draw colour passed through unchanged.
    static const char* kDefaultTransVS = R"GLSL(#version 120
uniform mat4 uViewWorldProjection;
varying vec4 vColor;
void main() {
    vec4 partial = gl_Vertex.y * uViewWorldProjection[1];
    partial = gl_Vertex.x * uViewWorldProjection[0] + partial;
    partial = gl_Vertex.z * uViewWorldProjection[2] + partial;
    gl_Position = uViewWorldProjection[3] + partial;
    vColor = gl_Color;
}
)GLSL";
    static const char* kDefaultTransFS = R"GLSL(#version 120
varying vec4 vColor;
void main() { gl_FragColor = vColor; }
)GLSL";
    m_defaultTransShaderReady = m_defaultTransShader.Compile(
        kDefaultTransVS, kDefaultTransFS);
    if (!m_defaultTransShaderReady)
        std::fprintf(stderr, "Renderer: DefaultTrans player shader unavailable; using fixed-function fallback\n");

    m_rawWidth = a_width;
    m_rawHeight = a_height;
    m_width = a_width;
    m_height = a_height;
    UpdateLetterboxBars();

    // Desktop render defaults. Sound Shapes is a PS3 title and does not
    // import OpenGL, so this is not a literal retail call sequence.
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    SetLineWidth(kDefaultLineWidth);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // Retail's device init clears colour AND depth once, glClear(0x4100).
    // Per-frame clears are colour-only (see BeginFrame).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

void Renderer::UpdateLetterboxBars() {
    if (m_rawWidth <= 0 || m_rawHeight <= 0) return;
    if (!SystemConfig::Get().letterbox) {
        // The pre-letterbox desktop mode used the complete host viewport.
        // Keep all logical screen-size and projection consumers in agreement
        // with the GL viewport when the bars are disabled.
        m_width = m_rawWidth;
        m_height = m_rawHeight;
        m_barX = 0;
        m_barY = 0;
        return;
    }
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

void Renderer::DrawLetterboxBars() {
    const int rightBar = m_rawWidth - (m_barX + m_width);
    const int topBar = m_rawHeight - (m_barY + m_height);
    if (m_barX == 0 && rightBar == 0 && m_barY == 0 && topBar == 0) return;

    GLint previousMatrixMode = GL_MODELVIEW;
    glGetIntegerv(GL_MATRIX_MODE, &previousMatrixMode);
    glPushAttrib(GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT);
    // Attribute stacks preserve the viewport and depth/current state, but
    // OpenGL matrix contents have their own stacks. Restore both so the
    // pixel-space bar projection cannot leak into the next frame's first
    // draw command.
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, m_rawWidth, m_rawHeight);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)(m_rawWidth - 1), 0.0, (double)(m_rawHeight - 1), -1.0, 1.0);
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

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glMatrixMode(static_cast<GLenum>(previousMatrixMode));
}

void Renderer::Shutdown() {
    m_distanceTextureShader.Destroy();
    m_distanceTextureShaderReady = false;
    m_textureTransShader.Destroy();
    m_textureTransShaderReady = false;
    m_textureTransFastShader.Destroy();
    m_textureTransFastShaderReady = false;
    m_selectionShader.Destroy();
    m_selectionShaderReady = false;
    m_textureSelectionShader.Destroy();
    m_textureSelectionShaderReady = false;
    m_textureDistSelectionShader.Destroy();
    m_textureDistSelectionShaderReady = false;
    m_paletteOverlayShader.Destroy();
    m_paletteOverlayShaderReady = false;
    m_glyphMaskShader.Destroy();
    m_glyphMaskShaderReady = false;
    m_fireGroundShader.Destroy();
    m_fireGroundShaderReady = false;
    m_defaultTransShader.Destroy();
    m_defaultTransShaderReady = false;
    m_window = nullptr;
}

void Renderer::BeginFrame() {
    m_loadingBarProgressShapeExpected = false;
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
void Renderer::SaveScreenshot(const std::filesystem::path& a_path) const {
    std::vector<unsigned char> pixels((size_t)m_rawWidth * m_rawHeight * 3);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, m_rawWidth, m_rawHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream f(a_path, std::ios::binary | std::ios::trunc);
    if (!f) return;
    f << "P6\n" << m_rawWidth << " " << m_rawHeight << "\n255\n";
    // glReadPixels rows are bottom-to-top; PPM wants top-to-bottom.
    for (int y = m_rawHeight - 1; y >= 0; --y) {
        f.write(reinterpret_cast<const char*>(pixels.data() + (size_t)y * m_rawWidth * 3),
                (std::streamsize)m_rawWidth * 3);
    }
}

unsigned int Renderer::LoadTexture(const std::string& a_path) {
    return LoadTexture(a_path, a_path);
}

unsigned int Renderer::LoadTexture(const std::string& a_path, const std::string& a_cacheKey) {
    auto it = m_texturesByPath.find(a_cacheKey);
    if (it != m_texturesByPath.end()) return it->second;

    Texture tex;
    if (!Texture::LoadFromFile(a_path, tex)) return 0;
    tex.sourcePath = a_path;

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

const std::string& Renderer::GetTextureSourcePath(unsigned int a_handle) const {
    static const std::string empty;
    const auto it = m_texturesByHandle.find(a_handle);
    return it == m_texturesByHandle.end() ? empty : it->second.sourcePath;
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

void Renderer::SetLineWidth(float a_width) {
    glLineWidth(a_width);
}


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
    const GLenum compare = VideoParamsToGLFunc(a_func);
    glStencilFunc(compare, a_ref, a_mask);
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

void Renderer::ApplyProjection() const {
    const float viewportAspect = GetAspect();

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
            const float fullWidth = static_cast<float>(
                static_cast<double>(m_cameraZ) * kWorldViewExtentBase * 4.0);
            const float fullHeight = fullWidth / viewportAspect;
            const float halfW = fullWidth * 0.5f;
            const float halfH = fullHeight * 0.5f;
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
            glOrtho((double)kGameBoardMinX, (double)kGameBoardMaxX,
                    (double)kGameBoardMinY, (double)kGameBoardMaxY, -1.0, 1.0);
            break;
        }
        case ViewMode::UnitOrtho: {
            glOrtho(0.0, 1.0, 0.0, 1.0, -1.0, 1.0);
            break;
        }
        case ViewMode::CustomOrtho: {
            glOrtho((double)m_orthoLeft, (double)m_orthoRight,
                    (double)m_orthoBottom, (double)m_orthoTop, -1.0, 1.0);
            break;
        }
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
            glOrtho(-(double)halfW, (double)halfW, -(double)halfH, (double)halfH, -1.0, 1.0);
            break;
        }
    }

    applyRetailFrameViewScale();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::DrawTexturedQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                                 float a_x2, float a_y2, float a_x3, float a_y3,
                                 unsigned int a_textureHandle, float a_z,
                                 bool a_distanceTexture, float a_distanceDiscardLevel,
                                 float a_distanceSmoothLevel, int a_textureShaderType,
                                 float a_uvScaleU, float a_uvScaleV,
                                 unsigned int a_primitiveMode) {
    if (a_textureHandle == 0) return;

    // Retail screen-space texture helpers express positions and dimensions in
    // logical screen units (derived from texture pixels and
    // Video.getScreenWidth/Height). The retail render-context matrix converts
    // those units before TextureTrans consumes them. Desktop's aspect-ortho
    // projection is already normalized, so apply the same conversion to
    // screen-space texture quads here. World-space textures use another view
    // mode and are left untouched. The loading progress bar is a special case:
    // its X coordinates are already aspect units and its Y coordinates encode
    // progress, so it keeps its dedicated Y mapping below.
    const std::string& texturePath = GetTextureSourcePath(a_textureHandle);
    // Texture paths retain their resolved filesystem spelling. Windows paths
    // use backslashes, while these renderer classifications use archive-style
    // forward slashes. Normalize only the local classification copy so the
    // same loading and distance-field behavior applies on every host.
    std::string normalizedTexturePath = texturePath;
    std::replace(normalizedTexturePath.begin(), normalizedTexturePath.end(), '\\', '/');
    const bool loadingScreenTexture =
        normalizedTexturePath.find("/loadingScreens/") != std::string::npos;
    // BitmapFont submits extFont icons after it has already laid them out in
    // font units and transformed them with the active DrawList matrix. They
    // are not screen-pixel-sized helper textures, so applying the viewport
    // texture factor here collapses a 1-em button glyph to sub-pixel size.
    const bool fontSpecialTexture =
        normalizedTexturePath.find("/extFont/") != std::string::npos;
    const bool loadingBarTexture = loadingScreenTexture &&
        normalizedTexturePath.size() >= std::strlen("/loadingBar.png") &&
        normalizedTexturePath.compare(
            normalizedTexturePath.size() - std::strlen("/loadingBar.png"),
            std::strlen("/loadingBar.png"), "/loadingBar.png") == 0;
    if (m_viewMode == ViewMode::AspectRatioOrtho && !loadingBarTexture &&
        !fontSpecialTexture) {
        const VideoState& video = GetEngineContext().video;
        const float logicalHeight = VideoState::RetailScreenExtent(video.numCellsY);
        if (logicalHeight > 0.0001f) {
            // The 0.45 factor is measured from the retail loading-screen
            // panel (which occupies about 47-48% of the active viewport),
            // against the 0.5 half-height of aspect-ortho. This is the best
            // shared conversion currently supported by the captures; exact
            // runtime values for retail's retained render context remain an
            // open parity check.
            const float screenUnitScale = 0.45f / logicalHeight;
            // Texture quads are commonly submitted in a local matrix after
            // their screen position has already been translated. Convert the
            // quad's extent around its own center so that translation remains
            // in the script's normalized layout units. Centering also leaves
            // the centered loading illustration unchanged apart from scale.
            const float centerX = (a_x0 + a_x1 + a_x2 + a_x3) * 0.25f;
            const float centerY = (a_y0 + a_y1 + a_y2 + a_y3) * 0.25f;
            a_x0 = centerX + (a_x0 - centerX) * screenUnitScale;
            a_x1 = centerX + (a_x1 - centerX) * screenUnitScale;
            a_x2 = centerX + (a_x2 - centerX) * screenUnitScale;
            a_x3 = centerX + (a_x3 - centerX) * screenUnitScale;
            a_y0 = centerY + (a_y0 - centerY) * screenUnitScale;
            a_y1 = centerY + (a_y1 - centerY) * screenUnitScale;
            a_y2 = centerY + (a_y2 - centerY) * screenUnitScale;
            a_y3 = centerY + (a_y3 - centerY) * screenUnitScale;
            m_loadingBarProgressShapeExpected = false;
        }
    } else if (m_viewMode == ViewMode::AspectRatioOrtho && loadingBarTexture) {
        const VideoState& video = GetEngineContext().video;
        const float logicalHeight = VideoState::RetailScreenExtent(video.numCellsY);
        const float minY = std::min(std::min(a_y0, a_y1), std::min(a_y2, a_y3));
        const float maxY = std::max(std::max(a_y0, a_y1), std::max(a_y2, a_y3));
        const float spanY = maxY - minY;
        if (logicalHeight > 0.0001f && spanY > 0.25f) {
            const float normalizedSpan = spanY / logicalHeight;
            auto normalizeBarY = [minY, spanY, normalizedSpan](float y) {
                return -0.5f + ((y - minY) / spanY) * normalizedSpan;
            };
            a_y0 = normalizeBarY(a_y0);
            a_y1 = normalizeBarY(a_y1);
            a_y2 = normalizeBarY(a_y2);
            a_y3 = normalizeBarY(a_y3);
            m_loadingBarProgressShapeExpected = true;
        } else {
            m_loadingBarProgressShapeExpected = false;
        }
    } else {
        m_loadingBarProgressShapeExpected = false;
    }

    // Native textured submission chooses its render config from TEXTURE+0x48.
    // Direct renderer callers omit the selector, so resolve it from the same
    // handle instead of silently defaulting every texture to TextureTrans.
    const int textureShaderType = a_textureShaderType < 0
        ? GetTextureShaderType(a_textureHandle) : a_textureShaderType;
    // Retail's script/API thresholds are passed through unchanged for normal
    // textures. The shipped NOTE_* distance fields render visibly too bold on
    // the desktop backend at those values, so retain the measured note-only
    // silhouette calibration. Apply it after selector resolution so both
    // TextureDist variants receive the same pair.
    const float textureDistanceBias =
        NoteTextureDistanceBias(normalizedTexturePath, textureShaderType);
    const float effectiveDiscardLevel = a_distanceDiscardLevel + textureDistanceBias;
    const float effectiveSmoothLevel = a_distanceSmoothLevel + textureDistanceBias;

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
        if (traceCount++ < 100000) {
            int tw = 0, th = 0;
            GetTextureDimensions(a_textureHandle, tw, th);
            GLint viewport[4] = {0, 0, 0, 0};
            GLfloat projection[16] = {};
            GLfloat modelview[16] = {};
            glGetIntegerv(GL_VIEWPORT, viewport);
            glGetFloatv(GL_PROJECTION_MATRIX, projection);
            glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
            std::fprintf(stderr,
                "[ui-quad] tex=%u %dx%d selector=%d program=%s primitive=%u distAttrs=%d thresholds=(%.6f,%.6f) effective=(%.6f,%.6f) view=%d aspect=%.5f scale=%.5f viewport=(%d,%d,%d,%d) projection=(%.6f,%.6f,%.6f,%.6f) modelview=(%.6f,%.6f,%.6f) bounds=(%.4f,%.4f)-(%.4f,%.4f) z=%.6f camz=%.6f depth=%d/%d stencil=%d:%d color=(%.3f,%.3f,%.3f,%.3f)\n",
                a_textureHandle, tw, th, textureShaderType,
                textureShaderType == 8 ? "TextureDist" :
                    (textureShaderType == 0 ? "TextureTrans" :
                     (textureShaderType == 1 ? "TextureTransFast" :
                      (textureShaderType == 2 ? "TextureTransFastNB" : "fallback"))),
                a_primitiveMode ? a_primitiveMode : (unsigned int)GL_QUADS,
                a_distanceTexture,
                a_distanceDiscardLevel, a_distanceSmoothLevel,
                effectiveDiscardLevel, effectiveSmoothLevel,
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
    const bool useTextureSelectionShader =
        GetDrawContext().selectionFragmentProgram && textureShaderType == 0 &&
        m_textureSelectionShaderReady;
    const bool useTextureDistSelectionShader =
        GetDrawContext().selectionFragmentProgram && textureShaderType == 8 &&
        m_textureDistSelectionShaderReady;
    const bool useDistanceShader = !useTextureDistSelectionShader &&
        textureShaderType == 8 && m_distanceTextureShaderReady;
    // Retail texture selectors 0, 1 and 2 map to TextureTrans,
    // TextureTransFast and TextureTransFastNB. Keep unsupported palette/grey
    // selectors on the fixed-function fallback until their programs exist.
    const bool useTextureTransShader = !useDistanceShader &&
                                       !useTextureSelectionShader && textureShaderType == 0 &&
                                       m_textureTransShaderReady;
    const bool useTextureTransFastShader = !useDistanceShader &&
        (textureShaderType == 1 || textureShaderType == 2) &&
        m_textureTransFastShaderReady;
    const bool knownQuadConfig = textureShaderType == 0 || textureShaderType == 1 ||
                                 textureShaderType == 2 || textureShaderType == 8;
    const bool hasShaderProgram = useTextureSelectionShader ||
                                  useTextureDistSelectionShader || useDistanceShader || useTextureTransShader ||
                                  useTextureTransFastShader;
    if (knownQuadConfig) {
        ApplyRenderConfigBlendState(textureShaderType != 2);
    }
    if (hasShaderProgram) {
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        if (useTextureSelectionShader || useTextureDistSelectionShader) {
            float selectionColour[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            const VideoState& video = GetEngineContext().video;
            const char* colourName = video.selectionPulseEnabled
                ? "edit_selected" : "screenClear";
            GetNamedColourRGB(colourName, selectionColour[0],
                              selectionColour[1], selectionColour[2]);
            Shader& shader = useTextureSelectionShader
                ? m_textureSelectionShader : m_textureDistSelectionShader;
            shader.Use();
            glUniform1i(shader.Uniform("u_texture"), 0);
            glUniform4fv(shader.Uniform("u_selectColour"), 1, selectionColour);
            glUniform1f(shader.Uniform("u_pulsePerc"), video.selectionPulsePerc);
            glUniform1f(shader.Uniform("u_alphaPerc"), video.selectionAlphaPerc);
            if (useTextureDistSelectionShader) {
                float sizeU = 1.0f, sizeV = 1.0f;
                GetTextureUVExtent(a_textureHandle, sizeU, sizeV);
                glUniform2f(shader.Uniform("u_size"), sizeU, sizeV);
                glUniform4fv(shader.Uniform("u_color"), 1, m_colour);
                    glUniform1f(shader.Uniform("u_discardLevel"),
                                effectiveDiscardLevel);
                    glUniform1f(shader.Uniform("u_smoothLevel"),
                                effectiveSmoothLevel);
            }
            glDisable(GL_ALPHA_TEST);
        } else if (useDistanceShader) {
            m_distanceTextureShader.Use();
            glUniform1i(m_distanceTextureShader.Uniform("u_texture"), 0);
            glUniform1f(m_distanceTextureShader.Uniform("u_discardLevel"),
                        effectiveDiscardLevel);
            glUniform1f(m_distanceTextureShader.Uniform("u_smoothLevel"),
                        effectiveSmoothLevel);
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

    std::vector<float> loadingProgressXY;
    const float* xy = a_xy;
    if (m_loadingBarProgressShapeExpected) {
        m_loadingBarProgressShapeExpected = false;
        if (a_count == 4) {
            float minY = a_xy[1], maxY = a_xy[1];
            for (int i = 1; i < a_count; ++i) {
                minY = std::min(minY, a_xy[i * 2 + 1]); maxY = std::max(maxY, a_xy[i * 2 + 1]);
            }
            const float logicalHeight = VideoState::RetailScreenExtent(
                GetEngineContext().video.numCellsY);
            // The remaining gray fill narrows and translates horizontally as
            // loading advances. Its width therefore stops spanning the view
            // well before its height changes. Keep the loading-bar Y mapping
            // independent of that animated X extent; coupling them makes the
            // fill jump back to its unconverted (~0.91 screen-unit) height,
            // briefly drawing a large gray panel over the loading artwork.
            if (logicalHeight > 0.0001f && maxY - minY > 0.25f &&
                minY < -0.35f && maxY > 0.25f) {
                const float spanY = maxY - minY;
                const float normalizedSpan = spanY / logicalHeight;
                loadingProgressXY.assign(a_xy, a_xy + a_count * 2);
                for (int i = 0; i < a_count; ++i) {
                    loadingProgressXY[i * 2 + 1] = -0.5f +
                        ((a_xy[i * 2 + 1] - minY) / spanY) * normalizedSpan;
                }
                xy = loadingProgressXY.data();
            }
        }
    }

    if (std::getenv("GE3_TRACE_UI_QUADS")) {
        static unsigned int traceCount = 0;
        if (traceCount++ < 100000) {
            float minX = xy[0], maxX = xy[0], minY = xy[1], maxY = xy[1];
            for (int i = 1; i < a_count; ++i) {
                minX = std::min(minX, xy[i * 2]); maxX = std::max(maxX, xy[i * 2]);
                minY = std::min(minY, xy[i * 2 + 1]); maxY = std::max(maxY, xy[i * 2 + 1]);
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
        glVertex3f(xy[i * 2 + 0], xy[i * 2 + 1], a_z);
    }
    glEnd();
}

void Renderer::DrawPolygonDefaultTrans(unsigned int a_glPrimitiveMode,
                                       const float* a_xy, int a_count, float a_z) {
    if (!m_defaultTransShaderReady || a_count <= 0) {
        DrawPolygon(a_glPrimitiveMode, a_xy, a_count, a_z);
        return;
    }

    ApplyProjection();
    glDisable(GL_TEXTURE_2D);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    GLfloat projection[16] = {}, modelview[16] = {}, matrix[16] = {};
    glGetFloatv(GL_PROJECTION_MATRIX, projection);
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    for (int column = 0; column < 4; ++column) {
        for (int row = 0; row < 4; ++row) {
            float value = 0.0f;
            for (int k = 0; k < 4; ++k)
                value += projection[k * 4 + row] * modelview[column * 4 + k];
            matrix[column * 4 + row] = value;
        }
    }
    GLint previousProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    m_defaultTransShader.Use();
    glUniformMatrix4fv(m_defaultTransShader.Uniform("uViewWorldProjection"),
                       1, GL_FALSE, matrix);

    glBegin(a_glPrimitiveMode);
    for (int i = 0; i < a_count; ++i)
        glVertex3f(a_xy[i * 2], a_xy[i * 2 + 1], a_z);
    glEnd();

    glUseProgram(static_cast<GLuint>(previousProgram));
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

    GLint previousProgram = 0;
    const int textureShaderType = a_textureShaderType < 0
        ? GetTextureShaderType(a_textureHandle) : a_textureShaderType;
    const float textureDistanceBias = NoteTextureDistanceBias(
        GetTextureSourcePath(a_textureHandle), textureShaderType);
    const float effectiveDiscardLevel =
        a_distanceDiscardLevel + textureDistanceBias;
    const float effectiveSmoothLevel =
        a_distanceSmoothLevel + textureDistanceBias;
    const bool selectionProgram = GetDrawContext().selectionFragmentProgram;
    const bool useSelectionShader = !textured && selectionProgram &&
                                    m_selectionShaderReady;
    const bool useTextureSelectionShader = textured && selectionProgram &&
        textureShaderType == 0 && m_textureSelectionShaderReady;
    const bool useTextureDistSelectionShader = textured && selectionProgram &&
        textureShaderType == 8 && m_textureDistSelectionShaderReady;
    const bool useDistanceShader = textured && !useTextureDistSelectionShader &&
                                   textureShaderType == 8 &&
                                   m_distanceTextureShaderReady;
    const bool useTextureTransShader = textured && !useDistanceShader &&
                                       !useTextureSelectionShader &&
                                       textureShaderType == 0 &&
                                       m_textureTransShaderReady;
    // Indexed foliage (e.g. SafeVine's selector-5 Palette4bTrans shape)
    // reaches this backend after its attached palette has already been
    // expanded into the RGBA texture by BindTextureWithUVScale. Reuse the
    // texture-only fragment shader: the sampled texel is already the final
    // palette colour and must not inherit the unrelated current draw colour
    // left by the preceding solid stem. Retail selects Palette4bTrans for
    // selector 5; its native Cg program is still awaiting direct decode, so
    // this is the closest evidence-backed desktop mapping for that route.
    const bool useTextureTransFastShader = textured && !useDistanceShader &&
                                           (textureShaderType == 1 ||
                                            textureShaderType == 2 ||
                                            textureShaderType == 5) &&
                                           m_textureTransFastShaderReady;
    if (useSelectionShader || useTextureSelectionShader ||
        useTextureDistSelectionShader || useDistanceShader ||
        useTextureTransShader || useTextureTransFastShader) {
        ApplyRenderConfigBlendState(useSelectionShader || useTextureSelectionShader ||
                                    textureShaderType != 2);
        glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
        if (useSelectionShader || useTextureSelectionShader ||
            useTextureDistSelectionShader) {
            float selectionColour[4] = {0.0f, 0.0f, 0.0f, 1.0f};
            const VideoState& video = GetEngineContext().video;
            const char* colourName = video.selectionPulseEnabled
                ? "edit_selected" : "screenClear";
            GetNamedColourRGB(colourName, selectionColour[0],
                              selectionColour[1], selectionColour[2]);
            Shader& shader = useSelectionShader ? m_selectionShader :
                (useTextureSelectionShader ? m_textureSelectionShader
                                           : m_textureDistSelectionShader);
            shader.Use();
            if (useTextureSelectionShader || useTextureDistSelectionShader)
                glUniform1i(shader.Uniform("u_texture"), 0);
            glUniform4fv(shader.Uniform("u_selectColour"), 1, selectionColour);
            glUniform1f(shader.Uniform("u_pulsePerc"), video.selectionPulsePerc);
            glUniform1f(shader.Uniform("u_alphaPerc"), video.selectionAlphaPerc);
            if (useTextureDistSelectionShader) {
                float sizeU = 1.0f, sizeV = 1.0f;
                GetTextureUVExtent(a_textureHandle, sizeU, sizeV);
                glUniform2f(shader.Uniform("u_size"), sizeU, sizeV);
                glUniform4fv(shader.Uniform("u_color"), 1, m_colour);
                glUniform1f(shader.Uniform("u_discardLevel"),
                            effectiveDiscardLevel);
                glUniform1f(shader.Uniform("u_smoothLevel"),
                            effectiveSmoothLevel);
            }
            // Selection variants perform their own source-alpha kill in the
            // fragment program, before alphaPerc is applied.
            glDisable(GL_ALPHA_TEST);
        } else if (useDistanceShader) {
            m_distanceTextureShader.Use();
            glUniform1i(m_distanceTextureShader.Uniform("u_texture"), 0);
            glUniform1f(m_distanceTextureShader.Uniform("u_discardLevel"),
                        effectiveDiscardLevel);
            glUniform1f(m_distanceTextureShader.Uniform("u_smoothLevel"),
                        effectiveSmoothLevel);
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

    if (useSelectionShader || useTextureSelectionShader ||
        useTextureDistSelectionShader || useDistanceShader ||
        useTextureTransShader || useTextureTransFastShader)
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
    (void)a_z; // retail's clear shape uses fixed clip-space positions.

    GLint previousMatrixMode = GL_MODELVIEW;
    GLint previousProgram = 0;
    GLint previousActiveTexture = GL_TEXTURE0;
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
    GLboolean previousStencilEnabled = GL_FALSE;
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
    previousStencilEnabled = glIsEnabled(GL_STENCIL_TEST);
    previousCullEnabled = glIsEnabled(GL_CULL_FACE);
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFace);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    // The retained clear shape is a single oversized CCW triangle. The
    // desktop renderer can enter this path with GL_FRONT culling left by a
    // preceding draw-list attribute state; that would cull the only primitive
    // and leave the freshly-cleared frame black (including fade transitions).
    // Retail submits this built-in clear shape through its own render config,
    // independent of an arbitrary caller's face-culling selection.
    glDisable(GL_CULL_FACE);
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

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(static_cast<GLenum>(previousDepthFunction));
    glDepthMask(previousDepthWrite);
    if (previousStencilEnabled) glEnable(GL_STENCIL_TEST);
    else glDisable(GL_STENCIL_TEST);
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
        // The bitmap-font path takes two style thresholds. Heavy/bold
        // FontHelper presets produce them in descending order after the
        // retail setter conversion; feeding that pair directly to this
        // host glyph shader makes its signed coverage span erase every
        // glyph. Normalize only the glyph mask inputs to a valid distance
        // interval. Generic TextureDist quads retain their recorded order.
        glUniform1f(m_glyphMaskShader.Uniform("u_discardLevel"), std::min(a_edge, a_width));
        glUniform1f(m_glyphMaskShader.Uniform("u_smoothLevel"), std::max(a_edge, a_width));
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
