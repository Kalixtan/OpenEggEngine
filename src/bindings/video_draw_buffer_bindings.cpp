#include "video_draw_buffer_bindings.h"

#include "../core/strict_calls.h"
#include "gmUserObject.h"

#include <algorithm>
#include <array>
#include <memory>
#include <vector>

namespace {

gmType g_videoDrawBufferType = -1;

struct DrawBufferShape {
    std::vector<unsigned char> vertices;
    int vertexCapacity = 0;
    int vertexCount = 0;
    unsigned int vertexFlags = 0;
    // Retail vertex records begin with colour -1; the remaining five words
    // are zeroed by FUN_00053870 and restored to these values by reset.
    unsigned int defaultColour = 0xffffffffu;
    int mode = 0;
    int matrixShape = -1;
};

struct DrawBufferMatrix {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
};

struct VideoDrawBufferData {
    // Retail FUN_00053758 allocates one initial 0x70-byte record and sets
    // current shape and matrix-stack indices (+0x54/+0x50) to -1.
    std::vector<DrawBufferShape> shapes{1};
    int currentShape = -1;
    int matrixStackIndex = -1;
    std::array<DrawBufferMatrix, 4> matrixStack{};
    bool destroyed = false;
};

struct VideoDrawBufferHandle {
    std::shared_ptr<VideoDrawBufferData> buffer;
};

std::vector<std::shared_ptr<VideoDrawBufferData>>& DrawBufferManager() {
    // Retail's Video manager owns the draw-buffer list independently from the
    // script wrapper. Keep that ownership when the GM wrapper is collected.
    static std::vector<std::shared_ptr<VideoDrawBufferData>> buffers;
    return buffers;
}

VideoDrawBufferData* ThisDrawBuffer(gmThread* a_thread) {
    auto* handle = static_cast<VideoDrawBufferHandle*>(
        a_thread->ThisUserCheckType(g_videoDrawBufferType));
    return handle && handle->buffer && !handle->buffer->destroyed
        ? handle->buffer.get() : nullptr;
}

int GM_CDECL gmf_VideoDrawBuffer_dummy(gmThread* a_thread) {
    if (!ThisDrawBuffer(a_thread)) return GM_EXCEPTION;
    // PPC 0x83504 returns zero without reading state or arguments.
    return GM_OK;
}

int GM_CDECL gmf_VideoDrawBuffer_draw(gmThread* a_thread) {
    if (!ThisDrawBuffer(a_thread)) return GM_EXCEPTION;
    // PPC 0x836dc reaches FUN_00053be8, whose body is a literal `blr`.
    return GM_OK;
}

// Retail FUN_00053870 grows the 0x70-byte shape-record array through index,
// frees that record's previous vertex allocation, then allocates
// vertexCapacity * 0x18 bytes and initializes each 24-byte vertex record.
int GM_CDECL gmf_VideoDrawBuffer_setReservedSize(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 2) return GM_EXCEPTION;
    // The PPC wrapper forwards both raw 32-bit payloads; no numeric-tag
    // conversion is performed by retail.
    const int index = a_thread->Param(0).m_value.m_int;
    const int capacity = a_thread->Param(1).m_value.m_int;
    if (index < 0 || capacity < 0) return GM_EXCEPTION;
    if ((size_t)index >= buffer->shapes.size())
        buffer->shapes.resize((size_t)index + 1);
    DrawBufferShape& shape = buffer->shapes[(size_t)index];
    shape.vertices.resize((size_t)capacity * 0x18);
    for (int vertex = 0; vertex < capacity; ++vertex) {
        unsigned char* record = shape.vertices.data() + (size_t)vertex * 0x18;
        const unsigned int initial[6] = {0xffffffffu, 0, 0, 0, 0, 0};
        std::copy(reinterpret_cast<const unsigned char*>(initial),
                  reinterpret_cast<const unsigned char*>(initial) + sizeof(initial), record);
    }
    shape.vertexCapacity = capacity;
    shape.vertexCount = 0;
    // FUN_00053870 does not clear the record's flags or stored matrix-shape
    // index when replacing only its vertex allocation.
    return GM_OK;
}

// FUN_00053a60 restores existing vertex records to their default colour and
// zero payload, then clears count/flags and sets record field +0x5c to -1.
int GM_CDECL gmf_VideoDrawBuffer_reset(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const int index = a_thread->Param(0).m_value.m_int;
    if (index < 0 || (size_t)index >= buffer->shapes.size()) return GM_OK;
    DrawBufferShape& shape = buffer->shapes[(size_t)index];
    for (int vertex = 0; vertex < shape.vertexCount; ++vertex) {
        unsigned char* record = shape.vertices.data() + (size_t)vertex * 0x18;
        const unsigned int zeroed[6] = {shape.defaultColour, 0, 0, 0, 0, 0};
        std::copy(reinterpret_cast<const unsigned char*>(zeroed),
                  reinterpret_cast<const unsigned char*>(zeroed) + sizeof(zeroed), record);
    }
    shape.vertexCount = 0;
    shape.vertexFlags = 0;
    shape.matrixShape = -1;
    return GM_OK;
}

// Retail FUN_00053ad8 resets each record, releases its vertex allocation and
// finally frees the record array. It leaves the buffer's other fields alone.
int GM_CDECL gmf_VideoDrawBuffer_resetAll(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    buffer->shapes.clear();
    return GM_OK;
}

// Retail tests index < recordCount and record.vertexCount != 0, then pushes
// a GM_INT boolean (0 or 1).
int GM_CDECL gmf_VideoDrawBuffer_isHasData(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const int index = a_thread->Param(0).m_value.m_int;
    const bool hasData = index >= 0 && (size_t)index < buffer->shapes.size() &&
                         buffer->shapes[(size_t)index].vertexCount != 0;
    a_thread->PushInt(hasData ? 1 : 0);
    return GM_OK;
}

GE3_REMOVED(gmf_VideoDrawBuffer_setShapeColour, "VIDEODRAWBUFFER.setShapeColour")
GE3_REMOVED(gmf_VideoDrawBuffer_setColour, "VIDEODRAWBUFFER.setColour")
// FUN_0008372c reserves/resets the selected shape, copies 0x44 bytes of the
// active matrix into its record, and stores the requested shape mode.
GE3_REMOVED(gmf_VideoDrawBuffer_beginShape, "VIDEODRAWBUFFER.beginShape")
// FUN_000838b4 writes -1 to buffer field +0x54; it reads no script args.
int GM_CDECL gmf_VideoDrawBuffer_endShape(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    buffer->currentShape = -1;
    return GM_OK;
}
GE3_REMOVED(gmf_VideoDrawBuffer_plotVertex, "VIDEODRAWBUFFER.plotVertex")
// FUN_000843cc increments the native stack index and zeroes the new
// 16-byte {x,y,z,angle} stack entry. Retail has four inline entries in the
// 0x60-byte matrix object; exceeding them is an unchecked native overrun.
int GM_CDECL gmf_VideoDrawBuffer_pushMatrix(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    const int index = ++buffer->matrixStackIndex;
    if (index < 0 || (size_t)index >= buffer->matrixStack.size())
        return GM_EXCEPTION;
    buffer->matrixStack[(size_t)index] = {};
    return GM_OK;
}

// FUN_0008443c only decrements the native stack index; there is no
// underflow branch in the PPC body.
int GM_CDECL gmf_VideoDrawBuffer_popMatrix(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    --buffer->matrixStackIndex;
    return GM_OK;
}

GE3_REMOVED(gmf_VideoDrawBuffer_translate, "VIDEODRAWBUFFER.translate")
// FUN_0008470c converts arg 0 using GM_FLOAT payload for tag 2 and the raw
// signed integer payload otherwise, then adds that scalar to current angle.
int GM_CDECL gmf_VideoDrawBuffer_rotateZ(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& argument = a_thread->Param(0);
    const float angle = argument.m_type == GM_FLOAT
        ? argument.m_value.m_float : (float)argument.m_value.m_int;
    const int index = buffer->matrixStackIndex;
    if (index < 0 || (size_t)index >= buffer->matrixStack.size()) return GM_EXCEPTION;
    buffer->matrixStack[(size_t)index].rotation += angle;
    return GM_OK;
}

// FUN_000847a0 with zero args calls FUN_00053b58(buffer,false); one arg uses
// the embedded 0.0f threshold at 0x84798 and asks the helper to clear all record
// matrix-shape indices only when arg0 > 0.0f. Counts above one do nothing.
int GM_CDECL gmf_VideoDrawBuffer_resetMatrixStack(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    const int count = a_thread->GetNumParams();
    if (count > 1) return GM_OK;
    bool resetShapes = false;
    if (count == 1) {
        const gmVariable& argument = a_thread->Param(0);
        const float value = argument.m_type == GM_FLOAT
            ? argument.m_value.m_float : (float)argument.m_value.m_int;
        resetShapes = 0.0f < value;
    }
    buffer->matrixStackIndex = -1;
    if (resetShapes) {
        for (DrawBufferShape& shape : buffer->shapes) shape.matrixShape = -1;
    }
    return GM_OK;
}

// FUN_0008487c converts arg 0 to the native record index and writes -1 to
// that record's field +0x5c.
int GM_CDECL gmf_VideoDrawBuffer_resetMatrixShape(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& argument = a_thread->Param(0);
    const int index = argument.m_type == GM_FLOAT
        ? (int)std::trunc(argument.m_value.m_float) : argument.m_value.m_int;
    if (index >= 0 && (size_t)index < buffer->shapes.size())
        buffer->shapes[(size_t)index].matrixShape = -1;
    return GM_OK;
}
GE3_REMOVED(gmf_VideoDrawBuffer_storeMatrix, "VIDEODRAWBUFFER.storeMatrix")

gmFunctionEntry s_videoDrawBufferLib[] = {
    {"dummy", gmf_VideoDrawBuffer_dummy, nullptr},
    {"setReservedSize", gmf_VideoDrawBuffer_setReservedSize, nullptr},
    {"reset", gmf_VideoDrawBuffer_reset, nullptr},
    {"resetAll", gmf_VideoDrawBuffer_resetAll, nullptr},
    {"isHasData", gmf_VideoDrawBuffer_isHasData, nullptr},
    {"draw", gmf_VideoDrawBuffer_draw, nullptr},
    {"setShapeColour", gmf_VideoDrawBuffer_setShapeColour, nullptr},
    {"setColour", gmf_VideoDrawBuffer_setColour, nullptr},
    {"beginShape", gmf_VideoDrawBuffer_beginShape, nullptr},
    {"endShape", gmf_VideoDrawBuffer_endShape, nullptr},
    {"plotVertex", gmf_VideoDrawBuffer_plotVertex, nullptr},
    {"pushMatrix", gmf_VideoDrawBuffer_pushMatrix, nullptr},
    {"popMatrix", gmf_VideoDrawBuffer_popMatrix, nullptr},
    {"translate", gmf_VideoDrawBuffer_translate, nullptr},
    {"rotateZ", gmf_VideoDrawBuffer_rotateZ, nullptr},
    {"resetMatrixStack", gmf_VideoDrawBuffer_resetMatrixStack, nullptr},
    {"resetMatrixShape", gmf_VideoDrawBuffer_resetMatrixShape, nullptr},
    {"storeMatrix", gmf_VideoDrawBuffer_storeMatrix, nullptr},
};

void GM_CDECL VideoDrawBufferGCDestruct(gmMachine*, gmUserObject* a_object) {
    delete static_cast<VideoDrawBufferHandle*>(a_object->m_user);
}

} // namespace

void RegisterVideoDrawBufferBindings(gmMachine& a_machine) {
    DrawBufferManager().clear();
    g_videoDrawBufferType = a_machine.CreateUserType("VIDEODRAWBUFFER");
    a_machine.RegisterTypeLibrary(g_videoDrawBufferType, s_videoDrawBufferLib,
        sizeof(s_videoDrawBufferLib) / sizeof(s_videoDrawBufferLib[0]));
    a_machine.RegisterUserCallbacks(g_videoDrawBufferType, nullptr,
                                    VideoDrawBufferGCDestruct, nullptr);
}

int GM_CDECL gmf_Video_createDrawBuffer(gmThread* a_thread) {
    if (g_videoDrawBufferType < 0) return GM_EXCEPTION;
    auto buffer = std::make_shared<VideoDrawBufferData>();
    DrawBufferManager().push_back(buffer);
    a_thread->PushNewUser(new VideoDrawBufferHandle{std::move(buffer)},
                          g_videoDrawBufferType);
    return GM_OK;
}

int GM_CDECL gmf_Video_destroyDrawBuffer(gmThread* a_thread) {
    // Retail reads arg 0's native payload directly and performs no count/tag
    // validation. For malformed desktop calls, avoid dereferencing a foreign
    // GM object; valid retail VIDEODRAWBUFFER arguments follow the exact path.
    gmUserObject* object = a_thread->ParamUserObject(0);
    if (!object || object->m_userType != g_videoDrawBufferType || !object->m_user)
        return GM_OK;
    auto* handle = static_cast<VideoDrawBufferHandle*>(object->m_user);
    if (!handle->buffer || handle->buffer->destroyed) return GM_OK;
    auto& buffers = DrawBufferManager();
    auto it = std::find(buffers.begin(), buffers.end(), handle->buffer);
    if (it != buffers.end()) buffers.erase(it);
    handle->buffer->shapes.clear();
    handle->buffer->destroyed = true;
    return GM_OK;
}

int GM_CDECL gmf_Video_getNumDrawBuffers(gmThread* a_thread) {
    a_thread->PushInt(static_cast<int>(DrawBufferManager().size()));
    return GM_OK;
}
