// Engine entry point. Boots the GameMonkeyScript VM, registers the confirmed
// native bindings (see src/bindings/), loads the retail game_data/Main.gm
// entry point by default (with game_data_min available as a fallback for
// shared test assets), and drives the render loop.
#include "platform_window.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <array>
#include <cmath>
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
#include "../ported_native/player_renderer.h"
#include "../ported_native/save_data_completion.h"
#include "../bindings/gamemonkeyscript_bindings.h"
#include "../bindings/animation_bindings.h"
#include "../bindings/gamesystem_bindings.h"
#include "../bindings/video_bindings.h"
#include "../bindings/constants_bindings.h"
#include "../bindings/system_bindings.h"
#include "../bindings/physics_bindings.h"
#include "../bindings/matrix4_bindings.h"
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
#include "frontend_factory.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/audio_bindings.h"
#include "../bindings/mathutil_bindings.h"
#include "../bindings/math_bindings.h"
#include "../bindings/foe_compat_bindings.h"
#include "gmCall.h"

#ifndef GE3_PRIMARY_DATA_ROOT
#define GE3_PRIMARY_DATA_ROOT "../game_data"
#endif
#ifndef GE3_FALLBACK_DATA_ROOT
#define GE3_FALLBACK_DATA_ROOT "../game_data_min"
#endif
#ifndef GE3_DEFAULT_SIMULATION_SPEED_MULTIPLIER
#define GE3_DEFAULT_SIMULATION_SPEED_MULTIPLIER 1.0f
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
int KeyboardEventAliasCodes(int scancode, int* codes, int capacity) {
    // Retail PS3 menus commonly accept both KEYBOARD_LEFT and JOYSTICK0_LEFT
    // in one key handler. Dispatching both for one PC arrow press navigates
    // twice. Keep joystick aliases in held-key/onKeyType polling, where some
    // UI scripts listen only for JOYSTICK0_UP/DOWN, but avoid the duplicate
    // discrete event. Physical controller events still use their own path.
    if (scancode == PlatformKeyCode::Left || scancode == PlatformKeyCode::Right ||
        scancode == PlatformKeyCode::Up || scancode == PlatformKeyCode::Down) {
        return 0;
    }
    return KeyboardJoystickAliasCodes(scancode, codes, capacity);
}

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

    // GE3_HEADLESS=1: the frontend window is created hidden, so
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

    // Optional overrides for probing against another data root, entry script,
    // or process:
    //   gameengine3 [dataRoot] [entryScript] [processName] [--clear-save]
    // With no args, launches the retail game_data/Main.gm entry point.
    if (trace) fprintf(trace, "[TRACE] parsing args\n"), fflush(trace);
    std::string dataRoot = (argc > 1) ? argv[1] : GE3_PRIMARY_DATA_ROOT;
    std::string entryScript = (argc > 2) ? argv[2] : "Main.gm";
    std::string processName = (argc > 3) ? argv[3] : "Main";

    if (trace) fprintf(trace, "[TRACE] creating Renderer object\n"), fflush(trace);
    std::unique_ptr<PlatformFrontend> hostFrontend = CreatePlatformFrontend();
    if (!hostFrontend || !hostFrontend->Create(800, 600, "GameEngine3 Reimplementation",
                                                std::getenv("GE3_HEADLESS") != nullptr)) {
        return 1;
    }
    harness.Init(*hostFrontend);
    Renderer renderer;
    if (trace) fprintf(trace, "[TRACE] calling renderer.Init()...\n"), fflush(trace);
    // 800x600 -- confirmed real retail default (live Wine capture of the
    // actual "flowers of error.exe", this session). This is deliberately 4:3,
    // not the game's 16:9 content aspect, which is why retail letterboxes at
    // this default size. Main.gm's restartVideo key handler passes dimensions,
    // but retail PPC ignores those arguments; they are not the source of this
    // initial window size.
    if (!renderer.Init(*hostFrontend, 800, 600)) {
        if (trace) fprintf(trace, "[TRACE] renderer.Init() failed\n"), fflush(trace);
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] renderer.Init() succeeded\n"), fflush(trace);

    // RETAIL BYTECODE: graphics-mode setup at EBOOT 0x574a0 writes 1 to
    // Video+0x8184 while installing its 1280x720 mode. This enables the
    // per-path PS3_ texture-factor lookup; the desktop enters its equivalent
    // initialized Video mode here, before any scripts can request textures.
    GetEngineContext().video.textureResizeOverrideEnabled = true;

    GmVm vm;
    vm.Files().AddRoot(dataRoot);
    vm.Files().AddRoot(GE3_FALLBACK_DATA_ROOT);

    // Arm the strict-call tripwire before anything can run script. Any call to
    // an engine function that hasn't been reimplemented 1-1 from the retail
    // build now stops the engine (or, in survey mode, gets listed) instead of
    // quietly returning a fabricated value -- see core/strict_calls.h.
    StrictCalls::Install(vm.Machine());

    if (trace) fprintf(trace, "[TRACE] loading fonts\n"), fflush(trace);
    BitmapFont normalFont, heavyFont, courierFont, russianFont;
    auto loadBitmapFont = [&](BitmapFont& font, const std::string& relativeJson) {
        auto jsonPath = vm.Files().Resolve(relativeJson);
        if (!jsonPath) return false;
        return font.Load(*jsonPath, std::filesystem::path(*jsonPath).parent_path().string(),
                         renderer, &vm.Files());
    };
    bool haveNormal = loadBitmapFont(normalFont, "HelveticaNeueLTCom-Lt.json");
    bool haveHeavy = loadBitmapFont(heavyFont, "HelveticaNeueLTCom-Hv.json");
    bool haveCourier = loadBitmapFont(courierFont, "courierFont.json");
    // The Video font-resource constructor has an exact Russian font-name
    // case, and game_data ships the corresponding bitmap atlas. Keep it on
    // the same metrics/atlas path as retail instead of falling back to TTF.
    bool haveRussian = loadBitmapFont(russianFont, "Helvetica_NeueLTCyrillic55_Russian.json");
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
    ctx.courierFont = haveCourier ? &courierFont : nullptr;
    ctx.russianFont = haveRussian ? &russianFont : nullptr;
    ctx.currentFont = ctx.normalFont;
    ctx.localization = &localization;

    RegisterConstantsBindings(vm.Machine());
    RegisterGameMonkeyScriptBindings(vm.Machine());
    RegisterGameSystemBindings(vm.Machine());
    RegisterProcessHandleBindings(vm.Machine());
    RegisterVideoBindings(vm.Machine());
    RegisterSystemBindings(vm.Machine());
    RegisterPhysicsBindings(vm.Machine());
    RegisterMatrix4Bindings(vm.Machine());
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
    RegisterInputBindings(vm.Machine(), *hostFrontend);
    RefreshPlatformControllers();
    RegisterPrintInfoBindings(vm.Machine());
    RegisterAudioBindings(vm.Machine(), *hostFrontend);
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
        gmVariable mainSequencer = CreateSequencerInstance(vm.Machine(), 98);
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
    bool isFullRetailTree = vm.Files().Resolve("Platformer/Main.gm").has_value();
    if (isFullRetailTree) {
        if (trace) fprintf(trace, "[TRACE] loading Platformer/CoryPopup.gm\n"), fflush(trace);
        vm.LoadScript("Platformer/CoryPopup.gm");
    }

    if (trace) fprintf(trace, "[TRACE] loading entry script: %s\n", entryScript.c_str()), fflush(trace);
    if (!vm.LoadScript(entryScript)) {
        std::fprintf(stderr, "main: %s failed to load, aborting\n", entryScript.c_str());
        ShutdownAudioOutput();
        ReleaseTextureResourceLeases(renderer);
        renderer.ReleaseAllTextures();
        renderer.Shutdown();
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] entry script loaded\n"), fflush(trace);

    if (trace) fprintf(trace, "[TRACE] starting process: %s\n", processName.c_str()), fflush(trace);
    if (processManager.StartInitialProcess(processName) < 0) {
        std::fprintf(stderr, "main: failed to create initial process '%s', aborting\n", processName.c_str());
        ShutdownAudioOutput();
        ReleaseTextureResourceLeases(renderer);
        renderer.ReleaseAllTextures();
        renderer.Shutdown();
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] process started\n"), fflush(trace);

    if (trace) fprintf(trace, "[TRACE] entering main loop\n"), fflush(trace);
    uint32_t lastTicks = hostFrontend->Milliseconds();
    const uint64_t performanceFrequency = hostFrontend->PerformanceFrequency();
    uint64_t lastPerformanceCounter = hostFrontend->PerformanceCounter();
    unsigned int simulationTickCount = 0;
    float simulationAccumulatorMs = 0.0f;
    // FUN_0018a2e8 uses these exact float constants: f23=0x41855555
    // (16.666666 ms) and the accumulated-delta cap=0x42480001
    // (50.0000038 ms). See timing_manager_loop_18a2e8.disasm.
    constexpr float kRetailSimulationStepMs = 16.666666f;
    constexpr float kRetailSimulationDeltaCapMs = 50.0000038f;
    // Diagnostic only: scales the retail script-selected simulationSpeed
    // before it enters the retail-style accumulator. Set to 10 to test whether
    // the current script speed (often 0.1) is the source of slow motion.
    float simulationSpeedMultiplier = GE3_DEFAULT_SIMULATION_SPEED_MULTIPLIER;
    if (const char* multiplierEnv = std::getenv("GE3_SIMULATION_SPEED_MULTIPLIER")) {
        char* end = nullptr;
        const float value = std::strtof(multiplierEnv, &end);
        if (end != multiplierEnv && *end == '\0' && std::isfinite(value) && value > 0.0f)
            simulationSpeedMultiplier = value;
    }
    if (simulationSpeedMultiplier != 1.0f)
        std::fprintf(stderr, "main: simulation speed multiplier %.6g (GE3_SIMULATION_SPEED_MULTIPLIER)\n",
                     simulationSpeedMultiplier);
    bool running = true;
    // Runtime physics-debug-draw toggle -- GE3_PHYSICS_DEBUG_DRAW=1 still
    // forces it on for a whole session, but that means relaunching just to
    // look at collision shapes for a few seconds. +/= (USB HID equals key
    // -- same physical key as '+' on a standard layout, no shift required)
    // flips it live instead. Starts from the env var so scripted/CI runs
    // that already rely on it keep working unchanged.
    const char* physicsDebugDrawEnv = std::getenv("GE3_PHYSICS_DEBUG_DRAW");
    // Treat the documented `=1` as the opt-in. Testing only whether the
    // variable exists made common launch settings such as `=0` enable the
    // collision overlay, which can look like white geometry on kinematic
    // fixtures and obscure the actual sprite/ground rendering being audited.
    bool physicsDebugDraw = physicsDebugDrawEnv &&
                            std::strcmp(physicsDebugDrawEnv, "1") == 0;
    const bool profileFrames = std::getenv("GE3_PROFILE_FRAMES") != nullptr;
    std::array<uint64_t, 9> phaseTicks{};
    int profiledFrames = 0;
    while (running) {
        uint64_t phaseStart = profileFrames ? hostFrontend->PerformanceCounter() : 0;
        auto recordPhase = [&](size_t phase) {
            if (!profileFrames) return;
            uint64_t now = hostFrontend->PerformanceCounter();
            phaseTicks[phase] += now - phaseStart;
            phaseStart = now;
        };
        // Scripted input for this frame (no-op unless GE3_HARNESS is set).
        // Dispatched through exactly the same two sinks as frontend events
        // below, so script sees no difference between a harness key and a
        // physical one.
        std::vector<int> hKeyDown, hKeyUp, hMouseDown, hMouseUp;
        harness.BeginFrame(hKeyDown, hKeyUp, hMouseDown, hMouseUp);
        for (int sc : hKeyDown) {
            processManager.DispatchKeyEvent(sc, true);
            DispatchKeyEventToEntities(sc, true);
            int aliases[32];
            int count = KeyboardEventAliasCodes(sc, aliases, 32);
            for (int i = 0; i < count; ++i) {
                processManager.DispatchKeyEvent(aliases[i], true);
                DispatchKeyEventToEntities(aliases[i], true);
            }
        }
        for (int sc : hKeyUp) {
            processManager.DispatchKeyReleaseEvent(sc);
            DispatchKeyEventToEntities(sc, false);
            int aliases[32];
            int count = KeyboardEventAliasCodes(sc, aliases, 32);
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

        PlatformEvent event;
        while (hostFrontend->PollEvent(event)) {
            switch (event.type) {
                case PlatformEventType::Quit:
                    running = false;
                    break;
                case PlatformEventType::KeyDown:
                    // Escape is an in-game cancel action; close the host
                    // window to quit rather than swallowing that key.
                    if (event.repeat) break;
                    if (event.code == PlatformKeyCode::Equals) physicsDebugDraw = !physicsDebugDraw;
                    processManager.DispatchKeyEvent(event.code, true);
                    DispatchKeyEventToEntities(event.code, true);
                    {
                        int aliases[32];
                        const int count = KeyboardEventAliasCodes(event.code, aliases, 32);
                        for (int i = 0; i < count; ++i) {
                            processManager.DispatchKeyEvent(aliases[i], true);
                            DispatchKeyEventToEntities(aliases[i], true);
                        }
                    }
                    break;
                case PlatformEventType::KeyUp:
                    processManager.DispatchKeyReleaseEvent(event.code);
                    DispatchKeyEventToEntities(event.code, false);
                    {
                        int aliases[32];
                        const int count = KeyboardEventAliasCodes(event.code, aliases, 32);
                        for (int i = 0; i < count; ++i) {
                            processManager.DispatchKeyReleaseEvent(aliases[i]);
                            DispatchKeyEventToEntities(aliases[i], false);
                        }
                    }
                    break;
                case PlatformEventType::ControllerAdded:
                case PlatformEventType::ControllerRemoved:
                    RefreshPlatformControllers();
                    break;
                case PlatformEventType::ControllerButtonDown:
                case PlatformEventType::ControllerButtonUp: {
                    int codes[32];
                    const int count = ControllerEventCodes(event.device, event.code, codes, 32);
                    const bool down = event.type == PlatformEventType::ControllerButtonDown;
                    for (int i = 0; i < count; ++i) {
                        if (down) processManager.DispatchKeyEvent(codes[i], true);
                        else processManager.DispatchKeyReleaseEvent(codes[i]);
                        DispatchKeyEventToEntities(codes[i], down);
                    }
                    break;
                }
                case PlatformEventType::MouseButtonDown:
                case PlatformEventType::MouseButtonUp: {
                    const bool down = event.type == PlatformEventType::MouseButtonDown;
                    const int code = event.code;
                    if (code >= 0) {
                        if (down) processManager.DispatchKeyEvent(code, true);
                        else processManager.DispatchKeyReleaseEvent(code);
                        DispatchKeyEventToEntities(code, down);
                    }
                    break;
                }
                case PlatformEventType::Resized:
                    renderer.OnWindowResized(event.width, event.height);
                    break;
                case PlatformEventType::None:
                    break;
            }
        }

        // onKeyType: level-triggered, fired every frame for every key
        // currently held -- see ProcessManager::DispatchKeyEvent's header
        // comment. Cheap: only iterates scancodes actually down right now.
        int keyCount = 0;
        const uint8_t* keyState = hostFrontend->KeyboardState(keyCount);
        for (int sc = 0; sc < keyCount; ++sc) {
            if (keyState[sc]) {
                processManager.DispatchKeyEvent(sc, false);
                int aliases[32];
                int count = KeyboardJoystickAliasCodes(sc, aliases, 32);
                for (int i = 0; i < count; ++i) processManager.DispatchKeyEvent(aliases[i], false);
            }
        }
        // Keys held by the harness script aren't in the physical keyboard
        // state, so they need their own onKeyType pass -- without this a
        // scripted `keydown Right` would fire once and then look released to
        // anything level-triggered (which is most movement code).
        for (int sc : harness.HeldKeys()) {
            if (!keyState[sc]) {
                processManager.DispatchKeyEvent(sc, false);
                int aliases[32];
                int count = KeyboardJoystickAliasCodes(sc, aliases, 32);
                for (int i = 0; i < count; ++i) processManager.DispatchKeyEvent(aliases[i], false);
            }
        }
        int heldControllerCodes[64];
        int heldControllerCount = HeldControllerCodes(heldControllerCodes, 64);
        for (int i = 0; i < heldControllerCount; ++i)
            processManager.DispatchKeyEvent(heldControllerCodes[i], false);

        uint32_t nowTicks = hostFrontend->Milliseconds();
        const uint64_t nowPerformanceCounter = hostFrontend->PerformanceCounter();
        const double wallElapsedMs = performanceFrequency != 0
            ? (static_cast<double>(nowPerformanceCounter - lastPerformanceCounter) * 1000.0 /
               static_cast<double>(performanceFrequency))
            : static_cast<double>(nowTicks - lastTicks);
        lastPerformanceCounter = nowPerformanceCounter;
        lastTicks = nowTicks;
        // GE3_FIXED_TIMESTEP_MS=<n> overrides only the measured elapsed time
        // supplied to the retail-style accumulator below. A rendered frame
        // may then execute zero or more fixed simulation updates, depending
        // on the carried remainder; rendering and hardware audio still use
        // the real outer-loop cadence.
        float elapsedMs = static_cast<float>(wallElapsedMs);
        static const char* s_fixedStepEnv = std::getenv("GE3_FIXED_TIMESTEP_MS");
        if (s_fixedStepEnv) elapsedMs = std::strtof(s_fixedStepEnv, nullptr);

        // Hardware voices advance from elapsed wall time between simulation
        // ticks in retail. Keep this independent of the fixed simulation
        // cadence so audio is not stalled on a render frame with no tick.
        AdvanceAudioVoices(static_cast<float>(wallElapsedMs));

        // RETAIL PPC: FUN_0019bbb0 calls FUN_0018a2e8 once as the retail main
        // loop (direct branch at 0x0019cbfc). The loop initializes f25 to
        // zero once, then carries its remainder across rendered frames,
        // accumulating elapsed*simulationSpeed, capping the total at
        // 50.0000038 ms, and dispatching 16.666666 ms updates. This desktop
        // accumulator has the same lifetime and tick threshold.
        const float speed = GetEngineContext().gameSystem.simulationSpeed * simulationSpeedMultiplier;
        // RETAIL PPC: FUN_0018a2e8 uses `fmadds f25,f24,f1,f25` (one
        // single-precision fused multiply-add) to add scaled elapsed time to
        // the carried remainder, then `fsel` caps the total at 50.0000038 ms.
        // Keep that fused rounding and total-cap order: splitting multiply
        // and add can move a frame across the 16.666666 ms tick threshold.
        const float accumulatedElapsedMs = std::fma(
            elapsedMs, speed, simulationAccumulatorMs);
        const float timeUntilCapMs = kRetailSimulationDeltaCapMs - accumulatedElapsedMs;
        simulationAccumulatorMs = timeUntilCapMs >= 0.0f
            ? accumulatedElapsedMs : kRetailSimulationDeltaCapMs;
        // RETAIL PPC: GameSystem.isLoading (field +0xf3c) replaces the
        // accumulator with exactly one 16.666666 ms tick each outer-loop
        // pass. This prevents load work from catching up with multiple
        // simulation ticks after a slow frame. It is separate from
        // shouldUpdateEntities (+0xda2); see the field-helper decomp and
        if (GetEngineContext().gameSystem.isLoadingFlag)
            simulationAccumulatorMs = kRetailSimulationStepMs;
        recordPhase(0); // frontend input and event dispatch
        while (simulationAccumulatorMs >= kRetailSimulationStepMs) {
            const float simDtMs = kRetailSimulationStepMs;
            // The vendored GameMonkey scheduler takes integer milliseconds;
            // distribute the retail 50 ms / 3 tick cadence as 17,17,16 so
            // its cumulative clock does not run 2% fast from rounding every
            // 16.666666 ms tick upward.
            const unsigned int scriptDtMs = (simulationTickCount % 3 == 2) ? 16u : 17u;
            ++simulationTickCount;
            GetEngineContext().video.frameTimeMs = simDtMs;

            // Retail's AnimationModule manager checks GameSystem.isPaused()
            // before visiting its animation list.
            if (!processManager.IsPaused()) AdvanceAnimationClock(simDtMs);

            ConfigureUnconfiguredEntities();
            vm.Tick(scriptDtMs);
            ServiceGameMonkeyScriptCleanPass(vm.Machine());
            GetEngineContext().gameSystem.currentTick++;
            EnsureCampaignSaveDataComplete();
            UpdateAllSequencers(vm.Machine(), simDtMs);

            // GameSystem.setIsShouldUpdateEntities is bytecode-backed and
            // gates both entity callbacks and physics during level loading.
            // A pause process also freezes the physics world while its script
            // callbacks are suspended.
            const bool gamePaused = processManager.IsPaused();
            const bool updateEntities = GetEngineContext().gameSystem.shouldUpdateEntities
                && !gamePaused;
            // Retail calls PhysicsManagerTick when the pause target (+0x48),
            // loading flag (+0xf3c), and startup suppression flag (+0x350)
            // are clear. Its bytecode does not gate that call on
            // shouldUpdateEntities (+0xda2), so keep the independent entity
            // dispatch flag from suppressing physics by itself. Desktop has
            // no separate +0x350 field: retail's only setter call is behind
            // a startup flag initialized to zero with no later write in that
            // path (see PHYSICS_TIMING_CROSSWALK.md). Revisit if another
            // startup entry or build provides a writer.
            const bool stepPhysics = !gamePaused
                && !GetEngineContext().gameSystem.isLoadingFlag;
            if (stepPhysics) {
                StepPhysicsBodyMovement(simDtMs);
                recordPhase(1);
                StepPhysicsWorld(simDtMs);
            } else recordPhase(1);
            recordPhase(2);

            processManager.UpdateAll(simDtMs, [&]() {
                if (GetEngineContext().gameSystem.shouldUpdateEntities)
                    UpdateEntitiesWithNativeUpdateHook(simDtMs);
            });
            CallGlobalHookIfPresent(vm.Machine(), "GAME_Update",
                                     static_cast<int>(scriptDtMs), true);
            if (updateEntities) PostUpdateEntitiesWithNativeHook(simDtMs);
            processManager.PostUpdateAll(simDtMs);
            FlushEntityManagerFrameBoundary();
            recordPhase(3);
            simulationAccumulatorMs -= kRetailSimulationStepMs;
        }

        renderer.BeginFrame();
        UpdateVideoTextureMaintenance();
        // Keep the shared DrawContext's colour-mask mirror in step with the
        // colour writes BeginFrame just re-enabled (see
        // Renderer::SetColourMask / BeginFrame). Without this, a
        // popAttributes() later in the frame can restore a stale "masked"
        // value that no longer reflects real GL state, re-disabling colour
        // writes for everything after it.
        GetDrawContext().colourMask = false;
        processManager.PreDrawAll();
        recordPhase(4); // clear and process preDraw lists
        // Video.setIsShouldDrawEntities/isShouldDrawEntities are bytecode-
        // verified accessors for the retail manager flag. Main.gm brackets
        // level construction with false and re-enables it after camera/screen
        // setup. The exact retail draw-loop consumer is still being traced;
        // use the exposed flag here so those real transitions suppress the
        // entity render passes instead of continuing to visit every object.
        std::vector<int> nativeDrawnUids;
        if (GetEngineContext().video.shouldDrawEntities) {
            // Retail entities have an independent preDraw callback. Player's
            // native draw caches this method for auxiliary geometry.
            PreDrawEntitiesWithNativeHook();
            // Script-authored per-entity draw and drawAlpha are separate
            // retail passes. The player body is native and draws after them.
            nativeDrawnUids = DrawEntitiesWithNativeDrawHook();
            DrawEntitiesWithNativeDrawAlphaHook();
            DrawPlayerNative();
        }
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
            const double msPerTick = 1000.0 / (double)hostFrontend->PerformanceFrequency() / profiledFrames;
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
            const uint32_t kFrameBudgetMs = 16; // 1000/60 = 16.67, floor to stay >= 60fps
            const uint32_t frameElapsed = hostFrontend->Milliseconds() - nowTicks;
            if (frameElapsed < kFrameBudgetMs) hostFrontend->Delay(kFrameBudgetMs - frameElapsed);
        }

        if (ctx.quitRequested) running = false;
        if (harness.QuitRequested()) running = false;
    }

    if (trace) fprintf(trace, "[TRACE] shutting down renderer\n"), fflush(trace);
    ShutdownAudioOutput();
    ReleaseTextureResourceLeases(renderer);
    renderer.ReleaseAllTextures();
    renderer.Shutdown();
    if (trace) fprintf(trace, "[TRACE] dumping survey\n"), fflush(trace);
    StrictCalls::DumpSurvey();
    if (trace) fprintf(trace, "[TRACE] returning success\n"), fflush(trace), fclose(trace);
    return 0;
}
