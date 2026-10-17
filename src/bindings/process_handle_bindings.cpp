#include "gmThread.h"
#include "process_handle_bindings.h"
#include "gmTableObject.h"
#include "gmUserObject.h"
#include "gmStringObject.h"
#include "gmCall.h"
#include "../core/engine_context.h"
#include "../core/process_manager.h"
#include "../core/strict_calls.h"
#include "../render/draw_list.h"
#include "../ported_native/ground_renderer.h"
#include "audio_bindings.h"
#include "physics_body_bindings.h"
#include "script_compat_bindings.h"
#include "vector2d_bindings.h"
#include "sequencer_bindings.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

gmType g_shaderType = -1;
struct ShaderHandleData {
    int processId;
    int shaderId;
    bool isDefaultPostProcess;
};

int GM_CDECL gmf_shader_getVertexShaderParamIndex(gmThread* a_thread) {
    ShaderHandleData* shader = (ShaderHandleData*)a_thread->ThisUserCheckType(g_shaderType);
    if (!shader || a_thread->GetNumParams() != 1 || a_thread->Param(0).m_type != GM_STRING)
        return GM_EXCEPTION;
    if (shader->isDefaultPostProcess) {
        GM_EXCEPTION_MSG("SHADER.getVertexShaderParamIndex: default post-process shader is null");
        return GM_EXCEPTION;
    }
    const char* name = a_thread->ParamString(0, "");
    static const char* params[] = {"aPosition", "aData", "aViewWorldProjection",
                                   "aColour", "aCurve", "aRMin", "aRMax"};
    if (shader->shaderId == 0) {
        for (int i = 0; i < 7; ++i) {
            if (std::strcmp(name, params[i]) == 0) { a_thread->PushInt(i); return GM_OK; }
        }
        a_thread->GetMachine()->GetLog().LogEntry(
            "Error: Param %s not found in shader (GetShaderParamID)", name);
        a_thread->PushInt(0);
        return GM_OK;
    }
    GM_EXCEPTION_MSG("SHADER.getVertexShaderParamIndex: unknown parameter %s on shader %d", name, shader->shaderId);
    return GM_EXCEPTION;
}

int GM_CDECL gmf_shader_test(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

void GM_CDECL ShaderGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (ShaderHandleData*)a_object->m_user;
}

gmFunctionEntry s_shaderLib[] = {
    {"test", gmf_shader_test, nullptr},
    {"getVertexShaderParamIndex", gmf_shader_getVertexShaderParamIndex, nullptr},
};

gmTableObject* ThisHandleTable(gmThread* a_thread) { return a_thread->ThisTable(); }

// Every method re-looks-up the Process by id rather than capturing a
// pointer -- see header comment. Returns nullptr (and raises a GM
// exception) if the process has since gone away.
Process* LookupProcess(gmThread* a_thread, gmTableObject* a_self) {
    gmVariable idVar = a_self->Get(a_thread->GetMachine(), "__id");
    if (idVar.m_type != GM_INT) return nullptr;
    return GetEngineContext().processManager->Get(idVar.m_value.m_int);
}

int GM_CDECL gmf_process_getShader(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* process = self ? LookupProcess(a_thread, self) : nullptr;
    // Retail PROCESS.getShader reads the first argument's raw 32-bit value
    // word and passes it to the unsigned shader-array selector. It does not
    // validate the GM type tag or reject trailing arguments. Keep the
    // one-or-more desktop boundary explicit; the no-argument native read is
    // outside the supported call contract.
    if (!process || a_thread->GetNumParams() < 1)
        return GM_EXCEPTION;
    const uint32_t shaderIndex = static_cast<uint32_t>(
        a_thread->Param(0).m_value.m_int);
    if (process->name != "Platformer_Main" || shaderIndex != 0u) {
        GM_EXCEPTION_MSG("PROCESS.getShader: unsupported process shader %s:%u",
                         process->name.c_str(), shaderIndex);
        return GM_EXCEPTION;
    }
    a_thread->PushNewUser(new ShaderHandleData{process->id, 0, false}, g_shaderType);
    return GM_OK;
}

int GM_CDECL gmf_process_callMethod(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    gmMachine& m = *a_thread->GetMachine();
    GM_STRING_PARAM(name, 0, "");
    gmCall call;
    if (call.BeginTableFunction(&m, name, self, gmVariable(self))) {
        for (int i = 1; i < a_thread->GetNumParams(); ++i) call.AddParam(a_thread->Param(i));
        call.End();
        a_thread->Push(call.GetReturnedVariable());
    } else {
        a_thread->PushNull();
    }
    return GM_OK;
}

int GM_CDECL gmf_setIsStealsInputFocus(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    proc->stealsInputFocus = (v != 0);
    return GM_OK;
}

int GM_CDECL gmf_setIsUpdatesEntities(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(v, 0, 0);
    proc->updatesEntities = (v != 0);
    return GM_OK;
}

// A few authored entity packs query their owning gameplay process while
// configuring (`this.gameProcess.isBeingEdited()`). In a normal play level
// there is no active editor operation, so retail returns false. This is
// distinct from an entity handle's per-entity editing flag.
int GM_CDECL gmf_process_isBeingEdited(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL gmf_process_drawGrounds(gmThread* a_thread) {
    gmUserObject* object = a_thread->ParamUserObject(0);
    if (!object || object->m_userType != GetDrawListType()) return GM_EXCEPTION;
    RecordGroundsNative(static_cast<DrawListData*>(object->m_user));
    return GM_OK;
}
int GM_CDECL gmf_process_onMusicTriggerDefault(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc || a_thread->GetNumParams() < 1) return GM_OK;
    const int step = a_thread->Param(0).m_value.m_int;
    if (step % 8 != 0) return GM_OK;

    proc->fireColourPhase = 1.0f;
    proc->fireAmplitudePhase = 1.0f;
    const float eightStepPeriod =
        GetMainSequencerMsPerStep(*a_thread->GetMachine()) * 8.0f;
    proc->fireAmplitudeScale = 2.0f / std::max(eightStepPeriod, 1.0f);
    return GM_OK;
}

int GM_CDECL gmf_process_refreshFunctionCache(gmThread*) { return GM_OK; }

// EntityRegistration.setFireThickness(value), retail wrapper 0x1d07cc:
// GM_FLOAT payload is stored directly; every other GM tag is treated as the
// signed integer payload and converted to f32. Retail ignores extra args.
int GM_CDECL gmf_process_setFireThickness(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& value = a_thread->Param(0);
    proc->fireThickness = value.m_type == GM_FLOAT
        ? value.m_value.m_float : static_cast<float>(value.m_value.m_int);
    return GM_OK;
}

// EntityRegistration.getFireThickness(), retail wrapper 0x1d074c, returns
// the f32 field at owner +0xb8 without additional conversion.
int GM_CDECL gmf_process_getFireThickness(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushFloat(proc->fireThickness);
    return GM_OK;
}

// EntityRegistration.setIsRefreshDrawListOnChange(value), retail wrapper
// 0x1d06b4, stores raw argument payload != 0 at owner +0x198. Interpreting
// the payload as an integer preserves retail behavior for all GM tags.
int GM_CDECL gmf_process_setRefreshDrawListsOnChange(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    proc->refreshGroundDrawListsOnChange =
        a_thread->Param(0).m_value.m_int != 0;
    return GM_OK;
}

int GM_CDECL gmf_process_setupDrawLists(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

int GM_CDECL gmf_process_registerGround(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(3);
    gmTableObject* entity = nullptr;
    if (!a_thread->ParamTable(0, entity) || !entity) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    gmVariable id = entity->Get(&machine, "__objectID");
    if (id.m_type != GM_INT) return GM_EXCEPTION;
    GM_INT_PARAM(style, 1, 0);
    GM_INT_PARAM(selected, 2, 0);
    // The script's registerEntity begins with unregisterEntity(e), then
    // appends the new shape to grounds. Preserve that exact replacement order.
    const int entityId = id.m_value.m_int;
    if (style == 2 && !proc->hasNativeGroundFireOwner) {
        // The native style factory constructs its fire owner when the first
        // fire ground is registered. Its first update event runs vtable +0x34
        // (shader setup); the following eligible update runs +0x38.
        proc->hasNativeGroundFireOwner = true;
        proc->nativeGroundFireUpdateReady = false;
        proc->fireAmplitudePhase = 0.0f;
        proc->fireAmplitudeRate = 1.0f;
        proc->fireAmplitudeScale = 2.0f / 800.0f;
        proc->fireColourPhase = 0.0f;
        proc->fireColourRate = 1.0f;
        proc->fireColourScale = 2.0f / 600.0f;
    }
    proc->groundRegistrations.erase(entityId);
    auto& drawOrder = proc->groundRegistrationOrder;
    drawOrder.erase(std::remove(drawOrder.begin(), drawOrder.end(), entityId), drawOrder.end());
    gmVariable physicsData = entity->Get(&machine, "PhysicsData");
    if (physicsData.m_type == GM_NULL)
        physicsData = entity->Get(&machine, "__physicsData");
    float minX = 0.0f, minY = 0.0f, maxX = 0.0f, maxY = 0.0f;
    GetPhysicsDataAllBodiesAABB(machine, physicsData, minX, minY, maxX, maxY);
    auto cell = [](float position, float reciprocal) {
        int result = static_cast<int>(position * reciprocal);
        if (position < 0.0f) --result;
        return result;
    };
    constexpr float kInvScreenCellWidth = 0.025614753365516663f;  // PPC 0x3cd1d608
    constexpr float kInvScreenCellHeight = 0.04553733766078949f; // PPC 0x3d3a855c
    proc->groundRegistrations[entityId] =
        Process::GroundRegistration{style, selected != 0,
            0.0f, 0.0f,
            cell(minX, kInvScreenCellWidth), cell(maxX, kInvScreenCellWidth),
            cell(minY, kInvScreenCellHeight), cell(maxY, kInvScreenCellHeight)};
    drawOrder.push_back(entityId);
    return GM_OK;
}

int GM_CDECL gmf_process_unregisterGround(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    gmTableObject* entity = nullptr;
    if (!a_thread->ParamTable(0, entity) || !entity) return GM_EXCEPTION;
    gmVariable id = entity->Get(a_thread->GetMachine(), "__objectID");
    if (id.m_type != GM_INT) return GM_EXCEPTION;
    const int entityId = id.m_value.m_int;
    proc->groundRegistrations.erase(entityId);
    auto& drawOrder = proc->groundRegistrationOrder;
    drawOrder.erase(std::remove(drawOrder.begin(), drawOrder.end(), entityId), drawOrder.end());
    return GM_OK;
}

int GM_CDECL gmf_process_getGroundCountOnScreen(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(2);
    GM_INT_PARAM(screenX, 0, 0);
    GM_INT_PARAM(screenY, 1, 0);
    const int count = GetEngineContext().processManager->GetGroundCountOnScreen(
        screenX, screenY);
    a_thread->PushInt(count);
    return GM_OK;
}

int GM_CDECL gmf_process_isLoading(gmThread* a_thread) {
    a_thread->PushInt(GetEngineContext().gameSystem.isLoadingFlag ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_terminate(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GetEngineContext().processManager->QueueTerminate(proc->id);
    return GM_OK;
}

int GM_CDECL gmf_isInited(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushInt(proc->initialized ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_setupPreDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    int w = (int)a_thread->ParamInt(0, 0);
    int h = (int)a_thread->ParamInt(1, 0);
    PushNewDrawList(a_thread, w, h);
    UnrootGmVariable(*a_thread->GetMachine(), proc->preDrawList); // no-op if s_null; see process_manager.h
    proc->preDrawList = *(a_thread->GetTop() - 1); // copy what we just pushed, see header comment
    RootGmVariable(*a_thread->GetMachine(), proc->preDrawList);
    return GM_OK;
}

int GM_CDECL gmf_setupPostDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    int w = (int)a_thread->ParamInt(0, 0);
    int h = (int)a_thread->ParamInt(1, 0);
    PushNewDrawList(a_thread, w, h);
    UnrootGmVariable(*a_thread->GetMachine(), proc->postDrawList); // no-op if s_null; see process_manager.h
    proc->postDrawList = *(a_thread->GetTop() - 1);
    RootGmVariable(*a_thread->GetMachine(), proc->postDrawList);
    return GM_OK;
}

int GM_CDECL gmf_getPreDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->preDrawList);
    return GM_OK;
}

int GM_CDECL gmf_getPostDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->postDrawList);
    return GM_OK;
}

// Real usage (Platformer/TouchEditProcessBase.gm, commented out) calls this
// with no further disambiguation between pre/post -- aliased to the pre
// list, matching how the un-suffixed this.setupPreDrawList/drawList is the
// "main" one in every real process that only sets up a single list.
int GM_CDECL gmf_getDrawList(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->preDrawList);
    return GM_OK;
}

int GM_CDECL gmf_getObjectID(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushInt(proc->id);
    return GM_OK;
}

// PROCESS.isHasErrors / getName / rename are entries in the retail PROCESS
// method table at 0x4228b0. isHasErrors dispatches the object's vtable slot
// +0x70, whose base Process implementation reads the error byte at +0x38.
// rename obtains the supplied GM string bytes, computes their C-string
// length, copies them into native string storage, then replaces the process
// name. getName converts the stored native name back to GM_STRING.
int GM_CDECL gmf_process_isHasErrors(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushInt(proc->hasErrors ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_process_getName(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->PushString(a_thread->GetMachine()->AllocStringObject(
        proc->name.c_str()));
    return GM_OK;
}

int GM_CDECL gmf_process_rename(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& arg = a_thread->Param(0);
    if (arg.m_type != GM_STRING || !arg.m_value.m_ref) return GM_EXCEPTION;
    auto* stringObject = static_cast<gmStringObject*>(
        a_thread->GetMachine()->GetObject(arg.m_value.m_ref));
    if (!stringObject) return GM_EXCEPTION;
    proc->name.assign(stringObject->GetString());
    return GM_OK;
}

int GM_CDECL gmf_setScaleIndex(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    self->Set(a_thread->GetMachine(), "currentScaleIndex", a_thread->Param(0));
    return GM_OK;
}
int GM_CDECL gmf_setTranspose(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    const gmVariable value = a_thread->Param(0);
    const gmint transpose = value.m_type == GM_FLOAT
        ? (gmint)value.m_value.m_float : value.m_value.m_int;
    self->Set(a_thread->GetMachine(), "transposeValue", gmVariable(transpose));
    return GM_OK;
}
int GM_CDECL gmf_getTranspose(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    const gmVariable value = self->Get(a_thread->GetMachine(), "transposeValue");
    const gmint transpose = value.m_type == GM_FLOAT
        ? (gmint)value.m_value.m_float :
          value.m_type == GM_INT ? value.m_value.m_int : 0;
    a_thread->PushInt(transpose);
    return GM_OK;
}
// getScaleIndex() -- native fallback, same guarded shape as setScaleIndex
// above (only fires if the process's own table doesn't already define a
// real one -- ScaleOfTheWorld.gm:247 DOES define its own real
// `getScaleIndex=function(){return this.currentScaleIndex;}`, so this
// should be redundant in principle, but strict survey caught a genuine
// live "unbound: getScaleIndex" (Main.gm:10938: `this.origScaleIndex=
// this.worldScaleProcess.getScaleIndex();`, part of the player-death
// music-rewind flow) -- meaning some real call path reaches this method on
// a table where the script's own version wasn't found. Cheap, safe,
// guarded insurance reading the exact same `currentScaleIndex` field
// either version would.
int GM_CDECL gmf_getScaleIndex(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) { a_thread->PushInt(0); return GM_OK; }
    gmVariable v = self->Get(a_thread->GetMachine(), "currentScaleIndex");
    a_thread->Push((v.m_type == GM_INT || v.m_type == GM_FLOAT) ? v : gmVariable((gmint)0));
    return GM_OK;
}

int GM_CDECL gmf_nextScale(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    gmVariable scalesVar = self->Get(machine, "scaleList");
    gmVariable indexVar = self->Get(machine, "currentScaleIndex");
    if (scalesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    auto* scales = static_cast<gmTableObject*>(machine->GetObject(scalesVar.m_value.m_ref));
    const int count = scales->Count();
    if (count <= 0) return GM_EXCEPTION;
    const int index = indexVar.m_type == GM_INT ? indexVar.m_value.m_int : 0;
    self->Set(machine, "currentScaleIndex", gmVariable((gmint)((index + 1) % count)));
    return GM_OK;
}

int GM_CDECL gmf_lastScale(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    gmVariable scalesVar = self->Get(machine, "scaleList");
    gmVariable indexVar = self->Get(machine, "currentScaleIndex");
    if (scalesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    auto* scales = static_cast<gmTableObject*>(machine->GetObject(scalesVar.m_value.m_ref));
    const int count = scales->Count();
    if (count <= 0) return GM_EXCEPTION;
    const int index = indexVar.m_type == GM_INT ? indexVar.m_value.m_int : 0;
    self->Set(machine, "currentScaleIndex",
        gmVariable((gmint)(index <= 0 ? count - 1 : index - 1)));
    return GM_OK;
}

int GM_CDECL gmf_getCurrentScaleDescription(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    gmMachine* machine = a_thread->GetMachine();
    gmVariable scalesVar = self->Get(machine, "scaleList");
    gmVariable indexVar = self->Get(machine, "currentScaleIndex");
    if (scalesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    auto* scales = static_cast<gmTableObject*>(machine->GetObject(scalesVar.m_value.m_ref));
    const int index = indexVar.m_type == GM_INT ? indexVar.m_value.m_int : 0;
    gmVariable scaleVar = scales->Get((gmint)index);
    if (scaleVar.m_type != GM_TABLE) return GM_EXCEPTION;
    auto* scale = static_cast<gmTableObject*>(machine->GetObject(scaleVar.m_value.m_ref));
    gmVariable description = scale->Get(machine, "description");
    if (description.m_type != GM_STRING) return GM_EXCEPTION;
    a_thread->Push(description);
    return GM_OK;
}

int GM_CDECL gmf_getNoteWithPositionAndOffset(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1 || a_thread->GetNumParams() > 2) return GM_EXCEPTION;
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* self = a_thread->ThisTable();
    if (!self) return GM_EXCEPTION;
    float x, y;
    if (!GetVector2D(machine, a_thread->Param(0), x, y)) return GM_EXCEPTION;
    const int offset = a_thread->GetNumParams() == 2 ? a_thread->ParamInt(1, 0) : 0;

    gmVariable scalesVar = self->Get(&machine, "scaleList");
    if (scalesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* scales = (gmTableObject*)machine.GetObject(scalesVar.m_value.m_ref);
    auto readInt = [&](gmTableObject* table, const char* key) {
        gmVariable v = table->Get(&machine, key);
        return v.m_type == GM_FLOAT ? (int)v.m_value.m_float
            : v.m_type == GM_INT ? (int)v.m_value.m_int : 0;
    };
    const int scaleIndex = readInt(self, "currentScaleIndex");
    gmVariable scaleVar = scales->Get(scaleIndex);
    if (scaleVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* scale = (gmTableObject*)machine.GetObject(scaleVar.m_value.m_ref);
    gmVariable notesVar = scale->Get(&machine, "noteList");
    if (notesVar.m_type != GM_TABLE) return GM_EXCEPTION;
    gmTableObject* notes = (gmTableObject*)machine.GetObject(notesVar.m_value.m_ref);
    int noteCount = 0;
    while (noteCount < 128 && notes->Get(noteCount).m_type != GM_NULL) ++noteCount;
    if (noteCount == 0 || noteCount == 128) return GM_EXCEPTION;

    constexpr float kCycle = 21.96000099182129f;
    constexpr float kStep = 2.440000057220459f;
    float phase = std::fmod(y, kCycle);
    if (phase < 0.0f) phase += kCycle;
    int absoluteIndex = (int)(phase / kStep) + offset - 1;
    if (absoluteIndex < 0) absoluteIndex = 0;
    const int octaveQuotient = absoluteIndex / noteCount;
    const int baseIndex = absoluteIndex % noteCount;
    const int mode = readInt(self, "modeIndex");
    int index, octaveShift;
    if (mode >= 0) {
        index = (baseIndex + mode % noteCount) % noteCount;
        octaveShift = 2 * (octaveQuotient + mode / noteCount);
    } else {
        const int positiveMode = -mode;
        index = (baseIndex + noteCount - positiveMode % noteCount) % noteCount;
        octaveShift = -(2 * (octaveQuotient + positiveMode / noteCount) + 12);
    }
    gmVariable noteVar = notes->Get(index);
    if (noteVar.m_type != GM_INT && noteVar.m_type != GM_FLOAT) return GM_EXCEPTION;
    const int baseNote = noteVar.m_type == GM_INT ? noteVar.m_value.m_int : (int)noteVar.m_value.m_float;
    const int transpose = readInt(self, "transposeValue");
    const int note = std::max(0, std::min(95, baseNote + transpose + octaveShift));
    a_thread->PushInt(note);
    return GM_OK;
}

int GM_CDECL gmf_getTable(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    a_thread->Push(proc->table);
    return GM_OK;
}

int GM_CDECL gmf_getScreenWidth(gmThread* a_thread) {
    a_thread->PushFloat(16.0f * 0.61f * 2.0f * 2.0f);
    return GM_OK;
}
int GM_CDECL gmf_getScreenHeight(gmThread* a_thread) {
    a_thread->PushFloat(9.0f * 0.61f * 2.0f * 2.0f);
    return GM_OK;
}

int GM_CDECL gmf_getScreenX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(x, 0, 0.0f);
    const float screenWidth = 16.0f * 0.61f * 2.0f * 2.0f;
    int sx = (int)(x / screenWidth);
    if (x < 0.0f) sx -= 1;
    a_thread->PushInt(sx);
    return GM_OK;
}
int GM_CDECL gmf_getScreenY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(y, 0, 0.0f);
    const float screenHeight = 9.0f * 0.61f * 2.0f * 2.0f;
    int sy = (int)(y / screenHeight);
    if (y < 0.0f) sy -= 1;
    a_thread->PushInt(sy);
    return GM_OK;
}

int GM_CDECL gmf_getCurrentScreenX(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "currentScreenX");
    a_thread->Push((v.m_type == GM_INT || v.m_type == GM_FLOAT) ? v : gmVariable((gmint)0));
    return GM_OK;
}
int GM_CDECL gmf_getCurrentScreenY(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    gmVariable v = self->Get(a_thread->GetMachine(), "currentScreenY");
    a_thread->Push((v.m_type == GM_INT || v.m_type == GM_FLOAT) ? v : gmVariable((gmint)0));
    return GM_OK;
}

// setCurrentScreenX/setCurrentScreenY -- the setters matching the two
// getters above, and the same "dead script code in a /* C++ */ comment,
// moved native" family as getScreenWidth/getScreenX (Platformer/Main.gm's
// own commented reference stores straight into this.currentScreenX).
//
// Real call site is Main.gm's per-frame screen-change handler, immediately
// after the getScreenX/getScreenY pair that computes the new indices:
//
//     this.setCurrentScreenX(cx);
//     this.setCurrentScreenY(cy);
//     this.destroyBullets();
//
// so leaving these unbound meant the game recomputed which screen the
// player had moved to and then never recorded it -- getCurrentScreenX/Y
// kept reporting the old screen forever, and every "did we change screens?"
// comparison downstream stayed permanently true.
//
// Stored on the process's own script table under the same key the getters
// read, so the two sides cannot drift apart.
int GM_CDECL gmf_setCurrentScreenX(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    self->Set(a_thread->GetMachine(), "currentScreenX", a_thread->Param(0));
    return GM_OK;
}
int GM_CDECL gmf_setCurrentScreenY(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    gmTableObject* self = ThisHandleTable(a_thread);
    if (!self) return GM_EXCEPTION;
    self->Set(a_thread->GetMachine(), "currentScreenY", a_thread->Param(0));
    return GM_OK;
}

int GM_CDECL gmf_setGlobalVolume(gmThread* a_thread) {
    gmTableObject* self = ThisHandleTable(a_thread);
    Process* proc = self ? LookupProcess(a_thread, self) : nullptr;
    if (!proc) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1);
    GM_FLOAT_OR_INT_PARAM(vol, 0, 1.0f);
    SetMasterVolumeNative(vol);
    return GM_OK;
}

int GM_CDECL gmf_setAsPauseProcess(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(active, 0, 0);

    ProcessManager& pm = *GetEngineContext().processManager;
    int exemptId = -1;

    gmTableObject* self = ThisHandleTable(a_thread);
    Process* selfProc = self ? LookupProcess(a_thread, self) : nullptr;
    if (selfProc) {
        exemptId = selfProc->id;
    } else if (a_thread->GetNumParams() >= 2) {
        gmVariable procVar = a_thread->Param(1);
        if (procVar.m_type == GM_TABLE) {
            gmTableObject* procTable = (gmTableObject*)a_thread->GetMachine()->GetObject(procVar.m_value.m_ref);
            gmVariable idVar = procTable ? procTable->Get(a_thread->GetMachine(), "__id") : gmVariable(GM_NULL, 0);
            if (idVar.m_type == GM_INT) exemptId = idVar.m_value.m_int;
        }
    } else {
        for (int i = 0; i < pm.GetCount(); ++i) {
            Process* p = pm.GetByIndex(i);
            if (p && p->stealsInputFocus) { exemptId = p->id; break; }
        }
    }

    pm.QueuePauseState(active != 0, exemptId);
    return GM_OK;
}

} // namespace

bool GetShaderStorageId(gmMachine& a_machine, const gmVariable& a_value, int& a_id) {
    if (a_value.m_type < GM_USER) return false;
    gmUserObject* object = (gmUserObject*)a_machine.GetObject(a_value.m_value.m_ref);
    if (!object || object->m_userType != g_shaderType) return false;
    const ShaderHandleData* shader = (ShaderHandleData*)object->m_user;
    if (shader->isDefaultPostProcess) return false;
    // Process IDs are monotonic within the desktop session; the negative
    // range is reserved for process-owned shader programs.
    a_id = -1 - shader->processId * 256 - shader->shaderId;
    return true;
}

gmVariable CreateDefaultPostProcessShaderHandle(gmMachine& a_machine) {
    // The Video object's +0x8150 shader pointer is initialized to null by
    // retail. Represent the distinct non-null SHADER wrapper while keeping
    // that underlying shader absent; don't give it a process shader identity.
    return gmVariable(a_machine.AllocUserObject(
        new ShaderHandleData{-1, -1, true}, g_shaderType));
}

void InjectProcessHandleMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId) {
    a_table->Set(&a_machine, "__id", gmVariable((gmint)a_processId));
    // Process behavior tables are authored GameMonkey tables. Native process
    // helpers fill methods that the behavior does not define, but must not
    // replace an authored method with the same name. Main.gm and
    // GroundProcessNEW.gm both define entity registration methods; replacing
    // those with the generic ground-process no-op silently leaves active
    // screens without entities, which also strands the player's first-spawn
    // state behind a disabled SpawnPoint.
    auto setNativeIfMissing = [&](const char* name, gmCFunction function) {
        if (a_table->Get(&a_machine, name).m_type != GM_FUNCTION) {
            a_table->Set(&a_machine, name,
                gmVariable(a_machine.AllocFunctionObject(function)));
        }
    };
    Process* shaderOwner = GetEngineContext().processManager->Get(a_processId);
    if (shaderOwner && shaderOwner->name == "Platformer_Main") {
        a_table->Set(&a_machine, "SOUNDPREVIEW_SHADERID", gmVariable((gmint)0));
        setNativeIfMissing("getShader", gmf_process_getShader);
    }
    setNativeIfMissing("refreshFunctionCache",
        gmf_process_refreshFunctionCache);
    if (a_table->Get(&a_machine, "onMusicTrigger").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "onMusicTrigger",
            gmVariable(a_machine.AllocFunctionObject(gmf_process_onMusicTriggerDefault)));
    }
    setNativeIfMissing("setIsStealsInputFocus", gmf_setIsStealsInputFocus);
    setNativeIfMissing("terminate", gmf_terminate);
    setNativeIfMissing("callMethod", gmf_process_callMethod);
    setNativeIfMissing("setIsUpdatesEntities", gmf_setIsUpdatesEntities);
    setNativeIfMissing("drawGrounds", gmf_process_drawGrounds);
    setNativeIfMissing("registerEntity", gmf_process_registerGround);
    setNativeIfMissing("unregisterEntity", gmf_process_unregisterGround);
    setNativeIfMissing("getGroundCountOnScreen", gmf_process_getGroundCountOnScreen);
    if (a_table->Get(&a_machine, "isLoading").m_type != GM_FUNCTION) {
        a_table->Set(&a_machine, "isLoading", gmVariable(a_machine.AllocFunctionObject(gmf_process_isLoading)));
    }
    setNativeIfMissing("setFireThickness", gmf_process_setFireThickness);
    setNativeIfMissing("getFireThickness", gmf_process_getFireThickness);
    setNativeIfMissing("setIsRefreshDrawListOnChange",
        gmf_process_setRefreshDrawListsOnChange);
    setNativeIfMissing("setupDrawLists", gmf_process_setupDrawLists);
    // Guarded: never stomp ScaleOfTheWorld.gm's own real setScaleIndex when
    // dual-injected onto its returned table -- see gmf_setScaleIndex's
    // header comment above.
    if (a_table->Get(&a_machine, "setScaleIndex").m_type != GM_FUNCTION) {
        setNativeIfMissing("setScaleIndex", gmf_setScaleIndex);
    }
    if (a_table->Get(&a_machine, "getScaleIndex").m_type != GM_FUNCTION) {
        setNativeIfMissing("getScaleIndex", gmf_getScaleIndex);
    }
    setNativeIfMissing("nextScale", gmf_nextScale);
    setNativeIfMissing("lastScale", gmf_lastScale);
    setNativeIfMissing("getCurrentScaleDescription", gmf_getCurrentScaleDescription);
    if (a_table->Get(&a_machine, "getNoteWithPositionAndOffset").m_type != GM_FUNCTION) {
        setNativeIfMissing("getNoteWithPositionAndOffset",
            gmf_getNoteWithPositionAndOffset);
    }
    if (a_table->Get(&a_machine, "setTranspose").m_type != GM_FUNCTION) {
        setNativeIfMissing("setTranspose", gmf_setTranspose);
    }
    if (a_table->Get(&a_machine, "getTranspose").m_type != GM_FUNCTION) {
        setNativeIfMissing("getTranspose", gmf_getTranspose);
    }
    setNativeIfMissing("isInited", gmf_isInited);
    setNativeIfMissing("setupPreDrawList", gmf_setupPreDrawList);
    setNativeIfMissing("setupPostDrawList", gmf_setupPostDrawList);
    setNativeIfMissing("setupDrawList", gmf_setupPreDrawList);
    setNativeIfMissing("getPreDrawList", gmf_getPreDrawList);
    setNativeIfMissing("getPostDrawList", gmf_getPostDrawList);
    setNativeIfMissing("getDrawList", gmf_getDrawList);
    setNativeIfMissing("getObjectID", gmf_getObjectID);
    setNativeIfMissing("getTable", gmf_getTable);
    setNativeIfMissing("isHasErrors", gmf_process_isHasErrors);
    setNativeIfMissing("getName", gmf_process_getName);
    setNativeIfMissing("rename", gmf_process_rename);
    setNativeIfMissing("getScreenWidth", gmf_getScreenWidth);
    setNativeIfMissing("getScreenHeight", gmf_getScreenHeight);
    setNativeIfMissing("getScreenX", gmf_getScreenX);
    setNativeIfMissing("getScreenY", gmf_getScreenY);
    setNativeIfMissing("setCurrentScreenX", gmf_setCurrentScreenX);
    setNativeIfMissing("setCurrentScreenY", gmf_setCurrentScreenY);
    setNativeIfMissing("getCurrentScreenX", gmf_getCurrentScreenX);
    setNativeIfMissing("getCurrentScreenY", gmf_getCurrentScreenY);
    setNativeIfMissing("setGlobalVolume", gmf_setGlobalVolume);
    setNativeIfMissing("setAsPauseProcess", gmf_setAsPauseProcess);
    setNativeIfMissing("isBeingEdited", gmf_process_isBeingEdited);

    setNativeIfMissing("getUniqueAlarmID", gmf_getUniqueAlarmID);
    setNativeIfMissing("addAlarm", gmf_addAlarm);
    setNativeIfMissing("deleteAlarm", gmf_deleteAlarm);
    setNativeIfMissing("getAlarm", gmf_getAlarm);
    setNativeIfMissing("getUniqueLFOID", gmf_getUniqueLFOID);
    setNativeIfMissing("addLFO", gmf_addLFO);
    setNativeIfMissing("deleteLFO", gmf_deleteLFO);
}

void RegisterProcessHandleBindings(gmMachine& a_machine) {
    g_shaderType = a_machine.CreateUserType("SHADER");
    a_machine.RegisterTypeLibrary(g_shaderType, s_shaderLib,
        sizeof(s_shaderLib) / sizeof(s_shaderLib[0]));
    a_machine.RegisterUserCallbacks(g_shaderType, nullptr, ShaderGCDestruct, nullptr);
    // GameSystem.setAsPauseProcess(active, process) -- see gmf_setAsPauseProcess's
    // header comment. GameSystem itself is registered elsewhere
    // (gamesystem_bindings.cpp); appended to the existing table here
    // (RegisterLibraryFunction's a_newTable defaults false, so this reuses
    // the table RegisterGameSystemBindings already created).
    a_machine.RegisterLibraryFunction("setAsPauseProcess", gmf_setAsPauseProcess, "GameSystem");
}

gmVariable CreateProcessHandle(gmMachine& a_machine, int a_processId) {
    gmTableObject* table = a_machine.AllocTableObject();
    InjectProcessHandleMethods(a_machine, table, a_processId);
    // ENGINE PATCH: this handle is entirely C++-built (unlike a process's own
    // returned behavior table, which is script-authored and must stay
    // unmarked -- see strict_calls.h's MarkNative), so a missing method here
    // is unambiguously an engine gap.
    StrictCalls::MarkScriptExtensibleNative(a_machine, table);
    return gmVariable(table);
}
