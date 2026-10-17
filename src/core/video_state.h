#pragma once
#include <cmath>
#include <cstdint>
#include <array>
#include <map>
#include <vector>

struct VideoState {
    std::map<uintptr_t, int> drawMapEntities;

    // Payload-only desktop adapter for the records appended to retail
    // Video+0x8240 by debug_setWorldMarker/debug_setWorldLineMarker. Retail
    // stores polymorphic native records behind intrusive queue nodes. The
    // desktop currently stores equivalent parsed values, but has no renderer
    // consumer or retail-equivalent native record lifetime yet.
    struct DebugWorldMarker {
        bool isLine = false;
        bool flag = false;
        std::array<float, 9> values{};
    };
    std::vector<DebugWorldMarker> debugWorldMarkers;

    int activeDeviceWidth = 800, activeDeviceHeight = 600;
    int configuredModeWidth = 1280, configuredModeHeight = 720;
    float camPosX = 0.0f, camPosY = 0.0f, camPosZ = 26.879999f;
    bool cameraLimitsEnabled = true;
    float cameraMinX = -2441.406005859375f;
    float cameraMinY = -2441.406005859375f;
    float cameraMinZ = -2441.406005859375f;
    float cameraMaxX = 9999999.0f;
    float cameraMaxY = 9999999.0f;
    float cameraMaxZ = 9999999.0f;
    float camRotAngle = 0.0f, camRotX = 0.0f, camRotY = 0.0f, camRotZ = 0.0f;
    float grayscalePerc = 0.0f;
    // Updated once per main-loop frame from the same elapsed-ms value sent to
    // the VM. Retail Video.getFrameTime reads the timing manager's +0xeb8.
    float frameTimeMs = 0.0f;
    // State behind GAME_SetSelectionPulseRate/AlphaPerc/IsSelectionPulseEnabled.
    // Retail's Video update callback advances the ping-pong scalar only while
    // selection pulsing is enabled; its range is configured to [0.65, 1.0],
    // contour (0.5, 0.5), and speed 2/rate (initial rate 700).
    bool selectionPulseEnabled = false;
    float selectionPulseRate = 700.0f;
    float selectionPulseScale = 2.0f / 700.0f;
    float selectionPulsePhase = 0.0f;
    float selectionPulseDirection = 1.0f;
    float selectionPulsePerc = 0.0f;
    float selectionAlphaPerc = 1.0f;

    void SetSelectionPulseRate(double a_rate) {
        // GE3_Internal_AnimationSetScalarTimeScale (0x15cb0): select 1.0
        // for rate <= 0, otherwise use rate; divide in double, narrow to f32,
        // then multiply by the f32 constant 2.0.
        const double denominator = a_rate > 0.0 ? a_rate : 1.0;
        const float reciprocal = static_cast<float>(1.0 / denominator);
        selectionPulseRate = static_cast<float>(a_rate);
        selectionPulseScale = reciprocal * 2.0f;
    }

    void SetSelectionAlphaPerc(double a_percentage) {
        // Retail helper stores the f64 argument as a single-precision global;
        // it does not clamp the value.
        selectionAlphaPerc = static_cast<float>(a_percentage);
    }

    void SetSelectionPulseEnabled(bool a_enabled) {
        selectionPulseEnabled = a_enabled;
    }

    void AdvanceSelectionPulse(double a_deltaMs) {
        if (!selectionPulseEnabled) {
            // The retail setter only stores its enable byte; the Video update
            // callback publishes the disabled phase and style row.
            selectionPulsePerc = 0.75f;
            return;
        }
        float step = static_cast<float>(a_deltaMs *
                                       static_cast<double>(selectionPulseScale));
        if (!(step > 0.0f)) step = 0.0f;
        if (step > 1.0f) step = 1.0f;

        float phase = static_cast<float>(static_cast<double>(step) *
                                         static_cast<double>(selectionPulseDirection) +
                                         static_cast<double>(selectionPulsePhase));
        if (phase < 0.0f) {
            selectionPulseDirection = -selectionPulseDirection;
            phase = std::fabs(phase);
        } else if (phase > 1.0f) {
            selectionPulseDirection = -selectionPulseDirection;
            phase = 2.0f - phase;
        }
        selectionPulsePhase = phase;

        // The configured LFO contour is (0.5,0.5), which is the identity
        // contour. Its endpoints are 0.65 and 1.0; the shader receives
        // reference(1.0) minus that value.
        const float lfoValue = static_cast<float>(
            static_cast<double>(0.35f) * static_cast<double>(phase) + 0.65);
        selectionPulsePerc = static_cast<float>(1.0 -
                                                  static_cast<double>(lfoValue));
    }
    int viewportOffsetX = 0;
    int viewportOffsetY = 0;
    // Retail constructor initializes queried +0x81b0 to zero. restartVideo's
    // optional depth argument goes to a distinct +0x81b4 field.
    int screenBitDepth = 0;
    int requestedVideoModeBitDepth = 0;
    bool isFullscreen = false;
    float aspectRatio = 16.0f / 9.0f;
    float screenViewScale = 1.0f;
    bool needSetScreenViewScale = false;
    bool isDrawingPhysics = false;
    bool showFPS = false;
    bool traceFPSInfo = false;
    float lineWidth = 0.0f;
    float textShadowSize = 0.0f;
    bool shouldDrawEntities = true;
    int currentScreenX = 0, currentScreenY = 0;
    int numCellsX = 16, numCellsY = 9;
    bool needsFlushStaticSurface = false;
    bool textureResizeOverrideEnabled = false;
    // Retail Video graphics-state +0x83a4. -1 disables delayed texture purge;
    // otherwise the Video frame callback decrements this once per visit.
    int delayedTexturePurgeFrames = -1;
    // Retail Video constructor creates the 8x8 transparent `badTexture_Yo`
    // fallback at +0x838c; savedata texture decode failures return this TEXTURE.
    unsigned int badTextureFallbackHandle = 0;

    static float RetailScreenExtent(int a_cells) {
        float extent = static_cast<float>(a_cells) * 0.61f;
        extent = extent * 2.0f;
        extent = extent * 2.0f;
        return extent;
    }
    static int RetailFctiwz(float a_value) {
        if (!std::isfinite(a_value) || a_value >= 2147483648.0f ||
            a_value < -2147483648.0f)
            return INT32_MIN;
        return static_cast<int>(a_value);
    }
    int GetRetailScreenX(float a_coordinate) const {
        const int cell = RetailFctiwz(a_coordinate / RetailScreenExtent(numCellsX));
        return a_coordinate < 0.0f
            ? (cell == INT32_MIN ? INT32_MAX : cell - 1) : cell;
    }
    int GetRetailScreenY(float a_coordinate) const {
        const int cell = RetailFctiwz(a_coordinate / RetailScreenExtent(numCellsY));
        return a_coordinate < 0.0f
            ? (cell == INT32_MIN ? INT32_MAX : cell - 1) : cell;
    }

    static double RetailBoundedCameraValue(float a_value, float a_boundA,
                                           float a_boundB) {
        const double value = static_cast<double>(a_value);
        const double boundA = static_cast<double>(a_boundA);
        const double boundB = static_cast<double>(a_boundB);
        const double d4 = boundA - value;
        const double d5 = boundB - value;
        const double d6 = boundA - boundB;
        const double d7 = boundA - boundB;
        const double selected4 = d4 >= 0.0 ? boundA : value;
        double selected1 = d5 >= 0.0 ? boundB : value;
        const double selected2 = d7 >= 0.0 ? boundA : boundB;
        selected1 = d6 >= 0.0 ? selected1 : selected4;
        const double d3 = selected2 - selected1;
        selected1 = d3 >= 0.0 ? selected1 : selected2;
        return selected1;
    }

    static float RetailGrayscaleValue(double a_value) {
        const double lowerBound = 0.0;
        const double upperBound = 1.0;
        const double lowerDifference = lowerBound - a_value;
        const double aboveLower = lowerDifference >= 0.0 ? lowerBound : a_value;
        const double upperDifference = upperBound - aboveLower;
        const double bounded = upperDifference >= 0.0 ? aboveLower : upperBound;
        return static_cast<float>(bounded);
    }

    int GetRetailFPS() const {
        const float fps = 1000.0f / frameTimeMs;
        if (!std::isfinite(fps) || fps >= 2147483648.0f || fps < -2147483648.0f)
            return INT32_MIN;
        return static_cast<int>(fps);
    }

    float GetRetailCameraX() const {
        return cameraLimitsEnabled
            ? RetailBoundedCameraValue(camPosX, cameraMinX, cameraMaxX)
            : camPosX;
    }
    float GetRetailCameraY() const {
        return cameraLimitsEnabled
            ? RetailBoundedCameraValue(camPosY, cameraMinY, cameraMaxY)
            : camPosY;
    }
    float GetRetailCameraZ() const {
        return cameraLimitsEnabled
            ? RetailBoundedCameraValue(camPosZ, cameraMinZ, cameraMaxZ)
            : camPosZ;
    }
};
