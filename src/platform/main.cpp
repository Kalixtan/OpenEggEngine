// Engine entry point. Boots the GameMonkeyScript VM, registers the confirmed
// native bindings (see src/bindings/), loads the retail hello-world test
// scene (game_data_min/Main.gm, falling back to the full game_data tree for
// shared engine scripts like FontHelper.gm the mini scene doesn't carry its
// own copy of -- mirrors the real Platformer/PS3_Platformer overlay
// pattern), and drives the render loop.
#include <SDL2/SDL.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <array>
#include <box2d/box2d.h>

#include "../core/gm_vm.h"
#include "../core/strict_calls.h"
#include "../core/save_data.h"
#include "../core/process_manager.h"
#include "../core/engine_context.h"
#include "../core/system_config.h"
#include "../core/localization.h"
#include "../render/renderer.h"
#include "../render/bitmap_font.h"
#include "../render/draw_list.h"
#include "../render/draw_context.h"
#include "../ported_native/ground_renderer.h"
#include "../ported_native/player_renderer.h"
#include "../ported_native/save_data_completion.h"
#include "../bindings/gamemonkeyscript_bindings.h"
#include "../bindings/gamesystem_bindings.h"
#include "../bindings/video_bindings.h"
#include "../bindings/constants_bindings.h"
#include "../bindings/system_bindings.h"
#include "../bindings/physics_bindings.h"
#include "../bindings/physics_body_bindings.h"
#include "../bindings/physics_global_bindings.h"
#include "../core/physics_world.h"
#include "../bindings/sequencer_bindings.h"
#include "../bindings/pulseperc_bindings.h"
#include "../bindings/colour_bindings.h"
#include "../bindings/lfo_bindings.h"
#include "../bindings/savedata_bindings.h"
#include "../bindings/stringhelper_bindings.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/input_bindings.h"
#include "../bindings/vector2d_bindings.h"
#include "../bindings/texture_bindings.h"
#include "../bindings/envelope_bindings.h"
#include "../bindings/printinfo_bindings.h"
#include "../bindings/online_coordinator_bindings.h"
#include "../bindings/watermelon_bindings.h"
#include "../bindings/videohelper_bindings.h"
#include "headless_harness.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/audio_bindings.h"
#include "../bindings/mathutil_bindings.h"
#include "../bindings/math_bindings.h"
#include "../bindings/foe_compat_bindings.h"
#include "gmCall.h"

#ifndef GE3_PRIMARY_DATA_ROOT
#define GE3_PRIMARY_DATA_ROOT "../game_data_min"
#endif
#ifndef GE3_FALLBACK_DATA_ROOT
#define GE3_FALLBACK_DATA_ROOT "../game_data"
#endif

// GAME_Update/GAME_PostDraw -- reserved top-level script hooks (Main.gm:
// `global GAME_Update=function(msElapsed){...}`, `global GAME_PostDraw=
// function(){...}`), confirmed real and load-bearing: GAME_Update drives
// GAMEUPDATEDATA.update(), which is the ONLY thing that advances the save
// state machine (SAVESTATE_SAVING -> SAVEDATA_isRequestDone() poll ->
// SAVESTATE_CHECKERRORS -> SAVESTATE_SAVECOMPLETE -> SAVESTATE_READY) --
// see bindings/savedata_bindings.cpp. Never being called meant any script
// path that kicks off a save (Screen Scale confirm, Editor's "Create
// Panels" GAMEUPDATEDATA.isSaveBusy() gate, etc.) got permanently stuck
// reporting busy, since the state machine that would clear that flag never
// (unlike per-process update()/draw(), these aren't dispatched through
// ProcessManager -- they're fixed, well-known globals the native engine
// core calls directly every frame, same convention as GAME_GetPreDrawList/
// GAME_GetPostDrawList already used by GM script). GAME_PreDraw is left
// uncalled: Main.gm only defines a commented-out stub for it ("commented
// out until it's needed"), so there is nothing real to invoke there.
namespace {
void CallGlobalHookIfPresent(gmMachine& a_machine, const char* a_name, int a_intParam, bool a_hasParam) {
    gmVariable fn = a_machine.GetGlobals()->Get(&a_machine, a_name);
    if (fn.m_type != GM_FUNCTION) return;
    gmCall call;
    if (!call.BeginGlobalFunction(&a_machine, a_name)) return;
    if (a_hasParam) call.AddParam(gmVariable(a_intParam));
    call.End();
}
} // namespace

int main(int argc, char** argv) {
    // DEBUG: write startup trace to file
    FILE* trace = fopen("/tmp/gameengine3_trace.log", "w");
    if (trace) {
        fprintf(trace, "[TRACE] main() entry point reached\n");
        fflush(trace);
    }

    // --clear-save / GE3_CLEAR_SAVE=1: wipe SaveData::Dir() (the real
    // "savedata/" JSON files next to the executable, see core/save_data.h)
    // before anything else runs, so a level/campaign/settings test starts
    // from a genuinely fresh boot -- SPLASHSCREEN_FinalizeSplashScreenAndGet-
    // Results (system_bindings.cpp) sees no save.json and reports
    // SAVEDATARESULT_NEWSAVE exactly like a brand-new PS3, no leftover
    // progress/unlocks/settings from a previous run. Scanned out of argv
    // BEFORE the positional dataRoot/entryScript/processName parsing below
    // so it can be combined freely with those, in any position
    // (`gameengine3 --clear-save`, `gameengine3 ../../game_data Main.gm Main
    // --clear-save`, ...).
    bool clearSave = std::getenv("GE3_CLEAR_SAVE") != nullptr;
    {
        int write = 1;
        for (int i = 1; i < argc; ++i) {
            if (std::strcmp(argv[i], "--clear-save") == 0) { clearSave = true; continue; }
            argv[write++] = argv[i];
        }
        argc = write;
    }
    if (clearSave) {
        SaveData::ClearAll();
    }

    // GE3_HEADLESS=1: the window is created hidden (see Renderer::Init), so
    // nothing appears on the host desktop and nothing competes for the
    // host's real input focus. Deterministic timing + no frame limiter are
    // what a scripted/CI run almost always wants, so they're implied here
    // unless explicitly set already.
    if (std::getenv("GE3_HEADLESS")) {
        setenv("GE3_FIXED_TIMESTEP_MS", "17", 0);
        setenv("GE3_NO_FPS_LIMIT", "1", 0);
    }

    // Scripted input + screenshot capture (GE3_HARNESS=<script>), see
    // headless_harness.h. Disabled and zero-cost when the env var is unset.
    HeadlessHarness harness;
    harness.Init();

    // Optional overrides for probing against the full retail game_data tree
    // instead of the game_data_min hello-world scene:
    //   gameengine3 [dataRoot] [entryScript] [processName] [--clear-save]
    // With no args, behaves exactly as before (game_data_min/Main.gm).
    if (trace) fprintf(trace, "[TRACE] parsing args\n"), fflush(trace);
    std::string dataRoot = (argc > 1) ? argv[1] : GE3_PRIMARY_DATA_ROOT;
    std::string entryScript = (argc > 2) ? argv[2] : "Main.gm";
    std::string processName = (argc > 3) ? argv[3] : "Main";

    if (trace) fprintf(trace, "[TRACE] creating Renderer object\n"), fflush(trace);
    Renderer renderer;
    if (trace) fprintf(trace, "[TRACE] calling renderer.Init()...\n"), fflush(trace);
    // 800x600 -- confirmed real retail default (live Wine capture of the
    // actual "flowers of error.exe", this session), not this engine's own
    // prior 1280x720 guess. Also matches Main.gm's own fullscreen-toggle
    // call (`Video.restartVideo(800,600,!Video.isFullScreen(),0)`, KEYBOARD_F
    // handler) -- 800x600 is the literal size it restores to. This is
    // deliberately 4:3, NOT the game's internal 16:9 content aspect, which
    // is why retail visibly letterboxes (black bars top/bottom) at this
    // default size -- see Renderer::UpdateViewportLetterbox.
    if (!renderer.Init(800, 600, "GameEngine3 Reimplementation")) {
        if (trace) fprintf(trace, "[TRACE] renderer.Init() failed\n"), fflush(trace);
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] renderer.Init() succeeded\n"), fflush(trace);

    GmVm vm;
    vm.Files().AddRoot(dataRoot);
    vm.Files().AddRoot(GE3_FALLBACK_DATA_ROOT);

    // Arm the strict-call tripwire before anything can run script. Any call to
    // an engine function that hasn't been reimplemented 1-1 from the retail
    // build now stops the engine (or, in survey mode, gets listed) instead of
    // quietly returning a fabricated value -- see core/strict_calls.h.
    StrictCalls::Install(vm.Machine());

    if (trace) fprintf(trace, "[TRACE] loading fonts\n"), fflush(trace);
    BitmapFont normalFont, heavyFont;
    bool haveNormal = normalFont.Load(dataRoot + "/HelveticaNeueLTCom-Lt.json", dataRoot, renderer);
    bool haveHeavy = heavyFont.Load(dataRoot + "/HelveticaNeueLTCom-Hv.json", dataRoot, renderer);
    // FoE's PC build ships no retail JSON+PNG atlas at all (a Sound Shapes-
    // only format) -- it has grayscale-bold.ttf instead (see
    // reveng_pc/README.TXT's file listing / bitmap_font.h's LoadTTF header
    // comment). Falls back to baking that TTF ourselves rather than leaving
    // every Video.print() call invisible, which is how FoE's whole title
    // screen (PekkoAndJon/Main.gm's postDraw() instructions/settings text)
    // was rendering as nothing at all.
    if (!haveNormal) {
        if (auto ttfPath = vm.Files().Resolve("grayscale-bold.ttf")) {
            haveNormal = normalFont.LoadTTF(*ttfPath, 64.0f, renderer);
            if (haveNormal) haveHeavy = heavyFont.LoadTTF(*ttfPath, 64.0f, renderer);
        }
    }
    if (!haveNormal) {
        std::fprintf(stderr, "main: failed to load normal font, text will not render\n");
    }
    if (trace) fprintf(trace, "[TRACE] fonts loaded\n"), fflush(trace);

    // Localization: real GNU gettext .mo catalog for the configured language
    // (SystemConfig -- easy to change, see core/system_config.h) plus
    // localization/ExtraStrings.gm, read as plain text per its own header
    // comment ("this is NOT a real gm file"), NOT compiled as
    // GameMonkeyScript -- see core/localization.h.
    Localization localization;
    std::string localeDir = Localization::LocaleDirForLanguageCode(SystemConfig::Get().languageCode);
    if (auto moPath = vm.Files().Resolve("localization/" + localeDir + "/strings.mo")) {
        if (localization.LoadMoFile(*moPath)) {
            std::fprintf(stderr, "main: loaded localization/%s/strings.mo (%zu entries)\n", localeDir.c_str(), localization.Size());
        }
    } else {
        std::fprintf(stderr, "main: no strings.mo for locale '%s'\n", localeDir.c_str());
    }
    if (auto extraPath = vm.Files().Resolve("localization/ExtraStrings.gm")) {
        localization.LoadExtraStringsFile(*extraPath);
    }

    EngineContext& ctx = GetEngineContext();
    // Main.gm's built-in command-line level route reads these five
    // GameSystem getters. Keep the script-facing state identical and expose
    // it through desktop environment variables for a reproducible level
    // launch; the retail PS3 command-line spelling is not established.
    if (const char* level = std::getenv("GE3_AUTO_LOAD_LEVEL")) {
        if (*level) {
            ctx.gameSystem.isAutoLoadLevel = true;
            ctx.gameSystem.autoLoadLevel = level;
            // The desktop autoload hook targets shipped game_data levels by
            // default. Area 0 is savedata; callers can request it explicitly
            // with GE3_AUTO_LOAD_AREA=0 for a user-authored level.
            ctx.gameSystem.autoLoadArea = 1;
            if (const char* path = std::getenv("GE3_AUTO_LOAD_PATH")) {
                if (*path) ctx.gameSystem.autoLoadPath = path;
            }
            if (const char* mode = std::getenv("GE3_AUTO_LOAD_MODE")) {
                ctx.gameSystem.autoLoadMode = mode;
            }
            if (const char* area = std::getenv("GE3_AUTO_LOAD_AREA")) {
                ctx.gameSystem.autoLoadArea = std::atoi(area);
            }
        }
    }
    ctx.renderer = &renderer;
    ctx.fileHelper = &vm.Files();
    ctx.machine = &vm.Machine();
    ctx.normalFont = haveNormal ? &normalFont : nullptr;
    ctx.heavyFont = haveHeavy ? &heavyFont : nullptr;
    ctx.currentFont = ctx.normalFont;
    ctx.localization = &localization;

    RegisterConstantsBindings(vm.Machine());
    RegisterGameMonkeyScriptBindings(vm.Machine());
    RegisterGameSystemBindings(vm.Machine());
    RegisterProcessHandleBindings(vm.Machine());
    RegisterVideoBindings(vm.Machine());
    RegisterSystemBindings(vm.Machine());
    RegisterPhysicsBindings(vm.Machine());
    RegisterPhysicsBodyBindings(vm.Machine());
    RegisterPhysicsGlobalBindings(vm.Machine());
    RegisterSequencerBindings(vm.Machine());
    RegisterPulsePercBindings(vm.Machine());
    RegisterColourBindings(vm.Machine());
    RegisterLfoBindings(vm.Machine());
    RegisterSaveDataBindings(vm.Machine());
    RegisterStringHelperBindings(vm.Machine());
    RegisterEntityBindings(vm.Machine());
    RegisterVector2DBindings(vm.Machine());
    RegisterTextureBindings(vm.Machine());
    RegisterInputBindings(vm.Machine());
    InitializeDesktopControllers();
    RegisterPrintInfoBindings(vm.Machine());
    RegisterAudioBindings(vm.Machine());
    InitializeAudioOutput();
    RegisterMathUtilBindings(vm.Machine());
    RegisterMathBindings(vm.Machine());
    RegisterEnvelopeBindings(vm.Machine());
    RegisterOnlineCoordinatorBindings(vm.Machine());
    RegisterWatermelonBindings(vm.Machine());
    RegisterVideoHelperBindings(vm.Machine());
    RegisterFoECompatBindings(vm.Machine());

    // ENGINE PATCH: tag every native-registered library global so
    // StrictCalls::OnNullCall can recognize "method missing on Video/
    // GameSystem/..." as a real engine gap and report it unconditionally,
    // without needing the method name pre-catalogued in retail_symbols.inc
    // (see strict_calls.cpp's MarkNative/IsNativeTable). RegisterLibrary
    // doesn't hand back the table it created, so these are looked up by name
    // from globals after all registration above has run.
    for (const char* lib : {"GameMonkeyScript", "GameSystem", "Video", "Physics",
                             "ColourManager", "EntityManager", "Input", "Audio"}) {
        gmVariable v = vm.Machine().GetGlobals()->Get(&vm.Machine(), lib);
        StrictCalls::MarkNative(vm.Machine(), v);
    }

    // No .gm script anywhere assigns "MainSequencer =" (confirmed by
    // grepping the whole game_data corpus) -- it has to come from the
    // native engine bootstrap, exactly like retail's must, and has to
    // exist before Main.gm loads since Main.gm references it directly.
    {
        gmVariable mainSequencer = CreateSequencerInstance(vm.Machine(), 120);
        vm.Machine().GetGlobals()->Set(&vm.Machine(), "MainSequencer", mainSequencer);
        StrictCalls::MarkNative(vm.Machine(), mainSequencer);
    }

    if (trace) fprintf(trace, "[TRACE] creating ProcessManager\n"), fflush(trace);
    ProcessManager processManager(vm.Machine());
    ctx.processManager = &processManager;
    if (trace) fprintf(trace, "[TRACE] ProcessManager created\n"), fflush(trace);

    // The real FontHelper.gm now compiles clean (see engine/vendor/GMSCRIPT_PATCHES.md
    // for the '?'/'->'  operator patches that fixed this) and overrides the
    // gmf_FONTHELPER_SetNormalLargeFont native stand-in registered above --
    // that stand-in stays registered as a fallback in case this ever fails
    // to load in some environment.
    if (trace) fprintf(trace, "[TRACE] loading FontHelper.gm\n"), fflush(trace);
    if (!vm.LoadScript("FontHelper.gm")) {
        std::fprintf(stderr, "main: FontHelper.gm failed to load, using native stand-in\n");
    }
    if (trace) fprintf(trace, "[TRACE] FontHelper.gm loaded\n"), fflush(trace);

    // real game_data/Main.gm's constructor() calls CoryPopup(...) directly
    // (Main.gm:474) with no GameMonkeyScript.loadScript(
    // "Platformer/CoryPopup.gm") anywhere ahead of it in the whole corpus
    // -- confirmed by grepping every loadScript call site. Unlike process
    // factories (GameSystem.startNewProcessList/runProcess, which now
    // auto-load their .gm file on demand by naming convention -- see
    // ProcessManager::TryAutoLoadScript), CoryPopup is called as a plain
    // function, so that mechanism doesn't cover it; it still needs a
    // native preload here. Checked via Ghidra string search (EBOOT.elf,
    // see ProcessManager::TryAutoLoadScript's comment) whether retail's
    // real mechanism is recoverable this way -- it isn't ("CoryPopup.gm"
    // doesn't appear as a string literal anywhere in the binary either),
    // so this stays a native preload rather than a confirmed match for
    // whatever retail actually does.
    //
    // Gated on the PRIMARY data root actually containing Platformer/Main.gm
    // itself (not resolved through the game_data fallback root):
    // game_data_min (a minimal test scene with no Platformer/ tree of its
    // own) only ever reaches this file through the fallback root, and
    // shouldn't have this bootstrap step applied to it -- confirmed
    // necessary after this same class of unconditional preload (originally
    // also covering MainMenu5.gm, before that moved to the on-demand
    // mechanism above) broke game_data_min's rendering by pulling in
    // Platformer/MainMenuScriptLoader.gm's large, global-state-mutating
    // script sweep.
    bool isFullRetailTree = std::filesystem::exists(dataRoot + "/Platformer/Main.gm");
    if (isFullRetailTree) {
        if (trace) fprintf(trace, "[TRACE] loading Platformer/CoryPopup.gm\n"), fflush(trace);
        vm.LoadScript("Platformer/CoryPopup.gm");
    }

    if (trace) fprintf(trace, "[TRACE] loading entry script: %s\n", entryScript.c_str()), fflush(trace);
    if (!vm.LoadScript(entryScript)) {
        std::fprintf(stderr, "main: %s failed to load, aborting\n", entryScript.c_str());
        ShutdownAudioOutput();
        renderer.Shutdown();
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] entry script loaded\n"), fflush(trace);

    if (trace) fprintf(trace, "[TRACE] starting process: %s\n", processName.c_str()), fflush(trace);
    if (processManager.StartNewProcessList(processName) < 0) {
        std::fprintf(stderr, "main: GameSystem.startNewProcessList(\"%s\") failed, aborting\n", processName.c_str());
        ShutdownAudioOutput();
        renderer.Shutdown();
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] process started\n"), fflush(trace);

    if (trace) fprintf(trace, "[TRACE] entering main loop\n"), fflush(trace);
    Uint32 lastTicks = SDL_GetTicks();
    bool running = true;
    // Runtime physics-debug-draw toggle -- GE3_PHYSICS_DEBUG_DRAW=1 still
    // forces it on for a whole session, but that means relaunching just to
    // look at collision shapes for a few seconds. +/= (SDL_SCANCODE_EQUALS
    // -- same physical key as '+' on a standard layout, no shift required)
    // flips it live instead. Starts from the env var so scripted/CI runs
    // that already rely on it keep working unchanged.
    bool physicsDebugDraw = std::getenv("GE3_PHYSICS_DEBUG_DRAW") != nullptr;
    const bool profileFrames = std::getenv("GE3_PROFILE_FRAMES") != nullptr;
    std::array<Uint64, 9> phaseTicks{};
    int profiledFrames = 0;
    while (running) {
        Uint64 phaseStart = profileFrames ? SDL_GetPerformanceCounter() : 0;
        auto recordPhase = [&](size_t phase) {
            if (!profileFrames) return;
            Uint64 now = SDL_GetPerformanceCounter();
            phaseTicks[phase] += now - phaseStart;
            phaseStart = now;
        };
        // Scripted input for this frame (no-op unless GE3_HARNESS is set).
        // Dispatched through exactly the same two sinks as real SDL events
        // below, so script sees no difference between a harness key and a
        // physical one.
        std::vector<int> hKeyDown, hKeyUp, hMouseDown, hMouseUp;
        harness.BeginFrame(hKeyDown, hKeyUp, hMouseDown, hMouseUp);
        for (int sc : hKeyDown) {
            processManager.DispatchKeyEvent(sc, true);
            DispatchKeyEventToEntities(sc, true);
            int aliases[32];
            int count = KeyboardJoystickAliasCodes((SDL_Scancode)sc, aliases, 32);
            for (int i = 0; i < count; ++i) {
                processManager.DispatchKeyEvent(aliases[i], true);
                DispatchKeyEventToEntities(aliases[i], true);
            }
        }
        for (int sc : hKeyUp) {
            processManager.DispatchKeyReleaseEvent(sc);
            DispatchKeyEventToEntities(sc, false);
            int aliases[32];
            int count = KeyboardJoystickAliasCodes((SDL_Scancode)sc, aliases, 32);
            for (int i = 0; i < count; ++i) {
                processManager.DispatchKeyReleaseEvent(aliases[i]);
                DispatchKeyEventToEntities(aliases[i], false);
            }
        }
        for (int code : hMouseDown) {
            processManager.DispatchKeyEvent(code, true);
            DispatchKeyEventToEntities(code, true);
        }
        for (int code : hMouseUp) {
            processManager.DispatchKeyReleaseEvent(code);
            DispatchKeyEventToEntities(code, false);
        }

        // Mirror the harness's held keys into the virtual key state that
        // Input.isKeyPressed reads (input_bindings.h). Menus poll that
        // directly rather than relying on the onKeyPress dispatch above, so
        // without this a harness run renders the game but can never
        // navigate it. No-op when the harness is disabled.
        if (harness.enabled()) {
            ClearVirtualKeys();
            for (int sc : harness.HeldKeys()) SetVirtualKeyDown(sc, true);
        }

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            // Escape used to force-quit here, but it's also the real
            // in-game JOYSTICK0_CANCEL/CIRCLE key (input_bindings.cpp) --
            // scripts need to actually see it via onKeyPress below, not
            // have it swallowed as a debug shortcut. Close the window to
            // quit instead.
            if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                if (event.key.keysym.scancode == SDL_SCANCODE_EQUALS) {
                    physicsDebugDraw = !physicsDebugDraw;
                }
                processManager.DispatchKeyEvent(event.key.keysym.scancode, true);
                // See DispatchKeyEventToEntities's header comment
                // (entity_bindings.h) -- Player is a registered Entity, not
                // a Process, so ProcessManager's own dispatch above never
                // reached it; jump (and anything else driven through
                // Player.gm's onKeyPress) was silently dead.
                DispatchKeyEventToEntities(event.key.keysym.scancode, true);
                int aliases[32];
                int count = KeyboardJoystickAliasCodes(event.key.keysym.scancode, aliases, 32);
                for (int i = 0; i < count; ++i) {
                    processManager.DispatchKeyEvent(aliases[i], true);
                    DispatchKeyEventToEntities(aliases[i], true);
                }
            }
            if (event.type == SDL_KEYUP) {
                processManager.DispatchKeyReleaseEvent(event.key.keysym.scancode);
                DispatchKeyEventToEntities(event.key.keysym.scancode, false);
                int aliases[32];
                int count = KeyboardJoystickAliasCodes(event.key.keysym.scancode, aliases, 32);
                for (int i = 0; i < count; ++i) {
                    processManager.DispatchKeyReleaseEvent(aliases[i]);
                    DispatchKeyEventToEntities(aliases[i], false);
                }
            }
            if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED)
                InitializeDesktopControllers();
            if (event.type == SDL_CONTROLLERBUTTONDOWN || event.type == SDL_CONTROLLERBUTTONUP) {
                int codes[32];
                int count = ControllerEventCodes(event.cbutton.which, event.cbutton.button, codes, 32);
                for (int i = 0; i < count; ++i) {
                    if (event.type == SDL_CONTROLLERBUTTONDOWN)
                        processManager.DispatchKeyEvent(codes[i], true);
                    else
                        processManager.DispatchKeyReleaseEvent(codes[i]);
                    DispatchKeyEventToEntities(codes[i], event.type == SDL_CONTROLLERBUTTONDOWN);
                }
            }
            // Mouse buttons share the same onKeyPress/onKeyType/onKeyRelease
            // dispatch as keyboard keys -- confirmed real (Main.gm's onKeyPress
            // checks `arg0 == MOUSE_LEFTBUTTON` right alongside
            // `arg0 == KEYBOARD_ESCAPE` etc, same handler, same integer
            // namespace: MOUSE_LEFTBUTTON/RIGHTBUTTON/MIDDLEBUTTON = 0/1/2,
            // input_bindings.cpp). "click starts" / "click and drag
            // everything" (title screen) both route through this -- was
            // entirely unwired (zero SDL_MOUSEBUTTON handling previously),
            // so no click ever reached script.
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int code = (event.button.button == SDL_BUTTON_LEFT) ? 0
                         : (event.button.button == SDL_BUTTON_RIGHT) ? 1
                         : (event.button.button == SDL_BUTTON_MIDDLE) ? 2 : -1;
                if (code >= 0) {
                    processManager.DispatchKeyEvent(code, true);
                    DispatchKeyEventToEntities(code, true);
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                int code = (event.button.button == SDL_BUTTON_LEFT) ? 0
                         : (event.button.button == SDL_BUTTON_RIGHT) ? 1
                         : (event.button.button == SDL_BUTTON_MIDDLE) ? 2 : -1;
                if (code >= 0) {
                    processManager.DispatchKeyReleaseEvent(code);
                    DispatchKeyEventToEntities(code, false);
                }
            }
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                // The user dragged the window border (SDL_WINDOW_RESIZABLE,
                // Renderer::Init) -- update our tracked size/viewport/aspect
                // to match; vertical-fit scaling (Renderer::kWorldHalfHeight)
                // means the top/bottom of the world stays pinned to the
                // window's top/bottom at any size.
                renderer.OnWindowResized(event.window.data1, event.window.data2);
            }
        }

        // onKeyType: level-triggered, fired every frame for every key
        // currently held -- see ProcessManager::DispatchKeyEvent's header
        // comment. Cheap: only iterates scancodes actually down right now.
        const Uint8* keyState = SDL_GetKeyboardState(nullptr);
        for (int sc = 0; sc < SDL_NUM_SCANCODES; ++sc) {
            if (keyState[sc]) {
                processManager.DispatchKeyEvent(sc, false);
                int aliases[32];
                int count = KeyboardJoystickAliasCodes((SDL_Scancode)sc, aliases, 32);
                for (int i = 0; i < count; ++i) processManager.DispatchKeyEvent(aliases[i], false);
            }
        }
        // Keys held by the harness script aren't in SDL's physical keyboard
        // state, so they need their own onKeyType pass -- without this a
        // scripted `keydown Right` would fire once and then look released to
        // anything level-triggered (which is most movement code).
        for (int sc : harness.HeldKeys()) {
            if (!keyState[sc]) {
                processManager.DispatchKeyEvent(sc, false);
                int aliases[32];
                int count = KeyboardJoystickAliasCodes((SDL_Scancode)sc, aliases, 32);
                for (int i = 0; i < count; ++i) processManager.DispatchKeyEvent(aliases[i], false);
            }
        }
        int heldControllerCodes[64];
        int heldControllerCount = HeldControllerCodes(heldControllerCodes, 64);
        for (int i = 0; i < heldControllerCount; ++i)
            processManager.DispatchKeyEvent(heldControllerCodes[i], false);

        Uint32 nowTicks = SDL_GetTicks();
        unsigned int deltaMs = nowTicks - lastTicks;
        lastTicks = nowTicks;
        // GE3_FIXED_TIMESTEP_MS=<n>: ignore real elapsed wall-clock time and
        // feed a constant delta to every per-frame update instead. Retail
        // runs at a real 60fps (16.67ms/frame, confirmed by the winedbg
        // frame-timing capture -- see RENDER_PIPELINE_VERIFICATION.md), so
        // GE3_FIXED_TIMESTEP_MS=17 makes this engine's frame N correspond to
        // retail's frame N in ANIMATION time, not just frame index. Without
        // it, headless software rendering (LIBGL_ALWAYS_SOFTWARE=1) produces
        // frame deltas an order of magnitude larger than retail's, so every
        // time-driven value (bg.gm's this.amt UV radius, Main.gm's
        // transitionPerc fade, the ang/ang2 rotations) races far ahead --
        // which is exactly what made earlier frame-by-frame comparisons
        // against retail screenshots meaningless (this engine's "frame 400"
        // was ~15x further through the animation than retail's frame 400).
        // Deterministic and reproducible, so two runs of THIS engine at the
        // same frame index are also byte-comparable to each other.
        static const char* s_fixedStepEnv = std::getenv("GE3_FIXED_TIMESTEP_MS");
        if (s_fixedStepEnv) deltaMs = (unsigned int)std::atoi(s_fixedStepEnv);

        // Hardware voices keep playing between VM ticks in retail. Advance
        // their desktop handles before this frame's script reads validity or
        // position, so a newly created voice starts at position zero.
        AdvanceAudioVoices((float)deltaMs);

        // Finish configuring any entity the level-load path left with
        // configureData set but configure() never called (see
        // ConfigureUnconfiguredEntities). Runs before update/draw so an
        // entity is never simulated or drawn with its configure()-assigned
        // fields still null. Idempotent: it skips anything already done.
        ConfigureUnconfiguredEntities();

        vm.Tick(deltaMs);
        ServiceGameMonkeyScriptCleanPass(vm.Machine());
        GetEngineContext().gameSystem.currentTick++; // see game_system_state.h's getCurrentTick comment
        // See save_data_completion.cpp's header comment -- patches a real
        // data gap between recordList.gm (save-seeding) and
        // recordList.json (the campaign menu's actual display source).
        // Cheap no-op once already applied; checked every frame since
        // there's no script-side hook to run it from exactly once instead.
        EnsureCampaignSaveDataComplete();
        UpdateAllSequencers(vm.Machine(), (float)deltaMs);
        // Recovered retail movement update (FUN_00031f20) runs immediately
        // before the single Box2D world step, using the same frame timestep;
        // see Physics_Movement_Analysis.md.
        // GameSystem.setIsShouldUpdateEntities -- confirmed real (decompiled
        // setter at 0x0007058c, with a matching isShouldUpdateEntities
        // getter), and this engine had the flag plumbed into state while
        // never actually honouring it: entities and physics stepped
        // unconditionally, including while a level was still loading.
        //
        // The corpus pins the contract exactly. Platformer/Main.gm ends its
        // level load with
        //     GameSystem.setIsShouldUpdateEntities(false);
        // and its finalize-load path re-enables it immediately AFTER
        // snapping the camera onto the player:
        //     this.snapCamera = true;
        //     GameSystem.setIsShouldUpdateEntities(true);
        //
        // So retail freezes the world for the whole load, places the player
        // at the spawn point, snaps the camera to the player's screen, and
        // only then resumes simulation.
        //
        // Ignoring it was what broke entering a level. The player is created
        // at the origin and only positioned at the spawn point once loading
        // finishes, so with the world running it simply fell: measured
        // y = 0 -> -26 over the loading screen, crossing from screen row 0
        // into row -1. That screen change retargets Main.gm's camera axis a
        // full screen (21.96 units) at once, and the lerp it feeds --
        // `cp += ((dest - cp) / t) * msElapsed * perc` with
        // getCameraMoveTime() == 1.0 and perc passed literally as 1 -- has a
        // factor of msElapsed (~17), so it overshot and diverged. Captured
        // the runaway directly: successive camera x values growing by a
        // factor of about -16 per step (7.7e29 -> -1.2e31 -> 2.0e32 -> ...)
        // until they hit inf and then NaN, which is a lerp error term
        // evolving as (1 - k) with k = 17. NaN camera then poisons
        // everything downstream, including the in-view cull (every
        // comparison against NaN is false, so nothing culls).
        //
        // Gated here rather than inside the physics/entity code so the
        // scope matches the flag's name: processes (the loading screen and
        // menus) keep updating, audio keeps running, only entity simulation
        // pauses.
        // A pause process freezes process/entity callbacks in ProcessManager,
        // so the global physics steps must stop with them. Otherwise bodies
        // continue moving while the scripts that drive kinematic motion,
        // joints, and reset state are suspended (visible as background
        // objects drifting or jittering behind the pause menu).
        const bool simulateEntities = GetEngineContext().gameSystem.shouldUpdateEntities
            && !processManager.IsPaused();
        recordPhase(0); // input and SDL event dispatch
        if (simulateEntities) {
            // Script movement runs in milliseconds. StepPhysicsWorld converts
            // that duration at the stock-Box2D boundary while preserving the
            // scripts' world-units-per-millisecond velocity contract.
            const float physicsDtMs = (float)deltaMs;
            StepPhysicsBodyMovement(physicsDtMs);
            recordPhase(1); // per-body movement logic
            StepPhysicsWorld(physicsDtMs);
        } else recordPhase(1);
        recordPhase(2); // Box2D step
        // Retail's entity update dispatchers FUN_0033e4d0 and
        // FUN_0033e7e0 both read the byte at manager+0xda2 before invoking
        // entity callbacks. GameSystem.setIsShouldUpdateEntities writes
        // exactly that byte via FUN_0018a2a8. Process updates continue
        // separately while entity simulation is paused for level loading.
        processManager.UpdateAll((float)deltaMs, [&]() {
            if (GetEngineContext().gameSystem.shouldUpdateEntities) {
                UpdateEntitiesWithNativeUpdateHook((float)deltaMs);
            }
        });
        CallGlobalHookIfPresent(vm.Machine(), "GAME_Update", (int)deltaMs, true);
        // Entity postUpdate is a separate callback phase. In particular,
        // Player.gm delegates it to its active movement state, which commits
        // its final velocity before Main.gm's process postUpdate updates the
        // camera. Keep its simulation and pause rules identical to update.
        if (simulateEntities) {
            PostUpdateEntitiesWithNativeHook((float)deltaMs);
        }
        processManager.PostUpdateAll((float)deltaMs);
        // Retail's native entity-manager callback commits queued entities and
        // drains deferred deletions at the end of its update event (FUN_000c3fb0
        // -> FUN_000c3e70). The portable loop owns that callback boundary, so
        // perform the same manager flush after process/entity update callbacks
        // and before any preDraw/draw hooks enumerate committed entities.
        FlushEntityManagerFrameBoundary();
        recordPhase(3); // process/entity callbacks

        renderer.BeginFrame();
        // Keep the shared DrawContext's colour-mask mirror in step with the
        // colour writes BeginFrame just re-enabled (see
        // Renderer::SetColourMask / BeginFrame). Without this, a
        // popAttributes() later in the frame can restore a stale "masked"
        // value that no longer reflects real GL state, re-disabling colour
        // writes for everything after it.
        GetDrawContext().colourMask = false;
        processManager.PreDrawAll();
        recordPhase(4); // clear and process preDraw lists
        // Real ground-piece rendering (engine/src/ported_native/
        // ground_renderer.h) -- retail itself ported this out of GM script
        // into native C++ (Platformer_GroundProcessNEW's real registered
        // process is a near-empty stub; the actual drawing logic only
        // exists as a dead, never-called reference function left in the
        // same file), so there's no script-level hook to lean on the way
        // entity draw hooks below work -- called directly, unconditionally,
        // every frame, positioned before entity draws so grounds paint
        // behind entities/the player (painter's algorithm, no real depth
        // test active in this 2D renderer).
        DrawGroundsNative();
        recordPhase(5); // native ground geometry
        // Retail entities have an independent preDraw callback. Player's
        // native draw caches this exact method and uses it for its auxiliary
        // geometry before the player body is rendered.
        PreDrawEntitiesWithNativeHook();
        // Real, script-authored per-entity draw hooks (EntityPacks/*.gm's
        // `t.draw=function(){...}`, confirmed real -- every entity type,
        // including the player, draws itself this way). MUST run
        // unconditionally, not gated behind a debug flag: this was
        // previously only reachable through DrawAllPhysicsBodies()'s
        // opt-in GE3_PHYSICS_DEBUG_DRAW path (a leftover from when that
        // function also drew the flat-colour debug overlay unconditionally
        // via drawStaticSurface()), which meant NO entity -- player,
        // ground, obstacles, anything -- ever actually drew itself in a
        // normal run once drawStaticSurface() was rewritten to just paint
        // the background. Confirmed via a real retail screenshot
        // retail shows the player sprite, ground, and hint text over the
        // background wash; this engine was showing only the background.
        std::vector<int> nativeDrawnUids = DrawEntitiesWithNativeDrawHook();
        // Second pass: drawAlpha() -- confirmed real, separate hook (see
        // DrawEntitiesWithNativeDrawAlphaHook's header comment, Ghidra-
        // confirmed against the FoE PC binary). Run after every entity's
        // normal draw() so alpha-blended content (PekkoAndJon's player/
        // Thing enemies draw ALL their visuals here, nothing in draw())
        // composites on top of already-drawn opaque geometry, matching
        // retail's real two-pass draw/drawAlpha split.
        DrawEntitiesWithNativeDrawAlphaHook();
        // Real player rendering (engine/src/ported_native/player_renderer.h)
        // -- retail's player draw is native C++ too (Entity.forceDraw
        // dispatches through a per-entity vtable slot, not a script draw()
        // hook), so it never runs through DrawEntitiesWithNativeDrawHook()
        // above (Player.gm defines no draw() for that to call). Drawn after
        // every other entity so the player reads on top, matching how a
        // controllable character is drawn in every real gameplay screenshot.
        DrawPlayerNative();
        recordPhase(6); // entity and player rendering
        // postDraw() -- confirmed real, reserved lifecycle key (see
        // ProcessManager::PostDrawAll's header comment) -- was never called
        // at all until now, silently dropping any script content drawn
        // there (e.g. Platformer/Main.gm's "press {ACCEPT} to continue"
        // loading-gate screen, which draws into this.postDrawList from
        // inside its own postDraw()). Run after PreDrawAll() every frame,
        // same as real retail's two-pass draw order.
        processManager.PostDrawAll();
        CallGlobalHookIfPresent(vm.Machine(), "GAME_PostDraw", 0, false);
        // Opt-in only (GE3_PHYSICS_DEBUG_DRAW=1) -- no real Video/DrawList
        // "debug draw" mode exists in retail to piggyback on (checked
        // Video.setDebugID, which is just a drawList display-name tag, not
        // a rendering mode). This flat-colour Box2D/entity-marker overlay
        // is an ad-hoc position/scale/rotation verification aid now that
        // entities draw themselves for real above, not the normal path.
        if (physicsDebugDraw) {
            DrawAllPhysicsBodies(nativeDrawnUids);
        }
        // Letterbox/pillarbox bars are part of the frame's rendered output
        // (retail draws them as geometry over the finished scene, see
        // Renderer::DrawLetterboxBars), so they must go down before the
        // screenshot capture below -- otherwise a capture shows the scene
        // stretched across the full window with no bars.
        renderer.DrawLetterboxBars();
        recordPhase(7); // process postDraw and final overlays

        // Debug: GE3_SCREENSHOT_FRAME=<n> dumps that frame (0-based) to
        // /tmp/gameengine3_frame<n>.ppm right before the swap, for visual
        // comparison against the real FoE PC build.
        static int s_frameIndex = 0;
        static const char* s_screenshotFrameEnv = std::getenv("GE3_SCREENSHOT_FRAME");
        if (s_screenshotFrameEnv && s_frameIndex == std::atoi(s_screenshotFrameEnv)) {
            char path[256];
            std::snprintf(path, sizeof(path), "/tmp/gameengine3_frame%d.ppm", s_frameIndex);
            renderer.SaveScreenshot(path);
            std::fprintf(stderr, "main: saved screenshot to %s\n", path);
        }
        // Harness `shot <path>` -- same point in the frame as the
        // GE3_SCREENSHOT_FRAME capture above (after letterbox bars, before
        // the swap), so both produce identical output for the same frame.
        if (!harness.PendingScreenshot().empty()) {
            renderer.SaveScreenshot(harness.PendingScreenshot().c_str());
            std::fprintf(stderr, "[harness] frame %d -> %s\n", s_frameIndex,
                         harness.PendingScreenshot().c_str());
        }
        ++s_frameIndex;
        renderer.EndFrame();
        recordPhase(8); // swap, including any GPU synchronization
        if (profileFrames && ++profiledFrames == 120) {
            const double msPerTick = 1000.0 / (double)SDL_GetPerformanceFrequency() / profiledFrames;
            static const char* names[] = {"input", "movement", "box2d", "update", "preDraw", "ground", "entities", "postDraw", "swap"};
            std::fprintf(stderr, "[profile] average ms/frame over %d frames:", profiledFrames);
            for (size_t i = 0; i < phaseTicks.size(); ++i) {
                std::fprintf(stderr, " %s=%.2f", names[i], phaseTicks[i] * msPerTick);
                phaseTicks[i] = 0;
            }
            const b2World& world = GetPhysicsWorld();
            const b2Profile& physics = world.GetProfile();
            std::fprintf(stderr, " bodies=%d contacts=%d proxies=%d b2collide=%.2f b2solve=%.2f\n",
                         world.GetBodyCount(), world.GetContactCount(), world.GetProxyCount(),
                         physics.collide, physics.solve);
            profiledFrames = 0;
        }

        // 60fps frame limiter -- retail runs at a real, vsync-locked 60fps
        // (confirmed by the winedbg frame-timing capture: ~1689 SwapBuffers
        // over an 8s run). Without a limiter this engine free-runs, so its
        // wall-clock frame rate (and therefore the real deltaMs feeding
        // every animation) has no relationship to retail's -- making
        // side-by-side timing comparisons unreliable. Sleeps out the
        // remainder of each 16.67ms budget rather than busy-waiting.
        // Disable with GE3_NO_FPS_LIMIT=1 (useful for headless batch
        // screenshot capture, where running flat-out is preferable and
        // GE3_FIXED_TIMESTEP_MS already decouples animation from wall time).
        static const char* s_noLimit = std::getenv("GE3_NO_FPS_LIMIT");
        if (!s_noLimit) {
            const Uint32 kFrameBudgetMs = 16; // 1000/60 = 16.67, floor to stay >= 60fps
            Uint32 frameElapsed = SDL_GetTicks() - nowTicks;
            if (frameElapsed < kFrameBudgetMs) SDL_Delay(kFrameBudgetMs - frameElapsed);
        }

        if (ctx.quitRequested) running = false;
        if (harness.QuitRequested()) running = false;
    }

    if (trace) fprintf(trace, "[TRACE] shutting down renderer\n"), fflush(trace);
    ShutdownAudioOutput();
    renderer.Shutdown();
    if (trace) fprintf(trace, "[TRACE] dumping survey\n"), fflush(trace);
    StrictCalls::DumpSurvey();
    if (trace) fprintf(trace, "[TRACE] returning success\n"), fflush(trace), fclose(trace);
    return 0;
}
