#include "process_manager.h"
#include "gmCall.h"
#include "gmTableObject.h"
#include "../bindings/process_base_bindings.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/foe_compat_bindings.h"
#include "../render/draw_list.h"
#include "engine_context.h"
#include <cstdio>

// Real drawList instances now record rather than draw immediately (see
// draw_list.h's RecordingTarget/DrawListData header comment) -- a process's
// own preDrawList/postDrawList (set up via setupPreDrawList/
// setupPostDrawList, process_handle_bindings.cpp) is exactly that kind of
// instance, so whatever its preDraw()/postDraw() hook just recorded needs
// an explicit replay to actually reach the screen. Every real hook already
// calls `this.preDrawList.reset()` as its first statement (confirmed real
// convention, see draw_list.cpp's gmf_reset header comment), which clears
// the list for the NEXT frame's rebuild -- so no clear is needed here,
// only the replay.
static void FlushDrawList(gmMachine& a_machine, const gmVariable& a_listVar) {
    // Every registered user type (VMDRAWLIST, VMSHAPE, ...) gets its own
    // distinct gmType id counting up FROM GM_USER (gmVariable.h: "User
    // types continue from here") -- comparing against the bare GM_USER
    // constant itself matches nothing real, same reasoning as
    // gmThread::ThisUserObject()'s own `>= GM_USER` check.
    if (a_listVar.m_type < GM_USER) return;
    gmUserObject* obj = (gmUserObject*)a_machine.GetObject(a_listVar.m_value.m_ref);
    if (!obj || obj->m_userType != GetDrawListType()) return;
    ReplayDrawList((DrawListData*)obj->m_user);
}

ProcessManager::ProcessManager(gmMachine& a_machine) : m_machine(a_machine) {}

// Convention-derived on-demand script load: confirmed for every real case
// found (Platformer_MainMenu5 -> Platformer/MainMenu5.gm,
// Platformer_ResizeScreenProcess -> Platformer/ResizeScreenProcess.gm,
// Platformer_PopupProcess -> Platformer/PopupProcess.gm) by grepping every
// GameSystem.startNewProcessList/runProcess call site in the corpus
// against the actual .gm filenames that exist -- but the exact mechanism
// retail itself uses to reach these ISN'T confirmed: checked via Ghidra
// string search (EBOOT.elf), and neither "MainMenu5.gm" nor
// "ResizeScreenProcess.gm" appear anywhere in the retail binary as a
// loadScript-style string literal the way every other real loadScript
// argument in the corpus does (they show up as embedded GM source text,
// e.g. Platformer/MainMenuScriptLoader.gm's own loadScript calls are
// directly greppable in the binary's string table; these three names
// are not). Some other mechanism (a directory auto-scan at boot, a
// precompiled bytecode manifest, ...) is more likely real. This
// on-demand convention-based load is a substitute: it gets the script
// loaded before it's needed without hardcoding a native-preload list
// that risks loading things unrelated scenes don't want -- see
// main.cpp's isFullRetailTree guard, added after exactly that kind of
// guess broke game_data_min.
bool ProcessManager::TryAutoLoadScript(const std::string& a_processName) {
    std::string rest = a_processName;
    const std::string prefix = "Platformer_";
    bool hasPrefix = a_processName.compare(0, prefix.size(), prefix) == 0;
    if (hasPrefix) rest = a_processName.substr(prefix.size());

    // Try flat first (e.g. Platformer_ResizeScreenProcess ->
    // Platformer/ResizeScreenProcess.gm -- confirmed real for every single-
    // segment case grepped). Some real process names embed an actual
    // subdirectory level though (Platformer_MainMenu5_PreTutorialScreen ->
    // Platformer/MainMenu5/PreTutorialScreen.gm, confirmed by the file
    // only existing at that nested path) -- if flat doesn't exist, retry
    // with every remaining underscore turned into a path separator.
    std::string source;
    std::string flatPath = hasPrefix ? ("Platformer/" + rest + ".gm") : (a_processName + ".gm");
    std::string path = flatPath;
    if (GetEngineContext().fileHelper->ReadTextFile(path, source)) {
        int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, path.c_str());
        return errors == 0;
    }
    if (hasPrefix) {
        std::string nested = rest;
        for (char& c : nested) if (c == '_') c = '/';
        path = "Platformer/" + nested + ".gm";
        if (path != flatPath && GetEngineContext().fileHelper->ReadTextFile(path, source)) {
            int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, path.c_str());
            return errors == 0;
        }
        return false;
    }

    // Generic form of the same convention, without a fixed "Platformer/"
    // root to key off of -- same on-demand process-factory-file loading,
    // just for process names that don't happen to start with that one
    // prefix (confirmed necessary for "PekkoAndJon_Main" -> PekkoAndJon/
    // Main.gm; the underscore-as-path-separator convention itself isn't
    // Platformer-specific, this engine's auto-load just never generalized
    // past the one prefix it was built against). Try turning each
    // underscore into a slash, one at a time from left to right, e.g. for
    // "Foo_Bar_Baz": "Foo/Bar_Baz.gm", then "Foo/Bar/Baz.gm" -- least- to
    // most-nested, first hit wins. (Flat "a_processName.gm" was already
    // tried above and failed, so this starts one level deeper.)
    std::string candidate = a_processName;
    for (size_t i = 0; i < candidate.size(); ++i) {
        if (candidate[i] != '_') continue;
        candidate[i] = '/';
        std::string tryPath = candidate + ".gm";
        if (GetEngineContext().fileHelper->ReadTextFile(tryPath, source)) {
            int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, tryPath.c_str());
            return errors == 0;
        }
    }
    return false;
}

int ProcessManager::StartNewProcessList(const std::string& a_name) {
    // Confirmed necessary by a real corpus bug that only makes sense if
    // retail's own startNewProcessList behaves this way: game_data/Main.gm's
    // top-level update() has no guard at all around
    // `GameSystem.startNewProcessList("Platformer_MainMenu5")` -- it
    // relies entirely on `this.stateCountDown <= 0.0`, which the code path
    // that calls startNewProcessList never resets (it `return`s
    // immediately after, deliberately skipping the reset code below it to
    // avoid an out-of-bounds `this.states[this.stateIndex]` access) --
    // meaning every frame after the boot sequence finishes, Main.gm calls
    // this again, forever, with no de-dup on the caller's end. Confirmed
    // NOT a problem for legitimately-repeatable calls like
    // `Platformer_PopupProcess` (started many times for distinct popups)
    // since real callers always terminate/dismiss a popup before
    // requesting the next one -- by the time a new one is requested, the
    // old one is no longer live, so this lookup naturally finds nothing
    // and falls through to actually creating one, same as before.
    if (Process* existing = GetByName(a_name)) {
        if (existing->constructed && !existing->wantsQuit) {
            return existing->id;
        }
    }

    gmVariable globalFunc = m_machine.GetGlobals()->Get(&m_machine, a_name.c_str());
    if (globalFunc.m_type != GM_FUNCTION && TryAutoLoadScript(a_name)) {
        globalFunc = m_machine.GetGlobals()->Get(&m_machine, a_name.c_str());
    }
    if (globalFunc.m_type != GM_FUNCTION) {
        std::fprintf(stderr, "ProcessManager::StartNewProcessList: global '%s' is not a function\n", a_name.c_str());
        return -1;
    }

    // Reserve the process slot (and id) before calling the factory, since
    // the factory needs a real process handle bound to that id as its `p`
    // argument -- confirmed real convention: every retail process factory
    // is `function(p) { return { Process=p, ... }; };` and stores it
    // itself (e.g. Platformer/OneTrailPlayMode.gm: `t.Process=p;`).
    int id = m_nextId++;
    Process proc;
    proc.id = id;
    proc.name = a_name;
    m_processes.push_back(proc);

    gmVariable handle = CreateProcessHandle(m_machine, id);

    gmCall call;
    if (!call.BeginGlobalFunction(&m_machine, a_name.c_str())) {
        std::fprintf(stderr, "ProcessManager::StartNewProcessList: failed to begin call to '%s'\n", a_name.c_str());
        m_processes.pop_back();
        return -1;
    }
    call.AddParam(handle);
    call.End();

    if (!call.DidReturnVariable() || call.GetReturnedVariable().m_type != GM_TABLE) {
        std::fprintf(stderr, "ProcessManager::StartNewProcessList: '%s(p)' did not return a table\n", a_name.c_str());
        m_processes.pop_back();
        return -1;
    }

    Process* p = Get(id);
    p->table = call.GetReturnedVariable();
    RootGmVariable(m_machine, p->table); // see process_manager.h -- GC can't see this otherwise
    FinishBringUp(id);
    return id;
}

void ProcessManager::FinishBringUp(int a_id) {
    Process* proc = Get(a_id);
    if (!proc) return; // already gone (shouldn't happen -- just reserved)
    gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc->table.m_value.m_ref);
    gmVariable tableVar = proc->table;

    InjectProcessBaseMethods(m_machine, table, a_id);

    // constructor() only -- init() is deferred, see Process::needsInit.
    gmCall ctorCall;
    if (ctorCall.BeginTableFunction(&m_machine, "constructor", table, tableVar)) {
        ctorCall.End();
    }

    proc = Get(a_id); // constructor() may have reallocated m_processes
    if (proc) proc->constructed = true;
}

void ProcessManager::RunInitPass() {
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (!proc.constructed || !proc.needsInit || proc.wantsQuit) continue;
        int id = proc.id;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;

        // "expecting init() to return a boolean. Assuming true."
        // Confirmed real async multi-step init convention (Platformer/Main.gm's
        // own init(): `if (!?this.groundProcess) { this.groundProcess=this.runProc(...);
        // return false; }` -- deliberately incomplete on this call, expecting
        // to be called AGAIN next frame once the dependency it just started
        // is ready) -- init() returning false must NOT be treated as done.
        // Previously marked needsInit=false unconditionally after a single
        // call, so this early-return half of Main.gm's init() ran exactly
        // once and the rest of its setup (camera position, deleteAllEntities,
        // etc, gated behind the SAME `if (!?this.groundProcess)` on the next
        // call) never executed -- update()/preDraw() still ran anyway since
        // `initialized` was set true regardless, leaving the process in a
        // half-initialized state for the rest of its life.
        gmCall initCall;
        gmint result = 1;
        if (initCall.BeginTableFunction(&m_machine, "init", table, tableVar)) {
            initCall.End();
            if (initCall.DidReturnVariable() && initCall.GetReturnedVariable().m_type == GM_INT) {
                initCall.GetReturnedInt(result);
            }
        }

        Process* p = Get(id); // init() may have reallocated m_processes
        if (p) {
            if (result) {
                p->needsInit = false;
                p->initialized = true;
            }
            // else: leave needsInit true so RunInitPass calls init() again
            // next frame; initialized stays false so update()/preDraw() don't
            // run on a still-initializing process (matches every other real
            // caller's `initialized` gate in this file).
        }
    }
}

bool ProcessManager::IsRunning(int a_id) const {
    for (const auto& proc : m_processes) {
        if (proc.id == a_id) return proc.initialized && !proc.wantsQuit;
    }
    return false;
}

Process* ProcessManager::GetByIndex(int a_index) {
    int n = (int)m_processes.size();
    if (a_index < 0) a_index += n; // Python-style: -1 = last, see process_manager.h
    if (a_index < 0 || a_index >= n) return nullptr;
    return &m_processes[a_index];
}

Process* ProcessManager::GetByName(const std::string& a_name) {
    for (auto& proc : m_processes) {
        if (proc.name == a_name) return &proc;
    }
    return nullptr;
}

void ProcessManager::UpdateAll(float a_deltaMs) {
    RunInitPass();
    TickAlarms(m_machine, a_deltaMs);

    // Index-based with a re-read size -- same reentrancy hazard as
    // FinishBringUp/PreDrawAll: update() can start further processes and
    // reallocate m_processes mid-iteration.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (!proc.initialized || proc.wantsQuit) continue;
        if (m_paused && proc.id != m_pauseExemptProcessId) continue; // see SetPauseState
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "update", table, tableVar)) {
            call.AddParamFloat(a_deltaMs);
            call.End();
        }
    }

    // postUpdate() -- confirmed real, reserved lifecycle key (see this
    // function's own header comment), but never actually called anywhere in
    // this engine until now. Platformer/Main.gm's own postUpdate() is where
    // `this.camera.update(msElapsed)` lives -- the per-frame camera-follow/
    // pan logic that reads the player's current screen and interpolates
    // Video.setCameraPos toward it. Without this call, the camera is
    // positioned exactly once at level load (via finalizeLoad's own direct
    // setCameraPos call) and then never moves again for the rest of the
    // level, no matter how far the player actually walks -- confirmed via a
    // live trace: the player's tracked body position moved ~19 world units
    // over several seconds of real movement while Video.getCameraPosX/Y()
    // stayed byte-identical the entire time. Run as its own pass, after
    // every process's update() this frame, matching postDraw's relationship
    // to preDraw (see ProcessManager::PostDrawAll's header comment).
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (!proc.initialized || proc.wantsQuit) continue;
        if (m_paused && proc.id != m_pauseExemptProcessId) continue; // see SetPauseState
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "postUpdate", table, tableVar)) {
            call.AddParamFloat(a_deltaMs);
            call.End();
        }
    }

    // Reap anything that called Process.terminate() this frame (or earlier),
    // then erase. Collect ids first: erasing from m_processes while a range over
    // it is still conceptually "in progress" (destructor() calls can also
    // start/terminate further processes) is safer done as a separate pass.
    std::vector<int> toRemove;
    for (const auto& proc : m_processes) {
        if (proc.wantsQuit) toRemove.push_back(proc.id);
    }
    for (int id : toRemove) {
        Process* proc = Get(id);
        if (!proc) continue; // already gone, e.g. a nested terminate()
        // The pause-exempt process going away without ever calling
        // setAsPauseProcess(false, ...) first is real, confirmed corpus
        // behavior -- ResizeScreenProcess.gm's onKeyPress just calls
        // this._process.terminate() directly on accept, with no matching
        // unpause anywhere in the file. Left unhandled, every other
        // process's update()/postUpdate() would stay frozen forever once
        // this one is reaped (see ProcessManager::SetPauseState). Clearing
        // pause here is the same safety net a real engine needs regardless
        // of what retail's own native process teardown does internally.
        if (id == m_pauseExemptProcessId) SetPauseState(false, -1);
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc->table.m_value.m_ref);
        gmVariable tableVar = proc->table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "destructor", table, tableVar)) {
            call.End();
        }
        for (auto it = m_processes.begin(); it != m_processes.end(); ++it) {
            if (it->id == id) {
                UnrootGmVariable(m_machine, it->table);
                UnrootGmVariable(m_machine, it->preDrawList);
                UnrootGmVariable(m_machine, it->postDrawList);
                m_processes.erase(it);
                break;
            }
        }
    }
}

void ProcessManager::DispatchKeyEvent(int a_keycode, bool a_isPress) {
    DispatchKeyEventNamed(a_keycode, a_isPress ? "onKeyPress" : "onKeyType");
}

void ProcessManager::DispatchKeyReleaseEvent(int a_keycode) {
    DispatchKeyEventNamed(a_keycode, "onKeyRelease");
}

void ProcessManager::DispatchKeyEventNamed(int a_keycode, const char* methodName) {
    // GameSystem.setIsInputEnabled(false) -- real blocking-OS-dialog input
    // gate (see game_system_state.h). While disabled, no key event reaches
    // any process/entity handler at all.
    if (!GetEngineContext().gameSystem.isInputEnabled) return;

    // Topmost (most-recently-created) process currently stealing focus, if
    // any -- see header comment.
    int focusId = -1;
    for (const auto& proc : m_processes) {
        if (proc.initialized && !proc.wantsQuit && proc.stealsInputFocus) focusId = proc.id;
    }

    if (focusId >= 0) {
        Process* proc = Get(focusId);
        if (!proc) return;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc->table.m_value.m_ref);
        gmVariable tableVar = proc->table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, methodName, table, tableVar)) {
            call.AddParamInt(a_keycode);
            call.End();
        }
        return;
    }

    // Nobody's stealing focus -- broadcast to every live process. Index-
    // based with a re-read size: a handler could start/stop further
    // processes mid-iteration (same reentrancy hazard as UpdateAll).
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (!proc.initialized || proc.wantsQuit) continue;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, methodName, table, tableVar)) {
            call.AddParamInt(a_keycode);
            call.End();
        }
    }
}

void ProcessManager::PreDrawAll() {
    // Index-based with a re-read size, not a range-based for over
    // m_processes: preDraw() can recursively start further processes
    // (same reentrancy hazard as FinishBringUp) and reallocate the vector
    // mid-iteration.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (!proc.initialized) continue;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        int id = proc.id;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "preDraw", table, tableVar)) {
            call.End();
        }
        // Re-fetch by id -- preDraw() may have recursively started/reaped
        // processes and reallocated m_processes (same hazard as
        // FinishBringUp), so `proc` itself can no longer be trusted here.
        if (Process* p = Get(id)) FlushDrawList(m_machine, p->preDrawList);
    }
}

void ProcessManager::PostDrawAll() {
    // Same reentrancy/iteration reasoning as PreDrawAll.
    for (size_t i = 0; i < m_processes.size(); ++i) {
        Process& proc = m_processes[i];
        if (!proc.initialized) continue;
        gmTableObject* table = (gmTableObject*)m_machine.GetObject(proc.table.m_value.m_ref);
        gmVariable tableVar = proc.table;
        int id = proc.id;
        gmCall call;
        if (call.BeginTableFunction(&m_machine, "postDraw", table, tableVar)) {
            call.End();
        }
        // See PreDrawAll's identical re-fetch -- same reentrancy hazard.
        if (Process* p = Get(id)) FlushDrawList(m_machine, p->postDrawList);
    }
}

void ProcessManager::SetPauseState(bool a_active, int a_exemptProcessId) {
    m_paused = a_active;
    m_pauseExemptProcessId = a_active ? a_exemptProcessId : -1;
}

bool ProcessManager::AnyWantQuit() const {
    for (const auto& proc : m_processes) {
        if (proc.wantsQuit) return true;
    }
    return false;
}

Process* ProcessManager::Get(int a_id) {
    for (auto& proc : m_processes) {
        if (proc.id == a_id) return &proc;
    }
    return nullptr;
}
