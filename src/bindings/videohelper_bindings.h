#pragma once
#include "gmMachine.h"

// VIDEOHELPER_* -- the subset that retail implements as REAL NATIVE CODE
// rather than in game_data/VideoHelper.gm.
//
// This distinction is easy to get wrong and matters a lot. VideoHelper.gm
// looks like it defines these in script, but the whole file is wrapped in
//
//     if(1){
//         ... the live definitions ...
//     }else{
//         ... VIDEOHELPER_circleShape, DrawFilledRect, DrawRect,
//             DrawFilledRectCentered, DrawCircle, ... DrawFilledPolygon,
//             DrawPolygon (lines 685-1323) ...
//     }
//
// -- so everything in that `else` branch is DEAD CODE in retail too. It is
// the game's own older script implementation, kept as a reference after the
// functions moved into the engine, and it calls things the shipped engine
// does not expose the same way any more (Video.beginShape/plotVertex/
// endShape). It must not be read as evidence of a script-provided binding,
// and a missing one of these must be fixed here rather than by touching
// game_data (retail ground truth).
//
// Confirmed native: both functions registered here have real decompiled
// 0x001b25d8 and VIDEOHELPER_DrawFilledPolygon at 0x001b5c78 -- and stock
// game_data reaches both (CoryPopup.gm:119 calls DrawFilledPolygon 18 times
// during boot; MainMenu5/CampaignScreenAlbumSelect.gm:80 calls
// DrawFilledRect), where they were the only two unimplemented engine
// functions left on that path.
//
// Despite the "Draw" in their names, neither draws anything immediately:
// both BUILD AND RETURN a shape for a later drawShape() call
// (`popup.quadShape=VIDEOHELPER_DrawFilledPolygon(verts,4,0)`,
// `this.recordCompletionShape=VIDEOHELPER_DrawFilledRect(0,0,1,1)`). The
// decompiled bodies confirm the return value and the argument forms, and the
// dead `else` branch independently corroborates the vertex layout.
void RegisterVideoHelperBindings(gmMachine& a_machine);
