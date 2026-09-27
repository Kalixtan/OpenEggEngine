#pragma once
#include "gmMachine.h"

// / VM control functions" cluster; loadScript's signature independently
// confirmed by game_data_min/Main.gm's own comment: 'matches
// GameMonkeyScript.loadScript("FontHelper.gm") in TESTPROG exactly').
// Most stock VM-introspection functions (print, format, table*, sys*, gc*)
// come for free from the real gmMachineLib() call in GmVm's constructor --
// this file only adds the engine-specific loadScript.
void RegisterGameMonkeyScriptBindings(gmMachine& a_machine);
