#pragma once
#include "file_helper.h"
#include "localization.h"
#include "game_system_state.h"
#include "video_state.h"
#include "../render/renderer.h"
#include "../render/bitmap_font.h"

class ProcessManager;
class gmMachine;

// Shared state GM native bindings reach into. Real GameEngine3 has singleton
// Video/GameSystem/FileHelper subsystems shared across all processes (per
// "Video.* and drawList.* really are (mostly) the same underlying command list")
// this mirrors that rather than threading pointers through every binding call.
struct EngineContext {
    Renderer* renderer = nullptr;
    FileHelper* fileHelper = nullptr;
    BitmapFont* normalFont = nullptr; // Video.setNormalFont() -- HelveticaNeueLTCom-Lt
    BitmapFont* heavyFont = nullptr;  // Video.setHeavyFont()   -- HelveticaNeueLTCom-Hv
    BitmapFont* currentFont = nullptr;
    ProcessManager* processManager = nullptr; // for GameSystem.startNewProcessList/runProcess
    gmMachine* machine = nullptr; // the one live gmMachine (GmVm::Machine()) -- for debug-draw code that needs table->Get(&m,key) outside a gmThread call
    Localization* localization = nullptr; // backs the real `_(str)` global function
    GameSystemState gameSystem; // flags/timers/autoload backing GameSystem.* -- see core/game_system_state.h
    VideoState video; // scalar state backing Video.* -- see core/video_state.h
    bool quitRequested = false; // set by GameSystem.quit()
};

inline EngineContext& GetEngineContext() {
    static EngineContext ctx;
    return ctx;
}
