#pragma once
#include <GL/glew.h>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include "shader.h"
#include "texture.h"
#include "../platform/platform_window.h"

// Compatibility OpenGL renderer for the desktop port's draw-command model.
// Ordinary geometry/textures use fixed-function calls; glyphs and explicitly marked
// distance-field textures use fragment shaders. Distance shader parameters
//
// Sound Shapes runs on PS3 RSX and does not use OpenGL. This OpenGL backend
// is a portability layer, not a direct retail renderer port. The old desktop
// Flowers of Error binary is not authoritative for Sound Shapes behavior;
// per-function retail evidence and remaining gaps are tracked in
//
// The frontend owns the native window and OpenGL context. Nothing from a
// windowing library draws or manages the renderer's GL state.
//
// Projection evidence: retail unit/aspect ortho and the unrotated world-view
// bounds/depth mapping are traced. The rotated world-view branch remains under
// audit. No FoE FUN_00421xxx OpenGL address is evidence for this PS3 binary.
class Renderer {
public:
    // FUN_00052568 computes the retail world-view width from camera Z using
    // this binary float. The complete matrix composition remains under audit.
    static constexpr float kWorldViewExtentBase = 0.44721359f;
    // FUN_0004cea0 supplies world near/far bounds -1023 and +1023 to its
    // orthographic matrix construction.
    static constexpr float kLayerDepthRange = 1023.0f;
    static constexpr float kNearClip = 1.0f;
    static constexpr float kFarClip = 1000000.0f;

    // Retail FUN_0003e4a0 passes vertical bounds [-0.5,+0.5] to
    // FUN_00302f30 for setupAspectRatioOrthoView.
    static constexpr float kUIOrthoHalfHeight = 0.5f;

    // Desktop GameBoard bounds. The former FUN_0040df90/FUN_004215a0
    // attribution was to the unrelated FoE reference and is not proof of
    // Sound Shapes world-ortho projection behavior.
    static constexpr float kGameBoardMinX = -2399.0f;
    static constexpr float kGameBoardMinY = -1339.0f;
    static constexpr float kGameBoardMaxX = 2399.0f;
    static constexpr float kGameBoardMaxY = 1339.0f;

    // Desktop default; whether this reproduces the retail RSX line width is
    // unverified.
    static constexpr float kDefaultLineWidth = 2.0f;

    // Preserve the script-level view modes independently. The world matrix
    // and GameBoard mapping remain desktop approximations; the retail PS3
    // projection is an RSX command path, not the unrelated FoE GL vtable.
    enum class ViewMode {
        WorldPerspective,   // setupWorldView
        WorldOrtho,         // setupWorldOrthoView -- GameBoard bounds
        UnitOrtho,          // setupOrthoView -- gluOrtho2D(0,1,0,1)
        CustomOrtho,        // setupOrthoView(l,r,b,t) -- explicit bounds
        AspectRatioOrtho,   // setupAspectRatioOrthoView -- +/-0.5 vertical
    };
    struct ViewState {
        ViewMode mode;
        float aspectOverride;
        bool hasAspectOverride;
        bool worldViewRotationEnabled;
    };

    // Frontend owns a current OpenGL context; renderer owns only GL resources.
    bool Init(PlatformWindow& a_window, int a_width, int a_height);
    void Shutdown();
    // Desktop platform utility for an explicit host-window resize. The
    // retail Video.restartVideo wrapper does not call this method.
    void Resize(int a_width, int a_height);
    // Desktop platform utility for an explicit host-window fullscreen change.
    // The retail Video.restartVideo wrapper only stores its native state flag.
    bool SetFullscreen(bool a_fullscreen);
    bool IsFullscreen() const { return m_isFullscreen; }
    // The user resized the host window; update the tracked logical viewport.
    void OnWindowResized(int a_width, int a_height);

    void BeginFrame();
    void EndFrame();

    // The LOGICAL (content) size -- what getScreenWidth/getScreenHeight
    // report and what the projection aspect derives from. Retail keeps this
    // separate from the raw window size (fields +0x3c/+0x40 vs +0x34/+0x38)
    // and letterboxes the difference with drawn black bars.
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    // Active display dimensions backing Video.getDeviceWidth/Height
    // (+0x819c/+0x81a0); distinct from the aspect-adjusted viewport above.
    int GetDeviceWidth() const { return m_rawWidth; }
    int GetDeviceHeight() const { return m_rawHeight; }
    float GetAspect() const {
        return (m_height > 0) ? (float)m_width / (float)m_height : 1.0f;
    }

    // Visible world half-extents from the traced retail world-view helper.
    // Callers that need "the world rect currently on screen"
    // (draw_list.cpp's fullscreen wash, ground_renderer.cpp, entity
    // culling in entity_bindings.cpp) use these to stay consistent with it.
    float GetWorldHalfHeight() const;
    float GetWorldHalfWidth() const;

    // Debug: capture the current framebuffer to a PPM file.
    void SaveScreenshot(const std::filesystem::path& a_path) const;

    // Returns a texture "handle" (GL texture id) for a_path, caching loads.
    unsigned int LoadTexture(const std::string& a_path);
    // Same loader with an explicit cache identity. This preserves distinct
    // FILEHELPER_AREA_* resource managers when two areas resolve the same
    // physical file path.
    unsigned int LoadTexture(const std::string& a_path, const std::string& a_cacheKey);
    // Loads encoded image bytes into the same path-keyed cache as file loads.
    unsigned int LoadTextureFromMemory(const unsigned char* a_data, std::size_t a_size,
                                       const std::string& a_cacheKey);
    // Uploads a raw RGBA8 buffer directly, no path/cache involved. Caller
    // owns the returned handle's lifetime; 0 on failure.
    unsigned int CreateTextureFromRGBA(const unsigned char* a_rgba, int a_width, int a_height,
                                       bool a_nearestFilter = false);
    void UnloadTexture(unsigned int a_handle);
    // Removes the path-cache entry for a live handle without invalidating
    // references that already hold the handle. A later LoadTexture(path)
    // creates a new resource, while the old one remains valid until released.
    bool DetachTexturePath(unsigned int a_handle);
    // Releases every renderer-owned texture handle and path cache. This is
    // renderer shutdown support, not the retail process-list cleanup path.
    void ReleaseAllTextures();
    std::size_t GetLoadedTextureCount() const { return m_texturesByHandle.size(); }
    // PORTABILITY ADAPTER: queries the active OpenGL driver's free GPU-memory
    // extensions when available; otherwise estimates free bytes in the
    // desktop texture pool from resident RGBA allocations.
    // Emulates the retail Video texture pool's capacity-minus-used counter.
    int GetTextureMemoryFreeBytes() const;
    // 0,0 if a_handle is unknown. Reports the decoded logical dimensions.
    void GetTextureDimensions(unsigned int a_handle, int& a_outWidth, int& a_outHeight) const;
    int GetTextureSourceBitsPerPixel(unsigned int a_handle) const;
    const std::string& GetTextureSourcePath(unsigned int a_handle) const;
    // Retail TEXTURE +0x34/+0x38 extent, or (1,1) for an unknown handle.
    void GetTextureUVExtent(unsigned int a_handle, float& a_uMax, float& a_vMax) const;
    void SetTextureShaderType(unsigned int a_handle, int a_shaderType);
    int GetTextureShaderType(unsigned int a_handle) const;
    bool UsesGeneratedTexturePalette(unsigned int a_handle) const;
    void MarkTexturePaletteDirty(unsigned int a_handle);
    void SetTextureGeneratedTint(unsigned int a_handle, float a_fillR, float a_fillG, float a_fillB,
                                  float a_outlineR, float a_outlineG, float a_outlineB);
    // Applies retail TEXTURE palette bytes (ABGR byte order) to an indexed PNG.
    // Returns false when the handle is not an indexed texture.
    bool SetTexturePalette(unsigned int a_handle, const unsigned char* a_paletteAbgr, size_t a_size);
    void ResetTexturePalette(unsigned int a_handle);
    bool GetTextureSourcePalette(unsigned int a_handle, std::vector<unsigned char>& a_outRgba) const;
    std::shared_ptr<PaletteData> GetTextureSubPalette(unsigned int a_handle) const;
    void BindTextureSubPalette(unsigned int a_handle, std::shared_ptr<PaletteData> a_palette);

    // Convenience mode used by legacy callers: true selects LEQUAL and false
    // selects ALWAYS. Retail opcode 0x1c supports all eight table entries via
    // DAT_00431684; opcode 0x1d controls depth writes independently.
    void SetDepthTest(bool a_enabled);
    // Retail DrawList opcode 0x1c: table index selects the compare and enables
    // depth testing. Index order is verified in EBOOT.elf at 0x44d4c.
    void SetDepthTestCompareIndex(int a_index);
    void SetDepthWrite(bool a_enabled);
    // Video.setLineWidth(w); retail stores it at graphics-state +0x8210.
    // The desktop backend passes the requested value to OpenGL directly.
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

    // Video.setCameraPos(x,y,z). Its retail world-view effect is represented
    // by a desktop approximation until FUN_0004cea0 is fully lifted.
    void SetCameraPos(float a_x, float a_y, float a_z = 0.0f);
    // Retail FUN_0004cea0 consumes Video.setCameraRotation's stored
    // axis-angle fields (+0x81fc/+0x8200/+0x8204/+0x8208). GL uses degrees,
    // while the shipped script quaternion helpers supply radians.
    void SetCameraRotation(float a_angleRadians, float a_axisX,
                           float a_axisY, float a_axisZ) {
        m_cameraRotationAngle = a_angleRadians;
        m_cameraRotationX = a_axisX;
        m_cameraRotationY = a_axisY;
        m_cameraRotationZ = a_axisZ;
    }
    void SetViewMode(ViewMode a_mode) {
        m_viewMode = a_mode;
        m_hasViewAspectOverride = false;
    }
    // FUN_0004cea0's boolean selects the axis-angle camera transform branch:
    // false jumps directly to the unrotated camera-centred bounds path.
    void SetWorldView(bool a_cameraRotationEnabled) {
        m_viewMode = ViewMode::WorldPerspective;
        m_hasViewAspectOverride = false;
        m_worldViewRotationEnabled = a_cameraRotationEnabled;
    }
    // RETAIL PPC: FUN_0003e4a0 reads Video+0x81bc while constructing the
    // aspect-ortho matrix. Keep that value with a deferred draw-list command
    // instead of consulting the later window/video state during replay.
    void SetAspectRatioOrtho(float a_aspect) {
        m_viewMode = ViewMode::AspectRatioOrtho;
        m_viewAspectOverride = a_aspect;
        m_hasViewAspectOverride = true;
    }
    // DrawList.setupOrthoView(left, right, bottom, top) -- Sound Shapes'
    // own 4-argument form of setupOrthoView (see DC_gmf_setupOrthoView).
    void SetCustomOrtho(float a_left, float a_right, float a_bottom, float a_top) {
        m_orthoLeft = a_left; m_orthoRight = a_right;
        m_orthoBottom = a_bottom; m_orthoTop = a_top;
        m_viewMode = ViewMode::CustomOrtho;
        m_hasViewAspectOverride = false;
    }
    ViewMode GetViewMode() const { return m_viewMode; }
    ViewState GetViewState() const {
        return {m_viewMode, m_viewAspectOverride, m_hasViewAspectOverride,
                m_worldViewRotationEnabled};
    }
    void RestoreViewState(const ViewState& a_state) {
        m_viewMode = a_state.mode;
        m_viewAspectOverride = a_state.aspectOverride;
        m_hasViewAspectOverride = a_state.hasAspectOverride;
        m_worldViewRotationEnabled = a_state.worldViewRotationEnabled;
    }
    // Compatibility shim for current desktop call sites: true selects the
    // approximate world view, false selects aspect-ratio ortho.
    void SetUseCameraView(bool a_use) {
        m_viewMode = a_use ? ViewMode::WorldPerspective : ViewMode::AspectRatioOrtho;
        m_hasViewAspectOverride = false;
        m_worldViewRotationEnabled = false;
    }
    float GetCameraX() const { return m_cameraX; }
    float GetCameraY() const { return m_cameraY; }
    float GetCameraZ() const { return m_cameraZ; }
    // Camera pitch in degrees; retail field mapping is known, but the desktop
    // GL rotation's equivalence to the RSX matrix path remains unverified.
    void SetCameraPitch(float a_degrees) { m_cameraPitch = a_degrees; }
    float GetCameraPitch() const { return m_cameraPitch; }

    // Draws a_count 2D vertices (world-space xy pairs) as a_glPrimitiveMode,
    // tinted by the current colour and untextured. a_z is the Z depth the
    // whole primitive draws at.
    void DrawPolygon(unsigned int a_glPrimitiveMode, const float* a_xy, int a_count, float a_z = 0.0f);
    // Execute the statically decoded retail DefaultTrans vertex/color path for
    // untextured retained player shapes. The host matrix remains an adapter.
    void DrawPolygonDefaultTrans(unsigned int a_glPrimitiveMode, const float* a_xy,
                                 int a_count, float a_z = 0.0f);

    // FireGroundShader2's recovered vertex operation: position.xy += data.xy * amp.
    // The source of retail TEXCOORD0/index 8 is unresolved; callers mark any
    // synthesized data stream as an approximation pending capture verification.
    void DrawFireGroundStrip(const float* a_xy, const float* a_data, int a_count,
                             float a_amp, float a_z);

    // Verification: PPC inspection of EBOOT 0x1ad708 and 0x4c094/0x4c0a0
    // establishes pre-scaled UVs for texture-configured shape meshes.
    // Verification: desktop OpenGL implementation. Textured shape UV extent
    // handling follows retail `CreateTextBubble` PPC; TextureDist selection
    // reuses the recovered fragment equation and category-5 draw-color
    // binding. TextureTrans selector 0 uses the linked Cg fragment operation
    // and verified category-5 color route. TextureTransFast selectors 1 and 2
    // use the shared bytecode-confirmed texture-only fragment; selector 2
    // disables blending because its native descriptor flag is zero.
    // TextureDist aSmoothLevel is native config row 3 (Cg paramno 4); the
    // setter/reset values are mapped, while default and depth parity remain
    // open (see TextureDist_Uniform_Analysis.md).
    // Per-vertex-coloured variant -- backs FoE's radial-gradient
    // beginShape/plotVertex/endShape idiom (PekkoAndJon/bg.gm alternates
    // alpha between a fan's centre and its outer ring to fade a colour out
    // to transparent). a_rgba has 4 floats per vertex. a_textureHandle == 0
    // draws untextured. a_uv supplies real per-vertex UVs when the shape has
    // them (ShapeData::uvs, via getVertex(i).u/.v); pass nullptr to fall
    // back to UV = vertex xy remapped into the shape's bounding box.
    // Set a_uvAlreadyIncludesTextureExtent for authored shape UVs that retail
    // has already multiplied by texture fields +0x34/+0x38; ordinary logical
    // image UVs continue to receive the renderer's occupied-POT scale.
    void DrawPolygonGradient(unsigned int a_glPrimitiveMode, const float* a_xy, const float* a_rgba,
                             const float* a_uv, int a_count, unsigned int a_textureHandle,
                             float a_z = 0.0f, bool a_uvAlreadyIncludesTextureExtent = false,
                             bool a_distanceTexture = false,
                             float a_distanceDiscardLevel = 0.45f,
                             float a_distanceSmoothLevel = 0.45f,
                             int a_textureShaderType = -1);
    // Retail's Palette4bOverlayTrans config draws one shape with a base and
    // overlay animation texture. Coordinates for the overlay use the retail
    // primary/overlay image-size ratio.
    bool DrawPolygonPaletteOverlay(unsigned int a_glPrimitiveMode, const float* a_xy,
                                   const float* a_uv, int a_count,
                                   unsigned int a_backTexture, unsigned int a_frontTexture,
                                   float a_frontScaleX, float a_frontScaleY,
                                   float a_alpha, float a_z = 0.0f);

    // Desktop approximation of the retail DrawList.clear opcode-4 triangle.
    void DrawFullscreenOverlay(float a_z = 0.0f, unsigned int a_mode = 9u);

    // drawList.drawTextureByHandle(x0,y0,...,x3,y3,handle) -- four
    // world-space corners, tinted by the current colour.
    void DrawTexturedQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                           float a_x2, float a_y2, float a_x3, float a_y3,
                           unsigned int a_textureHandle, float a_z = 0.0f,
                           bool a_distanceTexture = false,
                           float a_distanceDiscardLevel = 0.5f,
                           float a_distanceSmoothLevel = 0.45f,
                           int a_textureShaderType = -1,
                           float a_uvScaleU = 1.0f,
                           float a_uvScaleV = 1.0f,
                           unsigned int a_primitiveMode = GL_QUADS);

    // A glyph quad with explicit UVs and the two thresholds encoded by
    // FUN_0003e040. Its alpha channel contains the grayscale glyph distance.
    void DrawGlyphQuad(float a_x0, float a_y0, float a_x1, float a_y1,
                        float a_x2, float a_y2, float a_x3, float a_y3,
                        unsigned int a_textureHandle, float a_u0, float a_v0, float a_u1, float a_v1,
                        float a_edge, float a_width, float a_z = 0.0f);

    // Desktop letterbox bars. Their equivalence to the retail RSX display
    // path has not been verified. EndFrame() calls this before buffer swap.
    void DrawLetterboxBars();

private:
    PlatformWindow* m_window = nullptr; // non-owning; frontend outlives Renderer
    bool m_isFullscreen = false;

    // m_width/m_height are the desktop logical size; m_rawWidth/m_rawHeight
    // are actual host-window pixels. The retail display-field correspondence
    // and bar calculation remain open.
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
    // its x/y). Any scene that never calls setCameraPos (diagnostic scene's
    // hello-world Main.gm included) rendered nothing at all until this was
    // fixed -- confirmed by instrumenting DrawTexturedQuad's own clip-space
    // math, not a guess.
    float m_cameraX = 0.0f, m_cameraY = 0.0f, m_cameraZ = 26.879999f;
    float m_cameraRotationAngle = 0.0f;
    float m_cameraRotationX = 0.0f, m_cameraRotationY = 0.0f,
          m_cameraRotationZ = 0.0f;
    // setupOrthoView(l,r,b,t)'s explicit bounds; only read in CustomOrtho
    // mode. Defaults are the 0-arg form's own (0,1,0,1) constants.
    float m_orthoLeft = 0.0f, m_orthoRight = 1.0f, m_orthoBottom = 0.0f, m_orthoTop = 1.0f;
    // Used verbatim, including zero/negative values, when the flag is set.
    float m_viewAspectOverride = 0.0f;
    bool m_hasViewAspectOverride = false;
    bool m_worldViewRotationEnabled = false;
    float m_cameraPitch = 0.0f;
    Shader m_distanceTextureShader;
    bool m_distanceTextureShaderReady = false;
    Shader m_textureTransShader;
    bool m_textureTransShaderReady = false;
    Shader m_textureTransFastShader;
    bool m_textureTransFastShaderReady = false;
    // Retail Selection_f and TextureSelection_f alternate configs. The
    Shader m_selectionShader;
    bool m_selectionShaderReady = false;
    Shader m_textureSelectionShader;
    bool m_textureSelectionShaderReady = false;
    Shader m_textureDistSelectionShader;
    bool m_textureDistSelectionShaderReady = false;
    Shader m_paletteOverlayShader;
    bool m_paletteOverlayShaderReady = false;
    Shader m_glyphMaskShader;
    bool m_glyphMaskShaderReady = false;
    Shader m_fireGroundShader;
    bool m_fireGroundShaderReady = false;
    Shader m_defaultTransShader;
    bool m_defaultTransShaderReady = false;
    bool m_loadingBarProgressShapeExpected = false;
    ViewMode m_viewMode = ViewMode::AspectRatioOrtho;

    std::unordered_map<std::string, unsigned int> m_texturesByPath;
    std::unordered_map<unsigned int, Texture> m_texturesByHandle;

    // Applies retail's projection + modelview for whichever view mode is
    // currently active, straight onto the GL matrix stack.
    void ApplyProjection() const;
    // Binds a texture and returns its native UV extent. Returns 1,1 for an
    // unknown handle.
    void BindTextureWithUVScale(unsigned int a_handle, float& a_uMax, float& a_vMax);
};
