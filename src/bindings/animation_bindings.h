#pragma once
#include "gmMachine.h"
#include <string>

// Real Animation type backing entity.loadAnimation(path) -- .fsa/.bci
// sprite-sheet animations (engine/src/render/fsa_bci.h,
// entity_bindings.cpp (getSourcePixelWidth/Height always 0, isDone()
// always false, no drawAnimation at all -- every animated entity in the
// corpus, e.g. Platformer/EntityPacks/SpawnPoint.gm's player spawner,
// silently drew nothing). See animation_bindings.cpp for the playback
// timing and palette-tinting design.
void RegisterAnimationBindings(gmMachine& a_machine);

// Entity.loadAnimation(path) -- called from entity_bindings.cpp so the
// Animation type stays defined in one place. a_resolvedPath is an
// already-FileHelper-resolved path (or empty/unresolvable -- pushes an
// Animation with zero frames rather than failing, same "honest empty
// stand-in" leniency as every other asset-load gap in this engine).
void PushNewAnimation(gmThread* a_thread, const std::string& a_resolvedPath);

// COLOURSCHEME_SetAnimationPaletteColour(anim, cn1, cn2) (colour_bindings.cpp)
// needs to reach into the Animation object from a different compilation
// unit -- exposed here rather than exporting the whole type. Returns false
// if a_animVar isn't actually an Animation object.
bool SetAnimationPaletteColour(gmMachine& a_machine, const gmVariable& a_animVar,
                                float a_outR, float a_outG, float a_outB,
                                float a_fillR, float a_fillG, float a_fillB);

// drawList.drawAnimation(anim) / Video.drawAnimation(anim) -- draws the
// animation's current frame as a unit quad (-0.5..0.5) under the current
// transform, matching every other drawShape-family primitive's contract
// (ENTITYBASE_DrawAnimationCenteredUsingBottomLeftMethod's real usage:
// pushMatrix/translate/scale(w,h) then drawAnimation(anim), so the quad
// itself must be unit-sized).
int GM_CDECL DC_gmf_drawAnimation(gmThread* a_thread);

// For entity_bindings.cpp's native SauceBase draw hook (see that function's
// header comment) -- SauceBase.gm's own commented-out real draw() picks
// between pulseAnimData/idleAnimData based on `anim.isPlaying()`; this lets
// the native equivalent make the same choice without a gmThread. False if
// a_animVar isn't actually an Animation object.
bool IsAnimationPlayingNow(gmMachine& a_machine, const gmVariable& a_animVar);

// For entity_bindings.cpp's native SauceBase draw hook: draws a_animVar's
// current frame as a unit (0,0)-(1,1) quad under an explicit, already-
// composed affine transform (a..f, matching DrawContext::Mat's layout) and
// Z, i.e. the same thing DC_gmf_drawAnimation's deferred lambda does, but
// callable immediately/outside any drawList recording. No-op if a_animVar
// isn't an Animation or has no valid tinted texture yet.
void DrawAnimationWithTransform(gmMachine& a_machine, const gmVariable& a_animVar,
                                 float a_a, float a_b, float a_c, float a_d, float a_e, float a_f, float a_z);
