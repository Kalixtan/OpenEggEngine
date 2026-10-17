#include "gmThread.h"
#include "script_compat_bindings.h"
#include "gmTableObject.h"
#include "gmCall.h"
#include "../core/engine_context.h"
#include "vector2d_bindings.h"
#include <cstdlib>
#include <cmath>
#include <vector>


namespace {
float Random01() {
    return (float)rand() / (float)RAND_MAX;
}
} // namespace

int GM_CDECL gmf_RANDOM_GetRandomReal(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n >= 2) {
        // Two-parameter form: (lo, hi)
        GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
        GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f);
        a_thread->PushFloat(lo + (hi - lo) * Random01());
    } else if (n >= 1) {
        // One-parameter form: (hi) -- lo=0
        GM_FLOAT_OR_INT_PARAM(hi, 0, 1.0f);
        a_thread->PushFloat(hi * Random01());
    } else {
        // Zero-parameter form: random [0, 1)
        a_thread->PushFloat(Random01());
    }
    return GM_OK;
}

int GM_CDECL gmf_RANDOM_GetRandomInteger(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f);
    GM_FLOAT_OR_INT_PARAM(hi, 1, 0.0f);
    int ilo = (int)lo, ihi = (int)hi;
    if (ihi < ilo) std::swap(ilo, ihi);
    int range = ihi - ilo + 1;
    a_thread->PushInt(ilo + (range > 0 ? (std::rand() % range) : 0));
    return GM_OK;
}
int GM_CDECL gmf_RANDOM_GetRandomBoolean(gmThread* a_thread) {
    a_thread->PushInt((std::rand() % 2) ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_REALPOINT(gmThread* a_thread) {
    (void)a_thread;
    return GM_EXCEPTION;
}

static int g_alarmIDCounter = 0;
int GM_CDECL gmf_getUniqueAlarmID(gmThread* a_thread) {
    a_thread->PushInt(g_alarmIDCounter++);
    return GM_OK;
}

static int g_lfoIDCounter = 0;
int GM_CDECL gmf_getUniqueLFOID(gmThread* a_thread) {
    a_thread->PushInt(g_lfoIDCounter++);
    return GM_OK;
}


struct AlarmState {
    gmVariable owner;      // the table addAlarm was called on (Entity or Process handle)
    int alarmId = 0;
    float intervalMs = 0.0f;
    float elapsedMs = 0.0f;
    bool repeat = false;
    bool enabled = true;
};
std::vector<AlarmState> g_alarms;

AlarmState* FindAlarm(gmMachine& a_machine, const gmVariable& a_owner, int a_id) {
    for (auto& a : g_alarms) {
        if (a.alarmId == a_id && a.owner.m_type == GM_TABLE && a_owner.m_type == GM_TABLE &&
            a.owner.m_value.m_ref == a_owner.m_value.m_ref) {
            return &a;
        }
    }
    return nullptr;
}

gmType g_alarmHandleType = -1;

struct AlarmHandleData {
    gmVariable owner;
    int alarmId;
};

AlarmState* ThisAlarm(gmThread* a_thread) {
    AlarmHandleData* h = (AlarmHandleData*)a_thread->ThisUserCheckType(g_alarmHandleType);
    if (!h) return nullptr;
    return FindAlarm(*a_thread->GetMachine(), h->owner, h->alarmId);
}

int GM_CDECL gmf_alarm_setInterval(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread); if (!a) return GM_OK; // leniency: stale/deleted alarm
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    a->intervalMs = ms;
    return GM_OK;
}
int GM_CDECL gmf_alarm_hardReset(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread); if (!a) return GM_OK;
    a->elapsedMs = 0.0f;
    a->enabled = true;
    return GM_OK;
}
int GM_CDECL gmf_alarm_reset(gmThread* a_thread) { return gmf_alarm_hardReset(a_thread); }
int GM_CDECL gmf_alarm_getInterval(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushFloat(a ? a->intervalMs : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_alarm_isAlarming(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushInt((a && a->enabled && a->elapsedMs >= a->intervalMs) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_alarm_getTimeTillAlarm(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushFloat(a ? std::max(0.0f, a->intervalMs - a->elapsedMs) : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_alarm_getAlarmPercentage(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    float pct = (a && a->intervalMs > 0.0001f) ? std::min(1.0f, a->elapsedMs / a->intervalMs) : 0.0f;
    a_thread->PushFloat(pct);
    return GM_OK;
}
int GM_CDECL gmf_alarm_getMSElapsed(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushFloat(a ? a->elapsedMs : 0.0f);
    return GM_OK;
}
int GM_CDECL gmf_alarm_isEnabled(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread);
    a_thread->PushInt((a && a->enabled) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_alarm_setIsEnabled(gmThread* a_thread) {
    AlarmState* a = ThisAlarm(a_thread); if (!a) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(v, 0, 1);
    a->enabled = (v != 0);
    return GM_OK;
}
int GM_CDECL gmf_alarm_noop(gmThread* a_thread) { return GM_OK; } // update()/randomReset()/phaseReset() -- ticked centrally, see TickAlarms

gmVariable MakeAlarmHandle(gmMachine& a_machine, const gmVariable& a_owner, int a_id) {
    if (g_alarmHandleType < 0) {
        g_alarmHandleType = a_machine.CreateUserType("ALARM");
        static gmFunctionEntry lib[] = {
            {"setInterval", gmf_alarm_setInterval, nullptr},
            {"hardReset", gmf_alarm_hardReset, nullptr},
            {"reset", gmf_alarm_reset, nullptr},
            {"phaseReset", gmf_alarm_noop, nullptr},
            {"randomReset", gmf_alarm_noop, nullptr},
            {"update", gmf_alarm_noop, nullptr},
            {"getInterval", gmf_alarm_getInterval, nullptr},
            {"isAlarming", gmf_alarm_isAlarming, nullptr},
            {"getTimeTillAlarm", gmf_alarm_getTimeTillAlarm, nullptr},
            {"getAlarmPercentage", gmf_alarm_getAlarmPercentage, nullptr},
            {"getMSElapsed", gmf_alarm_getMSElapsed, nullptr},
            {"isEnabled", gmf_alarm_isEnabled, nullptr},
            {"setIsEnabled", gmf_alarm_setIsEnabled, nullptr},
        };
        a_machine.RegisterTypeLibrary(g_alarmHandleType, lib, sizeof(lib) / sizeof(lib[0]));
    }
    AlarmHandleData* h = new AlarmHandleData{a_owner, a_id};
    gmVariable v; v.SetUser(a_machine.AllocUserObject(h, g_alarmHandleType));
    return v;
}

// addAlarm(id, intervalMs, repeat) -- see this section's header comment.
// Real callers all ignore the return value except via a SEPARATE
// getAlarm(id) call later, but returning a real handle here (rather than
// null) costs nothing and matches retail's own `ALARM(...)`-return shape.
int GM_CDECL gmf_addAlarm(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(id, 0, 0);
    GM_FLOAT_OR_INT_PARAM(intervalMs, 1, 0.0f);
    bool startReady = (a_thread->GetNumParams() >= 3) ? (a_thread->ParamInt(2, 0) != 0) : false;
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = gmVariable(self);
    AlarmState* existing = FindAlarm(m, owner, id);
    if (existing) {
        existing->intervalMs = intervalMs;
        existing->repeat = true;
        existing->elapsedMs = startReady ? intervalMs : 0.0f;
        existing->enabled = true;
    } else {
        AlarmState a;
        a.owner = owner; a.alarmId = id; a.intervalMs = intervalMs; a.repeat = true;
        a.elapsedMs = startReady ? intervalMs : 0.0f;
        g_alarms.push_back(a);
    }
    a_thread->Push(MakeAlarmHandle(m, owner, id));
    return GM_OK;
}

int GM_CDECL gmf_deleteAlarm(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(id, 0, 0);
    gmVariable owner = gmVariable(self);
    for (size_t i = 0; i < g_alarms.size(); ++i) {
        if (g_alarms[i].alarmId == id && g_alarms[i].owner.m_type == GM_TABLE &&
            g_alarms[i].owner.m_value.m_ref == owner.m_value.m_ref) {
            g_alarms.erase(g_alarms.begin() + (long)i);
            break;
        }
    }
    return GM_OK;
}

int GM_CDECL gmf_getAlarm(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_INT_PARAM(id, 0, 0);
    gmMachine& m = *a_thread->GetMachine();
    gmVariable owner = gmVariable(self);
    if (!FindAlarm(m, owner, id)) { a_thread->PushNull(); return GM_OK; }
    a_thread->Push(MakeAlarmHandle(m, owner, id));
    return GM_OK;
}

// Ticks every live alarm and fires owner.onAlarm(alarmId) for any that
// elapsed this frame. ONE registry, called ONCE per frame from
// ProcessManager::UpdateAll -- covers both process-owned alarms (e.g.
// this.Process.addAlarm in Main.gm) and entity-owned alarms (e.g.
// this.Entity.addAlarm in Thing.gm) since `owner` is stored generically at
// addAlarm() time, whichever handle type it was called on. Entity handles
// split method/state across __behaviorOwner in this engine (unlike
// retail), so onAlarm is looked up there first if the bare handle doesn't
// define it directly -- mirrors gmf_callMethodByName's exact fallback.
void TickAlarms(gmMachine& a_machine, float a_deltaMs) {
    // Snapshot ids, not pointers -- onAlarm() can itself call addAlarm/
    // deleteAlarm (real usage: Main.gm's onAlarm handler calls hardReset()
    // on the SAME alarm it's handling), which would reallocate g_alarms
    // out from under a live loop.
    struct FireEvent { gmVariable owner; int id; };
    std::vector<FireEvent> toFire;
    for (auto& a : g_alarms) {
        if (!a.enabled) continue;
        a.elapsedMs += a_deltaMs;
        if (a.elapsedMs >= a.intervalMs) {
            toFire.push_back({a.owner, a.alarmId});
            if (a.repeat) a.elapsedMs -= a.intervalMs;
            else a.enabled = false;
        }
    }
    for (auto& ev : toFire) {
        if (ev.owner.m_type != GM_TABLE) continue;
        gmTableObject* self = (gmTableObject*)a_machine.GetObject(ev.owner.m_value.m_ref);
        if (!self) continue;
        gmTableObject* target = self;
        gmVariable targetVar = ev.owner;
        if (self->Get(&a_machine, "onAlarm").m_type != GM_FUNCTION) {
            gmVariable behaviorOwner = self->Get(&a_machine, "__behaviorOwner");
            if (behaviorOwner.m_type == GM_TABLE) {
                gmTableObject* ownerTbl = (gmTableObject*)a_machine.GetObject(behaviorOwner.m_value.m_ref);
                if (ownerTbl && ownerTbl->Get(&a_machine, "onAlarm").m_type == GM_FUNCTION) {
                    target = ownerTbl; targetVar = behaviorOwner;
                }
            }
        }
        if (target->Get(&a_machine, "onAlarm").m_type != GM_FUNCTION) continue;
        gmCall call;
        if (call.BeginTableFunction(&a_machine, "onAlarm", target, targetVar)) {
            call.AddParamInt(ev.id);
            call.End();
        }
    }
}

gmType g_lfoType = -1;
struct LFOData { float phase = 0.0f; float rate = 1.0f; float lo = 0.0f; float hi = 1.0f; int waveform = 0; };
LFOData* ThisLFO(gmThread* a_thread) { return (LFOData*)a_thread->ThisUserCheckType(g_lfoType); }
int GM_CDECL gmf_lfo_setPhase(gmThread* a_thread) { LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 0.0f); l->phase = v; return GM_OK; }
int GM_CDECL gmf_lfo_setRate(gmThread* a_thread) { LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK; GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(v, 0, 1.0f); l->rate = v; return GM_OK; }
int GM_CDECL gmf_lfo_setRange(gmThread* a_thread) { LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK; GM_CHECK_NUM_PARAMS(2); GM_FLOAT_OR_INT_PARAM(lo, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(hi, 1, 1.0f); l->lo = lo; l->hi = hi; return GM_OK; }
int GM_CDECL gmf_lfo_getValue(gmThread* a_thread) {
    LFOData* l = ThisLFO(a_thread);
    if (!l) { a_thread->PushFloat(0.0f); return GM_OK; }
    float t = l->phase - std::floor(l->phase); // [0,1) sawtooth phase
    float shaped;
    if (l->waveform == 1) { // PULSELFO -- square
        shaped = (t < 0.5f) ? 0.0f : 1.0f;
    } else if (l->waveform == 2) { // TRIANGLELFO
        shaped = (t < 0.5f) ? (t * 2.0f) : (2.0f - t * 2.0f);
    } else { // SAWLFO
        shaped = t;
    }
    a_thread->PushFloat(l->lo + (l->hi - l->lo) * shaped);
    return GM_OK;
}
int GM_CDECL gmf_lfo_update(gmThread* a_thread) {
    LFOData* l = ThisLFO(a_thread); if (!l) return GM_OK;
    GM_CHECK_NUM_PARAMS(1); GM_FLOAT_OR_INT_PARAM(ms, 0, 0.0f);
    l->phase += (ms * 0.001f) * l->rate;
    return GM_OK;
}
gmVariable MakeLFO(gmMachine& a_machine, int a_waveform) {
    if (g_lfoType < 0) {
        g_lfoType = a_machine.CreateUserType("LFO");
        static gmFunctionEntry lib[] = {
            {"setPhase", gmf_lfo_setPhase, nullptr},
            {"setRandomPhase", gmf_lfo_setPhase, nullptr},
            {"setRate", gmf_lfo_setRate, nullptr},
            {"setRange", gmf_lfo_setRange, nullptr},
            {"setContour", gmf_alarm_noop, nullptr},
            {"getValue", gmf_lfo_getValue, nullptr},
            {"update", gmf_lfo_update, nullptr},
        };
        a_machine.RegisterTypeLibrary(g_lfoType, lib, sizeof(lib) / sizeof(lib[0]));
    }
    LFOData* l = new LFOData();
    l->waveform = a_waveform;
    gmVariable v; v.SetUser(a_machine.AllocUserObject(l, g_lfoType));
    return v;
}
int GM_CDECL gmf_SAWLFO(gmThread* a_thread) { a_thread->Push(MakeLFO(*a_thread->GetMachine(), 0)); return GM_OK; }
int GM_CDECL gmf_PULSELFO(gmThread* a_thread) { a_thread->Push(MakeLFO(*a_thread->GetMachine(), 1)); return GM_OK; }
// No gmf_TRIANGLELFO here -- lfo_bindings.cpp registers the real global
// "TRIANGLELFO" constructor now (see RegisterScriptCompatBindings's own
// comment on why this file no longer competes with it).

int GM_CDECL gmf_addLFO(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    a_thread->Push(a_thread->Param(1));
    return GM_OK;
}
int GM_CDECL gmf_deleteLFO(gmThread* a_thread) { return GM_OK; }

constexpr float kGameBoardMinX = -2399.0f;
constexpr float kGameBoardMinY = -1339.0f;
constexpr float kGameBoardMaxX = 2399.0f;
constexpr float kGameBoardMaxY = 1339.0f;
constexpr float kGameBoardHalfWidth = kGameBoardMaxX;   // board is origin-centred
constexpr float kGameBoardHalfHeight = kGameBoardMaxY;

int GM_CDECL gmf_GameBoard_getMinPos(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), -kGameBoardHalfWidth, -kGameBoardHalfHeight));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getMaxPos(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), kGameBoardHalfWidth, kGameBoardHalfHeight));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getMidPos(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), 0.0f, 0.0f));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getWidth(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfWidth * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getHeight(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfHeight * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getRangeX(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfWidth * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getRangeY(gmThread* a_thread) { a_thread->PushFloat(kGameBoardHalfHeight * 2.0f); return GM_OK; }
int GM_CDECL gmf_GameBoard_getRange(gmThread* a_thread) {
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), kGameBoardHalfWidth * 2.0f, kGameBoardHalfHeight * 2.0f));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_getHypotenuse(gmThread* a_thread) {
    float w = kGameBoardHalfWidth * 2.0f, h = kGameBoardHalfHeight * 2.0f;
    a_thread->PushFloat(std::sqrt(w * w + h * h));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_makeRandomPos(gmThread* a_thread) {
    float x = -kGameBoardHalfWidth + (kGameBoardHalfWidth * 2.0f) * Random01();
    float y = -kGameBoardHalfHeight + (kGameBoardHalfHeight * 2.0f) * Random01();
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x, y));
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_isInGameBoard(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    float x = 0, y = 0;
    bool ok = false;
    if (a_thread->GetNumParams() >= 2) {
        GM_FLOAT_OR_INT_PARAM(px, 0, 0.0f); GM_FLOAT_OR_INT_PARAM(py, 1, 0.0f);
        x = px; y = py; ok = true;
    } else {
        ok = GetVector2D(m, a_thread->Param(0), x, y);
    }
    bool inBoard = ok && x >= -kGameBoardHalfWidth && x <= kGameBoardHalfWidth &&
                    y >= -kGameBoardHalfHeight && y <= kGameBoardHalfHeight;
    a_thread->PushInt(inBoard ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_GameBoard_setBoundsNoop(gmThread* a_thread) { return GM_OK; }

gmFunctionEntry s_gameBoardLib[] = {
    {"getMinPos", gmf_GameBoard_getMinPos, nullptr},
    {"getMaxPos", gmf_GameBoard_getMaxPos, nullptr},
    {"getMidPos", gmf_GameBoard_getMidPos, nullptr},
    {"getWidth", gmf_GameBoard_getWidth, nullptr},
    {"getHeight", gmf_GameBoard_getHeight, nullptr},
    {"getRangeX", gmf_GameBoard_getRangeX, nullptr},
    {"getRangeY", gmf_GameBoard_getRangeY, nullptr},
    {"getRange", gmf_GameBoard_getRange, nullptr},
    {"getHypotenuse", gmf_GameBoard_getHypotenuse, nullptr},
    {"makeRandomPos", gmf_GameBoard_makeRandomPos, nullptr},
    {"makeRandomPosOutsideOfGameBoard", gmf_GameBoard_makeRandomPos, nullptr},
    {"isInGameBoard", gmf_GameBoard_isInGameBoard, nullptr},
    {"setMinPos", gmf_GameBoard_setBoundsNoop, nullptr},
    {"setMaxPos", gmf_GameBoard_setBoundsNoop, nullptr},
};

// setTexture() -- Stub (not in binary)
int GM_CDECL gmf_setTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    return GM_OK;
}

// setIsUsesTexture() -- Stub
int GM_CDECL gmf_setIsUsesTexture(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    return GM_OK;
}

// isUsesTexture() -- Stub
int GM_CDECL gmf_isUsesTexture(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

// beginTexture() -- Stub (not in binary)
int GM_CDECL gmf_beginTexture(gmThread* a_thread) {
    return GM_OK;
}

// endTexture() -- Stub (not in binary)
int GM_CDECL gmf_endTexture(gmThread* a_thread) {
    return GM_OK;
}

// loadTexture() -- Stub
int GM_CDECL gmf_loadTexture(gmThread* a_thread) {
    a_thread->PushNull();
    return GM_OK;
}

// deleteLoadedTextures() -- Stub
int GM_CDECL gmf_deleteLoadedTextures(gmThread* a_thread) {
    return GM_OK;
}

// drawTexture() -- Stub
int GM_CDECL gmf_drawTexture(gmThread* a_thread) {
    return GM_OK;
}

void RegisterScriptCompatBindings(gmMachine& a_machine) {
    // Global random functions
    a_machine.GetGlobals()->Set(&a_machine, "RANDOM_GetRandomReal",
        gmVariable(a_machine.AllocFunctionObject(gmf_RANDOM_GetRandomReal)));
    a_machine.GetGlobals()->Set(&a_machine, "RANDOM_GetRandomInteger",
        gmVariable(a_machine.AllocFunctionObject(gmf_RANDOM_GetRandomInteger)));
    a_machine.GetGlobals()->Set(&a_machine, "RANDOM_GetRandomBoolean",
        gmVariable(a_machine.AllocFunctionObject(gmf_RANDOM_GetRandomBoolean)));
    a_machine.GetGlobals()->Set(&a_machine, "SAWLFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_SAWLFO)));
    a_machine.GetGlobals()->Set(&a_machine, "PULSELFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_PULSELFO)));

    // REALPOINT type (2D point/vector)
    a_machine.GetGlobals()->Set(&a_machine, "REALPOINT",
        gmVariable(a_machine.AllocFunctionObject(gmf_REALPOINT)));

    // Alarm ID management
    a_machine.GetGlobals()->Set(&a_machine, "getUniqueAlarmID",
        gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueAlarmID)));
    a_machine.GetGlobals()->Set(&a_machine, "addAlarm",
        gmVariable(a_machine.AllocFunctionObject(gmf_addAlarm)));
    a_machine.GetGlobals()->Set(&a_machine, "deleteAlarm",
        gmVariable(a_machine.AllocFunctionObject(gmf_deleteAlarm)));
    a_machine.GetGlobals()->Set(&a_machine, "getAlarm",
        gmVariable(a_machine.AllocFunctionObject(gmf_getAlarm)));

    // LFO ID management
    a_machine.GetGlobals()->Set(&a_machine, "getUniqueLFOID",
        gmVariable(a_machine.AllocFunctionObject(gmf_getUniqueLFOID)));
    a_machine.GetGlobals()->Set(&a_machine, "addLFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_addLFO)));
    a_machine.GetGlobals()->Set(&a_machine, "deleteLFO",
        gmVariable(a_machine.AllocFunctionObject(gmf_deleteLFO)));

    // Texture functions
    a_machine.GetGlobals()->Set(&a_machine, "setTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_setTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "setIsUsesTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_setIsUsesTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "isUsesTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_isUsesTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "loadTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_loadTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "deleteLoadedTextures",
        gmVariable(a_machine.AllocFunctionObject(gmf_deleteLoadedTextures)));
    a_machine.GetGlobals()->Set(&a_machine, "drawTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_drawTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "beginTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_beginTexture)));
    a_machine.GetGlobals()->Set(&a_machine, "endTexture",
        gmVariable(a_machine.AllocFunctionObject(gmf_endTexture)));

    a_machine.RegisterLibrary(s_gameBoardLib, sizeof(s_gameBoardLib) / sizeof(s_gameBoardLib[0]), "GameBoard");
}

bool IsPointInGameBoard(float a_x, float a_y) {
    return a_x >= -kGameBoardHalfWidth && a_x <= kGameBoardHalfWidth &&
           a_y >= -kGameBoardHalfHeight && a_y <= kGameBoardHalfHeight;
}
