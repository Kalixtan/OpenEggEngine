#pragma once
#include "gmThread.h"
#include "gmMachine.h"
#include "gmTableObject.h"

// ---------------------------------------------------------------------------
// Strict call checking -- the tripwire for the 1-1 reimplementation pass.
// ---------------------------------------------------------------------------
//
// This engine used to fill every hole in its API surface with something that
// let scripts keep running: a `return GM_OK;` no-op, a plausible-looking
// constant, a handle counter, an empty table. That made the game boot, and it
// made it impossible to tell which of the ~1300 engine functions are actually
// retail behavior and which are invention -- a wrong answer and a right one
// are indistinguishable from the outside.
//
// The goal now is that every function matches the real PS3 build, derived from
// swapped from libgcm/RSX to SDL + OpenGL. So the fabricated implementations
// are gone, and calling one is a hard stop rather than a silent wrong answer.
//
// Two ways a call gets caught:
//
//   1. A binding whose fabricated body was removed. Its name is still
//      registered, but bound to a trap thunk (GE3_REMOVED below), so the
//      report knows the exact name, argument count and argument values.
//
//   2. A function that was never bound at all. That call reaches
//      gmThread::PushStackFrame's null-callee path -- which retail
//      deliberately treats as a silent no-op (vendor/GMSCRIPT_PATCHES.md #4)
//      -- and OnNullCall() picks it up from there, recovering the name from
//      the failed lookup where the VM can attribute one.
//
// Either way the report names the function, the .gm file and line that called
// it, the arguments it was given, and -- via the generated retail symbol index
// implementation to port. That last part is the point: the trap is meant to
// hand you the next function to write, not just tell you something broke.
//
// Modes, via the GE3_STRICT environment variable:
//
//   halt   (default) Report the first offending call in full, then exit. This
//                    is "tell me the next function to implement".
//   survey           Report each distinct offender once, do not stop, and
//                    print an ordered summary at exit. Callers get null back,
//                    i.e. the old lenient behavior. This is "show me the whole
//                    work queue in the order the game needs it".
//   off              Restore the pre-strict behavior entirely.
//
namespace StrictCalls {

enum class Mode { Halt, Survey, Off };

// Reads GE3_STRICT once and caches. Default is Halt.
Mode GetMode();

// Installs the VM-side null-call hook. Call once, after the machine exists.
void Install(gmMachine& a_machine);

// Tags a table this engine created (a RegisterLibrary global, an entity/
// process handle or behavior table, or any other native-owned object table)
// so OnNullCall can recognize a missing method on it as a real engine gap
// and report it unconditionally -- no need to already know the method's name
// (contrast with GE3_REMOVED/retail_symbols.inc, which require the gap to be
// named in advance). Script-authored tables are never tagged, so retail's
// documented null-callback leniency (GMSCRIPT_PATCHES.md #4) stays silent
// for those, same as before. Safe to call multiple times on the same table.
void MarkNative(gmMachine& a_machine, gmTableObject* a_table);

// Convenience overload for a gmVariable known to hold a GM_TABLE.
void MarkNative(gmMachine& a_machine, const gmVariable& a_tableVar);

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
