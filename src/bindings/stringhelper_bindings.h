#pragma once
#include "gmMachine.h"

// STRINGHELPER_* -- confirmed real bare-global native family (widely used
// in ColourEditorProcess.gm etc, none of it defined anywhere in the .gm
// corpus, so it's native, not a script helper). Only MakeTimeTable is
// implemented here: it's the one member of this family that's actually
// load-bearing for normal gameplay, not just the (unused-in-play-mode)
// in-editor colour picker tool. Confirmed real usage, both call sites
// passing its return value straight into `drawList.print(...)` as the text
// argument (Platformer/PauseProcessBase.gm:151/779 `this.elapsedTime =
// STRINGHELPER_MakeTimeTable(GameSystem.getMSSincePlayStart());` .. `
// drawList.print(0,0,0,this.parent.elapsedTime,...)`, and
// Platformer/WinScreenProcessBase.gm:122/1084 identically under the name
// `elapsedTimeString`) -- confirming it returns a plain display-ready
// string, not a literal GM table desp 8ite the "Table" in its name. With
// this unregistered, the call fell back to gmThread's undefined-global-
// call leniency (null), and `this.elapsedTime`/`elapsedTimeString` being
// null made the pause/win screen's own `drawList.print()` call throw
// "print: expecting a string text argument" every single frame, visibly
// breaking those screens' timer display. No decompiled body exists
// zero-padded) is the standard convention for an in-level elapsed-time
// readout and is not modelled beyond that (no hours field: no real level
// runs anywhere near 60 minutes).
void RegisterStringHelperBindings(gmMachine& a_machine);
