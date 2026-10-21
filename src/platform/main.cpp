#include "platform_window.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <array>
#include <cmath>
#include <box2d/box2d.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

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
#include "../bindings/script_compat_bindings.h"
#include "gmCall.h"

namespace {
bool SetEnvironmentIfUnset(const char* a_name, const char* a_value) {
    if (std::getenv(a_name)) return true;
#if defined(_WIN32)
    return _putenv_s(a_name, a_value) == 0;
#else
    return setenv(a_name, a_value, 0) == 0;
#endif
}

FILE* OpenTextFileForWrite(const std::filesystem::path& a_path) {
#if defined(_WIN32)
    return _wfopen(a_path.c_str(), L"w");
#else
    return std::fopen(a_path.c_str(), "w");
#endif
}

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

bool ShowStrictCallDialog(void* context, const char* report) {
    auto* frontend = static_cast<PlatformFrontend*>(context);
    return frontend && frontend->ShowUnimplementedFunctionDialog(
        "Unimplemented engine function", report);
}
} // namespace

int main(int argc, char** argv) {
    // DEBUG: write startup trace to file
    std::error_code tempPathError;
    std::filesystem::path tempDirectory = std::filesystem::temp_directory_path(tempPathError);
    if (tempPathError) {
        tempPathError.clear();
        tempDirectory = std::filesystem::current_path(tempPathError);
        if (tempPathError) tempDirectory = ".";
    }
    FILE* trace = OpenTextFileForWrite(tempDirectory / "OpenEgg_trace.log");
    if (trace) {
        fprintf(trace, "[TRACE] main() entry point reached\n");
        fflush(trace);
    }

    // GE3_HEADLESS=1: the frontend window is created hidden, so
    // nothing appears on the host desktop and nothing competes for the
    // host's real input focus. Deterministic timing + no frame limiter are
    // what a scripted/CI run almost always wants, so they're implied here
    // unless explicitly set already.
    if (std::getenv("GE3_HEADLESS")) {
        SetEnvironmentIfUnset("GE3_FIXED_TIMESTEP_MS", "17");
        SetEnvironmentIfUnset("GE3_NO_FPS_LIMIT", "1");
    }

    // Scripted input + screenshot capture (GE3_HARNESS=<script>), see
    // headless_harness.h. Disabled and zero-cost when the env var is unset.
    HeadlessHarness harness;

    if (trace) fprintf(trace, "[TRACE] parsing args\n"), fflush(trace);
    const std::filesystem::path executableDirectory =
        std::filesystem::u8path(SaveData::ExecutableDirectory());
    const std::filesystem::path systemConfigPath = executableDirectory / "OpenEgg.ini";
    if (SystemConfig::Get().LoadFromIni(systemConfigPath.u8string())) {
        std::fprintf(stderr, "system config: loaded '%s' (os_id=%s, language_code=%d, retail_build=%s)\n",
                     systemConfigPath.u8string().c_str(), SystemConfig::Get().OsIdName(),
                     SystemConfig::Get().languageCode,
                     SystemConfig::Get().isRetailBuild ? "true" : "false");
    } else {
        std::fprintf(stderr, "system config: '%s' not found; using retail defaults (os_id=%s, language_code=%d)\n",
                     systemConfigPath.u8string().c_str(), SystemConfig::Get().OsIdName(),
                     SystemConfig::Get().languageCode);
    }
    const std::string defaultDataRoot =
        (executableDirectory / "game_data").lexically_normal().u8string();
    const std::string archiveRoot = executableDirectory.u8string();
    std::filesystem::path requestedDataRoot = argc > 1
        ? std::filesystem::u8path(argv[1]) : std::filesystem::u8path(defaultDataRoot);
    if (requestedDataRoot.is_relative()) requestedDataRoot = executableDirectory / requestedDataRoot;
    std::string dataRoot = requestedDataRoot.lexically_normal().u8string();
    std::string entryScript = (argc > 2) ? argv[2] : "Main.gm";
    std::string processName = (argc > 3) ? argv[3] : "Main";

    if (trace) fprintf(trace, "[TRACE] creating Renderer object\n"), fflush(trace);
    std::unique_ptr<PlatformFrontend> hostFrontend = CreatePlatformFrontend();
    if (!hostFrontend || !hostFrontend->Create(800, 600, "OpenEgg",
                                                std::getenv("GE3_HEADLESS") != nullptr)) {
        return 1;
    }
    if (!std::getenv("GE3_HEADLESS")) {
        StrictCalls::SetHaltDialogHandler(&ShowStrictCallDialog, hostFrontend.get());
    }

    GmVm vm;
    vm.Files().AddRoot(dataRoot, archiveRoot);
    if (!vm.Files().Resolve(entryScript)) {
        const char* message =
            "No runnable game files were found beside the executable. Place "
            "game_data.psarc there, optionally with game_data/ and any "
            "patch_data001.psarc through patch_data004.psarc files.";
        std::fprintf(stderr, "main: %s\n", message);
        if (trace) {
            std::fprintf(trace, "[TRACE] no runnable game files beside executable\n");
            std::fclose(trace);
            trace = nullptr;
        }
        hostFrontend->ShowErrorMessage("Game files not found", message);
        hostFrontend->Destroy();
        return 1;
    }

    harness.Init(*hostFrontend);
    Renderer renderer;
    if (trace) fprintf(trace, "[TRACE] calling renderer.Init()...\n"), fflush(trace);
    if (!renderer.Init(*hostFrontend, 800, 600)) {
        if (trace) fprintf(trace, "[TRACE] renderer.Init() failed\n"), fflush(trace);
        return 1;
    }
    if (trace) fprintf(trace, "[TRACE] renderer.Init() succeeded\n"), fflush(trace);

    GetEngineContext().video.textureResizeOverrideEnabled = true;

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
        return font.Load(*jsonPath, std::filesystem::u8path(*jsonPath).parent_path().u8string(),
                         renderer, &vm.Files());
    };
    bool haveNormal = loadBitmapFont(normalFont, "HelveticaNeueLTCom-Lt.json");
    bool haveHeavy = loadBitmapFont(heavyFont, "HelveticaNeueLTCom-Hv.json");
    bool haveCourier = loadBitmapFont(courierFont, "courierFont.json");
    // The Video font-resource constructor has an exact Russian font-name
    // case, and game_data ships the corresponding bitmap atlas. Keep it on
    // the same metrics/atlas path as retail instead of falling back to TTF.
    bool haveRussian = loadBitmapFont(russianFont, "Helvetica_NeueLTCyrillic55_Russian.json");
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
    RegisterScriptCompatBindings(vm.Machine());

    for (const char* lib : {"GameMonkeyScript", "GameSystem", "Video", "Physics",
                             "ColourManager", "EntityManager", "Input", "Audio"}) {
        gmVariable v = vm.Machine().GetGlobals()->Get(&vm.Machine(), lib);
        StrictCalls::MarkNative(vm.Machine(), v);
    }

    {
        gmVariable mainSequencer = CreateSequencerInstance(vm.Machine(), 98);
        vm.Machine().GetGlobals()->Set(&vm.Machine(), "MainSequencer", mainSequencer);
        StrictCalls::MarkNative(vm.Machine(), mainSequencer);
    }

    if (trace) fprintf(trace, "[TRACE] creating ProcessManager\n"), fflush(trace);
    ProcessManager processManager(vm.Machine());
    ctx.processManager = &processManager;
    if (trace) fprintf(trace, "[TRACE] ProcessManager created\n"), fflush(trace);

    if (trace) fprintf(trace, "[TRACE] loading FontHelper.gm\n"), fflush(trace);
    if (!vm.LoadScript("FontHelper.gm")) {
        std::fprintf(stderr, "main: FontHelper.gm failed to load, using native stand-in\n");
    }
    if (trace) fprintf(trace, "[TRACE] FontHelper.gm loaded\n"), fflush(trace);

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
    constexpr float kRetailSimulationStepMs = 16.666666f;
    constexpr float kRetailSimulationDeltaCapMs = 50.0000038f;
    // Temporary slow-motion diagnostic: keep the effective simulation speed
    // at 1.0 until the recurring script-selected 0.1 value is understood.
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
                    if (HandleImeKeyDown(event.code)) break;
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
                case PlatformEventType::TextInput:
                    HandleImeText(event.text);
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

        const float speed = 1.0f;
        const float accumulatedElapsedMs = std::fma(
            elapsedMs, speed, simulationAccumulatorMs);
        const float timeUntilCapMs = kRetailSimulationDeltaCapMs - accumulatedElapsedMs;
        simulationAccumulatorMs = timeUntilCapMs >= 0.0f
            ? accumulatedElapsedMs : kRetailSimulationDeltaCapMs;
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
            // Retail Video's selection render-state callback advances its
            // ping-pong scalar with the current fixed simulation delta.
            GetEngineContext().video.AdvanceSelectionPulse(
                static_cast<double>(simDtMs));

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
            // Retail calls AudioManager's fixed-step update after its process
            // manager update. Voices created or resumed by a process therefore
            // receive this tick before the next process sample.
            AdvanceAudioVoices(simDtMs);
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
        std::vector<int> nativeDrawnUids;
        if (GetEngineContext().video.shouldDrawEntities) {
            // Retail entities have an independent preDraw callback. Player's
            // native draw caches this method for auxiliary geometry.
            PreDrawEntitiesWithNativeHook();
            // Native/script entity draw lists, including the player list at
            // retail key 0x200000ff, are sorted and replayed in this pass.
            // Script-authored drawAlpha is a later, separate retail pass.
            nativeDrawnUids = DrawEntitiesWithNativeDrawHook();
            DrawEntitiesWithNativeDrawAlphaHook();
        }
        recordPhase(6); // entity and player rendering
        processManager.PostDrawAll();
        CallGlobalHookIfPresent(vm.Machine(), "GAME_PostDraw", 0, false);
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

        static int s_frameIndex = 0;
        static const char* s_screenshotFrameEnv = std::getenv("GE3_SCREENSHOT_FRAME");
        if (s_screenshotFrameEnv && s_frameIndex == std::atoi(s_screenshotFrameEnv)) {
            const std::filesystem::path path = tempDirectory /
                ("OpenEgg_frame" + std::to_string(s_frameIndex) + ".ppm");
            renderer.SaveScreenshot(path);
            std::fprintf(stderr, "main: saved screenshot to %s\n", path.string().c_str());
        }
        // Harness `shot <path>` -- same point in the frame as the
        // GE3_SCREENSHOT_FRAME capture above (after letterbox bars, before
        // the swap), so both produce identical output for the same frame.
        if (!harness.PendingScreenshot().empty()) {
            renderer.SaveScreenshot(std::filesystem::u8path(harness.PendingScreenshot()));
            std::fprintf(stderr, "[harness] frame %d -> %s\n", s_frameIndex,
                         harness.PendingScreenshot().c_str());
            if (std::getenv("GE3_TRACE_ENTITY_CULL")) {
                Process* entityProcess = processManager.GetEntityUpdateProcess();
                int processX = -999, processY = -999;
                if (entityProcess && entityProcess->table.m_type == GM_TABLE) {
                    gmTableObject* table = (gmTableObject*)vm.Machine().GetObject(
                        entityProcess->table.m_value.m_ref);
                    if (table) {
                        const gmVariable x = table->Get(&vm.Machine(), "currentScreenX");
                        const gmVariable y = table->Get(&vm.Machine(), "currentScreenY");
                        if (x.m_type == GM_INT) processX = x.m_value.m_int;
                        if (y.m_type == GM_INT) processY = y.m_value.m_int;
                    }
                }
                float playerX = 0.0f, playerY = 0.0f;
                bool havePlayerPos = false;
                const gmVariable playerVar = FindPlayerEntity(vm.Machine());
                if (playerVar.m_type == GM_TABLE) {
                    gmTableObject* player = (gmTableObject*)vm.Machine().GetObject(
                        playerVar.m_value.m_ref);
                    if (player) {
                        const gmVariable physicsData = player->Get(&vm.Machine(), "PhysicsData");
                        havePlayerPos = GetPhysicsDataPos(vm.Machine(), physicsData,
                                                          playerX, playerY);
                    }
                }
                std::fprintf(stderr,
                    "[entity-cull-frame] frame=%d process=%s cell=(%d,%d) "
                    "videoCell=(%d,%d) camera=(%.3f,%.3f)\n",
                    s_frameIndex, entityProcess ? entityProcess->name.c_str() : "none",
                    processX, processY, GetEngineContext().video.currentScreenX,
                    GetEngineContext().video.currentScreenY,
                    renderer.GetCameraX(), renderer.GetCameraY());
                if (havePlayerPos)
                    std::fprintf(stderr, "[entity-cull-frame] player-position=(%.3f,%.3f)\n",
                                 playerX, playerY);
            }
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

#if defined(_WIN32)
// The distributable Windows build uses the GUI subsystem so Explorer does not
// flash a command window. Keep the existing main() and its argument handling;
// MinGW exposes the CRT parsed command line through __argc/__argv.
extern "C" int __argc;
extern "C" char** __argv;

static LONG WINAPI ReportUnhandledWindowsException(EXCEPTION_POINTERS* exceptionInfo) {
    const DWORD code = exceptionInfo && exceptionInfo->ExceptionRecord
        ? exceptionInfo->ExceptionRecord->ExceptionCode : 0;
    const void* address = exceptionInfo && exceptionInfo->ExceptionRecord
        ? exceptionInfo->ExceptionRecord->ExceptionAddress : nullptr;
    void* frames[32] = {};
    const USHORT frameCount = CaptureStackBackTrace(
        0, static_cast<DWORD>(std::size(frames)), frames, nullptr);
    const void* instructionPointer = nullptr;
    if (exceptionInfo && exceptionInfo->ContextRecord) {
#if defined(_M_X64) || defined(__x86_64__)
        instructionPointer = reinterpret_cast<const void*>(
            exceptionInfo->ContextRecord->Rip);
#elif defined(_M_IX86) || defined(__i386__)
        instructionPointer = reinterpret_cast<const void*>(
            exceptionInfo->ContextRecord->Eip);
#elif defined(_M_ARM64) || defined(__aarch64__)
        instructionPointer = reinterpret_cast<const void*>(
            exceptionInfo->ContextRecord->Pc);
#endif
    }
    char message[256];
    std::snprintf(message, sizeof(message),
        "GameEngine3 stopped after an unhandled Windows exception.\n\n"
        "Exception code: 0x%08lx\nFault address: %p\nInstruction pointer: %p\n\n"
        "More detail was written to OpenEgg.log.",
        (unsigned long)code, address, instructionPointer);
    std::fprintf(stderr,
        "\n[fatal] unhandled Windows exception code=0x%08lx address=%p ip=%p\n",
        (unsigned long)code, address, instructionPointer);
    for (USHORT i = 0; i < frameCount; ++i)
        std::fprintf(stderr, "[fatal] stack[%u]=%p\n", (unsigned)i, frames[i]);
    std::fflush(stderr);
    MessageBoxA(nullptr, message, "GameEngine3 crashed",
                MB_OK | MB_ICONERROR | MB_TASKMODAL);
    return EXCEPTION_EXECUTE_HANDLER;
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    const char* showConsole = std::getenv("GE3_CONSOLE");
    if (showConsole && showConsole[0] != '\0' && std::strcmp(showConsole, "0") != 0) {
        if (AllocConsole()) {
            std::freopen("CONOUT$", "w", stdout);
            std::freopen("CONOUT$", "w", stderr);
        }
    } else {
        // A GUI process launched from Explorer has no console handles. Keep
        // normal debug prints hidden so they cannot stall the game, but retain
        // errors and strict-call reports beside the executable. Strict mode
        // exits on the first unimplemented engine call, so discarding stderr
        // made that exit indistinguishable from an ordinary crash to testers.
        std::freopen("NUL", "w", stdout);
        wchar_t executablePath[MAX_PATH] = {};
        const DWORD pathLength = GetModuleFileNameW(
            nullptr, executablePath, static_cast<DWORD>(std::size(executablePath)));
        if (pathLength > 0 && pathLength < std::size(executablePath)) {
            std::filesystem::path logPath(executablePath);
            logPath = logPath.parent_path() / L"OpenEgg.log";
            if (_wfreopen(logPath.c_str(), L"w", stderr) == nullptr)
                std::freopen("NUL", "w", stderr);
        } else {
            std::freopen("NUL", "w", stderr);
        }
    }
    SetUnhandledExceptionFilter(&ReportUnhandledWindowsException);
    return main(__argc, __argv);
}
#endif
