#include "renderer.h"
#include "../core/engine_context.h"
#include <GL/glu.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>

// Fixed-function OpenGL renderer -- a direct port of retail Flowers of
// Error's real GL backend (the cluster 0x00420c90-0x00421d90). See
// renderer.h's header comment and reveng_pc/RETAIL_GL_GROUND_TRUTH.md.
//
// TEXTURE ORIENTATION CONVENTION (deliberate, documented divergence that is
// pixel-identical): retail stores textures TOP-DOWN and compensates by
// emitting flipped V coordinates in its quad draw (FUN_00421d90 emits
// vMax,vMax,0,0 across the four corners). This engine stores textures
// BOTTOM-UP (Texture::LoadFromFile uses stbi_set_flip_vertically_on_load)
// and emits straight V coordinates. The two conventions compose to the same
// on-screen result; adopting retail's would require re-deriving
// bitmap_font.cpp's atlas V math and animation_bindings' frame uploads for
// zero visible difference, so the engine's convention is kept.
//
// What is NOT cosmetic, and is now matched: retail's power-of-two texture
// padding, and the uMax/vMax UV scaling that compensates for it. Because
// retail never sets GL_TEXTURE_WRAP_S/T (leaving the GL default GL_REPEAT),
// UVs outside [0,1] tile across the PADDED extent -- padding included in the
// repeat period. Every UV this renderer emits is therefore scaled by the
// texture's uMax/vMax.



namespace {

constexpr float kPi = 3.1415927f; // retail's own literal (0x004edba4)

// Retail's world-view modelview transform (FUN_004213f0, exact):
//
//   glRotatef(pitch, 1, 0, 0);
//   angleRad = (pitch - 90.0f) * PI / 180.0f;
//   s = sinf(angleRad);  if (s == 0.0f) s = 0.001f;   // guard, 0x3a83126f
//   yOff = fabsf(camZ) * cosf(angleRad) / s;
//   glTranslatef(-camX, yOff - camY, -camZ);
//
// The constants 90.0f and 180.0f are literals at 0x004edc94/0x004edc90, the
// sin==0 guard compares against 0.0f at 0x004ed438 and substitutes 0.001f.
// FUN_004da7ee (applied to camZ) is fabs -- its non-special-case path is a
// plain `AND 0x7fffffff` on the double's high dword.
//
// Sanity check at pitch == 0, which is what all of FoE's own content uses:
// angleRad = -PI/2, sin = -1 (no guard), cos ~= 0, so yOff ~= 0 and the
// transform reduces to the expected glTranslatef(-camX, -camY, -camZ).
// Currently unreferenced: the world view is orthographic and camera-centred
// (see ApplyProjection's WorldPerspective case), so there is no perspective
// eye translation to offset. Kept because it is a faithful transcription of
// FoE's FUN_004213f0 and is what the perspective form would need if the RSX
// projection path turns out to be perspective after all.
float CameraYOffset(float a_pitchDeg, float a_camZ) {
    const float angleRad = (a_pitchDeg - 90.0f) * kPi / 180.0f;
    float s = std::sin(angleRad);
    if (s == 0.0f) s = 0.001f;
    return std::fabs(a_camZ) * std::cos(angleRad) / s;
}

} // namespace



bool Renderer::Init(int a_width, int a_height, const char* a_title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }

    // COMPATIBILITY profile, not core: retail's entire renderer is
    // fixed-function immediate mode (glBegin/glEnd, the GL matrix stack,
    // gluPerspective/gluOrtho2D), none of which exists in a core profile.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    // Retail uses no stencil at all; requested only because Sound Shapes
    // content in this tree still drives the stencil path.
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // GE3_HEADLESS=1 (see platform/headless_harness.h): create the window
    // hidden instead of shown. The real GPU driver and a real GL context are
    // still used -- only the window never appears on the host desktop and
    // never takes input focus, which is what a scripted/screenshot run
    // wants. SDL's "offscreen" video driver would avoid the window entirely
    // but hands back a context that renders nothing here (every captured
    // frame came out pure black), so a hidden real window is the working
    // headless path.
    Uint32 visibility = std::getenv("GE3_HEADLESS") ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN;
    m_window = SDL_CreateWindow(a_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 a_width, a_height, SDL_WINDOW_OPENGL | visibility | SDL_WINDOW_RESIZABLE);
    if (!m_window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return false;
    }

    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::fprintf(stderr, "SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_SetSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::fprintf(stderr, "glewInit failed: %s\n", glewGetErrorString(glewErr));
        return false;
    }
    glGetError(); // glew often leaves a spurious GL_INVALID_ENUM on init

    m_rawWidth = a_width;
    m_rawHeight = a_height;
    m_width = a_width;
    m_height = a_height;
    UpdateLetterboxBars();

    // RETAIL DEVICE INIT (exact order and values, retail 0x00421160 region).
    // Retail has exactly one call site each for glHint, glLineWidth,
    // glBlendFunc and glDepthMask in the entire binary -- this state is set
    // once here and never touched again per-frame or per-draw, except via
    // the two script-exposed depth wrappers (SetDepthTest/SetDepthWrite).
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    // RETAIL MATCH (retail FUN_00421a10): line width is NOT passed straight
    // to glLineWidth -- it is scaled against an 800px-wide reference:
    //     glLineWidth((deviceWidth / 800.0f) * requestedWidth);
    // At the default 800x600 the factor is exactly 1.0, which is why a plain
    // glLineWidth(2.0) matched by coincidence; at any other resolution it
    // did not. PekkoAndJon/Main.gm calls Video.setLineWidth() with no
    // arguments, i.e. "reset to default", and that default is 2.0.
    SetLineWidth(kDefaultLineWidth);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // Retail's device init clears colour AND depth once, glClear(0x4100).
    // Per-frame clears are colour-only (see BeginFrame).
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // This default (before any script calls DrawList.setAttrDepthTest) does
    // not matter much in practice -- see SetDepthTest()'s header comment:
    // Sound Shapes' real dispatcher drives glDepthFunc itself via that
    // opcode (GL_EQUAL/GL_ALWAYS), unlike Flowers-of-Error's binary (which
    // never touches glDepthFunc at all, hence the stale comment this used
    // to carry). GL_LESS is just a harmless placeholder until the first
    // real setAttrDepthTest call.
    glDepthFunc(GL_LESS);

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
    if (m_window) SDL_SetWindowSize(m_window, a_width, a_height);
}

void Renderer::OnWindowResized(int a_width, int a_height) {
    if (a_width <= 0 || a_height <= 0) return;
    m_rawWidth = a_width;
    m_rawHeight = a_height;
    UpdateLetterboxBars();
}

// Retail's FUN_00421160: the logical content size is fitted inside the raw
// window at the target aspect ratio, and the leftover margin on each axis
// becomes the black bar thickness:
//     barX = (rawW - logicalW) / 2,  barY = (rawH - logicalH) / 2
//
// Unlike this engine's previous approach, the VIEWPORT IS NOT NARROWED.
// Retail's binary contains exactly one glViewport call site, inside the
// letterbox function, and it sets the viewport to the FULL raw window --
// the bars are drawn as real geometry on top (DrawLetterboxBars) rather than
// being leftover space outside a shrunken viewport.
void Renderer::UpdateLetterboxBars() {
    if (m_rawWidth <= 0 || m_rawHeight <= 0) return;
    float targetAspect = GetEngineContext().video.aspectRatio;
    if (targetAspect <= 0.0001f) targetAspect = 16.0f / 9.0f;
    const float rawAspect = (float)m_rawWidth / (float)m_rawHeight;

    int logicalW, logicalH;
    if (rawAspect > targetAspect) {
        logicalH = m_rawHeight;
        logicalW = (int)(m_rawHeight * targetAspect + 0.5f);
    } else {
        logicalW = m_rawWidth;
        logicalH = (int)(m_rawWidth / targetAspect + 0.5f);
    }
    if (logicalW < 1) logicalW = 1;
    if (logicalH < 1) logicalH = 1;

    m_width = logicalW;
    m_height = logicalH;
    m_barX = (m_rawWidth - logicalW) / 2;
    m_barY = (m_rawHeight - logicalH) / 2;
}

// Retail's FUN_00420c90 (filled-bar mode). Pixel-space ortho over the whole
// raw window, depth test off, solid black, wrapped in glPushAttrib/
// glPopAttrib with GL_CURRENT_BIT | GL_DEPTH_BUFFER_BIT | GL_VIEWPORT_BIT
// (0x901).
void Renderer::DrawLetterboxBars() {
    if (m_barX == 0 && m_barY == 0) return;

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
    if (m_barY > 0) {
        const float b = (float)m_barY;
        // bottom bar
        glVertex2f(0.0f, 0.0f);  glVertex2f(w, 0.0f);  glVertex2f(w, b);      glVertex2f(0.0f, b);
        // top bar
        glVertex2f(0.0f, h - b); glVertex2f(w, h - b); glVertex2f(w, h);      glVertex2f(0.0f, h);
    }
    if (m_barX > 0) {
        const float b = (float)m_barX;
        // left bar
        glVertex2f(0.0f, 0.0f);  glVertex2f(b, 0.0f);  glVertex2f(b, h);      glVertex2f(0.0f, h);
        // right bar
        glVertex2f(w - b, 0.0f); glVertex2f(w, 0.0f);  glVertex2f(w, h);      glVertex2f(w - b, h);
    }
    glEnd();

    glPopAttrib();
}

void Renderer::Shutdown() {
    if (m_glContext) SDL_GL_DeleteContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
}

void Renderer::BeginFrame() {
    UpdateLetterboxBars();
    // RETAIL MATCH (retail FUN_00420c40, the per-frame viewport+clear):
    //     glViewport(barX, barY, rawW - 2*barX, rawH - 2*barY);
    //     glClearColor(0, 0, 0, 0);
    //     glClear(GL_COLOR_BUFFER_BIT);          // 0x4000, colour ONLY
    //
    // So retail DOES narrow the viewport to the centred content rect. An
    // earlier revision of this port removed the narrowing entirely, on the
    // strength of "the binary contains exactly one glViewport call site, in
    // the letterbox function, and it uses the full window". That was wrong:
    // this function did not exist as a function at the time (Ghidra had not
    // created it), so its glViewport call was invisible. There are two call
    // sites -- this one (content rect, every frame) and the letterbox
    // function's (full window, temporarily, inside glPushAttrib).
    glViewport(m_barX, m_barY,
               m_rawWidth - 2 * m_barX, m_rawHeight - 2 * m_barY);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    // Colour only. Retail's per-frame clear does NOT touch depth: the depth
    // buffer is cleared once at device init (glClear(0x4100) = colour|depth)
    // and thereafter only if a script calls Video.clearDepthBuffer (retail
    // FUN_00421970, glClear(0x100)), which FoE's content never does.
    glClear(GL_COLOR_BUFFER_BIT);
    // Depth and stencil ARE cleared per frame here, which the retail-FoE
    // citation above does not cover. Both buffers are consumed by
    // compare-against-existing-contents passes in this tree (Sound Shapes'
    // setAttrDepthTest/setAttrStencil paths), and a buffer that is only
    // cleared once at device init carries the previous frame's contents
    // forever -- every such compare then tests against stale data. Leaving
    // them uncleared is also what made the depth compare unrecoverable:
    // stencil sat at 0 everywhere while scripts tested GL_EQUAL against
    // ref 10, discarding every fragment.
    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    // Colour writes are forced back ON at the top of every frame, and the
    // shared DrawContext's mirror of that state is resynced with them in
    // main.cpp's frame loop. The colour mask is a within-frame stencil-pass
    // tool (SetColourMask's own comment), but real content leaves it
    // unbalanced: a full-game run logs 2213 mask-on calls against 4278
    // mask-off ones, and the frame ends with writes still disabled. Since
    // nothing else ever re-enables them, ONE unbalanced pass permanently
    // blanked every subsequent frame -- the game kept submitting its full
    // draw load into a framebuffer that could not accept colour.
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    // NO per-frame depth-state reset. This previously forced
    //     glEnable(GL_DEPTH_TEST); glDepthMask(GL_TRUE);
    // every frame, which was invented, not ported -- and it silently stomped
    // the script's own state every frame.
    //
    // Verified against the binary: the ONLY glDepthMask call site in the
    // whole executable is retail's Video.setAttrDepthWrite wrapper
    // (FUN_00421e60: `glDepthMask(b != 0);`), and the only depth-test
    // enable/disable outside one-time device init is its
    // Video.setAttrDepthTest wrapper (FUN_00421e40: `if (b)
    // glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);`). Retail has
    // no frame-loop depth reset at all -- depth state persists across frames
    // exactly as the last script call left it.
    //
    // This matters for FoE specifically: PekkoAndJon/Main.gm's postDraw()
    // runs `pushAttributes(); setAttrDepthTest(0); setAttrDepthWrite(0);`
    // every frame and never calls popAttributes(), and nothing anywhere in
    // the corpus ever sets either back to 1. So from the second frame
    // onward, retail draws with depth test AND depth write OFF permanently,
    // and the 8-layer alpha-blended kaleidoscope backdrop composites purely
    // back-to-front with no depth interaction. Forcing them back on each
    // frame made those layers depth-fight each other and collapse into a
    // flat wash.
}

void Renderer::EndFrame() {
    // Bars go on last, over the finished scene -- retail draws them as
    // geometry rather than reserving space for them.
    DrawLetterboxBars();
    SDL_GL_SwapWindow(m_window);
}

// Only meaningful in WorldPerspective mode -- the other three modes have
// fixed extents that do not depend on the camera.
float Renderer::GetWorldHalfHeight() const {
    // Visible world half-height at the Z=0 plane under
    // gluPerspective(53.13010, ...): halfH = tan(fovY/2) * distance, and
    // tan(fovY/2) is exactly 0.5 (see kTanHalfFovY). The camera sits at
    // +camZ looking toward -Z, so the distance to the Z=0 plane is |camZ|.
    return kTanHalfFovY * std::fabs(m_cameraZ);
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

void Renderer::ClearScreen() {
    glClearColor(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

unsigned int Renderer::LoadTexture(const std::string& a_path) {
    auto it = m_texturesByPath.find(a_path);
    if (it != m_texturesByPath.end()) return it->second;

    Texture tex;
    if (!Texture::LoadFromFile(a_path, tex)) return 0;

    m_texturesByPath[a_path] = tex.id;
    m_texturesByHandle[tex.id] = tex;
    return tex.id;
}

unsigned int Renderer::CreateTextureFromRGBA(const unsigned char* a_rgba, int a_width, int a_height) {
    Texture tex;
    if (!Texture::LoadFromMemoryRGBA(a_rgba, a_width, a_height, tex)) return 0;
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

void Renderer::BindTextureWithUVScale(unsigned int a_handle, float& a_uMax, float& a_vMax) const {
    a_uMax = 1.0f;
    a_vMax = 1.0f;
    auto it = m_texturesByHandle.find(a_handle);
    if (it != m_texturesByHandle.end()) {
        a_uMax = it->second.uMax;
        a_vMax = it->second.vMax;
    }
    glBindTexture(GL_TEXTURE_2D, a_handle);
}

// RETAIL MATCH (Sound Shapes, not Flowers-of-Error -- the 0x00421e40/
// 0x00421e60 addresses this comment used to cite are FoE's x86/OpenGL
// binary; this is a different game on a different platform). The old
// plain glEnable/glDisable(GL_DEPTH_TEST) below was ported unmodified from
// the shared engine bootstrap and never verified against Sound Shapes'
// own PS3 binary.
//
// Confirmed from the real DrawList opcode dispatcher (FUN_00044d4c in
// EBOOT.elf, opcode 0x1c = DrawList.setAttrDepthTest): the pushed argument
// section 3), matching this codebase's own VIDEOPARAMS_EQUAL/VIDEOPARAMS_ALWAYS.
// The dispatcher stores this value into a device-state field and, whenever
// either depth attribute changes, re-emits BOTH the compare-function and
// the write-enable together as a single paired GPU command (confirmed via
// the LZCOUNT-based boolean re-derivation of the other attribute's current
// state in the same branch) -- i.e. depth TEST is never toggled off at the
// GPU at all; only the comparison function changes. This is genuine
// Z-layer compositing control (enabled -> draw only where depth already
// matches; disabled -> draw regardless of depth), not an enable/disable
// switch.
// REVERTED to plain enable/disable, deliberately, despite the reasoning
// above. The GL_EQUAL reading blanked the entire game: a depth compare of
// "draw only where depth already equals this fragment's z" passes nothing
// unless some earlier pass primed the depth buffer with exactly those z
// values, and no such priming pass exists in this engine. Every frame still
// submitted its full draw load (verified by instrumenting the submission
// path: ~50 calls/frame -- 6 textured quads, 13 gradient polys, 31 text
// glyphs, valid context, no GL error) and every captured frame came out
// pure black.
//
// EQUAL/ALWAYS reading as "a strong inference, not a dispatcher-confirmed
// fact", and says explicitly: seeing the actual dispatcher body "would be
// worth doing before committing to glDepthFunc(GL_EQUAL) vs GL_ALWAYS over
// the current plain enable/disable". That advice was right. Until the
// dispatcher's use of the 2/7 value is genuinely decoded, this keeps the
// behavior that actually renders -- which is also what the last known-good
// build of this engine did (commit e825c77, "old ss engine").
void Renderer::SetDepthTest(bool a_enabled) {
    if (a_enabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

void Renderer::SetDepthWrite(bool a_enabled) {
    glDepthMask(a_enabled ? GL_TRUE : GL_FALSE);
}

// Video.setLineWidth(w) -- retail FUN_00421a10 scales the requested width by
// the device width over an 800px reference, so lines keep a constant
// apparent thickness relative to the 800x600 design resolution.
void Renderer::SetLineWidth(float a_width) {
    const float ref = 800.0f;
    const float scale = (m_width > 0) ? ((float)m_width / ref) : 1.0f;
    glLineWidth(scale * a_width);
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
        glFrontFace(GL_CW);
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

// Retail FUN_004213f0 (world view) and FUN_00421680 (ortho view), applied
// straight onto the GL matrix stack.
void Renderer::ApplyProjection() const {
    const float aspect = GetAspect();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    switch (m_viewMode) {
        case ViewMode::WorldPerspective: {
            // SOUND SHAPES: an ORTHOGRAPHIC world view, camera-centred, with
            // the vertical extent driven by camera Z. NOT gluPerspective.
            //
            // The perspective form this replaced (retail FoE's FUN_004213f0:
            // gluPerspective(53.13010, aspect, 1, 1e6) + glTranslatef(-camX,
            // yOff-camY, -camZ)) was inherited from Flowers-of-Error's x86
            // binary during the shared-engine bootstrap. It cannot be a 1-1
            // match for Sound Shapes on its own terms: this is a PS3 title
            // whose renderer is RSX/libgcm and whose binary imports no
            // OpenGL at all, so neither gluPerspective nor gluOrtho2D
            // appears anywhere in EBOOT.elf -- see README.md's own warning
            // that nothing about FoE's renderer should be assumed correct
            // here without independent verification against Sound Shapes'
            // own binary.
            //
            // It is also empirically wrong. Real content positions world
            // draws just off the camera plane: a stock game_data boot draws
            // every vertex at world z = 21.8340 with the camera at
            // (19.520, 10.980, 21.824), i.e. camZ + 0.01. Under the
            // perspective form that is 0.01 BEHIND the eye (the camera
            // looks down -Z), so eye.z = +0.01 and clip.w = -0.01: every
            // vertex lands outside the frustum and the whole frame clips
            // away. Measured directly by instrumenting the emitted
            // vertices -- NDC came out x:[-2196 .. 4323] y:[-2196 .. 4302]
            // with w:[-0.01 .. -0.01], against a visible range of -1..1.
            // Under an orthographic view that same content renders fine,
            // because z then only orders depth and does not divide x/y --
            // which is exactly what Sound Shapes' content expects, being a
            // 2D game that uses z purely as a layering value (ZINCER
            // multiples, see any MainMenu5 screen's drawList.translate).
            //
            // records the world view as orthographic with a fixed vertical
            // half-extent (the old kWorldHalfHeight, empirically calibrated
            // to 14.7 against a real RPCS3 capture of game_data_min), and
            // that is the configuration under which this engine last
            // rendered the real title screen -- "the logo, the 'start'
            // button, 'By: Queasy Games', all pulled from the actual
            // game_data corpus with zero [gm] script errors".
            //
            // The half-extent here is kTanHalfFovY * |camZ| rather than a
            // fixed constant, so "visible world height equals the camera's
            // Z distance" still holds (kTanHalfFovY is exactly 0.5) -- that
            // relation is what makes Video.setCameraPos's z behave as a
            // zoom the way real content uses it, and it reproduces the old
            // fitted 14.7 to within ~10% at game_data_min's own default
            // camera (0.5 * 26.879999 = 13.44) without needing a fitted
            // number at all.
            //
            // STILL TO CONFIRM 1-1: the real projection lives in the RSX
            // command path behind DrawList.setupWorldView's recorded opcode
            // FUN_0004cea0). That body is NOT missing analysis -- a full
            // Ghidra pass completes on it -- it is hand-written VMX/Altivec
            // matrix code (it spills v20-v31 on entry), and both Ghidra's
            // decompiler and capstone stop at the same unsupported vector
            // opcode about 38 instructions in. Lifting it is its own task.
            //
            // Two things ARE already established about it: it branches on
            // setupWorldView's optional bool (clrlwi r5,r5,0x18; cmpwi r5,0),
            // so that flag really does select between two projection setups;
            // and unlike the two ortho views it does NOT route through the
            // shared ortho-matrix builder FUN_00302f30 that FUN_0003e220 and
            // FUN_0003e4a0 both use.
            //
            // So: until that VMX body is decoded, the form below is the
            // variant that matches observed content behaviour, NOT a
            // confirmed transcription, and is deliberately not presented as
            // one. What is certain is that the gluPerspective form it
            // replaced was a different game's, and clipped every frame.
            const float halfH = kTanHalfFovY * std::fabs(m_cameraZ);
            const float halfW = halfH * aspect;
            // DEPTH RANGE: a tight band around the camera plane, NOT the
            // +/-kFarClip (1e6) range this first used. Content layers itself
            // with ZINCER, which is 0.01 (constants_bindings.cpp), and the
            // *_Z constants it is added to span roughly 0..100. A 2e6-deep
            // range quantised into a 24-bit depth buffer resolves only
            // 2e6 / 2^24 ~= 0.119 world units -- twelve times COARSER than
            // ZINCER itself, so every layer collapsed onto one depth value
            // and, under GL_LESS, whichever drew first won.
            //
            // That is not an abstract precision worry: it is exactly what
            // made the PS3 main menu render as a featureless white screen.
            // PreTutorialScreen.gm draws a full-screen cory_white quad and
            // then puts its logo and labels on top at z + ZINCER; with the
            // layers quantised together the quad kept winning and every
            // later element was depth-rejected.
            //
            // +/-kLayerDepthRange resolves 2*200 / 2^24 ~= 2.4e-5 world
            // units, about 400x finer than one ZINCER step, while still
            // covering the whole *_Z band either side of the camera.
            glOrtho((double)(m_cameraX - halfW), (double)(m_cameraX + halfW),
                    (double)(m_cameraY - halfH), (double)(m_cameraY + halfH),
                    -(double)kLayerDepthRange, (double)kLayerDepthRange);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            // Camera-relative depth: content sits just off the camera plane
            // (measured: world z 21.8340 against camera z 21.824), so
            // without this the whole scene would sit at one end of the band
            // above and waste its precision.
            glTranslatef(0.0f, 0.0f, -m_cameraZ);
            // Pitch is retail's own camera field (+0x64) fed straight into
            // glRotatef; every piece of content in this tree leaves it at 0,
            // where this is the identity.
            glRotatef(m_cameraPitch, 1.0f, 0.0f, 0.0f);
            return;
        }
        case ViewMode::WorldOrtho: {
            // retail FUN_004215a0: gluOrtho2D over the GameBoard's own AABB.
            gluOrtho2D((double)kGameBoardMinX, (double)kGameBoardMaxX,
                       (double)kGameBoardMinY, (double)kGameBoardMaxY);
            break;
        }
        case ViewMode::UnitOrtho: {
            // Sound Shapes' own setupOrthoView with no arguments is
            // gluOrtho2D(0, 1, 0, 1) -- CONFIRMED against its binary, not
            // just inherited from FoE's FUN_00421640: the decompiled body
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
        // CONFIRMED against Sound Shapes' own binary (not merely inherited
        // from FoE's FUN_00421680): setupAspectRatioOrthoView's recorder
        // FUN_0003e4a0 builds its matrix as
        //     ortho(aspect * DAT_0003e450, aspect * DAT_0003e454,
        //           DAT_0003e450, DAT_0003e454, DAT_0003e458, DAT_0003e45c)
        // where `aspect` is the device field at DAT_0041f880+0x81bc, and
        // those four constants read -0.5f, +0.5f, -1.0f, +1.0f out of
        // EBOOT.elf. That is exactly ortho(-aspect/2, +aspect/2, -0.5, +0.5)
        // with near/far -1/+1 -- i.e. precisely gluOrtho2D(-a/2, a/2, -0.5,
        // 0.5), which is what this already did.
        case ViewMode::AspectRatioOrtho:
        default: {
            // retail FUN_00421680: gluOrtho2D(-v/2, +v/2, -0.5, +0.5) where v
            // is the aspect ratio, so 1.0 spans the full screen height.
            float halfH = kUIHalfHeight;
            float halfW = kUIHalfHeight * aspect;
            // Video.getScreenViewScale/setScreenViewScale is a Sound Shapes
            // TV-overscan control that does not exist in retail FoE; it
            // defaults to 1.0, so this is a no-op unless that screen sets it.
            const float scale = GetEngineContext().video.screenViewScale;
            if (scale > 0.0001f) { halfH /= scale; halfW /= scale; }
            gluOrtho2D(-(double)halfW, (double)halfW, -(double)halfH, (double)halfH);
            break;
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Retail FUN_00421d90. GLenum 7 == GL_QUADS.
void Renderer::DrawTexturedQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                                 float a_x2, float a_y2, float a_x3, float a_y3,
                                 unsigned int a_textureHandle, float a_z) {
    ApplyProjection();

    float uMax, vMax;
    glEnable(GL_TEXTURE_2D);
    BindTextureWithUVScale(a_textureHandle, uMax, vMax);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);  glVertex3f(a_x0, a_y0, a_z);
    glTexCoord2f(uMax, 0.0f);  glVertex3f(a_x1, a_y1, a_z);
    glTexCoord2f(uMax, vMax);  glVertex3f(a_x2, a_y2, a_z);
    glTexCoord2f(0.0f, vMax);  glVertex3f(a_x3, a_y3, a_z);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

void Renderer::DrawPolygon(unsigned int a_glPrimitiveMode, const float* a_xy, int a_count, float a_z) {
    if (a_count <= 0) return;

    ApplyProjection();
    glDisable(GL_TEXTURE_2D);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    glBegin(a_glPrimitiveMode);
    for (int i = 0; i < a_count; ++i) {
        glVertex3f(a_xy[i * 2 + 0], a_xy[i * 2 + 1], a_z);
    }
    glEnd();
}

void Renderer::DrawPolygonGradient(unsigned int a_glPrimitiveMode, const float* a_xy, const float* a_rgba,
                                    const float* a_uv, int a_count, unsigned int a_textureHandle, float a_z) {
    if (a_count <= 0) return;

    ApplyProjection();

    float uMax = 1.0f, vMax = 1.0f;
    const bool textured = (a_textureHandle != 0);
    if (textured) {
        glEnable(GL_TEXTURE_2D);
        BindTextureWithUVScale(a_textureHandle, uMax, vMax);
    } else {
        glDisable(GL_TEXTURE_2D);
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
            // Script-supplied UVs are in logical-image space, so they scale
            // by uMax/vMax exactly like the textured-quad path -- this is
            // what makes out-of-range UVs tile across the padded extent the
            // way retail's do (PekkoAndJon/bg.gm's animated kaleidoscope).
            const float u = a_uv ? a_uv[i * 2 + 0] : (x - minX) / spanX;
            const float v = a_uv ? a_uv[i * 2 + 1] : (y - minY) / spanY;
            glTexCoord2f(u * uMax, v * vMax);
        }
        glColor4f(a_rgba[i * 4 + 0], a_rgba[i * 4 + 1], a_rgba[i * 4 + 2], a_rgba[i * 4 + 3]);
        glVertex3f(x, y, a_z);
    }
    glEnd();

    if (textured) glDisable(GL_TEXTURE_2D);
}

void Renderer::DrawFullscreenOverlay(float a_z) {
    // RETAIL MATCH (retail FUN_00421980, the body of Video.clear's blended
    // path -- Video.clear (FUN_00435d40) dispatches to binding slot +0x8c,
    // which is device slot +0xb0, which is this function):
    //
    //   <buffer bookkeeping>
    //   vtable[0x40]  ->  device +0x64  ->  gluOrtho2D(0, 1, 0, 1)
    //   glEnableClientState(GL_VERTEX_ARRAY);
    //   glDisableClientState(GL_NORMAL_ARRAY);
    //   glDisableClientState(GL_COLOR_ARRAY);
    //   glVertexPointer(2, GL_FLOAT, 0, &DAT_00513b20);
    //   glColor4f(r, g, b, a);
    //   glDrawArrays(GL_QUADS, 0, 4);
    //
    // DAT_00513b20 is verified to hold exactly the unit quad
    // (0,0) (1,0) (1,1) (0,1).
    //
    // So retail does NOT compute the quad from the camera/world extents --
    // it switches to a fixed unit-square ortho projection and draws a unit
    // quad, which covers the viewport regardless of camera state. This
    // implementation previously derived world/UI half-extents from the
    // camera and also forced glDepthMask(GL_FALSE); both were invented.
    // Retail touches neither the depth mask nor the camera here.
    //
    // Note Video.clear is ALWAYS this blended quad in retail -- it is never
    // a hardware glClear. The only glClear call sites are the per-frame
    // colour clear (FUN_00420c40) and Video.clearDepthBuffer
    // (FUN_00421970), neither of which is Video.clear.
    (void)a_z; // retail's quad carries no Z -- it is a 2D unit-ortho draw

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 1.0, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_TEXTURE_2D);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    static const float kUnitQuad[8] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
    };
    glEnableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glVertexPointer(2, GL_FLOAT, 0, kUnitQuad);
    glDrawArrays(GL_QUADS, 0, 4);
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Renderer::DrawGlyphQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                              float a_x2, float a_y2, float a_x3, float a_y3,
                              unsigned int a_textureHandle, float a_u0, float a_v0, float a_u1, float a_v1,
                              float a_edge, float a_width, float a_z) {
    // a_edge/a_width are intentionally unused -- retail has no shaders and
    // no signed-distance-field text path. See renderer.h.
    (void)a_edge;
    (void)a_width;

    ApplyProjection();

    float uMax, vMax;
    glEnable(GL_TEXTURE_2D);
    BindTextureWithUVScale(a_textureHandle, uMax, vMax);
    glColor4f(m_colour[0], m_colour[1], m_colour[2], m_colour[3]);

    // The atlas UVs bitmap_font computes are fractions of the LOGICAL atlas
    // image, so they need the same power-of-two padding compensation as
    // every other UV this renderer emits.
    const float u0 = a_u0 * uMax, u1 = a_u1 * uMax;
    const float v0 = a_v0 * vMax, v1 = a_v1 * vMax;

    glBegin(GL_QUADS);
    glTexCoord2f(u0, v0);  glVertex3f(a_x0, a_y0, a_z);
    glTexCoord2f(u1, v0);  glVertex3f(a_x1, a_y1, a_z);
    glTexCoord2f(u1, v1);  glVertex3f(a_x2, a_y2, a_z);
    glTexCoord2f(u0, v1);  glVertex3f(a_x3, a_y3, a_z);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}
