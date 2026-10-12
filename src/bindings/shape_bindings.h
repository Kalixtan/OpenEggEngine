#pragma once
#include "gmMachine.h"

// The VMSHAPE native GM type returned by Video.beginShape/SHAPE(). The
// Video library's beginShape/plotVertex/endShape entries are bytecode-audited
// separately from VMSHAPE methods: Video.beginShape creates a shape object,
// Video.plotVertex's helpers are no-ops, and Video.endShape is a no-op.
// `gmf_Shape_*Current` and the generic beginShape callback remain compatibility
// handlers for DrawList and are not assumed to match the Video method table.
//
// SHAPE()/setAsFilledCircle()/getQuality()/plotVertices()/draw() are NOT
// part of that -- also no decompiled body, but derived from the SAME
// retail script file's own dead-but-literal reference implementation
// (VideoHelper.gm's `if(0){...}` block), a materially stronger evidentiary
// bar than "we inferred this from how scripts call it" -- see
// shape_bindings.cpp's gmf_setAsFilledCircle comment for the full
// reasoning. These stay implemented.
void RegisterShapeBindings(gmMachine& a_machine);

// Returns the host primitive corresponding to a retained VMSHAPE, or zero
// when the value is not a live shape object. Used by the textured-quad path
// that receives the caller's draw-buffer primitive as its final argument.
unsigned int GetShapePrimitiveMode(gmMachine& a_machine,
                                  const gmVariable& a_shape);

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

// Compatibility handlers for non-Video callers; their retail ownership and
// behavior remain under audit.
int GM_CDECL gmf_Shape_beginShape(gmThread* a_thread);
int GM_CDECL gmf_Video_beginShape(gmThread* a_thread);
int GM_CDECL gmf_Video_plotVertex(gmThread* a_thread);
int GM_CDECL gmf_Video_endShape(gmThread* a_thread);
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

// Animation.setShape retains a VMSHAPE and drawAnimation replays its local
// vertices with the current animation frame. Returns false for an empty or
// invalid shape so the caller can use the standard unit quad.
bool DrawAnimationShapeWithTransform(gmMachine& a_machine, const gmVariable& a_shape,
                                     unsigned int a_texture, float a_alpha,
                                     float a_a, float a_b, float a_c, float a_d,
                                     float a_e, float a_f, float a_z);
bool DrawAnimationShapeOverlayWithTransform(gmMachine& a_machine, const gmVariable& a_shape,
                                            unsigned int a_backTexture,
                                            unsigned int a_frontTexture,
                                            float a_frontScaleX, float a_frontScaleY,
                                            float a_alpha,
                                            float a_a, float a_b, float a_c, float a_d,
                                            float a_e, float a_f, float a_z);

// Draw-list shader parameter commands update the selected entity shader at
// replay time; shape rendering reads the resulting values by native object ID.
void SetEntityShaderFloat(int a_entityId, int a_slot, int a_index, float a_value);
