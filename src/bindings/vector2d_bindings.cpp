#include "gmThread.h"
#include "vector2d_bindings.h"
#include "gmStringObject.h"
#include "../core/process_manager.h"
#include <cmath>
#include <random>

gmType g_vector2DType = -1;

namespace {

struct Vector2DData {
    float x = 0.0f;
    float y = 0.0f;
    gmMachine* machine = nullptr;
    gmVariable owner = gmVariable::s_null;
    bool ownsOwnerRoot = false;
    void* refContext = nullptr;
    Vector2DRefRead refRead = nullptr;
    Vector2DRefWrite refWrite = nullptr;
};

void Refresh(Vector2DData* v) {
    if (v && v->refRead) v->refRead(v->refContext, v->x, v->y);
}

Vector2DData* ThisVec2(gmThread* a_thread) {
    Vector2DData* v = (Vector2DData*)a_thread->ThisUserCheckType(g_vector2DType);
    Refresh(v);
    return v;
}
Vector2DData* AsVec2(gmMachine& a_machine, const gmVariable& a_var) {
    Vector2DData* v = (Vector2DData*)((gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref))->m_user;
    Refresh(v);
    return v;
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
int GM_CDECL gmf_getRightNormal(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    a_thread->Push(MakeVector2D(*a_thread->GetMachine(), v->y, -v->x));
    return GM_OK;
}
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
int GM_CDECL gmf_isPrettyMuchEqual(gmThread* a_thread) {
    Vector2DData* v = ThisVec2(a_thread); if (!v) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(1); GM_CHECK_USER_PARAM(Vector2DData*, g_vector2DType, other, 0);
    const float kEpsilon = 0.03125f;
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
int GM_CDECL gmf_normalize(gmThread* a_thread) {
    GM_EXCEPTION_MSG("normalize: unavailable");
    return GM_EXCEPTION;
}

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
    bool changed = false;
    if (member->GetLength() == 1 && s[0] == 'x') { v->x = newVal; changed = true; }
    else if (member->GetLength() == 1 && s[0] == 'y') { v->y = newVal; changed = true; }
    if (changed && v->refWrite) v->refWrite(v->refContext, v->x, v->y);
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
    Vector2DData* v = (Vector2DData*)a_object->m_user;
    if (v->machine && v->ownsOwnerRoot) UnrootGmVariable(*v->machine, v->owner);
    (void)a_machine;
    delete v;
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

gmVariable MakeVector2DRef(gmMachine& a_machine, const gmVariable& a_owner,
                           void* a_context, Vector2DRefRead a_read,
                           Vector2DRefWrite a_write) {
    Vector2DData* data = new Vector2DData();
    data->machine = &a_machine;
    data->owner = a_owner;
    data->ownsOwnerRoot = a_owner.IsReference() &&
        !a_machine.IsCPPOwnedGMObject(a_machine.GetObject(a_owner.m_value.m_ref));
    data->refContext = a_context;
    data->refRead = a_read;
    data->refWrite = a_write;
    if (data->ownsOwnerRoot) RootGmVariable(a_machine, data->owner);
    Refresh(data);
    gmUserObject* obj = a_machine.AllocUserObject(data, g_vector2DType);
    return gmVariable(obj);
}

bool GetVector2D(gmMachine& a_machine, const gmVariable& a_var, float& a_outX, float& a_outY) {
    if (a_var.m_type != g_vector2DType) return false;
    Vector2DData* v = (Vector2DData*)((gmUserObject*)a_machine.GetObject(a_var.m_value.m_ref))->m_user;
    Refresh(v);
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
