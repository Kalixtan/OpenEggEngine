#pragma once

#include "gmMachine.h"

class gmThread;

void RegisterScriptCompatBindings(gmMachine& a_machine);

int GM_CDECL gmf_getUniqueAlarmID(gmThread* a_thread);
int GM_CDECL gmf_getUniqueLFOID(gmThread* a_thread);
int GM_CDECL gmf_addLFO(gmThread* a_thread);
int GM_CDECL gmf_deleteLFO(gmThread* a_thread);
int GM_CDECL gmf_addAlarm(gmThread* a_thread);
int GM_CDECL gmf_deleteAlarm(gmThread* a_thread);
int GM_CDECL gmf_getAlarm(gmThread* a_thread);

// Advance registered alarms once during each process-manager update.
void TickAlarms(gmMachine& a_machine, float a_deltaMs);

// Test whether a point lies within the script-visible game board bounds.
bool IsPointInGameBoard(float a_x, float a_y);
