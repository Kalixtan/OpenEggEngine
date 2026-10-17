#include "matrix4_bindings.h"
#include "gmThread.h"
#include <cmath>

namespace {

gmType g_matrix4Type = -1;

Matrix4Handle* ThisMatrix4(gmThread* a_thread) {
    return (Matrix4Handle*)a_thread->ThisUserCheckType(g_matrix4Type);
}

int GM_CDECL gmf_MATRIX4(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushNewUser(new Matrix4Handle(
        std::make_shared<DrawContext::Mat>()), g_matrix4Type);
    return GM_OK;
}

int GM_CDECL gmf_test(gmThread* a_thread) {
    if (!ThisMatrix4(a_thread)) return GM_EXCEPTION;
    return GM_OK;
}

int GM_CDECL gmf_translate(gmThread* a_thread) {
    Matrix4Handle* handle = ThisMatrix4(a_thread);
    if (!handle) return GM_EXCEPTION;
    const float x = a_thread->ParamFloatOrInt(0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(1, 0.0f);
    const float z = a_thread->ParamFloatOrInt(2, 0.0f);
    DrawContext::Mat translation;
    translation.e = x;
    translation.f = y;
    translation.z = z;
    **handle = DC_Multiply(**handle, translation);
    return GM_OK;
}

int GM_CDECL gmf_rotateZ(gmThread* a_thread) {
    Matrix4Handle* handle = ThisMatrix4(a_thread);
    if (!handle) return GM_EXCEPTION;
    const float angle = a_thread->ParamFloatOrInt(0, 0.0f);
    const float c = std::cos(angle), s = std::sin(angle);
    DrawContext::Mat rotation;
    rotation.a = c;
    rotation.b = s;
    rotation.c = -s;
    rotation.d = c;
    **handle = DC_Multiply(**handle, rotation);
    return GM_OK;
}

int GM_CDECL gmf_scale(gmThread* a_thread) {
    Matrix4Handle* handle = ThisMatrix4(a_thread);
    if (!handle) return GM_EXCEPTION;
    const float x = a_thread->ParamFloatOrInt(0, 0.0f);
    const float y = a_thread->ParamFloatOrInt(1, 0.0f);
    DrawContext::Mat scale;
    scale.a = x;
    scale.d = y;
    **handle = DC_Multiply(**handle, scale);
    return GM_OK;
}

int GM_CDECL gmf_reset(gmThread* a_thread) {
    Matrix4Handle* handle = ThisMatrix4(a_thread);
    if (!handle) return GM_EXCEPTION;
    GM_CHECK_NUM_PARAMS(0);
    **handle = DrawContext::Mat{};
    return GM_OK;
}

gmFunctionEntry s_matrix4Lib[] = {
    {"test", gmf_test, nullptr},
    {"translate", gmf_translate, nullptr},
    {"rotateZ", gmf_rotateZ, nullptr},
    {"scale", gmf_scale, nullptr},
    {"reset", gmf_reset, nullptr},
};

void GM_CDECL Matrix4GCDestruct(gmMachine*, gmUserObject* a_object) {
    delete (Matrix4Handle*)a_object->m_user;
}

} // namespace

Matrix4Handle GetMatrix4Handle(gmMachine& a_machine,
                               const gmVariable& a_value) {
    if (a_value.m_type != g_matrix4Type) return {};
    gmObject* object = a_machine.GetObject(a_value.m_value.m_ref);
    if (!object) return {};
    auto* handle = (Matrix4Handle*)((gmUserObject*)object)->m_user;
    return handle ? *handle : Matrix4Handle{};
}

void RegisterMatrix4Bindings(gmMachine& a_machine) {
    g_matrix4Type = a_machine.CreateUserType("MATRIX4");
    a_machine.RegisterTypeLibrary(g_matrix4Type, s_matrix4Lib,
        sizeof(s_matrix4Lib) / sizeof(s_matrix4Lib[0]));
    a_machine.RegisterUserCallbacks(g_matrix4Type, nullptr,
                                    Matrix4GCDestruct, nullptr);
    a_machine.RegisterLibraryFunction("MATRIX4", gmf_MATRIX4);
}
