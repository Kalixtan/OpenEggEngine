#pragma once
#include "gmMachine.h"

// Its internal constructor/update/pulse/isDone/reset routines were recovered
// from retail EBOOT.elf addresses 0x161a8, 0x16284, 0x163fc, 0x16414 and
// 0x16500. The desktop implementation follows those routines, including the
// exponential rate mode and linear time mode.
void RegisterPulsePercBindings(gmMachine& a_machine);
