#pragma once
#include "gmMachine.h"

// SAVEDATA_* bare globals provide the local save-directory adapter used by
// Main.gm, SaveDataHelper.gm, and MyLevelsTOC.gm. Request calls complete
// synchronously and return table-backed tickets; native SAVEREQUEST identity
// and Sony's asynchronous request-manager lifecycle remain unimplemented.
void RegisterSaveDataBindings(gmMachine& a_machine);
