#pragma once
#include "gmMachine.h"

gmVariable CreateProcessHandle(gmMachine& a_machine, int a_processId);

// Sets `__id` and fills in missing native methods on the given table -- used
// both for the `p` handle above and (via process_base_bindings.h's
// InjectProcessBaseMethods) for a process's own factory-returned table.
// Script-authored methods already present on the behavior table keep
// precedence. Real scripts call `.isInited()`/etc directly on that table too,
// not only through a stored `.Process` handle. See process_base_bindings.h.
void InjectProcessHandleMethods(gmMachine& a_machine, gmTableObject* a_table, int a_processId);

// Registers GameSystem.setAsPauseProcess(active, process) onto the existing
// GameSystem library table. Call after RegisterGameSystemBindings.
void RegisterProcessHandleBindings(gmMachine& a_machine);

// Return the persistent parameter-state key for a native SHADER user object.
// Negative keys keep these distinct from Entity's nonnegative object IDs.
bool GetShaderStorageId(gmMachine& a_machine, const gmVariable& a_value, int& a_id);

// Construct the SHADER user type returned by Video.getDefaultPostProcessShader.
// Retail wraps the Video-owned native shader pointer in a native SHADER
// object; the retail Video constructor currently leaves that pointer null.
gmVariable CreateDefaultPostProcessShaderHandle(gmMachine& a_machine);
