#pragma once
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <string>
#include <unordered_map>
#include "texture.h"

// Fixed-function OpenGL renderer, matching retail Flowers of Error's real
// graphics backend call-for-call.
//
// RETAIL ARCHITECTURE (reveng_pc/RETAIL_GL_GROUND_TRUTH.md, recovered from a
// full decompilation of the whole binary after fixing the OPENGL32/GLU32
// import prototypes): retail uses raw Win32 windowing plus *legacy
// fixed-function immediate-mode OpenGL* -- glBegin/glEnd, glVertex3f,
// glTexCoord2f, glColor4f, the GL matrix stack, gluPerspective/gluOrtho2D.
// It has no shaders, no VBOs, and no vertex array objects. Its entire GL
// backend is one tight cluster of functions, 0x00420c90-0x00421d90.
//
// This class previously implemented an OpenGL 3.3 core-profile,
// shader-and-VBO renderer, which was a deliberate architectural divergence.
// Every visible mismatch that divergence caused had to be papered over with
// empirically-fitted constants. It is now a direct port of retail's actual
// call sequences instead, so those fitted constants are gone.
//
// SDL is still used -- but only for window creation, the event pump, and GL
// context creation. Nothing SDL draws or manages any GL state.
//
// PROJECTION (retail FUN_004213f0 / FUN_00421680):
//
//   world/camera view:  gluPerspective(53.13010, aspect, 1.0, 1000000.0)
//                       glRotatef(pitch, 1,0,0)
//                       glTranslatef(-camX, yOff - camY, -camZ)
//   screen/UI view:     gluOrtho2D(-aspect/2, +aspect/2, -0.5, +0.5)
//
// The camera view is a genuine PERSPECTIVE projection, not the orthographic
// one this engine used to apply. That single fact invalidated a whole family
// of previously-fitted constants -- see kTanHalfFovY below.
class Renderer {
public:
    // RETAIL CONSTANTS (exact, read out of the binary -- not fitted).
    //
    // Retail computes its vertical field of view as
    //     fovY = 2 * atan2(800.0, 1600.0)  in degrees
    // from a hardcoded literal 800/1600 pair (NOT derived from window size,
    // so the fov is fixed and only `aspect` varies). That is
    // 2*atan(0.5) = 53.130102 degrees.
    static constexpr float kFovYDegrees = 53.13010235f;
    // tan(fovY/2) = tan(atan(0.5)) = exactly 0.5. This is the whole reason
    // the original developer picked an 800:1600 ratio: it makes the
    // perspective mapping trivial -- at any camera distance d, the visible
    // world half-height at the Z=0 plane is exactly 0.5*d, i.e. the visible
    // world HEIGHT equals the camera's Z distance, 1:1.
    //
    // This supersedes the old `kWorldHalfHeight` constant, which does not
    // exist in a perspective projection: visible extent is a function of
    // camera Z, not a fixed number. (For the record, that constant's last
    // empirically-fitted value was 1650, fitted against a screenshot taken
    // with PekkoAndJon/Main.gm's baseCameraZ = 2688, whose true half-height
    // is 0.5*2688 = 1344. Fitting an orthographic half-height to a
    // perspective image is exactly the kind of ~20% error that produces.)
    // Use GetWorldHalfHeight()/GetWorldHalfWidth() instead.
    static constexpr float kTanHalfFovY = 0.5f;
    // Half-depth of the orthographic world view's z band, centred on the
    // camera plane -- see ApplyProjection's WorldPerspective case for why a
    // tight band matters (ZINCER-sized layering vs depth-buffer precision).
    static constexpr float kLayerDepthRange = 200.0f;
    static constexpr float kNearClip = 1.0f;
    static constexpr float kFarClip = 1000000.0f;

    // Retail's setupOrthoView (FUN_00421680) is
    //   gluOrtho2D(-v/2, +v/2, -0.5, +0.5)
    // i.e. the vertical extent is exactly +/-0.5 ("1.0 == full screen
    // height") and the horizontal is +/-aspect/2. This CONFIRMS the value
    // this engine had already derived independently for UI mode.
    static constexpr float kUIHalfHeight = 0.5f;

    // RETAIL MATCH: the GameBoard's bounds, read as literals out of its
    // constructor (retail FUN_0040df90 writes 0xc515f000, 0xc4a76000,
    // 0x4515f000, 0x44a76000 as minX, minY, maxX, maxY). Used by the
    // WorldOrtho view mode, which is gluOrtho2D over exactly these bounds
    // (retail FUN_004215a0). Note the board is NOT 16:9 -- 4798x2678 is
    // aspect 1.7916 -- so the axes are independent literals.
    static constexpr float kGameBoardMinX = -2399.0f;
    static constexpr float kGameBoardMinY = -1339.0f;
    static constexpr float kGameBoardMaxX = 2399.0f;
    static constexpr float kGameBoardMaxY = 1339.0f;

    // Retail device init sets glLineWidth(2.0) once, right after enabling
    // GL_LINE_SMOOTH, and never changes it. PekkoAndJon/Main.gm's init()
    // calls Video.setLineWidth() with zero arguments, i.e. "reset to
    // default" -- this is that default.
    static constexpr float kDefaultLineWidth = 2.0f;

    // RETAIL MATCH: retail has FOUR distinct view/projection modes, each a
    // separate device vtable slot. The GM bindings dispatch at
    // binding_offset, which is device_offset - 0x24 (one vtable seen through
    // a C++ secondary interface pointer -- see
    // reveng_pc/RETAIL_GL_GROUND_TRUTH.md):
    //
    //   Video.setupWorldView            binding +0x38 -> device +0x5c
    //       FUN_004213f0: gluPerspective(53.13010, aspect, 1, 1e6) + camera
    //   Video.setupWorldOrthoView       binding +0x3c -> device +0x60
    //       FUN_004215a0: gluOrtho2D(gbMinX, gbMaxX, gbMinY, gbMaxY)
    //   Video.setupOrthoView            binding +0x40 -> device +0x64
    //       FUN_00421640: gluOrtho2D(0, 1, 0, 1)
    //   Video.setupAspectRatioOrthoView binding +0x44 -> device +0x68
    //       FUN_00421680: gluOrtho2D(-a/2, a/2, -0.5, 0.5)
    //
    // This engine previously collapsed all four into two (camera-perspective
    // vs a single UI ortho), with setupOrthoView and
    // setupAspectRatioOrthoView both doing the aspect-ratio form. That is
    // wrong for setupOrthoView, which should be the 0..1 unit square.
    // FoE's own content only ever calls setupWorldView and
    // setupAspectRatioOrthoView, so the collapse happened to be harmless
    // for this game's content -- but it is not a faithful port.
    //
    // NOTE on an earlier error in this port's own notes: setupWorldView was
    // documented as "a genuine no-op in retail", on the grounds that device
    // slots +0x30/+0x34/+0x38 all point at a function whose body is
    // `return;`. Those slots really do share a no-op, but they are not the
    // view setups -- setupWorldView dispatches at BINDING +0x38, which is
    // device +0x5c, the perspective setup. The no-op claim was an
    // off-by-0x24 misreading.
    enum class ViewMode {
        WorldPerspective,   // setupWorldView
        WorldOrtho,         // setupWorldOrthoView -- GameBoard bounds
        UnitOrtho,          // setupOrthoView -- gluOrtho2D(0,1,0,1)
        CustomOrtho,        // setupOrthoView(l,r,b,t) -- explicit bounds
        AspectRatioOrtho,   // setupAspectRatioOrthoView -- +/-0.5 vertical
    };

    bool Init(int a_width, int a_height, const char* a_title);
    void Shutdown();
    // Backs Video.restartVideo(w,h,...) -- resizes the window/viewport AND
    // tells SDL to change the actual window size (engine-initiated).
    void Resize(int a_width, int a_height);
    // The user dragged the window border -- SDL already resized the
    // window itself, this just updates our tracked size to match.
    void OnWindowResized(int a_width, int a_height);

    void BeginFrame();
    void EndFrame();

    // Backs drawList.clear(VIDEO_ALLSURFACES). Clears to whatever the
    // CURRENT draw colour is (m_colour), same convention as every other
    // draw call.
    void ClearScreen();

    // The LOGICAL (content) size -- what getScreenWidth/getScreenHeight
    // report and what the projection aspect derives from. Retail keeps this
    // separate from the raw window size (fields +0x3c/+0x40 vs +0x34/+0x38)
    // and letterboxes the difference with drawn black bars.
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetAspect() const {
        return (m_height > 0) ? (float)m_width / (float)m_height : 1.0f;
    }

    // Visible world extent at the Z=0 plane under the real perspective
    // projection. Replaces the old fixed kWorldHalfHeight constant -- see
    // kTanHalfFovY. Callers that need "the world rect currently on screen"
    // (draw_list.cpp's fullscreen wash, ground_renderer.cpp, entity
    // culling in entity_bindings.cpp) must use these so they stay
    // consistent with the projection automatically as the camera moves.
    float GetWorldHalfHeight() const;
    float GetWorldHalfWidth() const { return GetWorldHalfHeight() * GetAspect(); }

    // Debug: capture the current framebuffer to a PPM file.
    void SaveScreenshot(const char* a_path) const;

    // Returns a texture "handle" (GL texture id) for a_path, caching loads.
    unsigned int LoadTexture(const std::string& a_path);
    // Uploads a raw RGBA8 buffer directly, no path/cache involved. Caller
    // owns the returned handle's lifetime; 0 on failure.
    unsigned int CreateTextureFromRGBA(const unsigned char* a_rgba, int a_width, int a_height);
    void UnloadTexture(unsigned int a_handle);
    // 0,0 if a_handle is unknown. Reports the LOGICAL (unpadded) size, not
    // the power-of-two padded upload size -- see Texture::potWidth.
    void GetTextureDimensions(unsigned int a_handle, int& a_outWidth, int& a_outHeight) const;
    void SetTextureGeneratedTint(unsigned int a_handle, float a_fillR, float a_fillG, float a_fillB,
                                  float a_outlineR, float a_outlineG, float a_outlineB);

    // Video.setAttrDepthTest / setAttrDepthWrite. These are the ONLY two
    // runtime GL state changes retail's scripts can make: retail's wrappers
    // (0x00421e40 / 0x00421e60) are a plain glEnable/glDisable(GL_DEPTH_TEST)
    // and a plain glDepthMask respectively.
    //
    // Note the binary imports NO glDepthFunc at all, so the comparison
    // function is never set and stays at OpenGL's default, GL_LESS. An
    // earlier theory here (that true/false selected GL_EQUAL/GL_ALWAYS as a
    // Z-layer compositing trick) came from Sound Shapes' DrawList.c and does
    // not apply to FoE.
    void SetDepthTest(bool a_enabled);
    void SetDepthWrite(bool a_enabled);
    // Video.setLineWidth(w). Retail scales w by deviceWidth/800 before
    // calling glLineWidth -- see the implementation.
    void SetLineWidth(float a_width);

    // NOTE ON THE FOUR METHODS BELOW: retail FoE's import table contains no
    // glColorMask, no glCullFace/glFrontFace, and no glStencil* of any kind
    // -- it never uses colour masking, face culling or the stencil buffer.
    // These are kept because Sound Shapes content in this tree
    // (ported_native/ground_renderer.cpp, draw_context.cpp's attribute
    // stack) still drives them, and removing them would regress that path.
    // They are inert for FoE, whose scripts never reach them.
    void SetColourMask(bool a_maskEnabled);
    void SetCulling(int a_mode);
    void FillStencilFullscreen(float a_z);
    // Default mask is retail's own literal -1 (0xFFFFFFFF), stored into the
    // sixth field of setAttrStencil's 0x18-byte command struct in both its
    // 3-arg and 5-arg forms (see DC_gmf_setAttrStencil) -- not GL's 0xFF
    // default.
    void SetStencilFunc(int a_func, int a_ref, unsigned int a_mask = 0xFFFFFFFFu);
    void SetStencilOp(int a_sfail, int a_dpfail, int a_dppass);
    void SetDefaultStencil();

    void SetColour(float a_r, float a_g, float a_b, float a_a);
    void GetColour(float& a_r, float& a_g, float& a_b, float& a_a) const {
        a_r = m_colour[0]; a_g = m_colour[1]; a_b = m_colour[2]; a_a = m_colour[3];
    }

    // Video.setCameraPos(x,y,z). Under the real perspective projection z is
    // a genuine EYE DISTANCE (it scales the visible world extent 1:1 --
    // see kTanHalfFovY), not the clip-range parameter this engine used to
    // treat it as.
    void SetCameraPos(float a_x, float a_y, float a_z = 0.0f);
    void SetViewMode(ViewMode a_mode) { m_viewMode = a_mode; }
    // DrawList.setupOrthoView(left, right, bottom, top) -- Sound Shapes'
    // own 4-argument form of setupOrthoView (see DC_gmf_setupOrthoView).
    void SetCustomOrtho(float a_left, float a_right, float a_bottom, float a_top) {
        m_orthoLeft = a_left; m_orthoRight = a_right;
        m_orthoBottom = a_bottom; m_orthoTop = a_top;
        m_viewMode = ViewMode::CustomOrtho;
    }
    ViewMode GetViewMode() const { return m_viewMode; }
    // Compatibility shim for the existing call sites (draw_list,
    // ground_renderer, player_renderer): true selects the world
    // perspective, false the aspect-ratio ortho -- the two modes FoE's
    // content actually uses.
    void SetUseCameraView(bool a_use) {
        m_viewMode = a_use ? ViewMode::WorldPerspective : ViewMode::AspectRatioOrtho;
    }
    float GetCameraX() const { return m_cameraX; }
    float GetCameraY() const { return m_cameraY; }
    float GetCameraZ() const { return m_cameraZ; }
    // Camera pitch in degrees about the X axis (retail field +0x64, fed
    // straight into glRotatef). FoE's own content leaves this at 0.
    void SetCameraPitch(float a_degrees) { m_cameraPitch = a_degrees; }
    float GetCameraPitch() const { return m_cameraPitch; }

    // Draws a_count 2D vertices (world-space xy pairs) as a_glPrimitiveMode,
    // tinted by the current colour and untextured. a_z is the Z depth the
    // whole primitive draws at.
    void DrawPolygon(unsigned int a_glPrimitiveMode, const float* a_xy, int a_count, float a_z = 0.0f);

    // Per-vertex-coloured variant -- backs FoE's radial-gradient
    // beginShape/plotVertex/endShape idiom (PekkoAndJon/bg.gm alternates
    // alpha between a fan's centre and its outer ring to fade a colour out
    // to transparent). a_rgba has 4 floats per vertex. a_textureHandle == 0
    // draws untextured. a_uv supplies real per-vertex UVs when the shape has
    // them (ShapeData::uvs, via getVertex(i).u/.v); pass nullptr to fall
    // back to UV = vertex xy remapped into the shape's bounding box.
    void DrawPolygonGradient(unsigned int a_glPrimitiveMode, const float* a_xy, const float* a_rgba,
                              const float* a_uv, int a_count, unsigned int a_textureHandle, float a_z = 0.0f);

    // drawList.clear()'s translucent-alpha path -- a quad spanning the
    // currently-visible extent, alpha-blended rather than a hardware clear,
    // so "fade to black" (setColour(0,0,0,perc); clear();) actually blends.
    void DrawFullscreenOverlay(float a_z = 0.0f);

    // drawList.drawTextureByHandle(x0,y0,...,x3,y3,handle) -- four
    // world-space corners, tinted by the current colour.
    void DrawTexturedQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                           float a_x2, float a_y2, float a_x3, float a_y3,
                           unsigned int a_textureHandle, float a_z = 0.0f);

    // A glyph quad with explicit UVs. a_edge/a_width are accepted and
    // IGNORED: they drove a signed-distance-field threshold in the old
    // shader pipeline, and retail has no shaders -- its text is plain
    // alpha-blended textured quads. The font atlas is uploaded as white RGB
    // with coverage in alpha (see Texture::LoadFromFile), so fixed-function
    // GL_MODULATE against glColor4f reproduces coloured antialiased text
    // directly.
    void DrawGlyphQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                        float a_x2, float a_y2, float a_x3, float a_y3,
                        unsigned int a_textureHandle, float a_u0, float a_v0, float a_u1, float a_v1,
                        float a_edge, float a_width, float a_z = 0.0f);

    // Retail's FUN_00420c90: draws the letterbox/pillarbox black bars as
    // real geometry in a pixel-space ortho projection over the FULL window,
    // depth test off, wrapped in glPushAttrib/glPopAttrib. Part of the
    // frame's rendered output, so it runs after all scene draws but before
    // the buffer swap -- and therefore before any screenshot capture.
    // EndFrame() calls this too; drawing it twice is harmless (identical
    // opaque black quads with the depth test disabled).
    void DrawLetterboxBars();

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;

    // m_width/m_height are the LOGICAL (content) size; m_rawWidth/
    // m_rawHeight are the actual SDL window pixel size. Retail keeps the
    // same split (+0x3c/+0x40 vs +0x34/+0x38) and derives its letterbox bar
    // thickness from the difference (FUN_00421160):
    //     barX = (rawW - logicalW)/2,  barY = (rawH - logicalH)/2
    int m_width = 0, m_height = 0;
    int m_rawWidth = 0, m_rawHeight = 0;
    int m_barX = 0, m_barY = 0;
    void UpdateLetterboxBars();

    float m_colour[4] = {1, 1, 1, 1};
    // getCameraPosZ()` reads 26.879999 on retail before any script ever
    // calls setCameraPos, not 0.0). This isn't just an accuracy nit: with
    // cameraZ=0, WorldPerspective content drawn at its own z=0 sits exactly
    // at the eye plane, which is degenerate for gluPerspective's projection
    // (eye.z=0 -> clip.w=0 -> the perspective divide produces NDC (0,0,0)
    // for every vertex, collapsing every triangle to a point regardless of
    // its x/y). Any scene that never calls setCameraPos (game_data_min's
    // hello-world Main.gm included) rendered nothing at all until this was
    // fixed -- confirmed by instrumenting DrawTexturedQuad's own clip-space
    // math, not a guess.
    float m_cameraX = 0.0f, m_cameraY = 0.0f, m_cameraZ = 26.879999f;
    // setupOrthoView(l,r,b,t)'s explicit bounds; only read in CustomOrtho
    // mode. Defaults are the 0-arg form's own (0,1,0,1) constants.
    float m_orthoLeft = 0.0f, m_orthoRight = 1.0f, m_orthoBottom = 0.0f, m_orthoTop = 1.0f;
    float m_cameraPitch = 0.0f;
    ViewMode m_viewMode = ViewMode::AspectRatioOrtho;

    std::unordered_map<std::string, unsigned int> m_texturesByPath;
    std::unordered_map<unsigned int, Texture> m_texturesByHandle;

    // Applies retail's projection + modelview for whichever view mode is
    // currently active, straight onto the GL matrix stack.
    void ApplyProjection() const;
    // Binds a texture and returns its uMax/vMax (power-of-two padding
    // compensation, see Texture::uMax). Returns 1,1 for an unknown handle.
    void BindTextureWithUVScale(unsigned int a_handle, float& a_uMax, float& a_vMax) const;
};
