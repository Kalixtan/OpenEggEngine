#pragma once

// Backing state for Video.*'s confirmed scalar getters/setters
// (Renderer's colour/depth-test flags, EngineContext's font pointers).
// Plain old data, no behaviour -- see src/bindings/video_bindings.cpp for
// the GM-facing side.
struct VideoState {
    // camPosZ default confirmed live on real RPCS3/retail (26.879999) via
    // game_data_testcart's TEST 410 -- presumably the fixed distance the
    // camera sits back from the 2D play plane, not an arbitrary value.
    float camPosX = 0.0f, camPosY = 0.0f, camPosZ = 26.879999f;
    float camRotAngle = 0.0f, camRotX = 0.0f, camRotY = 0.0f, camRotZ = 0.0f;
    float grayscalePerc = 0.0f;
    float aspectRatio = 16.0f / 9.0f;
    float screenViewScale = 1.0f;
    bool needSetScreenViewScale = false;
    bool isDrawingPhysics = false;
    bool showFPS = false;
    // Ghidra-confirmed default (this session): retail's GL device-init sets
    // this to 2.0, not 1.0 -- see gmf_setLineWidth's header comment.
    float lineWidth = 2.0f;
    float textShadowSize = 0.0f;
    bool shouldDrawEntities = true;
    int currentScreenX = 0, currentScreenY = 0;
    bool needsFlushStaticSurface = false;
};
