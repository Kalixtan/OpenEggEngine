#include "gmThread.h"
#include "savedata_bindings.h"
#include "../core/save_data.h"
#include <cstdio>

namespace {

// Comfortably above every real REQUIRED_QUOTA_* in the corpus (512kb for
// edit, similar order of magnitude for play) -- see header comment.
constexpr int kFreeSpaceKb = 1000000;

int GM_CDECL gmf_SAVEDATA_getSystemFreeSpaceQuota(gmThread* a_thread) {
    a_thread->PushInt(kFreeSpaceKb);
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_getSystemFreeSpaceMedia(gmThread* a_thread) {
    a_thread->PushInt(kFreeSpaceKb);
    return GM_OK;
}

// ---------------------------------------------------------------------
// SAVEDATA_setSaveSystemSafe/doRequest/isRequestDone/processRequest --
// 0x000ada74, 0x000adccc, 0x000adba8; 1-arg/2-arg/0-arg/1-arg respectively,
// matching the decompiled stack-slot reads), and confirmed load-bearing by
// direct usage: game_data/Main.gm's InitGame() drives a real state machine
// through them --
//   SAVESTATE_READY.init()   -> SAVEDATA_setSaveSystemSafe(true)
//   SAVESTATE_SAVING.init()  -> SAVEDATA_setSaveSystemSafe(false);
//                                SAVEDATA_doRequest(null, forceNoSpaceCheck)
//   SAVESTATE_SAVING.update()-> polls SAVEDATA_isRequestDone() each frame
//   SAVESTATE_CHECKERRORS    -> SAVEDATA_processRequest(true) for the error
// -- but the decompiled bodies themselves only reach three internal,
// unnamed FUN_ helpers apiece (FUN_0018e67c + one more each) -- real Sony
// SaveData API glue with no public spec to decompile further, and nothing
// this reimplementation has an equivalent subsystem for (see core/
// save_data.h's header comment: there's no PS3 save-data block here, no
// async ticket queue, just files). What IS fully recoverable, and is what
// actually matters for matching game behavior, is the script-visible
// CONTRACT every real call site above depends on: doRequest() kicks off a
// save that completes essentially immediately (real hardware I/O this
// fast doesn't exist, but the actual bytes are already written by the time
// this runs -- see GameMonkeyScript.saveTableToJsonSaveRequest, which is
// always called BEFORE doRequest() in every real call site, e.g.
// SAVEDATAHELPER_SaveToFile), isRequestDone() reflects that immediately,
// and processRequest() reports "no error" (null) since a local file write
// either already succeeded (in which case there's nothing to report) or
// this reimplementation has no realistic way to simulate a real PS3-style
// write failure to report instead. setSaveSystemSafe is a real flag
// (gates cellSaveDataAutoSave/system-level save interrupts on actual
// hardware) with nothing here to gate -- stored for completeness/parity,
// not currently read back anywhere.
// ---------------------------------------------------------------------

bool g_saveSystemSafe = true;

int GM_CDECL gmf_SAVEDATA_setSaveSystemSafe(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(safe, 0, 0);
    g_saveSystemSafe = (safe != 0);
    return GM_OK;
}

// 2 args (callback, forceNoSpaceCheck) per the decompiled stack reads.
// `callback` -- a GM function, per SAVEDATAHELPER_SaveToFile's real call
// `GAMEUPDATEDATA.startSave(callback, forceOverwrite)`, which is a SEPARATE
// script-side wrapper, not this function directly, and no real call site
// passes doRequest() itself a non-null callback (Main.gm's own
// `SAVEDATA_doRequest(null, parent.forceNoSpaceCheck)` is the only real
// call site) -- accepted for signature-completeness, not invoked (nothing
// asynchronous here for it to be a completion callback FOR).
// `forceNoSpaceCheck` has no observable effect: this reimplementation
// doesn't model running out of local disk space at all, matching
// SAVEDATA_getSystemFreeSpaceQuota/Media's own "always plenty of space"
// stance above.
int GM_CDECL gmf_SAVEDATA_doRequest(gmThread* a_thread) {
    (void)a_thread; // both params accepted-and-ignored, see header comment
    return GM_OK;
}

int GM_CDECL gmf_SAVEDATA_isRequestDone(gmThread* a_thread) {
    a_thread->PushInt(1); // synchronous save path -- always true the instant it's asked
    return GM_OK;
}

int GM_CDECL gmf_SAVEDATA_processRequest(gmThread* a_thread) {
    a_thread->PushNull(); // no error -- see header comment
    return GM_OK;
}

// 0x000adc68), called once per save completion (Main.gm:133
// SAVESTATE_SAVECOMPLETE.init) to reset the request state. Synchronous path
// here means nothing queued to clear -- this is a no-op.
int GM_CDECL gmf_SAVEDATA_clearRequest(gmThread* a_thread) {
    (void)a_thread; // no-op, see header comment
    return GM_OK;
}

// 0x000adff8), no-arg. Real call site (Main.gm:621's `abortLoad()`:
// `SAVEDATA_abortLoad(); while (!SAVEDATA_isRequestDone()) {} // this is
// BULLSHIT`) immediately busy-waits on SAVEDATA_isRequestDone(), which
// this engine's synchronous save path already always answers true the
// instant it's asked -- so the loop exits on its first check regardless
// of what abortLoad itself does. Same real-Sony-API-glue-with-nothing-to-
// model situation as doRequest/clearRequest above.
int GM_CDECL gmf_SAVEDATA_abortLoad(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

} // namespace

void RegisterSaveDataBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("SAVEDATA_getSystemFreeSpaceQuota", gmf_SAVEDATA_getSystemFreeSpaceQuota);
    a_machine.RegisterLibraryFunction("SAVEDATA_getSystemFreeSpaceMedia", gmf_SAVEDATA_getSystemFreeSpaceMedia);
    a_machine.RegisterLibraryFunction("SAVEDATA_setSaveSystemSafe", gmf_SAVEDATA_setSaveSystemSafe);
    a_machine.RegisterLibraryFunction("SAVEDATA_doRequest", gmf_SAVEDATA_doRequest);
    a_machine.RegisterLibraryFunction("SAVEDATA_isRequestDone", gmf_SAVEDATA_isRequestDone);
    a_machine.RegisterLibraryFunction("SAVEDATA_processRequest", gmf_SAVEDATA_processRequest);
    a_machine.RegisterLibraryFunction("SAVEDATA_clearRequest", gmf_SAVEDATA_clearRequest);
    a_machine.RegisterLibraryFunction("SAVEDATA_abortLoad", gmf_SAVEDATA_abortLoad);
}
