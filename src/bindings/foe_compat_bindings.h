#pragma once
#include "gmMachine.h"

class gmThread;

// Register Flowers of Error compatibility bindings: functions present in the
// FoE PC build but not in the Sound Shapes engine's standard bindings.
// See foe_compat_bindings.cpp for implementation notes and Ghidra decompilation
// sources.
void RegisterFoECompatBindings(gmMachine& a_machine);

// FoE alarm/LFO management functions (injected as methods on Process/Entity)
int GM_CDECL gmf_getUniqueAlarmID(gmThread* a_thread);
int GM_CDECL gmf_getUniqueLFOID(gmThread* a_thread);
int GM_CDECL gmf_addLFO(gmThread* a_thread);
int GM_CDECL gmf_deleteLFO(gmThread* a_thread);
int GM_CDECL gmf_addAlarm(gmThread* a_thread);
int GM_CDECL gmf_deleteAlarm(gmThread* a_thread);
int GM_CDECL gmf_getAlarm(gmThread* a_thread);

// Ticks every live ALARM (see foe_compat_bindings.cpp's ALARM section
// header comment), firing owner.onAlarm(alarmId) for any that elapsed. ONE
// global registry covers alarms owned by either a Process or an Entity
// handle (owner is stored generically) -- call exactly ONCE per frame
// (ProcessManager::UpdateAll), not also from the entity update loop, or
// elapsed time would double-count.
void TickAlarms(gmMachine& a_machine, float a_deltaMs);

// Shared GameBoard bounds test -- see foe_compat_bindings.cpp's GameBoard
// section header comment for how kGameBoardHalfWidth/Height were derived.
// Exposed so Entity.isInGameBoard/setIsBoundedToGameBoard (entity_bindings.cpp)
// test against the EXACT same bounds GameBoard.isInGameBoard() itself uses.
bool IsPointInGameBoard(float a_x, float a_y);
