#pragma once
#include "gmThread.h"
#include "gmMachine.h"
#include "gmTableObject.h"

namespace StrictCalls {

enum class Mode { Halt, Survey, Off };
using HaltDialogHandler = bool (*)(void* context, const char* report);

// The desktop frontend may show a Continue/Quit dialog for a strict halt.
// Returning true means continue with the legacy null fallback.
void SetHaltDialogHandler(HaltDialogHandler handler, void* context);

// Reads GE3_STRICT once and caches. Default is Halt.
Mode GetMode();

// Installs the VM-side null-call hook. Call once, after the machine exists.
void Install(gmMachine& a_machine);

void MarkNative(gmMachine& a_machine, gmTableObject* a_table);

// Convenience overload for a gmVariable known to hold a GM_TABLE.
void MarkNative(gmMachine& a_machine, const gmVariable& a_tableVar);

// Entity and process handles are native-owned tables that scripts extend
// with callbacks. An unknown missing member on one of these may be an
// optional script callback; only indexed retail native calls are reported.
void MarkScriptExtensibleNative(gmMachine& a_machine, gmTableObject* a_table);
void MarkScriptExtensibleNative(gmMachine& a_machine, const gmVariable& a_tableVar);

// Body of a binding whose fabricated implementation was removed.
// a_name is the script-visible name, qualified where it lives on a library or
// type table ("Video.setTexture", "drawList.plotVertex", "SAVEDATA_getSystemFreeSpaceQuota").
// In Survey mode this pushes null and returns GM_OK so the run continues.
int Report(gmThread* a_thread, const char* a_name);

// Prints the survey summary. No-op outside Survey mode.
void DumpSurvey();

} // namespace StrictCalls

// Defines the binding body for a removed function. One line per function, so
// the registration tables keep their full, real API surface -- the names still
// document what retail exposes, they just no longer pretend to implement it.
//
//   GE3_REMOVED(gmf_Video_setTexture, "Video.setTexture")
#define GE3_REMOVED(ident, qualifiedName)                       \
    int GM_CDECL ident(gmThread* a_thread) {                    \
        return ::StrictCalls::Report(a_thread, qualifiedName);  \
    }
