#include "draw_list.h"
#include "draw_context.h"
#include "renderer.h"
#include "../core/engine_context.h"
#include "../core/physics_world.h"
#include "../bindings/entity_bindings.h"
#include "../bindings/shape_bindings.h"
#include "../bindings/process_handle_bindings.h"
#include "../bindings/animation_bindings.h"
#include "../bindings/colour_bindings.h"
#include "../core/strict_calls.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <box2d/box2d.h>
#include <GL/glew.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <cstdint>

namespace {

gmType g_drawListType = -1;

} // namespace

DrawListData* RecordingTarget(gmThread* a_thread) {
    gmUserObject* obj = a_thread->ThisUserObject();
    if (obj && obj->m_userType == g_drawListType) {
        auto* data = static_cast<DrawListData*>(obj->m_user);
        return data && data->commandBuffer ? data : nullptr;
    }
    return nullptr;
}

void ReplayDrawList(DrawListData* a_data) {
    if (!a_data) return;
    DrawContext& context = GetDrawContext();
    const bool previousSelectionProgram = context.selectionFragmentProgram;
    context.selectionFragmentProgram = a_data->useSelectionFragmentProgram;
    ReplayDrawList(a_data->commandBuffer);
    context.selectionFragmentProgram = previousSelectionProgram;
}

void ReplayDrawList(const std::shared_ptr<DrawListData::CommandBuffer>& a_buffer) {
    if (!a_buffer) return;
    for (auto& cmd : a_buffer->commands) cmd();
}

DrawListSubmission CaptureDrawListSubmission(DrawListData* a_list,
                                             const char* a_debugLabel) {
    DrawListSubmission submission;
    if (a_list) {
        submission.commandBuffer = a_list->commandBuffer;
        submission.orderKey = a_list->orderKey;
        submission.useSelectionFragmentProgram =
            a_list->useSelectionFragmentProgram;
        if (a_debugLabel) submission.debugLabel = a_debugLabel;
    }
    return submission;
}

void ReplayDrawListsInOrder(std::vector<DrawListSubmission>& a_lists) {
    std::stable_sort(a_lists.begin(), a_lists.end(), [](const DrawListSubmission& a,
                                                         const DrawListSubmission& b) {
        if (!a.commandBuffer) return static_cast<bool>(b.commandBuffer);
        if (!b.commandBuffer) return false;
        return a.orderKey < b.orderKey;
    });
    DrawContext& context = GetDrawContext();
    for (const DrawListSubmission& submission : a_lists) {
        if (!submission.commandBuffer) continue;
        // Retail sorts the native list pointers, then replays their command
        // streams against one live render-state tuple. Do not restore state
        // captured while collecting each list: opcode setters and push/pop
        // commands mutate the state at replay time and later lists inherit it.
        const bool previousSelectionProgram = context.selectionFragmentProgram;
        context.selectionFragmentProgram =
            submission.useSelectionFragmentProgram;
        Renderer* renderer = GetEngineContext().renderer;
        if (std::getenv("GE3_TRACE_DRAWLISTS")) {
            float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
            if (renderer) renderer->GetColour(r, g, b, a);
            std::fprintf(stderr,
                "[drawlist-replay] begin label=%s key=%08x commands=%zu depth=%d/%d mask=%d stencil=%d:%d color=(%.3f,%.3f,%.3f,%.3f)\n",
                submission.debugLabel.empty() ? "<unnamed>" : submission.debugLabel.c_str(),
                submission.orderKey, submission.commandBuffer->commands.size(),
                context.depthTest, context.depthWrite, context.colourMask,
                context.stencilFunc, context.stencilRef, r, g, b, a);
        }
        ReplayDrawList(submission.commandBuffer);
        if (std::getenv("GE3_TRACE_DRAWLISTS")) {
            float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
            if (renderer) renderer->GetColour(r, g, b, a);
            std::fprintf(stderr,
                "[drawlist-replay] end label=%s depth=%d/%d mask=%d stencil=%d:%d color=(%.3f,%.3f,%.3f,%.3f)\n",
                submission.debugLabel.empty() ? "<unnamed>" : submission.debugLabel.c_str(),
                context.depthTest, context.depthWrite, context.colourMask,
                context.stencilFunc, context.stencilRef, r, g, b, a);
        }
        context.selectionFragmentProgram = previousSelectionProgram;
    }
    a_lists.clear();
}

namespace {

// Retail reset clears a list's commands and retained references. It leaves
// shared render state untouched until the sorted command streams replay.
int GM_CDECL gmf_reset(gmThread* a_thread) {
    DrawListData* d = (DrawListData*)a_thread->ThisUserCheckType(g_drawListType);
    if (!d || !d->commandBuffer) return GM_EXCEPTION;
    // Retail reset is administrative: clear the owned stream and retained
    // references. Shared render state changes only when commands replay.
    d->commandBuffer->commands.clear();
    d->commandBuffer->retainedObjects.clear();
    d->commandBuffer->retainedBuffers.clear();
    d->orderKey = 0x200000ffu;
    return GM_OK;
}

int GM_CDECL gmf_releaseReference(gmThread* a_thread) {
    auto* data = static_cast<DrawListData*>(
        a_thread->ThisUserCheckType(g_drawListType));
    if (!data) return GM_EXCEPTION;
    data->commandBuffer.reset();
    return GM_OK;
}

void DrawEntityMarkers(Renderer& a_renderer, const std::vector<int>& a_skipUids); // defined below

// Draw collision geometry after the scene without obscuring the frame.
void DrawPhysicsBodiesImmediate(const std::vector<int>& a_nativeDrawnUids) {
    Renderer* renderer = GetEngineContext().renderer;
    if (!renderer) return;
    // Defensive: whatever the script's own preceding draws left the
    // depth-test/depth-write state at
    // (`drawList.setAttrDepthTest`/`setAttrDepthWrite`, both real, e.g.
    // used for real ground/vibe z-layering) must not affect this. Left
    // enabled from an earlier same-frame draw, GL_DEPTH_TEST's default
    // func (GL_LESS) silently discards every later fragment at the SAME
    // z (this renderer is purely 2D -- every shape is drawn at z=0), so
    // only the very first fixture in the loop below would ever actually
    // reach the framebuffer.
    // Ignore script draw-list depth state while rendering the overlay.
    renderer->SetDepthTest(false);
    // Same reasoning as the old background quad's own comment (now
    // removed): drawStaticSurface() runs after PostDrawAll, which can
    // legitimately leave the renderer in screen-space UI mode
    // (setupAspectRatioOrthoView) -- force world/camera-relative mode back
    // on so every position/radius below lines up with the real gameplay
    // it's overlaid on, regardless of what the last script draw call left
    // active.
    // Use world coordinates regardless of the last script draw state.
    renderer->SetUseCameraView(true);

    auto drawShape = [&](const b2Fixture* fx, const b2Transform& xf, float a_r, float a_g, float a_b, float a_fillA, float a_outlineA) {
        const b2Shape* shape = fx->GetShape();
        static std::vector<float> verts;
        verts.clear();
        if (shape->GetType() == b2Shape::e_circle) {
            const b2CircleShape* c = (const b2CircleShape*)shape;
            b2Vec2 center = b2Mul(xf, c->m_p);
            constexpr int kSegs = 24;
            for (int i = 0; i < kSegs; ++i) {
                float ang = (float)i / (float)kSegs * 2.0f * (float)M_PI;
                verts.push_back(center.x + std::cos(ang) * c->m_radius);
                verts.push_back(center.y + std::sin(ang) * c->m_radius);
            }
        } else if (shape->GetType() == b2Shape::e_polygon) {
            const b2PolygonShape* p = (const b2PolygonShape*)shape;
            for (int i = 0; i < p->m_count; ++i) {
                b2Vec2 v = b2Mul(xf, p->m_vertices[i]);
                verts.push_back(v.x);
                verts.push_back(v.y);
            }
        } else {
            return; // edge/chain -- no real usage creates these, nothing to draw
        }
        int count = (int)verts.size() / 2;
        if (count < 2) return;
        renderer->SetColour(a_r, a_g, a_b, a_fillA);
        renderer->DrawPolygon(GL_TRIANGLE_FAN, verts.data(), count);
        renderer->SetColour(a_r, a_g, a_b, a_outlineA);
        renderer->DrawPolygon(GL_LINE_LOOP, verts.data(), count);
    };

    b2World& world = GetPhysicsWorld();
    for (b2Body* body = world.GetBodyList(); body; body = body->GetNext()) {
        const b2Transform& xf = body->GetTransform();
        bool isDynamic = (body->GetType() == b2_dynamicBody);
        for (b2Fixture* fx = body->GetFixtureList(); fx; fx = fx->GetNext()) {
            if (fx->IsSensor()) {
                drawShape(fx, xf, 1.0f, 0.95f, 0.15f, 0.12f, 0.55f); // yellow, faint fill -- "here but not solid"
            } else if (isDynamic) {
                drawShape(fx, xf, 1.0f, 0.15f, 0.1f, 0.28f, 0.9f);   // red -- dynamic bodies
            } else if (body->GetType() == b2_staticBody) {
                // Solid/near-opaque means the body was explicitly made
                // static. Retail-created zero-density bodies remain dynamic.
                drawShape(fx, xf, 0.15f, 0.55f, 1.0f, 0.75f, 0.95f); // blue, solid -- static (mass 0 -> truly immovable)
            } else {
                drawShape(fx, xf, 0.9f, 0.9f, 0.9f, 0.22f, 0.8f);    // white -- kinematic (none exist yet, but distinct in case)
            }
        }
    }

    renderer->SetDepthTest(false);
    renderer->SetUseCameraView(true);
    DrawEntityMarkers(*renderer, a_nativeDrawnUids);
}

void DrawEntityMarkers(Renderer& a_renderer, const std::vector<int>& a_skipUids) {
    constexpr float kHalf = 0.45f;
    for (const EntityDebugInfo& e : GetAllEntityDebugPositions()) {
        if (!e.hasPos) continue;
        if (std::find(a_skipUids.begin(), a_skipUids.end(), e.uid) != a_skipUids.end()) continue;
        if (e.isPlayer) a_renderer.SetColour(0.15f, 0.95f, 0.95f, 0.7f);  // cyan -- player entity marker
        else a_renderer.SetColour(0.95f, 0.9f, 0.15f, 0.7f);             // yellow -- every other entity marker
        float verts[8] = {
            e.x,          e.y + kHalf,
            e.x + kHalf,  e.y,
            e.x,          e.y - kHalf,
            e.x - kHalf,  e.y,
        };
        a_renderer.DrawPolygon(GL_TRIANGLE_FAN, verts, 4);
    }
}

int GM_CDECL gmf_drawStaticSurface(gmThread* a_thread) {
    return DC_RecordOrRun(a_thread, [] {
        Renderer* renderer = GetEngineContext().renderer;
        if (!renderer) return;
        float cx = renderer->GetCameraX(), cy = renderer->GetCameraY();
        // Visible world extent now comes from the real perspective
        // projection (it scales with camera Z) rather than a fixed
        // half-height extent -- see Renderer::GetWorldHalfHeight().
        float halfH = renderer->GetWorldHalfHeight() * 1.05f; // slight margin so edges never peek through
        float halfW = renderer->GetWorldHalfWidth() * 1.05f;
        float quad[8] = {
            cx - halfW, cy - halfH,
            cx + halfW, cy - halfH,
            cx + halfW, cy + halfH,
            cx - halfW, cy + halfH,
        };
        renderer->SetDepthTest(false);
        renderer->SetDepthWrite(false);
        renderer->DrawPolygon(GL_TRIANGLE_FAN, quad, 4);
        renderer->SetDepthWrite(true);
        renderer->SetDepthTest(true);
    });
}

int GM_CDECL gmf_setDebugID(gmThread* a_thread) {
    if (!a_thread->ThisUserCheckType(g_drawListType)) return GM_EXCEPTION;
    return GM_OK;
}

int GM_CDECL gmf_callDrawList(gmThread* a_thread) {
    gmUserObject* obj = a_thread->ParamUserObject(0);
    if (!obj || obj->m_userType != g_drawListType) return GM_OK; // tolerant-null, same convention as drawShape
    DrawListData* target = (DrawListData*)obj->m_user;
    if (DrawListData* rec = RecordingTarget(a_thread)) {
        if (target && target->commandBuffer) {
            // Keep the target wrapper alive as well as its command buffer:
            // selector state is read at replay time, as retail reads the
            // target DrawList+0x10 each time its commands are entered.
            rec->commandBuffer->retainedObjects.push_back(obj);
            rec->commandBuffer->commands.push_back(
                [target] { ReplayDrawList(target); });
        }
    } else {
        ReplayDrawList(target);
    }
    return GM_OK;
}

int GM_CDECL gmf_setVertexShaderParam(gmThread* a_thread) {
    const int argc = a_thread->GetNumParams();
    if (argc != 3 && argc != 4) return GM_EXCEPTION;
    const gmVariable target = a_thread->Param(0);
    gmMachine& machine = *a_thread->GetMachine();
    int entityId = 0;
    if (target.m_type == GM_TABLE) {
        gmTableObject* entity = (gmTableObject*)machine.GetObject(target.m_value.m_ref);
        const gmVariable objectId = entity->Get(&machine, "__objectID");
        if (objectId.m_type != GM_INT) return GM_EXCEPTION;
        entityId = objectId.m_value.m_int;
    } else if (argc != 3 || !GetShaderStorageId(machine, target, entityId)) {
        return GM_EXCEPTION;
    }
    const gmVariable slotArg = argc == 4 ? a_thread->Param(1) : gmVariable((gmint)0);
    const gmVariable indexArg = a_thread->Param(argc - 2);
    const gmVariable valueArg = a_thread->Param(argc - 1);
    if (slotArg.m_type != GM_INT || indexArg.m_type != GM_INT ||
        (valueArg.m_type != GM_FLOAT && valueArg.m_type != GM_INT)) return GM_EXCEPTION;
    // The retail four-argument wrapper routes negative slots to slot zero.
    const int slot = std::max(0, (int)slotArg.m_value.m_int);
    const int index = indexArg.m_value.m_int;
    const float value = valueArg.m_type == GM_FLOAT ? valueArg.m_value.m_float : (float)valueArg.m_value.m_int;
    return DC_RecordOrRun(a_thread, [entityId, slot, index, value] {
        SetEntityShaderFloat(entityId, slot, index, value);
    });
}

int GM_CDECL gmf_setAttrAlphaBlend(gmThread*) { return -1; }
int GM_CDECL gmf_invalidStencilSetter(gmThread*) { return -1; }
int GM_CDECL gmf_nop(gmThread* a_thread) {
    DrawListData* data = RecordingTarget(a_thread);
    if (!data) return GM_EXCEPTION;
    data->commandBuffer->commands.emplace_back([] {});
    return GM_OK;
}

struct RetailSplitGlyph {
    uint32_t codepoint;
    size_t byteStart;
    size_t byteEnd;
    float advance;
};

uint32_t DecodeSplitGlyph(const std::string& a_text, size_t& a_offset) {
    const uint8_t lead = static_cast<uint8_t>(a_text[a_offset++]);
    const size_t length = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 :
                          lead < 0xf8 ? 4 : lead < 0xfc ? 5 : 6;
    const uint8_t masks[] = {0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe};
    uint32_t codepoint = lead & static_cast<uint8_t>(~masks[length - 1]);
    for (size_t i = 1; i < length; ++i) {
        const uint8_t next = a_offset < a_text.size()
            ? static_cast<uint8_t>(a_text[a_offset]) : 0;
        if (a_offset < a_text.size()) ++a_offset;
        codepoint = (codepoint << 6) | (next & 0x3f);
    }
    return codepoint;
}

bool IsRetailSplitOpportunity(uint32_t a_cp) {
    switch (a_cp) {
    case 0x09: case 0x20:
    case 0x3001: case 0x3002: case 0x30fb: case 0x30fc:
    case 0xff01: case 0xff09: case 0xff0c: case 0xff1a: case 0xff1f:
    case 0x29:
        return true;
    default:
        return false;
    }
}

struct RetailTextRange { size_t start; size_t length; };

std::vector<RetailTextRange> SplitTextLikeRetail(const BitmapFont* a_font,
                                                  const std::string& a_text,
                                                  float a_width, float a_scale) {
    std::vector<RetailTextRange> ranges;
    std::vector<RetailSplitGlyph> glyphs;
    size_t offset = 0;
    float fullWidth = 0.0f;
    while (offset < a_text.size()) {
        const size_t start = offset;
        const uint32_t cp = DecodeSplitGlyph(a_text, offset);
        const float advance = a_font ? a_font->GetRetailGlyphAdvance(cp) : 0.0f;
        const float scaled = static_cast<float>(static_cast<double>(advance) * a_scale);
        glyphs.push_back({cp, start, offset, scaled});
        fullWidth = static_cast<float>(static_cast<double>(fullWidth) + scaled);
    }
    if (glyphs.empty()) return ranges;

    // Retail helper's initial width and threshold constants are both 0.0f.
    if (a_width == 0.0f || fullWidth < a_width) {
        ranges.push_back({0, a_text.size()});
        return ranges;
    }

    size_t lineStartByte = 0;
    size_t lastBreakByte = 0;
    size_t lastBreakGlyph = 0;
    size_t lastEmittedGlyphEnd = 0;
    float committedWidth = 0.0f;
    float wordWidth = 0.0f;

    for (size_t i = 0; i < glyphs.size(); ++i) {
        const RetailSplitGlyph& glyph = glyphs[i];
        if (glyph.codepoint == 0x0a || glyph.codepoint == 0x0d || glyph.codepoint == 0x5e) {
            ranges.push_back({lineStartByte, glyph.byteStart - lineStartByte});
            lineStartByte = glyph.byteEnd;
            lastBreakByte = 0;
            lastBreakGlyph = 0;
            committedWidth = 0.0f;
            wordWidth = 0.0f;
            lastEmittedGlyphEnd = i + 1;
            if (glyph.codepoint == 0x0d && i + 1 < glyphs.size() &&
                glyphs[i + 1].codepoint == 0x0a) {
                lineStartByte = glyphs[i + 1].byteEnd;
                ++i;
                lastEmittedGlyphEnd = i + 1;
            }
            continue;
        }

        const float candidateWidth = static_cast<float>(
            static_cast<double>(committedWidth) + wordWidth + glyph.advance);
        if (a_width > 0.0f && a_width < candidateWidth) {
            if (committedWidth != 0.0f && lastBreakByte >= lineStartByte) {
                ranges.push_back({lineStartByte, lastBreakByte - lineStartByte});
                lineStartByte = lastBreakByte;
                lastEmittedGlyphEnd = lastBreakGlyph;
            } else {
                ranges.push_back({lineStartByte, glyph.byteStart - lineStartByte});
                lineStartByte = glyph.byteStart;
                lastEmittedGlyphEnd = i;
                committedWidth = 0.0f;
                wordWidth = 0.0f;
            }
            lastBreakByte = 0;
            lastBreakGlyph = 0;
            committedWidth = 0.0f;
        }

        if (IsRetailSplitOpportunity(glyph.codepoint)) {
            committedWidth = static_cast<float>(
                static_cast<double>(committedWidth) + wordWidth + glyph.advance);
            wordWidth = 0.0f;
            lastBreakByte = glyph.byteEnd;
            lastBreakGlyph = i + 1;
        } else {
            wordWidth = static_cast<float>(static_cast<double>(wordWidth) + glyph.advance);
        }
    }

    // Retail compares the last emitted codepoint position to UTF-8 byte
    // length here; preserve that quirk for multibyte strings and trailing
    // explicit separators.
    if (lastEmittedGlyphEnd < a_text.size()) {
        ranges.push_back({lineStartByte, a_text.size() - lineStartByte});
    }
    return ranges;
}

int GM_CDECL gmf_splitStringRetail(gmThread* a_thread) {
    const int count = a_thread->GetNumParams();
    const gmVariable& leftArg = count > 0 ? a_thread->Param(0) : gmVariable();
    const gmVariable& rightArg = count > 1 ? a_thread->Param(1) : gmVariable();
    const gmVariable& textArg = count > 2 ? a_thread->Param(2) : gmVariable();
    const gmVariable& scaleArg = count > 3 ? a_thread->Param(3) : gmVariable();

    // Retail only consumes float tags for bounds/scale and only a string tag
    // for text; other provided types leave the corresponding zero/empty
    // default intact rather than coercing or rejecting the call.
    const float left = leftArg.m_type == GM_FLOAT ? leftArg.m_value.m_float : 0.0f;
    const float right = rightArg.m_type == GM_FLOAT ? rightArg.m_value.m_float : 0.0f;
    const float scale = scaleArg.m_type == GM_FLOAT ? scaleArg.m_value.m_float : 0.0f;
    std::string text;
    if (textArg.m_type == GM_STRING) {
        gmStringObject* object = static_cast<gmStringObject*>(
            a_thread->GetMachine()->GetObject(textArg.m_value.m_ref));
        if (object) text = object->GetString();
    }

    const auto ranges = SplitTextLikeRetail(GetEngineContext().currentFont,
                                             text, right - left, scale);
    gmMachine* machine = a_thread->GetMachine();
    gmTableObject* result = machine->AllocTableObject();
    for (size_t i = 0; i < ranges.size(); ++i) {
        gmTableObject* range = machine->AllocTableObject();
        range->Set(machine, "startIndex", gmVariable(static_cast<gmint>(ranges[i].start)));
        range->Set(machine, "length", gmVariable(static_cast<gmint>(ranges[i].length)));
        result->Set(machine, static_cast<int>(i), gmVariable(range));
    }
    a_thread->PushTable(result);
    return GM_OK;
}

// Retail exposes these shader methods, but the portable renderer has no
// equivalent program/parameter backend yet. Keep them bound so strict survey
// reports real call sites instead of failing later as an unresolved native.
GE3_REMOVED(gmf_setFragmentShaderParam, "VMDRAWLIST.setFragmentShaderParam")
GE3_REMOVED(gmf_hackSetFragmentShaderParam, "VMDRAWLIST.hack_setFragmentShaderParam")

gmFunctionEntry s_drawListLib[] = {
    {"reset", gmf_reset, nullptr},
    {"releaseReference", gmf_releaseReference, nullptr},
    {"nop", gmf_nop, nullptr},
    {"setDebugID", gmf_setDebugID, nullptr},
    {"callDrawList", gmf_callDrawList, nullptr},
    {"drawStaticSurface", gmf_drawStaticSurface, nullptr},
    {"drawShape", DC_gmf_drawShape, nullptr},
    {"setVertexShaderParam", gmf_setVertexShaderParam, nullptr},
    {"setFragmentShaderParam", gmf_setFragmentShaderParam, nullptr},
    {"hack_setFragmentShaderParam", gmf_hackSetFragmentShaderParam, nullptr},
    {"drawShapeWithTexture", DC_gmf_drawShapeWithTexture, nullptr},
    {"beginShape", gmf_Shape_beginShape, nullptr},
    {"plotVertex", gmf_Shape_plotVertexCurrent, nullptr},
    {"endShape", gmf_Shape_endShapeCurrent, nullptr},
    {"setupWorldView", DC_gmf_setupWorldView, nullptr},
    {"setupOrthoView", DC_gmf_setupOrthoView, nullptr},
    {"setupAspectRatioOrthoView", DC_gmf_setupAspectRatioOrthoView, nullptr},
    {"pushAttributes", DC_gmf_pushAttributes, nullptr},
    {"popAttributes", DC_gmf_popAttributes, nullptr},
    {"setAttrDepthTest", DC_gmf_setAttrDepthTest, nullptr},
    {"setAttrDepthWrite", DC_gmf_setAttrDepthWrite, nullptr},
    {"setAttrAlphaBlend", gmf_setAttrAlphaBlend, nullptr},
    {"setStencil", gmf_invalidStencilSetter, nullptr},
    {"setDefaultStencil", gmf_invalidStencilSetter, nullptr},
    {"setColourMask", DC_gmf_setColourMask, nullptr},
    {"setAttrStencil", DC_gmf_setAttrStencil, nullptr},
    {"setAttrDefaultStencil", DC_gmf_setAttrDefaultStencil, nullptr},
    {"setAttrCulling", DC_gmf_setAttrCulling, nullptr},
    {"setOrderTransparent", DC_gmf_setOrderTransparent, nullptr},
    {"setOrderAlphaTest", DC_gmf_setOrderAlphaTest, nullptr},
    {"setOrderLast", DC_gmf_setOrderLast, nullptr},
    {"setDistanceTextureRenderAttributes", DC_gmf_setDistanceTextureRenderAttributes, nullptr},
    {"setColour", gmf_DC_setColour_drawList, nullptr},
    {"setColourHSL", DrawList_SetColourHSL, nullptr},
    {"setColourWithLightnessShift", DrawList_SetColourWithLightnessShift, nullptr},
    {"setInterpolatedColour", DrawList_SetInterpolatedColour, nullptr},
    {"setColourRef", gmf_DC_setColourRef, nullptr},
    {"drawModTexture", DC_gmf_drawModTexture, nullptr},
    {"drawTextureByHandle", DC_gmf_drawTextureByHandle, nullptr},
    {"setTexture", DC_gmf_setTexture, nullptr},
    {"setTextureByHandle", DC_gmf_setTextureByHandle, nullptr},
    {"clearTexture", DC_gmf_clearTexture, nullptr},
    {"setTextureSubImg", DC_gmf_setTextureSubImg, nullptr},
    {"drawTexture", DC_gmf_drawTexture, nullptr},
    {"drawAnimation", DC_gmf_drawAnimation, nullptr},
    {"setFontRenderAttributes", DC_gmf_setFontRenderAttributes, nullptr},
    {"resetFontRenderAttributes", DC_gmf_resetFontRenderAttributes, nullptr},
    {"print", DC_gmf_print, nullptr},
    {"printTranslated", DC_gmf_printTranslated, nullptr},
    {"printUntranslated", DC_gmf_printUntranslated, nullptr},
    {"printArray", DC_gmf_printArray, nullptr},
    {"splitString", gmf_splitStringRetail, nullptr},
    {"splitStringMidWord", gmf_splitStringRetail, nullptr},
    {"pushMatrix", gmf_DC_pushMatrix, nullptr},
    {"popMatrix", gmf_DC_popMatrix, nullptr},
    {"translate", gmf_DC_translate, nullptr},
    {"rotate", gmf_DC_rotate, nullptr},
    {"rotateZ", gmf_DC_rotate, nullptr}, // 2D renderer -- rotateZ is the only rotation axis that means anything
    {"scale", gmf_DC_scale, nullptr},
    {"multMatrix", gmf_DC_multMatrix, nullptr},
    {"clear", DC_gmf_clear, nullptr},
};

void GM_CDECL DrawListGCDestruct(gmMachine* a_machine, gmUserObject* a_object) {
    delete (DrawListData*)a_object->m_user;
}

#if GM_USE_INCGC
bool GM_CDECL DrawListGCTrace(gmMachine* a_machine, gmUserObject* a_object,
                              gmGarbageCollector* a_gc, const int a_workLeftToGo,
                              int& a_workDone) {
    auto* data = static_cast<DrawListData*>(a_object->m_user);
    if (data && data->commandBuffer) {
        using Buffer = DrawListData::CommandBuffer;
        std::vector<std::shared_ptr<Buffer>> pending{data->commandBuffer};
        std::unordered_set<Buffer*> visited;
        while (!pending.empty()) {
            auto buffer = std::move(pending.back());
            pending.pop_back();
            if (!buffer || !visited.insert(buffer.get()).second) continue;
            for (gmObject* object : buffer->retainedObjects) {
                if (!object) continue;
                a_gc->GetNextObject(object);
                ++a_workDone;
            }
            pending.insert(pending.end(), buffer->retainedBuffers.begin(),
                           buffer->retainedBuffers.end());
        }
    }
    ++a_workDone;
    return true;
}
#else
void GM_CDECL DrawListGCMark(gmMachine* a_machine, gmUserObject* a_object, gmuint32 a_mark) {
    auto* data = static_cast<DrawListData*>(a_object->m_user);
    if (!data || !data->commandBuffer) return;
    using Buffer = DrawListData::CommandBuffer;
    std::vector<std::shared_ptr<Buffer>> pending{data->commandBuffer};
    std::unordered_set<Buffer*> visited;
    while (!pending.empty()) {
        auto buffer = std::move(pending.back());
        pending.pop_back();
        if (!buffer || !visited.insert(buffer.get()).second) continue;
        for (gmObject* object : buffer->retainedObjects) {
            if (object && object->NeedsMark(a_mark)) object->Mark(a_machine, a_mark);
        }
        pending.insert(pending.end(), buffer->retainedBuffers.begin(),
                       buffer->retainedBuffers.end());
    }
}
#endif

} // namespace

gmType RegisterDrawListType(gmMachine& a_machine) {
    g_drawListType = a_machine.CreateUserType("VMDRAWLIST");
    a_machine.RegisterTypeLibrary(g_drawListType, s_drawListLib,
                                   sizeof(s_drawListLib) / sizeof(s_drawListLib[0]));
#if GM_USE_INCGC
    a_machine.RegisterUserCallbacks(g_drawListType, DrawListGCTrace, DrawListGCDestruct, nullptr);
#else
    a_machine.RegisterUserCallbacks(g_drawListType, DrawListGCMark, DrawListGCDestruct, nullptr);
#endif
    return g_drawListType;
}

gmType GetDrawListType() { return g_drawListType; }

void PushNewDrawList(gmThread* a_thread, int a_width, int a_height) {
    DrawListData* data = new DrawListData();
    data->width = a_width;
    data->height = a_height;
    a_thread->PushNewUser(data, g_drawListType);
}

bool SetDrawListFontRenderAttributes(gmUserObject* a_drawListObj, float a_edge, float a_width) {
    if (!a_drawListObj || a_drawListObj->m_userType != g_drawListType) return false;
    GetDrawContext().fontEdge = a_edge < 0.0f ? 0.25f : 0.5f;
    // The native helper adds widened doubles, rounds the sum to float, then
    // multiplies by its single-precision 0.5 constant.
    GetDrawContext().fontWidth = static_cast<float>(
        static_cast<double>(a_width) + std::fabs(static_cast<double>(a_edge))) * 0.5f;
    return true;
}

void DrawAllPhysicsBodies(const std::vector<int>& a_nativeDrawnUids) {
    DrawPhysicsBodiesImmediate(a_nativeDrawnUids);
}
