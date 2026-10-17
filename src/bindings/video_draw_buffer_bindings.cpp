#include "video_draw_buffer_bindings.h"

#include "colour_bindings.h"
#include "vector2d_bindings.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "gmUserObject.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace {

gmType g_videoDrawBufferType = -1;

struct DrawBufferMatrix {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float rotation = 0.0f;
};

struct DrawBufferShape {
    std::vector<unsigned char> vertices;
    int vertexCapacity = 0;
    int vertexCount = 0;
    unsigned int vertexFlags = 0;
    // Retail vertex records begin with colour -1; the remaining five words
    // are zeroed by FUN_00053870 and restored to these values by reset.
    unsigned int defaultColour = 0xffffffffu;
    int mode = 0;
    // FUN_00053b9c copies 0x44 bytes from manager+0x10 to record+0x1c:
    // four {x,y,z,rotation} stack entries (0x40 bytes), followed by the
    // manager's stack index. resetMatrixShape/resetMatrixStack write -1 to
    // the copied stack-index word at record+0x5c.
    int matrixStackSnapshotIndex = -1;
    std::array<DrawBufferMatrix, 4> matrixSnapshot{};
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

void StoreVertexWord(unsigned char* a_vertex, size_t a_offset, uint32_t a_value) {
    // These are PPC record bytes, not host-native words. Retail stores each
    // vertex field with stw/stfs on a big-endian PowerPC target.
    a_vertex[a_offset + 0] = static_cast<unsigned char>(a_value >> 24);
    a_vertex[a_offset + 1] = static_cast<unsigned char>(a_value >> 16);
    a_vertex[a_offset + 2] = static_cast<unsigned char>(a_value >> 8);
    a_vertex[a_offset + 3] = static_cast<unsigned char>(a_value);
}

void StoreVertexColour(unsigned char* a_vertex, uint32_t a_rgba) {
    // Retail packs RRGGBBAA and then stores the word on big-endian PPC.
    StoreVertexWord(a_vertex, 0, a_rgba);
}

void StoreVertexFloat(unsigned char* a_vertex, size_t a_offset, float a_value) {
    uint32_t bits = 0;
    std::memcpy(&bits, &a_value, sizeof(bits));
    StoreVertexWord(a_vertex, a_offset, bits);
}

void SnapshotMatrixState(VideoDrawBufferData& a_buffer, DrawBufferShape& a_shape) {
    a_shape.matrixSnapshot = a_buffer.matrixStack;
    a_shape.matrixStackSnapshotIndex = a_buffer.matrixStackIndex;
}

struct VideoDrawBufferHandle {
    std::shared_ptr<VideoDrawBufferData> buffer;
};

std::vector<std::shared_ptr<VideoDrawBufferData>>& DrawBufferManager() {
    // Retail's Video manager owns the draw-buffer list independently from the
    // script wrapper. Keep that ownership when the GM wrapper is collected.
    static std::vector<std::shared_ptr<VideoDrawBufferData>> buffers;
    return buffers;
}

std::vector<std::vector<unsigned char>>& ResetAllOrphanedVertexStorage() {
    // FUN_00053ad8 resets records and frees the record array but never frees
    // the per-record vertex allocations. Keep those allocations alive to
    // reproduce that retail lifetime side effect; destroyDrawBuffer uses the
    // separate FUN_000537d0 path, which does release them.
    static std::vector<std::vector<unsigned char>> storage;
    return storage;
}

void ResetDrawBufferShape(DrawBufferShape& a_shape) {
    for (int vertex = 0; vertex < a_shape.vertexCount; ++vertex) {
        unsigned char* record = a_shape.vertices.data() + static_cast<size_t>(vertex) * 0x18;
        std::fill(record, record + 0x18, 0);
        StoreVertexColour(record, a_shape.defaultColour);
    }
    a_shape.vertexCount = 0;
    a_shape.vertexFlags = 0;
    a_shape.matrixStackSnapshotIndex = -1;
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
        std::fill(record, record + 0x18, 0);
        StoreVertexColour(record, 0xffffffffu);
    }
    shape.vertexCapacity = capacity;
    shape.vertexCount = 0;
    // FUN_00053870 does not clear the record's flags or copied matrix-stack
    // index when replacing only its vertex allocation.
    return GM_OK;
}

// FUN_00053a60 restores existing vertex records to their default colour and
// zero payload, then clears count/flags and sets the copied stack-index word
// at record+0x5c to -1.
int GM_CDECL gmf_VideoDrawBuffer_reset(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const int index = a_thread->Param(0).m_value.m_int;
    if (index < 0 || (size_t)index >= buffer->shapes.size()) return GM_OK;
    ResetDrawBufferShape(buffer->shapes[(size_t)index]);
    return GM_OK;
}

// Retail FUN_00053ad8 resets each record and frees the record array but leaks
// the per-record vertex allocations. Keep those allocations alive to match.
int GM_CDECL gmf_VideoDrawBuffer_resetAll(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    auto& orphaned = ResetAllOrphanedVertexStorage();
    for (DrawBufferShape& shape : buffer->shapes) {
        ResetDrawBufferShape(shape);
        if (shape.vertices.capacity() != 0)
            orphaned.emplace_back(std::move(shape.vertices));
    }
    std::vector<DrawBufferShape>().swap(buffer->shapes);
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

uint8_t RetailDrawBufferColourByte(float a_value) {
    // FUN_00053c10 uses fmuls(value, 255.0f), fctidz, then keeps the low
    // byte. PPC's invalid-conversion result has zero in that byte.
    volatile float scaled = a_value * 255.0f;
    constexpr double kInt64Min = -9223372036854775808.0;
    constexpr double kInt64Limit = 9223372036854775808.0;
    if (!std::isfinite(scaled) || static_cast<double>(scaled) < kInt64Min ||
        static_cast<double>(scaled) >= kInt64Limit)
        return 0;
    const int64_t converted = static_cast<int64_t>(std::trunc(static_cast<double>(scaled)));
    return static_cast<uint8_t>(static_cast<uint64_t>(converted) & 0xffu);
}

float DrawBufferScalarAsFloat(const gmVariable& a_value);

int GM_CDECL gmf_VideoDrawBuffer_setShapeColour(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    const int count = a_thread->GetNumParams();
    if (!buffer) return GM_EXCEPTION;
    if (count != 2 && count != 3) return GM_EXCEPTION;

    // The PPC wrapper forwards arg 0's raw payload as the record index and
    // expects arg 1's native COLOUR object (RGB floats at its user payload).
    const int index = a_thread->Param(0).m_value.m_int;
    gmMachine* machine = a_thread->GetMachine();
    const gmVariable& colourArg = a_thread->Param(1);
    if (colourArg.m_type != g_colourType) return GM_EXCEPTION;
    gmObject* colourObject = machine->GetObject(colourArg.m_value.m_ref);
    float red = 0.0f, green = 0.0f, blue = 0.0f;
    if (!GetColourObjectRGB(colourObject, red, green, blue)) return GM_EXCEPTION;

    // 0x838f0 defaults alpha to the literal 1.0f; the optional third arg
    // follows the retail GM_FLOAT-vs-signed-integer payload conversion.
    float alpha = 1.0f;
    if (count == 3) {
        const gmVariable& alphaArg = a_thread->Param(2);
        alpha = alphaArg.m_type == GM_FLOAT
            ? alphaArg.m_value.m_float : static_cast<float>(alphaArg.m_value.m_int);
    }
    // Retail's helper treats the signed index as unsigned and only recolours
    // when it is below recordCount. Out-of-range indices therefore do nothing.
    if (index >= 0 && static_cast<size_t>(index) < buffer->shapes.size()) {
        DrawBufferShape& shape = buffer->shapes[static_cast<size_t>(index)];
        const uint32_t packed = (static_cast<uint32_t>(RetailDrawBufferColourByte(red)) << 24) |
                                (static_cast<uint32_t>(RetailDrawBufferColourByte(green)) << 16) |
                                (static_cast<uint32_t>(RetailDrawBufferColourByte(blue)) << 8) |
                                RetailDrawBufferColourByte(alpha);
        for (int vertex = 0; vertex < shape.vertexCount; ++vertex) {
            unsigned char* record = shape.vertices.data() + static_cast<size_t>(vertex) * 0x18;
            StoreVertexColour(record, packed);
        }
    }
    return GM_OK;
}

void ApplyDrawBufferColour(VideoDrawBufferData& a_buffer, float a_red,
                           float a_green, float a_blue) {
    if (a_buffer.currentShape < 0 ||
        static_cast<size_t>(a_buffer.currentShape) >= a_buffer.shapes.size())
        return;

    // FUN_00053d08 uses these exact f32 constants and fmuls/fmadds sequence
    // to form its grayscale source, then interpolates toward it using Video's
    // grayscale percentage. The literal pool is at PPC 0x53cd8..0x53cec.
    constexpr float kRedWeight = 0x1.030304p-1f;   // 0x3f018182
    constexpr float kGreenWeight = 0x1.b9b9bap-1f; // 0x3f5cdcdd
    constexpr float kBlueWeight = 0x1.151516p-2f;  // 0x3e8a8a8b
    const float weighted = std::fma(a_blue, kBlueWeight,
        std::fma(a_red, kRedWeight, a_green * kGreenWeight));
    const float gray = GetEngineContext().video.grayscalePerc;
    const float inverseGray = 1.0f - gray;
    const float red = std::clamp(std::fma(gray, weighted, inverseGray * a_red), 0.0f, 1.0f);
    const float green = std::clamp(std::fma(gray, weighted, inverseGray * a_green), 0.0f, 1.0f);
    const float blue = std::clamp(std::fma(gray, weighted, inverseGray * a_blue), 0.0f, 1.0f);
    const uint32_t packed = (static_cast<uint32_t>(RetailDrawBufferColourByte(red)) << 24) |
                            (static_cast<uint32_t>(RetailDrawBufferColourByte(green)) << 16) |
                            (static_cast<uint32_t>(RetailDrawBufferColourByte(blue)) << 8) |
                            0xffu;
    a_buffer.shapes[static_cast<size_t>(a_buffer.currentShape)].defaultColour = packed;
}

int GM_CDECL gmf_VideoDrawBuffer_setColour(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    const int count = a_thread->GetNumParams();
    if (!buffer) return GM_EXCEPTION;
    if (count == 1 || count == 2) {
        float red = 0.0f, green = 0.0f, blue = 0.0f;
        if (!ResolveColourRGB(a_thread, a_thread->Param(0), red, green, blue))
            return GM_EXCEPTION;
        // The optional alpha is converted by retail and forwarded, but
        // FUN_00053d08 writes an opaque alpha byte (0xff) unconditionally.
        if (count == 2) (void)DrawBufferScalarAsFloat(a_thread->Param(1));
        ApplyDrawBufferColour(*buffer, red, green, blue);
        return GM_OK;
    }
    if (count == 3 || count == 4) {
        const float red = DrawBufferScalarAsFloat(a_thread->Param(0));
        const float green = DrawBufferScalarAsFloat(a_thread->Param(1));
        const float blue = DrawBufferScalarAsFloat(a_thread->Param(2));
        if (count == 4) (void)DrawBufferScalarAsFloat(a_thread->Param(3));
        ApplyDrawBufferColour(*buffer, red, green, blue);
        return GM_OK;
    }
    return GM_EXCEPTION;
}
// FUN_0008372c reserves/resets the selected shape, copies four complete
// {x,y,z,rotation} matrix-stack entries plus the stack index (0x44 bytes)
// into record+0x1c, and stores the requested shape mode.
int GM_CDECL gmf_VideoDrawBuffer_beginShape(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;

    const gmVariable& modeArg = a_thread->Param(0);
    const int mode = modeArg.m_type == GM_FLOAT
        ? static_cast<int>(std::trunc(modeArg.m_value.m_float))
        : modeArg.m_value.m_int;
    const int capacity = a_thread->GetNumParams() >= 2
        ? a_thread->Param(1).m_value.m_int : 0;
    int index = static_cast<int>(buffer->shapes.size());
    if (a_thread->GetNumParams() >= 3) {
        index = a_thread->Param(2).m_value.m_int;
        if (index < 0) index = static_cast<int>(buffer->shapes.size());
    }

    // FUN_00053870 grows the record array to index+1, replaces that record's
    // vertex allocation with capacity 0x18-byte records, and initializes each
    // vertex colour to -1 with the remaining words zero. Retail trusts these
    // values; reject sizes that cannot be represented safely on desktop.
    if (index < 0 || capacity < 0 ||
        static_cast<size_t>(capacity) > std::vector<unsigned char>().max_size() / 0x18)
        return GM_EXCEPTION;
    if (static_cast<size_t>(index) >= buffer->shapes.size())
        buffer->shapes.resize(static_cast<size_t>(index) + 1);

    DrawBufferShape& shape = buffer->shapes[static_cast<size_t>(index)];
    shape.vertices.resize(static_cast<size_t>(capacity) * 0x18);
    for (int vertex = 0; vertex < capacity; ++vertex) {
        unsigned char* record = shape.vertices.data() + static_cast<size_t>(vertex) * 0x18;
        const unsigned int initial[6] = {0xffffffffu, 0, 0, 0, 0, 0};
        std::copy(reinterpret_cast<const unsigned char*>(initial),
                  reinterpret_cast<const unsigned char*>(initial) + sizeof(initial), record);
    }
    shape.vertexCapacity = capacity;
    shape.vertexCount = 0;
    shape.vertexFlags = 0;
    shape.matrixStackSnapshotIndex = -1;
    SnapshotMatrixState(*buffer, shape);
    shape.mode = mode;
    buffer->currentShape = index;
    return GM_OK;
}
// FUN_000838b4 writes -1 to buffer field +0x54; it reads no script args.
int GM_CDECL gmf_VideoDrawBuffer_endShape(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer) return GM_EXCEPTION;
    buffer->currentShape = -1;
    return GM_OK;
}
float DrawBufferScalarAsFloat(const gmVariable& a_value) {
    return a_value.m_type == GM_FLOAT
        ? a_value.m_value.m_float : static_cast<float>(a_value.m_value.m_int);
}

int GM_CDECL gmf_VideoDrawBuffer_plotVertex(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    const int count = a_thread->GetNumParams();
    if (!buffer) return GM_EXCEPTION;
    if (count < 2 || count > 6) return GM_EXCEPTION;
    const int shapeIndex = buffer->currentShape;
    if (shapeIndex < 0 || static_cast<size_t>(shapeIndex) >= buffer->shapes.size())
        return GM_EXCEPTION;

    DrawBufferShape& shape = buffer->shapes[static_cast<size_t>(shapeIndex)];
    if (shape.vertexCount < 0 || shape.vertexCount >= shape.vertexCapacity)
        return GM_EXCEPTION;
    unsigned char* vertex = shape.vertices.data() + static_cast<size_t>(shape.vertexCount) * 0x18;
    const float x = DrawBufferScalarAsFloat(a_thread->Param(0));
    const float y = DrawBufferScalarAsFloat(a_thread->Param(1));
    const uint32_t colour = count == 4 || count == 6
        ? static_cast<uint32_t>(a_thread->Param(3).m_value.m_int)
        : shape.defaultColour;

    StoreVertexColour(vertex, colour);
    if (count == 2) {
        StoreVertexFloat(vertex, 0x0c, x);
        StoreVertexFloat(vertex, 0x10, y);
        const float zero = 0.0f;
        StoreVertexFloat(vertex, 0x14, zero);
        shape.vertexFlags |= 1;
    } else {
        const float z = DrawBufferScalarAsFloat(a_thread->Param(2));
        if (count == 3 || count == 4) {
            StoreVertexFloat(vertex, 0x0c, x);
            StoreVertexFloat(vertex, 0x10, y);
            StoreVertexFloat(vertex, 0x14, z);
            shape.vertexFlags |= count == 3 ? 1u : 5u;
        } else if (count == 5) {
            const float u = DrawBufferScalarAsFloat(a_thread->Param(3));
            const float v = DrawBufferScalarAsFloat(a_thread->Param(4));
            StoreVertexFloat(vertex, 0x04, u);
            StoreVertexFloat(vertex, 0x08, v);
            StoreVertexFloat(vertex, 0x0c, x);
            StoreVertexFloat(vertex, 0x10, y);
            StoreVertexFloat(vertex, 0x14, z);
            shape.vertexFlags |= 3;
        } else { // six args: color, UV, XYZ are all supplied by the caller
            const float u = DrawBufferScalarAsFloat(a_thread->Param(4));
            const float v = DrawBufferScalarAsFloat(a_thread->Param(5));
            StoreVertexFloat(vertex, 0x04, u);
            StoreVertexFloat(vertex, 0x08, v);
            StoreVertexFloat(vertex, 0x0c, x);
            StoreVertexFloat(vertex, 0x10, y);
            StoreVertexFloat(vertex, 0x14, z);
            shape.vertexFlags |= 7;
        }
    }
    ++shape.vertexCount;
    // FUN_00083d14 writes a valid six-argument vertex, then returns -1.
    return count == 6 ? GM_EXCEPTION : GM_OK;
}
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

int GM_CDECL gmf_VideoDrawBuffer_translate(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    const int count = a_thread->GetNumParams();
    if (!buffer) return GM_EXCEPTION;
    if (count < 1 || count > 3) return GM_EXCEPTION;
    const int stackIndex = buffer->matrixStackIndex;
    if (stackIndex < 0 || static_cast<size_t>(stackIndex) >= buffer->matrixStack.size())
        return GM_EXCEPTION;

    DrawBufferMatrix& matrix = buffer->matrixStack[static_cast<size_t>(stackIndex)];
    float x = 0.0f, y = 0.0f, z = 0.0f;
    bool updatesZ = false;
    if (count == 1) {
        if (!GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y))
            return GM_EXCEPTION;
    } else if (count == 2) {
        if (GetVector2D(*a_thread->GetMachine(), a_thread->Param(0), x, y)) {
            z = DrawBufferScalarAsFloat(a_thread->Param(1));
            updatesZ = true;
        } else {
            x = DrawBufferScalarAsFloat(a_thread->Param(0));
            y = DrawBufferScalarAsFloat(a_thread->Param(1));
        }
    } else {
        x = DrawBufferScalarAsFloat(a_thread->Param(0));
        y = DrawBufferScalarAsFloat(a_thread->Param(1));
        z = DrawBufferScalarAsFloat(a_thread->Param(2));
    }
    // Retail uses fadds in x, then y, then z order, leaving rotation intact.
    matrix.x = matrix.x + x;
    matrix.y = matrix.y + y;
    if (updatesZ || count == 3)
        matrix.z = matrix.z + z;
    return GM_OK;
}
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
// the embedded 0.0f threshold at 0x84798 and asks the helper to clear all
// copied matrix-stack indices only when arg0 > 0.0f. Counts above one do nothing.
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
        for (DrawBufferShape& shape : buffer->shapes)
            shape.matrixStackSnapshotIndex = -1;
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
        buffer->shapes[(size_t)index].matrixStackSnapshotIndex = -1;
    return GM_OK;
}
int GM_CDECL gmf_VideoDrawBuffer_storeMatrix(gmThread* a_thread) {
    VideoDrawBufferData* buffer = ThisDrawBuffer(a_thread);
    if (!buffer || a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    const gmVariable& argument = a_thread->Param(0);
    const int index = argument.m_type == GM_FLOAT
        ? static_cast<int>(std::trunc(argument.m_value.m_float))
        : argument.m_value.m_int;
    if (index < 0 || static_cast<size_t>(index) >= buffer->shapes.size())
        return GM_OK; // retail's unchecked write is bounded here for safety
    SnapshotMatrixState(*buffer, buffer->shapes[static_cast<size_t>(index)]);
    return GM_OK;
}

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
    std::vector<DrawBufferShape>().swap(handle->buffer->shapes);
    handle->buffer->destroyed = true;
    return GM_OK;
}

int GM_CDECL gmf_Video_getNumDrawBuffers(gmThread* a_thread) {
    a_thread->PushInt(static_cast<int>(DrawBufferManager().size()));
    return GM_OK;
}
