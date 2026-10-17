#pragma once
#include "gmThread.h"
#include "gmMachine.h"
#include "draw_context.h"
#include <vector>
#include <functional>
#include <memory>
#include <string>

// "Video.* and drawList.* really are (mostly) the same underlying command
// list"; drawTextureByHandle/print/setColour/pushAttributes/popAttributes/
// setAttrDepthTest/setupWorldView/reset/setFontRenderAttributes all
//
// A real drawList instance now genuinely records (see
// DrawListData::CommandBuffer::commands below) rather than drawing
// immediately -- the earlier milestone's immediate-execution shortcut broke
// the real "build a sub-list once,
// callDrawList() it every frame" composition pattern used throughout the
// corpus (QuadShadowed.gm's drop shadows, EntityMenu_*Cell.gm's nested
// widget draws, ...): with every drawList call executing immediately at
// build time, geometry drawn once during an occasional rebuild simply never
// reappeared on later frames, since nothing ever replayed it again. The
// bare `Video.*` global form is unaffected -- it still executes immediately,
// matching real retail's own Video-vs-drawList split (Video IS effectively
// "whichever list is being replayed for the frame"; see RecordingTarget).
gmType RegisterDrawListType(gmMachine& a_machine);
gmType GetDrawListType();

struct DrawListData {
    int width = 0;
    int height = 0;
    // Retail native DrawList+0x10. GAME_SetIsUsingSelectionFragmentProgram
    // toggles this flag on the Entity's owned drawList; ReplayDrawListCommands
    // copies it into render context. Alternate-config shader dispatch remains
    // open in the portable renderer.
    bool useSelectionFragmentProgram = false;
    // Retail draw-list constructor initializes native +0x30 to 0x200000ff.
    // Order setters replace its pass tier and quantized Z component.
    unsigned int orderKey = 0x200000ffu;
    struct CommandBuffer {
        std::vector<std::function<void()>> commands;
        // Strong GC roots for objects referenced by recorded native commands
        // whose retail payload stores a live raw pointer (e.g. opcode 0x0c).
        std::vector<gmObject*> retainedObjects;
        // callDrawList retains the called list's buffer independently of its
        // wrapper reference, matching the retail native refcount contract.
        std::vector<std::shared_ptr<CommandBuffer>> retainedBuffers;
    };
    std::shared_ptr<CommandBuffer> commandBuffer =
        std::make_shared<CommandBuffer>();
};

struct DrawListSubmission {
    std::shared_ptr<DrawListData::CommandBuffer> commandBuffer;
    unsigned int orderKey = 0x200000ffu;
    bool useSelectionFragmentProgram = false;
    std::string debugLabel;
};

// Returns the drawList this native call's receiver is, if it was made
// through an explicit instance (`someList.pushMatrix()`) rather than the
// bare `Video.pushMatrix()` form -- the signal every function shared
// between Video and drawList (draw_context.cpp/shape_bindings.cpp) uses to
// decide whether to defer (push a replay closure onto the instance's own
// command list) or execute immediately.
DrawListData* RecordingTarget(gmThread* a_thread);

// Runs every command recorded on a_data, in order, against whatever the
// CURRENT DrawContext/transform state is right now -- this is the actual
// "draw" moment for anything built via a real drawList instance.
// Recursion-safe: a_data's own commands can themselves be a nested
// callDrawList() replay closure.
void ReplayDrawList(DrawListData* a_data);
void ReplayDrawList(const std::shared_ptr<DrawListData::CommandBuffer>& a_buffer);
DrawListSubmission CaptureDrawListSubmission(DrawListData* a_list,
                                               const char* a_debugLabel = nullptr);

// Retail's frame renderer sorts each queued set of native draw-list pointers
// by the unsigned order key at native DrawList+0x30 before replaying it.
void ReplayDrawListsInOrder(std::vector<DrawListSubmission>& a_lists);

// Creates a new drawList user object sized a_width x a_height (per
// this.setupPreDrawList(w,h)'s confirmed call shape). Pushed as the return
// value on a_thread.
void PushNewDrawList(gmThread* a_thread, int a_width, int a_height);

// Lets native FONTHELPER_* stand-ins (see video_bindings.cpp) drive a
// drawList's font-render attributes the same way the real
// drawList.setFontRenderAttributes(edge,width) binding does, without going
// through a GM call. Returns false if a_drawListObj isn't really a drawList.
bool SetDrawListFontRenderAttributes(gmUserObject* a_drawListObj, float a_edge, float a_width);

// Opt-in debug overlay (GE3_PHYSICS_DEBUG_DRAW=1) -- flat bounding-box
// fills for every live Box2D body/fixture plus diamond markers for
// entities with no body, an invented visual for verifying physics/
// position state, NOT real retail output. a_nativeDrawnUids is the list
// of entity uids that already drew themselves for real THIS frame (see
// main.cpp's unconditional DrawEntitiesWithNativeDrawHook() call), so the
// marker overlay only fills in entities that didn't.
void DrawAllPhysicsBodies(const std::vector<int>& a_nativeDrawnUids);
