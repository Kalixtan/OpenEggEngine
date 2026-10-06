#pragma once
#include "gmMachine.h"

// The VMSHAPE native GM type returned by Video.beginShape/SHAPE() and
// (previously) drawn via plotVertex()/endShape().
//
// beginShape/plotVertex/endShape and the whole 2D matrix stack
// (pushMatrix/popMatrix/translate/rotate(Z)/scale, draw_context.h) have NO
// Ghidra cross-reference search this environment doesn't have tool access
// to run (their NAMES are confirmed real -- found directly in EBOOT.elf's
// own VIDEO/VMDRAWLIST method registration tables, see draw_context.cpp's
// header comments -- but a name's presence in that table doesn't hand you
// the function body it points at). Per explicit direction, these are
// routed through the strict-call trap (core/strict_calls.h) rather than
// running on what was previously a corpus-usage-inferred implementation --
// see gmf_Shape_beginShape/gmf_Shape_plotVertexCurrent/
// gmf_Shape_endShapeCurrent below, and DrawList/Video's registration of the
// same trap thunks for the bare `Video.plotVertex(...)`/`drawList.
// translate(...)` etc. forms.
//
// SHAPE()/setAsFilledCircle()/getQuality()/plotVertices()/draw() are NOT
// part of that -- also no decompiled body, but derived from the SAME
// retail script file's own dead-but-literal reference implementation
// (VideoHelper.gm's `if(0){...}` block), a materially stronger evidentiary
// bar than "we inferred this from how scripts call it" -- see
// shape_bindings.cpp's gmf_setAsFilledCircle comment for the full
// reasoning. These stay implemented.
void RegisterShapeBindings(gmMachine& a_machine);

// DrawList.c) but was entirely unregistered on the drawList type (only an
// unrelated `Video.drawShape` no-op stub existed), silently no-op'ing every
// real call via gmThread's undefined-method-call leniency. Redraws a
// shape's stored geometry (built via setAsFilledCircle(), the only
// vertex-population path left once plotVertex is routed through the trap)
// under the CURRENT transform/colour. Exposed here (not static in
// shape_bindings.cpp) so draw_list.cpp can register it directly on
// VMDRAWLIST's function table, matching how DC_gmf_* functions are shared
// between the `Video` global and any `drawList` instance.
int GM_CDECL DC_gmf_drawShape(gmThread* a_thread);
// See shape_bindings.cpp's header comment on the implementation for the
// bounding-box-quad approximation this uses.
int GM_CDECL DC_gmf_drawShapeWithTexture(gmThread* a_thread);

// SHAPETYPE_* constant values are internally assigned here (not decompiled
// -- real scripts only ever reference them symbolically, never as raw
// numbers, so any consistent assignment is externally equivalent).

// beginShape/plotVertex/endShape trap thunks -- see this header's top
// comment. Exposed (not static in shape_bindings.cpp) so video_bindings.cpp/
// draw_list.cpp can register them directly, matching how the real
// implementations they replaced were shared between `Video` and any
// `drawList` instance.
int GM_CDECL gmf_Shape_beginShape(gmThread* a_thread);
int GM_CDECL gmf_Shape_plotVertexCurrent(gmThread* a_thread);
int GM_CDECL gmf_Shape_endShapeCurrent(gmThread* a_thread);

// Builds (or repopulates) a VMSHAPE natively from an explicit local-space
// vertex list and pushes it as the calling thread's return value. Exposed so
// videohelper_bindings.cpp's VIDEOHELPER_DrawFilledRect/DrawFilledPolygon --
// which retail implements as real native functions, not script (see that
// file's header comment) -- can build shape geometry without going through
// the beginShape/plotVertex/endShape trap thunks above.
//
// a_shapeType uses the same SHAPETYPE_* numbering as SHAPE()/beginShape().
// a_xy is a_count consecutive x,y pairs. a_existing, when non-null, is
// repopulated instead of allocating a new shape. Flags allow native helpers
// with a leading-shape overload to preserve that shape's primitive type and
// mirror whether retail pushes the object back as a return value.
void PushShapeFromVerts(gmThread* a_thread, int a_shapeType, const float* a_xy,
                        int a_count, float a_z, gmUserObject* a_existing,
                        bool a_preserveExistingMode = false,
                        bool a_pushExisting = true);

// Returns a_var as a VMSHAPE user object, or null if it isn't one -- lets
// videohelper_bindings.cpp detect retail's leading-SHAPE overload without
// duplicating the user-type id lookup.
gmUserObject* AsShapeObject(gmMachine& a_machine, const gmVariable& a_var);
