#pragma once
#include "gmMachine.h"
#include <memory>

void RegisterShapeBindings(gmMachine& a_machine);

// Returns the host primitive corresponding to a retained VMSHAPE, or zero
// when the value is not a live shape object. Used by the textured-quad path
// that receives the caller's draw-buffer primitive as its final argument.
unsigned int GetShapePrimitiveMode(gmMachine& a_machine,
                                  const gmVariable& a_shape);

int GM_CDECL DC_gmf_drawShape(gmThread* a_thread);
// See shape_bindings.cpp's header comment on the implementation for the
// bounding-box-quad approximation this uses.
int GM_CDECL DC_gmf_drawShapeWithTexture(gmThread* a_thread);


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

// Allocates a retained VMSHAPE from native geometry with explicit per-vertex
// UVs. Used by AnimationModule's retail FSA default-layout shape builder.
gmVariable CreateShapeFromVertsAndUVs(gmMachine& a_machine, int a_shapeType,
                                      const float* a_xy, const float* a_uv,
                                      int a_count, float a_z = 0.0f);

// Rebuilds a retained native shape owned by a GM table. Used by native
// renderer ports whose retail classes keep mutable VMSHAPE buffers between
// frames. The returned variable is stored on a_owner under a_fieldName, so
// it remains rooted for the lifetime of that native entity.
gmVariable UpdateNativeOwnedShape(gmMachine& a_machine, gmTableObject* a_owner,
                                  const char* a_fieldName, int a_shapeType,
                                  const float* a_xy, int a_count, float a_z,
                                  int a_nativeCapacity = -1,
                                  int a_nativeAllocationStride = 12);

// Keeps a VMSHAPE alive while an order-sorted native draw command is pending.
std::shared_ptr<void> RetainNativeShapeForDeferredDraw(gmMachine& a_machine,
                                                       const gmVariable& a_shape);

// Returns a_var as a VMSHAPE user object, or null if it isn't one -- lets
// videohelper_bindings.cpp detect retail's leading-SHAPE overload without
// duplicating the user-type id lookup.
gmUserObject* AsShapeObject(gmMachine& a_machine, const gmVariable& a_var);

// Draws an existing VMSHAPE through the normal shape submission path under
// an explicit affine transform. Used by native renderer ports whose retail
// command stream submits a live shape object after changing the matrix stack.
bool DrawShapeObjectWithTransform(gmMachine& a_machine, const gmVariable& a_shape,
                                  float a_a, float a_b, float a_c, float a_d,
                                  float a_e, float a_f, float a_z,
                                  bool a_retailPpcMatrixMath = false);

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
