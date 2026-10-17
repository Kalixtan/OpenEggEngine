#pragma once
#include <GL/glew.h>
#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include "shader.h"
#include "texture.h"
#include "../platform/platform_window.h"

class Renderer {
public:
    static constexpr float kWorldViewExtentBase = 0.44721359f;
    static constexpr float kLayerDepthRange = 1023.0f;
    static constexpr float kNearClip = 1.0f;
    static constexpr float kFarClip = 1000000.0f;

    static constexpr float kUIOrthoHalfHeight = 0.5f;

    static constexpr float kGameBoardMinX = -2399.0f;
    static constexpr float kGameBoardMinY = -1339.0f;
    static constexpr float kGameBoardMaxX = 2399.0f;
    static constexpr float kGameBoardMaxY = 1339.0f;

    // Desktop default; whether this reproduces the retail RSX line width is
    // unverified.
    static constexpr float kDefaultLineWidth = 2.0f;

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

    void SetDepthTest(bool a_enabled);
    void SetDepthTestCompareIndex(int a_index);
    void SetDepthWrite(bool a_enabled);
    // Video.setLineWidth(w); retail stores it at graphics-state +0x8210.
    // The desktop backend passes the requested value to OpenGL directly.
    void SetLineWidth(float a_width);

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

    void SetCameraPos(float a_x, float a_y, float a_z = 0.0f);
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
    void SetWorldView(bool a_cameraRotationEnabled) {
        m_viewMode = ViewMode::WorldPerspective;
        m_hasViewAspectOverride = false;
        m_worldViewRotationEnabled = a_cameraRotationEnabled;
    }
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
