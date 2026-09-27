#include "gmThread.h"
#include "vector2d_bindings.h"
#include "gmStringObject.h"
#include <cmath>
#include <random>

gmType g_vector2DType = -1;

namespace {

struct Vector2DData {
    float x = 0.0f;
    float y = 0.0f;
};

Vector2DData* ThisVec2(gmThread* a_thread) {
    return (Vector2DData*)a_thread->ThisUserCheckType(g_vector2DType);
}
Vector2DData* AsVec2(gmMachine& a_machine, const gmVariable& a_var) {
    return (Vector2DData*)((gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref))->m_user;
}

float Length(const Vector2DData& v) { return std::sqrt(v.x * v.x + v.y * v.y); }


int GM_CDECL gmf_length(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    a_thread->PushFloat(Length(*v));
    return GM_OK;
}
int GM_CDECL gmf_lengthSquared(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    a_thread->PushFloat(v->x * v->x + v->y * v->y);
    return GM_OK;
}
int GM_CDECL gmf_isZeroVector(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    a_thread->PushInt((v->x == 0.0f && v->y == 0.0f) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_getNormalized(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    float len = Length(*v);
    float nx = len > 0.0f ? v->x / len : 0.0f;
    float ny = len > 0.0f ? v->y / len : 0.0f;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), nx, ny));
    return GM_OK;
}
// Confirmed real: swaps components and negates the new y (rotate -90deg).
// Was pushing (v.x, -v.y) -- a Y-axis mirror, not a rotation at all, and not
// what the comment above (already present before this fix) describes. For
// flat ground (normal (0,1)) this produced (0,-1) -- straight back down into
// the ground -- instead of (1,0), so every left/right walk-direction
// comparison against a contact normal (checkHangTime's core mechanism,
// Player.gm) was comparing against a nonsense vector, making the walk-
// direction pick effectively random/unstable frame to frame instead of
// tracking input direction. Real rotate-by--90deg is (y,-x), matching
// getLeftNormal's own (correctly implemented) +90deg sibling below.
int GM_CDECL gmf_getRightNormal(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), v->y, -v->x));
    return GM_OK;
}
// Confirmed real: swaps components and negates the new x (rotate +90deg).
int GM_CDECL gmf_getLeftNormal(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), -v->y, v->x));
    return GM_OK;
}
int GM_CDECL gmf_getVectorToPoint(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, other, 0);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), other->x - v->x, other->y - v->y));
    return GM_OK;
}
int GM_CDECL gmf_getVectorToPointNormalized(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, other, 0);
    float dx = other->x - v->x, dy = other->y - v->y;
    float len = std::sqrt(dx * dx + dy * dy);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), len > 0.0f ? dx / len : 0.0f, len > 0.0f ? dy / len : 0.0f));
    return GM_OK;
}
int GM_CDECL gmf_distance(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, other, 0);
    float dx = other->x - v->x, dy = other->y - v->y;
    a_thread->PushFloat(std::sqrt(dx * dx + dy * dy));
    return GM_OK;
}
// FIXED (this session): epsilon was a guessed 0.0001f. Confirmed real
// `DAT_000e56f4 / (DAT_000e56f0 ^ 5)` via an inlined fast-exponentiation
// loop, where DAT_000e56f4=1.0f and DAT_000e56f0=2.0f were read directly
// out of the retail EBOOT.elf -- i.e. epsilon = 1.0/(2.0^5) = 1/32 =
// 0.03125 exactly, compared independently against |dx| and |dy| (not a
// combined distance).
int GM_CDECL gmf_isPrettyMuchEqual(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, other, 0);
    const float kEpsilon = 0.03125f; // 1.0f / (2.0f^5), confirmed via EBOOT.elf constant extraction
    a_thread->PushInt((std::fabs(v->x - other->x) <= kEpsilon && std::fabs(v->y - other->y) <= kEpsilon) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_scalarProjectOnto(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, onto, 0);
    float len = Length(*onto);
    float nx = len > 0.0f ? onto->x / len : 0.0f;
    float ny = len > 0.0f ? onto->y / len : 0.0f;
    a_thread->PushFloat(v->x * nx + v->y * ny);
    return GM_OK;
}
int GM_CDECL gmf_vectorProjectOnto(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, onto, 0);
    float len = Length(*onto);
    float nx = len > 0.0f ? onto->x / len : 0.0f;
    float ny = len > 0.0f ? onto->y / len : 0.0f;
    float scalar = v->x * nx + v->y * ny;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), nx * scalar, ny * scalar));
    return GM_OK;
}
// no `this` pointer read, matching the `vector2D(...)` constructor's own
// registration style below): a uniform-random unit vector. Retail calls
// its random-real helper with bounds (0.0, 6.28) -- not 2*pi (6.283185...)
// -- then takes (cos, sin) of the result, both constants read directly out
// of the EBOOT.elf. Reproduced bug-for-bug with the same slightly-truncated
// upper bound rather than a precise 2*pi. Not registered until now (no
// corpus call sites found either, same class as RANDOM -- see Random.c).
int GM_CDECL gmf_MakeRandomDir(gmThread* a_thread) {
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<float> dist(0.0f, 6.28f);
    float ang = dist(rng);
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), std::cos(ang), std::sin(ang)));
    return GM_OK;
}

int GM_CDECL gmf_isvector2D(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    a_thread->PushInt(a_thread->Param(0).m_type == g_vector2DType ? 1 : 0);
    return GM_OK;
}
// Confirmed real decompiled body always returns the GM exception sentinel
// -- not modeled (no real corpus call sites either, see vector2d_bindings.h).
int GM_CDECL gmf_normalize(gmThread* a_thread) {
    GM_EXCEPTION_MSG("normalize: not a real callable method (matches confirmed decompiled body)");
    return GM_EXCEPTION;
}

// --- vector2D(...) constructor -- confirmed real 0/1/2-arg overload set,
// (confirmed: tag-3 branch copies fields directly with no transform), or
// treats a plain number as an angle in radians and builds a unit vector
// (cos, sin) -- the trig call itself (FUN_001dd1ac) wasn't independently
// decompiled, but this is the only interpretation consistent with real
// usage (GroundProcessNEW.gm: `t.pos+vector2D(ang)*t.radius` for placing
// points around a circle; Main.gm: `this.pos+=vector2D(this.dirAng)*this.speed*msElapsed`
// for directional movement).
int GM_CDECL gmf_vector2D(gmThread* a_thread) {
    int n = a_thread->GetNumParams();
    if (n > 2) { GM_EXCEPTION_MSG("vector2D: expecting 0-2 params"); return GM_EXCEPTION; }
    float x = 0.0f, y = 0.0f;
    if (n == 1) {
        gmVariable p0 = a_thread->Param(0);
        if (p0.m_type == g_vector2DType) {
            Vector2DData* src = AsVec2(*a_thread->GetMachine(), p0);
            x = src->x; y = src->y;
        } else if (p0.m_type == GM_FLOAT || p0.m_type == GM_INT) {
            float angle = a_thread->ParamFloatOrInt(0, 0.0f);
            x = std::cos(angle); y = std::sin(angle);
        } else {
            GM_EXCEPTION_MSG("vector2D: expecting param 0 as vector2D, float, or int");
            return GM_EXCEPTION;
        }
    } else if (n == 2) {
        x = a_thread->ParamFloatOrInt(0, 0.0f);
        y = a_thread->ParamFloatOrInt(1, 0.0f);
    }
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), x, y));
    return GM_OK;
}

// --- operators (RegisterTypeOperator -- same mechanism GameMonkeyScript's
// own vendored Vector3 sample uses, see vector2d_bindings.h) ---

void GM_CDECL OpGetDot(gmThread* a_thread, gmVariable* a_operands) {
    (void)a_thread;
    Vector2DData* v = AsVec2(*a_thread->GetMachine(), a_operands[0]);
    gmStringObject* member = (gmStringObject*)a_thread->GetMachine()->GetObject(a_operands[1].m_value.m_ref);
    const char* s = member->GetString();
    if (member->GetLength() == 1 && s[0] == 'x') a_operands[0].SetFloat(v->x);
    else if (member->GetLength() == 1 && s[0] == 'y') a_operands[0].SetFloat(v->y);
    else a_operands[0].Nullify();
}
void GM_CDECL OpSetDot(gmThread* a_thread, gmVariable* a_operands) {
    Vector2DData* v = AsVec2(*a_thread->GetMachine(), a_operands[0]);
    gmStringObject* member = (gmStringObject*)a_thread->GetMachine()->GetObject(a_operands[2].m_value.m_ref);
    const char* s = member->GetString();
    float newVal = (a_operands[1].m_type == GM_FLOAT) ? a_operands[1].m_value.m_float
                 : (a_operands[1].m_type == GM_INT) ? (float)a_operands[1].m_value.m_int : 0.0f;
    if (member->GetLength() == 1 && s[0] == 'x') v->x = newVal;
    else if (member->GetLength() == 1 && s[0] == 'y') v->y = newVal;
}
bool GetOperandAsVec(gmMachine& a_machine, const gmVariable& a_var, Vector2DData& a_out) {
    if (a_var.m_type == g_vector2DType) { a_out = *AsVec2(a_machine, a_var); return true; }
    return false;
}
bool GetOperandAsScalar(const gmVariable& a_var, float& a_out) {
    if (a_var.m_type == GM_FLOAT) { a_out = a_var.m_value.m_float; return true; }
    if (a_var.m_type == GM_INT) { a_out = (float)a_var.m_value.m_int; return true; }
    return false;
}
// ENGINE PATCH (2026-08-16): vector2D+scalar/scalar+vector2D broadcast
// (scalar added to both components) -- confirmed real via Player.gm:3548's
// `vector2D(jumpNormal+GameSystem.getRandomReal(d,d))` (jumpAction's jump-
// particle spawn, GameSystem.getRandomReal returns a plain float). Only
// vector+vector was previously supported, so this nullified to GM_NULL and
// the enclosing vector2D(null) constructor threw "expecting param 0 as
// vector2D, float, or int", aborting jumpAction() before it could call
// finalizeJump()/finishStickyJump() -- which is what transitions the player
// OUT of HANG_STATEID. So the jump velocity set a few lines earlier
// (pb.setVel(v)) never got to stick: still-active HANG_STATEID per-frame
// ground-anchoring logic immediately re-pinned the player back down within
// a frame or two, making jump look like a no-op even though state said
// "grounded" and a real jump velocity had briefly been applied.
void GM_CDECL OpAdd(gmThread* a_thread, gmVariable* a_operands) {
    Vector2DData a, b; float s;
    if (GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a) && GetOperandAsVec(*a_thread->GetMachine(), a_operands[1], b)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x + b.x, a.y + b.y);
    } else if (GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a) && GetOperandAsScalar(a_operands[1], s)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x + s, a.y + s);
    } else if (GetOperandAsScalar(a_operands[0], s) && GetOperandAsVec(*a_thread->GetMachine(), a_operands[1], a)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x + s, a.y + s);
    } else {
        a_operands[0].Nullify();
    }
}
void GM_CDECL OpSub(gmThread* a_thread, gmVariable* a_operands) {
    Vector2DData a, b; float s;
    if (GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a) && GetOperandAsVec(*a_thread->GetMachine(), a_operands[1], b)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x - b.x, a.y - b.y);
    } else if (GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a) && GetOperandAsScalar(a_operands[1], s)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x - s, a.y - s);
    } else {
        a_operands[0].Nullify();
    }
}
// Confirmed real usage is always vector*scalar or scalar*vector (never
// vector*vector componentwise -- no corpus call sites for that shape).
void GM_CDECL OpMul(gmThread* a_thread, gmVariable* a_operands) {
    Vector2DData a; float s;
    if (GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a) && GetOperandAsScalar(a_operands[1], s)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x * s, a.y * s);
    } else if (GetOperandAsScalar(a_operands[0], s) && GetOperandAsVec(*a_thread->GetMachine(), a_operands[1], a)) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x * s, a.y * s);
    } else {
        a_operands[0].Nullify();
    }
}
// Confirmed real usage: vector/scalar only (Platformer/GroundProcessNEW.gm:
// `p1.getVectorToPoint(p2)/MATH_REAL(q-1)`).
void GM_CDECL OpDiv(gmThread* a_thread, gmVariable* a_operands) {
    Vector2DData a; float s;
    if (GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a) && GetOperandAsScalar(a_operands[1], s) && s != 0.0f) {
        a_operands[0] = MakeVector2D(*a_thread->GetMachine(), a.x / s, a.y / s);
    } else {
        a_operands[0].Nullify();
    }
}
void GM_CDECL OpNeg(gmThread* a_thread, gmVariable* a_operands) {
    Vector2DData a;
    if (!GetOperandAsVec(*a_thread->GetMachine(), a_operands[0], a)) { a_operands[0].Nullify(); return; }
    a_operands[0] = MakeVector2D(*a_thread->GetMachine(), -a.x, -a.y);
}

gmFunctionEntry s_vector2DLib[] = {
    {"length", gmf_length, nullptr},
    {"lengthSquared", gmf_lengthSquared, nullptr},
    {"isZeroVector", gmf_isZeroVector, nullptr},
    {"getNormalized", gmf_getNormalized, nullptr},
    {"getRightNormal", gmf_getRightNormal, nullptr},
    {"getLeftNormal", gmf_getLeftNormal, nullptr},
    {"getVectorToPoint", gmf_getVectorToPoint, nullptr},
    {"getVectorToPointNormalized", gmf_getVectorToPointNormalized, nullptr},
    {"distance", gmf_distance, nullptr},
    {"isPrettyMuchEqual", gmf_isPrettyMuchEqual, nullptr},
    {"scalarProjectOnto", gmf_scalarProjectOnto, nullptr},
    {"vectorProjectOnto", gmf_vectorProjectOnto, nullptr},
    {"isvector2D", gmf_isvector2D, nullptr},
    {"normalize", gmf_normalize, nullptr},
};

void GM_CDECL Vector2DGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    (void)a_machine;
    delete (Vector2DData*)a_object->m_user;
}
void GM_CDECL Vector2DAsString(gmUserObject* a_object, char* a_buffer, int a_bufferLen) {
    Vector2DData* v = (Vector2DData*)a_object->m_user;
    std::snprintf(a_buffer, a_bufferLen, "(%g, %g)", v->x, v->y);
}

} // namespace

gmVariable MakeVector2D(gmMachine& a_machine, float a_x, float a_y) {
    Vector2DData* data = new Vector2DData{a_x, a_y};
    gmUserObject* obj = a_machine.AllocUserObject(data, g_vector2DType);
    return gmVariable(obj);
}

bool GetVector2D(gmMachine& a_machine, const gmVariable& a_var, float& a_outX, float& a_outY) {
    if (a_var.m_type != g_vector2DType) return false;
    Vector2DData* v = (Vector2DData*)((gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref))->m_user;
    a_outX = v->x; a_outY = v->y;
    return true;
}

void RegisterVector2DBindings(gmMachine& a_machine) {
    g_vector2DType = a_machine.CreateUserType("vector2D");
    a_machine.RegisterTypeLibrary(g_vector2DType, s_vector2DLib, sizeof(s_vector2DLib) / sizeof(s_vector2DLib[0]));
    a_machine.RegisterUserCallbacks(g_vector2DType, nullptr, Vector2DGCDestruct, Vector2DAsString);

    a_machine.RegisterTypeOperator(g_vector2DType, O_GETDOT, nullptr, OpGetDot);
    a_machine.RegisterTypeOperator(g_vector2DType, O_SETDOT, nullptr, OpSetDot);
    a_machine.RegisterTypeOperator(g_vector2DType, O_ADD, nullptr, OpAdd);
    a_machine.RegisterTypeOperator(g_vector2DType, O_SUB, nullptr, OpSub);
    a_machine.RegisterTypeOperator(g_vector2DType, O_MUL, nullptr, OpMul);
    a_machine.RegisterTypeOperator(g_vector2DType, O_DIV, nullptr, OpDiv);
    a_machine.RegisterTypeOperator(g_vector2DType, O_NEG, nullptr, OpNeg);

    a_machine.RegisterLibraryFunction("vector2D", gmf_vector2D);
    a_machine.RegisterLibraryFunction("MakeRandomDir", gmf_MakeRandomDir);
}
