#include "gmThread.h"
#include "gamesystem_bindings.h"
#include "../core/engine_context.h"
#include "../core/process_manager.h"
#include "gmTableObject.h"
#include "gmCall.h"
#include "vector2d_bindings.h"
#include "../core/strict_calls.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <cmath>
#include <vector>

namespace {

float Random01() {
    return GetEngineContext().gameSystem.NextRandom01();
}

bool IsUnavailableCommunityProcess(const std::string& a_name) {
    // The PS3 community screens depend on COMMONCOMMUNITYSTUFF_* providers
    // supplied by the online service layer. That provider is absent from the
    // mounted retail archives, and this desktop build intentionally keeps
    // Network/Watermelon offline. Letting the process constructor run reaches
    // missing globals during construction and leaves a half-created process.
    // The offline compatibility path below supplies a ready no-op handle so
    // stock menu code can keep its process list complete without changing any
    // retail GameMonkey source.
    return a_name == "Platformer_MainMenu5PS3_CommunityScreen" ||
           a_name == "Platformer_MainMenu5_CommunityScreen";
}

int GM_CDECL OfflineCommunityProcessIsInited(gmThread* a_thread) {
    a_thread->PushInt(1);
    return GM_OK;
}

int GM_CDECL OfflineCommunityProcessNoOp(gmThread*) {
    return GM_OK;
}

int GM_CDECL OfflineCommunityProcessZero(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL OfflineCommunityProcessMinusOne(gmThread* a_thread) {
    a_thread->PushInt(-1);
    return GM_OK;
}

gmVariable MakeOfflineCommunityProcess(gmMachine& a_machine) {
    // Retail menu scripts retain the returned process in their process list
    // and wait for every entry's isInited() before completing the menu fade.
    // A null return leaves a permanent hole; launchCommunity() can also
    // replace its launcher with the result and terminate the launcher. Keep
    // the online feature unavailable while giving those retail call paths an
    // inert, already-ready process handle.
    gmTableObject* stub = a_machine.AllocTableObject();
    if (!stub) return gmVariable::s_null;
    stub->Set(&a_machine, "isInited",
              gmVariable(a_machine.AllocFunctionObject(&OfflineCommunityProcessIsInited)));
    static const char* kNoOpMethods[] = {
        "setOrigin", "refreshFunctionCache", "terminate", "update",
        "preDraw", "postDraw", "onScroll", "onKeyPress", "onKeyRelease",
        "onTouchDown", "onTouchUp",
    };
    for (const char* method : kNoOpMethods) {
        stub->Set(&a_machine, method,
                  gmVariable(a_machine.AllocFunctionObject(&OfflineCommunityProcessNoOp)));
    }

    // MainMenu5 calls PERSISTENTDATAHELPER_SaveMainMenuState with this
    // process handle even while all online services are disabled. That helper
    // reads the profile and online-album state tables before checking the
    // online flag, so an empty process table raises a null-index exception on
    // every menu update. Supply the inert shape the serializer expects; no
    // network request or community UI is enabled by these placeholders.
    gmTableObject* sidePanel = a_machine.AllocTableObject();
    gmTableObject* state = a_machine.AllocTableObject();
    gmTableObject* stateData = a_machine.AllocTableObject();
    gmTableObject* profileState = a_machine.AllocTableObject();
    gmTableObject* albumsState = a_machine.AllocTableObject();
    gmTableObject* internalStates = a_machine.AllocTableObject();
    gmTableObject* albumListState = a_machine.AllocTableObject();
    gmTableObject* selectedButton = a_machine.AllocTableObject();
    if (!sidePanel || !state || !stateData || !profileState || !albumsState ||
        !internalStates || !albumListState || !selectedButton)
        return gmVariable::s_null;

    const gmVariable zeroFunction(
        a_machine.AllocFunctionObject(&OfflineCommunityProcessZero));
    const gmVariable noOpFunction(
        a_machine.AllocFunctionObject(&OfflineCommunityProcessNoOp));

    profileState->Set(&a_machine, "getCurrentUserID", zeroFunction);
    profileState->Set(&a_machine, "getCurrentButton", zeroFunction);
    stateData->Set(&a_machine, 1, gmVariable(profileState));
    stateData->Set(&a_machine, 6, gmVariable(albumsState));
    state->Set(&a_machine, "stateData", gmVariable(stateData));
    state->Set(&a_machine, "getState",
               gmVariable(a_machine.AllocFunctionObject(&OfflineCommunityProcessMinusOne)));
    state->Set(&a_machine, "changeState", noOpFunction);

    albumListState->Set(&a_machine, "persistentData", gmVariable::s_null);
    albumListState->Set(&a_machine, "scrollOffset", gmVariable((gmint)0));
    albumListState->Set(&a_machine, "currentSelection", gmVariable((gmint)0));
    internalStates->Set(&a_machine, 0, gmVariable(albumListState));
    albumsState->Set(&a_machine, "currentState", gmVariable((gmint)0));
    albumsState->Set(&a_machine, "albumID", gmVariable::s_null);
    albumsState->Set(&a_machine, "titleText", gmVariable::s_null);
    albumsState->Set(&a_machine, "sidePanelURL", gmVariable::s_null);
    albumsState->Set(&a_machine, "description", gmVariable::s_null);
    albumsState->Set(&a_machine, "linerNotes", gmVariable::s_null);
    albumsState->Set(&a_machine, "ALBUMLIST_INTERNALSTATEID", gmVariable((gmint)0));
    albumsState->Set(&a_machine, "LEVELLIST_INTERNALSTATEID", gmVariable((gmint)-1));
    albumsState->Set(&a_machine, "internalStates", gmVariable(internalStates));

    selectedButton->Set(&a_machine, "isPanel", zeroFunction);
    selectedButton->Set(&a_machine, "select", noOpFunction);
    selectedButton->Set(&a_machine, "deselect", noOpFunction);
    stub->Set(&a_machine, "sidePanel", gmVariable(sidePanel));
    stub->Set(&a_machine, "persistentData_popupArguments", gmVariable::s_null);
    stub->Set(&a_machine, "tabs", gmVariable::s_null);
    stub->Set(&a_machine, "currentTab", gmVariable((gmint)0));
    stub->Set(&a_machine, "selectedButton", gmVariable(selectedButton));
    stub->Set(&a_machine, "moreButton", gmVariable(selectedButton));
    stub->Set(&a_machine, "DAILY_BUTTON_INDEX", gmVariable((gmint)100));
    stub->Set(&a_machine, "ONLINEALBUMS_BUTTON_INDEX", gmVariable((gmint)101));
    stub->Set(&a_machine, "HITS_BUTTON_INDEX", gmVariable((gmint)102));
    stub->Set(&a_machine, "DOWNLOADS_BUTTON_INDEX", gmVariable((gmint)103));
    sidePanel->Set(&a_machine, "FRIENDS_STATE_NEW", gmVariable((gmint)1));
    sidePanel->Set(&a_machine, "ONLINEALBUMS_STATE", gmVariable((gmint)6));
    sidePanel->Set(&a_machine, "state", gmVariable(state));
    return gmVariable(stub);
}


int GM_CDECL gmf_getCurrentTick(gmThread* a_thread) {
    a_thread->PushInt((gmint)GetEngineContext().gameSystem.currentTick);
    return GM_OK;
}
int GM_CDECL gmf_isPaused(gmThread* a_thread) {
    ProcessManager* pm = GetEngineContext().processManager;
    a_thread->PushInt(pm && pm->IsPaused() ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_isBeingNice(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isBeingNice ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsBeingNice(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isBeingNice = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_isShouldUpdateEntities(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.shouldUpdateEntities ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsShouldUpdateEntities(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.shouldUpdateEntities = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_setALevelLoaded(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isALevelLoaded = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsLoading(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isLoadingFlag = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_isLoading(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isLoadingFlag ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_setIsInputEnabled(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.isInputEnabled = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_debug_setStop(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    GetEngineContext().gameSystem.debugStopped = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_debug_isStopped(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.debugStopped ? 1 : 0);
    return GM_OK;
}


gmType g_processListRefType = -1;

struct ProcessListRefData {
    std::vector<gmVariable> processes;
    size_t index = 0; // size() is the linked-list sentinel/end position
};

int GM_CDECL gmf_processListRef_isValid(gmThread* a_thread) {
    auto* it = (ProcessListRefData*)a_thread->ThisUserCheckType(g_processListRefType);
    a_thread->PushInt(it && it->index < it->processes.size() ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_processListRef_goToNext(gmThread* a_thread) {
    auto* it = (ProcessListRefData*)a_thread->ThisUserCheckType(g_processListRefType);
    if (it) it->index = (it->index + 1) % (it->processes.size() + 1);
    return GM_OK;
}
int GM_CDECL gmf_processListRef_get(gmThread* a_thread) {
    auto* it = (ProcessListRefData*)a_thread->ThisUserCheckType(g_processListRefType);
    if (it && it->index < it->processes.size()) a_thread->Push(it->processes[it->index]);
    else a_thread->PushNull();
    return GM_OK;
}
int GM_CDECL gmf_processListRef_reset(gmThread* a_thread) {
    auto* it = (ProcessListRefData*)a_thread->ThisUserCheckType(g_processListRefType);
    if (it) it->index = 0;
    return GM_OK;
}
int GM_CDECL gmf_processListRef_size(gmThread* a_thread) {
    auto* it = (ProcessListRefData*)a_thread->ThisUserCheckType(g_processListRefType);
    a_thread->PushInt(it ? (gmint)it->processes.size() : 0);
    return GM_OK;
}
void GM_CDECL ProcessListRefGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (ProcessListRefData*)a_object->m_user;
}
gmFunctionEntry s_processListRefLib[] = {
    {"get", gmf_processListRef_get, nullptr},
    {"isValid", gmf_processListRef_isValid, nullptr},
    {"goToNext", gmf_processListRef_goToNext, nullptr},
    {"reset", gmf_processListRef_reset, nullptr},
    {"size", gmf_processListRef_size, nullptr},
};

int GM_CDECL gmf_flushProcesses(gmThread* a_thread) {
    return GM_OK;
}

int GM_CDECL gmf_startNewProcessList(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    if (a_thread->GetNumParams() > 2) { GM_EXCEPTION_MSG("expecting 1 or 2 param(s)"); return GM_EXCEPTION; }
    GM_STRING_PARAM(name, 0, "");
    bool cleanupPrevious = true;
    if (a_thread->GetNumParams() == 2) cleanupPrevious = a_thread->Param(1).m_value.m_int != 0;
    ProcessManager* pm = GetEngineContext().processManager;
    int id = pm ? pm->StartNewProcessList(name, cleanupPrevious) : -1;
    Process* proc = (id >= 0 && pm) ? pm->Get(id) : nullptr;

    if (proc) a_thread->Push(proc->table); else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL OfflineLoginErrorNoOp(gmThread*) { return GM_OK; }

// RunLoginPopup passes its LoginPopup table through PopupProcess.configure.
// Some stock callers pass `{}` as the error callback, but LoginPopup and the
// PS3 popup button both invoke `.execute()` unconditionally. Give only that
// recognized callback table a no-op execute method; this preserves offline
// behavior without changing GameMonkey sources or making other missing
// methods silently succeed.
void EnsureLoginPopupErrorCallback(gmMachine& a_machine,
                                   const gmVariable& a_configureArgument) {
    if (a_configureArgument.m_type != GM_TABLE) return;
    auto* popupList = static_cast<gmTableObject*>(
        a_machine.GetObject(a_configureArgument.m_value.m_ref));
    if (!popupList) return;
    gmVariable popupVar = popupList->Get((gmint)0);
    if (popupVar.m_type != GM_TABLE) return;
    auto* popup = static_cast<gmTableObject*>(
        a_machine.GetObject(popupVar.m_value.m_ref));
    if (!popup) return;
    gmVariable bodyVar = popup->Get(&a_machine, "bodyTable");
    if (bodyVar.m_type != GM_TABLE) return;
    auto* body = static_cast<gmTableObject*>(
        a_machine.GetObject(bodyVar.m_value.m_ref));
    if (!body) return;
    gmVariable stateRowVar = body->Get((gmint)2);
    if (stateRowVar.m_type != GM_TABLE) return;
    auto* stateRow = static_cast<gmTableObject*>(
        a_machine.GetObject(stateRowVar.m_value.m_ref));
    if (!stateRow || stateRow->Get(&a_machine, "createStates").m_type != GM_FUNCTION)
        return;
    gmVariable errorVar = stateRow->Get(&a_machine, "errorTable");
    if (errorVar.m_type != GM_TABLE) return;
    auto* errorTable = static_cast<gmTableObject*>(
        a_machine.GetObject(errorVar.m_value.m_ref));
    if (!errorTable || errorTable->Get(&a_machine, "execute").m_type != GM_NULL)
        return;
    errorTable->Set(&a_machine, "execute",
        gmVariable(a_machine.AllocFunctionObject(OfflineLoginErrorNoOp)));
}

int GM_CDECL gmf_popupProcessConfigureOfflineCompat(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    if (a_thread->GetNumParams() > 0)
        EnsureLoginPopupErrorCallback(machine, a_thread->Param(0));

    gmCall call;
    gmVariable result = gmVariable::s_null;
    bool didReturn = false;
    if (call.BeginTableFunction(&machine, "__ge3_popupConfigureOriginal",
                                self, gmVariable(self))) {
        for (int i = 0; i < a_thread->GetNumParams(); ++i)
            call.AddParam(a_thread->Param(i));
        call.End();
        didReturn = call.DidReturnVariable();
        if (didReturn) result = call.GetReturnedVariable();
    }
    if (didReturn && result.m_type != GM_NULL) a_thread->Push(result);
    return GM_OK;
}

int GM_CDECL gmf_runProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    if (a_thread->GetNumParams() > 2) { GM_EXCEPTION_MSG("expecting 1 or 2 param(s)"); return GM_EXCEPTION; }
    GM_STRING_PARAM(name, 0, "");
    bool replaceExisting = false;
    if (a_thread->GetNumParams() == 2) replaceExisting = a_thread->Param(1).m_value.m_int != 0;
    if (IsUnavailableCommunityProcess(name)) {
        static bool reportedUnavailableCommunity = false;
        if (!reportedUnavailableCommunity) {
            std::fprintf(stderr,
                "[offline] skipped community screen process '%s'; online provider is unavailable\n",
                name);
            reportedUnavailableCommunity = true;
        }
        a_thread->Push(MakeOfflineCommunityProcess(*a_thread->GetMachine()));
        return GM_OK;
    }
    ProcessManager* pm = GetEngineContext().processManager;
    int id = pm ? pm->RunProcess(name, replaceExisting) : -1;
    Process* proc = (id >= 0 && pm) ? pm->Get(id) : nullptr;
    if (proc && name == "Platformer_PopupProcess") {
        gmVariable processTable = proc->table;
        if (processTable.m_type == GM_TABLE) {
            gmMachine& machine = *a_thread->GetMachine();
            gmTableObject* table = static_cast<gmTableObject*>(
                machine.GetObject(processTable.m_value.m_ref));
            if (table && table->Get(&machine, "__ge3_popupConfigureWrapped").m_type != GM_INT) {
                gmVariable configure = table->Get(&machine, "configure");
                if (configure.m_type == GM_FUNCTION) {
                    table->Set(&machine, "__ge3_popupConfigureOriginal", configure);
                    table->Set(&machine, "configure",
                        gmVariable(machine.AllocFunctionObject(
                            gmf_popupProcessConfigureOfflineCompat)));
                    table->Set(&machine, "__ge3_popupConfigureWrapped",
                               gmVariable((gmint)1));
                }
            }
        }
    }
    if (proc) a_thread->Push(proc->table); else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_isProcessRunning(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    ProcessManager* pm = GetEngineContext().processManager;
    Process* proc = nullptr;
    if (pm) {
        const gmVariable& p = a_thread->Param(0);
        if (p.m_type == GM_STRING) {
            char buf[256];
            proc = pm->GetByName(p.AsString(a_thread->GetMachine(), buf, sizeof(buf)));
        }
    }
    a_thread->PushInt(proc ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_getProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable& p = a_thread->Param(0);
    if (p.m_type != GM_STRING && p.m_type != GM_INT) return GM_EXCEPTION;
    ProcessManager* pm = GetEngineContext().processManager;
    Process* proc = nullptr;
    if (pm) {
        if (p.m_type == GM_STRING) {
            char buf[256];
            proc = pm->GetByName(p.AsString(a_thread->GetMachine(), buf, sizeof(buf)));
        } else if (p.m_type == GM_INT) {
            proc = pm->Get(p.m_value.m_int);
        }
    }
    if (proc) a_thread->Push(proc->table);
    else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_getEntityUpdateProcess(gmThread* a_thread) {
    ProcessManager* pm = GetEngineContext().processManager;
    Process* proc = pm ? pm->GetEntityUpdateProcess() : nullptr;
    if (proc) a_thread->Push(proc->table); else a_thread->PushNull();
    return GM_OK;
}

// Retail copies the current process registry into a native linked-list
// snapshot and returns PROCESSLISTREFCONTAINER (get/isValid/goToNext/reset/
// size). Snapshot tables remain rooted by ProcessManager during the normal
// same-frame iteration lifecycle.
int GM_CDECL gmf_getAllProcesses(gmThread* a_thread) {
    auto* it = new ProcessListRefData();
    ProcessManager* pm = GetEngineContext().processManager;
    if (pm) {
        int n = pm->GetCount();
        for (int i = 0; i < n; ++i) {
            Process* p = pm->GetByIndex(i);
            if (p) it->processes.push_back(p->table);
        }
    }
    a_thread->PushNewUser(it, g_processListRefType);
    return GM_OK;
}

int GM_CDECL gmf_getProcessCount(gmThread* a_thread) {
    ProcessManager* pm = GetEngineContext().processManager;
    a_thread->PushInt(pm ? pm->GetCount() : 0);
    return GM_OK;
}

int GM_CDECL gmf_getProcessByIndex(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    // Retail reads the raw 32-bit argument value as the list index without
    // checking the GM type tag; preserve that behavior for malformed or
    // non-integer callers as well as ordinary integer indexes.
    const gmint index = a_thread->Param(0).m_value.m_int;
    ProcessManager* pm = GetEngineContext().processManager;
    Process* p = pm ? pm->GetByIndex(index) : nullptr;
    if (p) a_thread->Push(p->table);
    else a_thread->PushNull();
    return GM_OK;
}

int GM_CDECL gmf_quit(gmThread* a_thread) {
    GetEngineContext().quitRequested = true;
    return GM_OK;
}

int GM_CDECL gmf_openConsole(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}


int GM_CDECL gmf_resetSimulationTimer(gmThread* a_thread) {
    GetEngineContext().gameSystem.ResetPlayTimer();
    return GM_OK;
}
int GM_CDECL gmf_getMSAlive(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().gameSystem.GetMsAlive());
    return GM_OK;
}
int GM_CDECL gmf_getMSSincePlayStart(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().gameSystem.GetMsSincePlayStart());
    return GM_OK;
}
int GM_CDECL gmf_getUpdateInterval(gmThread* a_thread) {
    a_thread->PushFloat(1000.0f / 60.0f);
    return GM_OK;
}
int GM_CDECL gmf_resetPlayTimer(gmThread* a_thread) {
    GetEngineContext().gameSystem.ResetPlayTimer();
    return GM_OK;
}


int GM_CDECL gmf_getRandomInteger(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 1, 0.0f);
    a_thread->PushInt(GetEngineContext().gameSystem.NextRandomInteger((int)lo, (int)hi));
    return GM_OK;
}

int GM_CDECL gmf_getRandomBoolean(gmThread* a_thread) {
    a_thread->PushInt(Random01() < 0.5f ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_getRandomDir(gmThread* a_thread) {
    float angle = Random01() * 6.28318530718f;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), std::cos(angle), std::sin(angle)));
    return GM_OK;
}

int GM_CDECL gmf_getRandomAngle(gmThread* a_thread) {
    a_thread->PushFloat(Random01() * 6.28318530718f);
    return GM_OK;
}

int GM_CDECL gmf_getRandomSign(gmThread* a_thread) {
    a_thread->PushFloat(Random01() < 0.5f ? -1.0f : 1.0f);
    return GM_OK;
}

int GM_CDECL gmf_getRandomSignAndReal(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    float lo, hi;
    if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(a, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(b, 1, 0.0f);
        lo = a; hi = b;
    } else if (n == 1) {
        GM_FLOAT_OR_INT_PARAM(b, 0, 0.0f);
        lo = 0.0f; hi = b;
    } else {
        return GM_EXCEPTION;
    }
    float sign = (Random01() < 0.5f) ? 1.0f : -1.0f;
    float mag = lo + (hi - lo) * Random01();
    a_thread->PushFloat(sign * mag);
    return GM_OK;
}

int GM_CDECL gmf_getRandomReal(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n >= 4) {
        GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
        GM_FLOAT_OR_INT_PARAM(biasLo, 2, 0.0f);
        GM_FLOAT_OR_INT_PARAM(biasHi, 3, 1.0f);
        float t = biasLo + (biasHi - biasLo) * Random01();
        a_thread->PushFloat(lo + (hi - lo) * t);
    } else if (n >= 2) {
        GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
        a_thread->PushFloat(lo + (hi - lo) * Random01());
    } else {
        a_thread->PushFloat(Random01());
    }
    return GM_OK;
}

int GM_CDECL gmf_getRandomRealExp(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(3);
    GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
    GM_FLOAT_OR_INT_PARAM(expParam, 2, 1.0f);
    float t = std::pow(Random01(), expParam);
    a_thread->PushFloat(lo + (hi - lo) * t);
    return GM_OK;
}

int GM_CDECL gmf_getSimulationSpeed(gmThread* a_thread) {
    a_thread->PushFloat(GetEngineContext().gameSystem.simulationSpeed);
    return GM_OK;
}
int GM_CDECL gmf_setSimulationSpeed(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(requestedSpeed, 0, 1.0f);
    GetEngineContext().gameSystem.simulationSpeed = requestedSpeed;
    return GM_OK;
}


int GM_CDECL gmf_TemporaryStorageCopy(gmThread* a_thread) {
    gmTableObject* thisTable = a_thread->ThisTable();
    if (!thisTable) return GM_EXCEPTION;
    GetEngineContext().gameSystem.temporaryStorage = *a_thread->GetThis();
    return GM_OK;
}

int GM_CDECL gmf_TemporaryStoragePaste(gmThread* a_thread) {
    GameSystemState& state = GetEngineContext().gameSystem;
    if (state.temporaryStorage.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* dst = a_thread->ThisTable();
    gmTableObject* src = (gmTableObject*)a_thread->GetMachine()->GetObject(state.temporaryStorage.m_value.m_ref);
    if (!dst || !src) return GM_EXCEPTION;
    // Shallow copy every key from the clipboard table onto `this`.
    gmTableIterator it;
    for (gmTableNode* node = src->GetFirst(it); !src->IsNull(it); node = src->GetNext(it)) {
        dst->Set(a_thread->GetMachine(), node->m_key, node->m_value);
    }
    return GM_OK;
}


int GM_CDECL gmf_isAutoLoadLevel(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isAutoLoadLevel ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadLevel(gmThread* a_thread) {
    a_thread->PushNewString(GetEngineContext().gameSystem.autoLoadLevel.c_str());
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadPath(gmThread* a_thread) {
    a_thread->PushNewString(GetEngineContext().gameSystem.autoLoadPath.c_str());
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadMode(gmThread* a_thread) {
    const std::string& mode = GetEngineContext().gameSystem.autoLoadMode;
    if (mode.empty()) a_thread->PushNull();
    else a_thread->PushNewString(mode.c_str());
    return GM_OK;
}
int GM_CDECL gmf_getAutoLoadArea(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.autoLoadArea);
    return GM_OK;
}

int GM_CDECL gmf_GameSystem_trace(gmThread* a_thread) {
    static const bool s_enabled = std::getenv("GE3_TRACE") != nullptr;
    if (!s_enabled) return GM_OK;

    using clock = std::chrono::steady_clock;
    static clock::time_point windowStart = clock::now();
    static int windowCount = 0;
    static bool suppressing = false;
    constexpr int kMaxPerWindow = 200;

    auto now = clock::now();
    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - windowStart).count() >= 1000) {
        windowStart = now;
        windowCount = 0;
        suppressing = false;
    }
    ++windowCount;
    if (windowCount > kMaxPerWindow) {
        if (!suppressing) {
            std::fprintf(stderr, "[trace] (suppressing further trace() calls this second -- rate exceeded %d/sec)\n", kMaxPerWindow);
            suppressing = true;
        }
        return GM_OK;
    }

    char buffer[256];
    std::fprintf(stderr, "[trace]");
    for (int i = 0; i < a_thread->GetNumParams(); ++i) {
        std::fprintf(stderr, " %s", a_thread->Param(i).AsString(a_thread->GetMachine(), buffer, sizeof(buffer)));
    }
    std::fprintf(stderr, "\n");
    return GM_OK;
}

gmFunctionEntry s_gameSystemLib[] = {
    {"getCurrentTick", gmf_getCurrentTick, nullptr},
    {"isPaused", gmf_isPaused, nullptr},
    {"isBeingNice", gmf_isBeingNice, nullptr},
    {"setIsBeingNice", gmf_setIsBeingNice, nullptr},
    {"isShouldUpdateEntities", gmf_isShouldUpdateEntities, nullptr},
    {"setIsShouldUpdateEntities", gmf_setIsShouldUpdateEntities, nullptr},
    {"setALevelLoaded", gmf_setALevelLoaded, nullptr},
    {"setIsLoading", gmf_setIsLoading, nullptr},
    {"isLoading", gmf_isLoading, nullptr},
    {"flushProcesses", gmf_flushProcesses, nullptr},
    {"startNewProcessList", gmf_startNewProcessList, nullptr},
    {"runProcess", gmf_runProcess, nullptr},
    {"isProcessRunning", gmf_isProcessRunning, nullptr},
    {"getProcess", gmf_getProcess, nullptr},
    {"getEntityUpdateProcess", gmf_getEntityUpdateProcess, nullptr},
    {"getAllProcesses", gmf_getAllProcesses, nullptr},
    {"getProcessCount", gmf_getProcessCount, nullptr},
    {"getProcessByIndex", gmf_getProcessByIndex, nullptr},
    {"quit", gmf_quit, nullptr},
    {"openConsole", gmf_openConsole, nullptr},
    {"resetSimulationTimer", gmf_resetSimulationTimer, nullptr},
    {"getMSAlive", gmf_getMSAlive, nullptr},
    {"getMSSincePlayStart", gmf_getMSSincePlayStart, nullptr},
    {"getUpdateInterval", gmf_getUpdateInterval, nullptr},
    {"resetPlayTimer", gmf_resetPlayTimer, nullptr},
    {"getRandomInteger", gmf_getRandomInteger, nullptr},
    {"getRandomBoolean", gmf_getRandomBoolean, nullptr},
    {"getRandomDir", gmf_getRandomDir, nullptr},
    {"getRandomAngle", gmf_getRandomAngle, nullptr},
    {"getRandomSign", gmf_getRandomSign, nullptr},
    {"getRandomSignAndReal", gmf_getRandomSignAndReal, nullptr},
    {"getRandomReal", gmf_getRandomReal, nullptr},
    {"getRandomRealExp", gmf_getRandomRealExp, nullptr},
    {"getSimulationSpeed", gmf_getSimulationSpeed, nullptr},
    {"setSimulationSpeed", gmf_setSimulationSpeed, nullptr},
    {"setIsInputEnabled", gmf_setIsInputEnabled, nullptr},
    {"debug_setStop", gmf_debug_setStop, nullptr},
    {"debug_isStopped", gmf_debug_isStopped, nullptr},
    {"TemporaryStorageCopy", gmf_TemporaryStorageCopy, nullptr},
    {"TemporaryStoragePaste", gmf_TemporaryStoragePaste, nullptr},
    {"isAutoLoadLevel", gmf_isAutoLoadLevel, nullptr},
    {"getAutoLoadLevel", gmf_getAutoLoadLevel, nullptr},
    {"getAutoLoadPath", gmf_getAutoLoadPath, nullptr},
    {"getAutoLoadMode", gmf_getAutoLoadMode, nullptr},
    {"getAutoLoadArea", gmf_getAutoLoadArea, nullptr},
    {"trace", gmf_GameSystem_trace, nullptr},
};

int GM_CDECL gmf_ClearTable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmVariable v = a_thread->Param(0);
    if (v.m_type == GM_TABLE) {
        gmMachine* m = a_thread->GetMachine();
        gmTableObject* t = (gmTableObject*)m->GetObject(v.m_value.m_ref);
        if (t) {
            std::vector<gmVariable> keys;
            gmTableIterator it;
            for (gmTableNode* node = t->GetFirst(it); !t->IsNull(it); node = t->GetNext(it)) {
                keys.push_back(node->m_key);
            }
            for (const gmVariable& key : keys) t->Set(m, key, gmVariable::s_null);
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_DumpObject(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmMachine* m = a_thread->GetMachine();
    gmVariable v = a_thread->Param(0);
    char buffer[256];
    if (v.m_type != GM_TABLE) {
        std::fprintf(stderr, "[DumpObject] %s\n", v.AsString(m, buffer, sizeof(buffer)));
        return GM_OK;
    }
    gmTableObject* t = (gmTableObject*)m->GetObject(v.m_value.m_ref);
    std::fprintf(stderr, "[DumpObject] table:0x%p {\n", (void*)t);
    gmTableIterator it;
    for (gmTableNode* node = t->GetFirst(it); !t->IsNull(it); node = t->GetNext(it)) {
        char keyBuf[128];
        std::fprintf(stderr, "  %s = %s\n",
                     node->m_key.AsString(m, keyBuf, sizeof(keyBuf)),
                     node->m_value.AsString(m, buffer, sizeof(buffer)));
    }
    std::fprintf(stderr, "}\n");
    return GM_OK;
}

int GM_CDECL gmf_Watermelon_Prepare_Screenshot(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_Watermelon_Clear_Prepared_Screenshot(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

} // namespace

void RegisterGameSystemBindings(gmMachine& a_machine) {
    g_processListRefType = a_machine.CreateUserType("PROCESSLISTREFCONTAINER");
    a_machine.RegisterTypeLibrary(g_processListRefType, s_processListRefLib,
                                  sizeof(s_processListRefLib) / sizeof(s_processListRefLib[0]));
    a_machine.RegisterUserCallbacks(g_processListRefType, nullptr, ProcessListRefGCDestruct, nullptr);
    a_machine.RegisterLibrary(s_gameSystemLib, sizeof(s_gameSystemLib) / sizeof(s_gameSystemLib[0]), "GameSystem");
    a_machine.RegisterLibraryFunction("DumpObject", gmf_DumpObject);
    a_machine.RegisterLibraryFunction("ClearTable", gmf_ClearTable);
    a_machine.RegisterLibraryFunction("Watermelon_Prepare_Screenshot", gmf_Watermelon_Prepare_Screenshot);
    a_machine.RegisterLibraryFunction("Watermelon_Clear_Prepared_Screenshot", gmf_Watermelon_Clear_Prepared_Screenshot);
}
