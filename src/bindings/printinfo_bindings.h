#pragma once
#include "gmMachine.h"

// plain data carrier"), pervasive across the corpus (ColourEditorProcess.gm,
// ConsoleProcess.gm, FontHelper.gm, Platformer/PopupProcessBase.gm, ...)
// always used the same way: `local pi = PRINTINFO(); pi.shouldDraw = false;`
// -- constructed, then has plain fields set/read directly, exactly a plain
// property bag. Implemented as a real GM table (not a true native user
// type) for the same reason ColourManager is (see colour_bindings.cpp):
// zero own methods means there's no behavior a native type would add over
// a table, and scripts can't tell the difference either way. Was
// previously entirely unregistered -- calling PRINTINFO() silently no-op'd
// via the null-call leniency patch (gmThread.cpp, GMSCRIPT_PATCHES.md),
// which left the caller's next local variable (`pi`) as null/undefined
// instead of a usable object, breaking every one of these call sites.
void RegisterPrintInfoBindings(gmMachine& a_machine);
