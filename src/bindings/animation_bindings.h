#pragma once
#include "gmMachine.h"
#include <string>

void RegisterAnimationBindings(gmMachine& a_machine);

// Advances the shared animation clock by the frame delta. Retail's animation
// manager updates active animations once per frame with that same delta.
void AdvanceAnimationClock(float a_msElapsed);
// Live decoded FSA resources held by module caches or Animation instances.
// Used by the retail-style process cleanup wait.
std::size_t GetLiveAnimationResourceCount();

// Entity.loadAnimation(path) -- called from entity_bindings.cpp so the
// Animation type stays defined in one place. a_resolvedPath is an
// already-FileHelper-resolved path (or empty/unresolvable -- pushes an
// Animation with zero frames rather than failing, same "honest empty
// stand-in" leniency as every other asset-load gap in this engine).
void PushNewAnimation(gmThread* a_thread, const std::string& a_resolvedPath,
                      bool a_attachToManager = true,
                      int a_fsaShapeLayout = -1);
// Releases an entity-owned animation's decoded frames and GPU textures after
// any retained drawList.drawAnimation commands have released their leases.
void UnloadAnimation(gmMachine& a_machine, const gmVariable& a_animVar);
// Removes an Animation instance from the retail-style animation manager
// update list without freeing the Animation object's resources.
void DetachAnimationFromManager(gmMachine& a_machine, const gmVariable& a_animVar);
// Entity.setIsEnabled removes/re-adds its owned animations to the retail
// animation manager while retaining their playback state and overlay links.
void SetAnimationManagerAttached(gmMachine& a_machine, const gmVariable& a_animVar,
                                 bool a_attached);

// COLOURSCHEME_SetAnimationPaletteColour(anim, cn1, cn2) (colour_bindings.cpp)
// needs to reach into the Animation object from a different compilation
// unit -- exposed here rather than exporting the whole type. Returns false
// if a_animVar isn't actually an Animation object.
bool SetAnimationPaletteColour(gmMachine& a_machine, const gmVariable& a_animVar,
                                float a_outR, float a_outG, float a_outB,
                                float a_fillR, float a_fillG, float a_fillB);
// Applies SauceBase's stored paletteBank index before its palette-colour
// update, matching Animation.setPaletteBank's native state transition.
bool SetAnimationPaletteBank(gmMachine& a_machine, const gmVariable& a_animVar,
                             int a_bank);

// drawList.drawAnimation(anim) / Video.drawAnimation(anim) -- draws the
// animation's current frame as a unit quad (-0.5..0.5) under the current
// transform, matching every other drawShape-family primitive's contract
// (ENTITYBASE_DrawAnimationCenteredUsingBottomLeftMethod's real usage:
// pushMatrix/translate/scale(w,h) then drawAnimation(anim), so the quad
// itself must be unit-sized).
int GM_CDECL DC_gmf_drawAnimation(gmThread* a_thread);

bool IsAnimationPlayingNow(gmMachine& a_machine, const gmVariable& a_animVar);

// For entity_bindings.cpp's native SauceBase draw hook: draws a_animVar's
// current frame as a unit (0,0)-(1,1) quad under an explicit, already-
// composed affine transform (a..f, matching DrawContext::Mat's layout) and
// Z, i.e. the same thing DC_gmf_drawAnimation's deferred lambda does, but
// callable immediately/outside any drawList recording. No-op if a_animVar
// isn't an Animation or has no valid tinted texture yet.
void DrawAnimationWithTransform(gmMachine& a_machine, const gmVariable& a_animVar,
                                 float a_a, float a_b, float a_c, float a_d, float a_e, float a_f, float a_z);
// Draws Animation.attachOverlayAnim's two resources through the retail paired
// palette shader on the primary animation's shape. Returns false if the pair
// is incomplete or the desktop shader could not be created.
bool DrawAnimationOverlayWithTransform(gmMachine& a_machine, const gmVariable& a_animVar,
                                       float a_a, float a_b, float a_c, float a_d,
                                       float a_e, float a_f, float a_z);

// Retail font special characters may be backed by an FSA. Their loader
// creates a looping Animation and draws its current frame texture, rather
// than treating the FSA as a static image. Returns the current direct-colour
// frame texture for an FSA whose decoded images can be uploaded without an
// Animation palette; unsupported indexed frames return false.
bool GetFontAnimationFrame(const std::string& a_resolvedPath,
                           unsigned int& a_texture, int& a_width, int& a_height);
