#include "gmThread.h"
#include "savedata_bindings.h"
#include "../core/save_data.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
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

// ---------------------------------------------------------------------
// The "ticket" (SaveData request) functions below -- SAVEDATA_reqGetFileSize/
// reqLoadFile/reqDelete/reqSaveFromLoadRequest/reqSaveScreenShot/
// reqCreateSave/reqOSFreeSpace/addTicket/calcNeededSpace/resetLastRequests/
// showNoSpaceOnMediaDialog/showOsMsgDialog/AddString/AddCompressedString/
// AddScreenShot/SaveAll -- were previously entirely unregistered, despite
// corpus call sites include game_data/Main.gm's own boot-time save preload
// (SAVEDATA_reqGetFileSize("save.json")/"myLevels/toc.json" ->
// .getSize() -> SAVEDATA_reqLoadFile(path, size) -> handed to
// GameMonkeyScript.loadTableFromJsonSaveRequest, see gamemonkeyscript_
// bindings.cpp), MyLevelsTOC.gm's file deletion, and SaveDataHelper.gm's
// save-to-disk path. Without these, the game could never load save.json
// or myLevels/toc.json on startup.
//
// Retail's decompiled bodies are almost entirely opaque Sony SaveData
// request-queue plumbing (ref-counted request objects, an async ticket
// system -- see save_data.h's header comment for why none of that is
// worth or able to be reproduced bit-for-bit); what's actually
// reproducible and matters is each function's confirmed arg shape (from
// the arg-count dispatch) and the script-visible contract real call sites
// depend on. A "ticket" here is a small GM table carrying whatever fields
// the real corpus call sites actually read off it (getSize/getFilename),
// consistent with this engine's existing "ticket = a value scripts hand
// back to another function, not a real async handle" convention
// (GameMonkeyScript.saveTableToJsonSaveRequest already uses a bare
// filename string as its ticket for the same reason).
// ---------------------------------------------------------------------

gmVariable g_getSizeFn;
gmVariable g_getFilenameFn;
gmVariable g_getIfDoneFn;
gmVariable g_getIfErrorFn;

bool GetTicketFilename(gmMachine& a_machine, const gmVariable& a_ticket, std::string& a_out) {
    gmVariable filename = a_ticket;
    if (a_ticket.m_type == GM_TABLE) {
        gmTableObject* table = (gmTableObject*)a_machine.GetObject(a_ticket.m_value.m_ref);
        if (!table) return false;
        filename = table->Get(&a_machine, "filename");
    }
    if (filename.m_type != GM_STRING) return false;
    gmStringObject* string = (gmStringObject*)a_machine.GetObject(filename.m_value.m_ref);
    if (!string) return false;
    a_out = string->GetString();
    return true;
}

int GM_CDECL gmf_ticket_getSize(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (self) { a_thread->Push(self->Get(a_thread->GetMachine(), "size")); return GM_OK; }
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_ticket_getFilename(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    if (self) { a_thread->Push(self->Get(a_thread->GetMachine(), "filename")); return GM_OK; }
    a_thread->PushNewString("");
    return GM_OK;
}

int GM_CDECL gmf_ticket_getIfDone(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    a_thread->PushInt(self && self->Get(a_thread->GetMachine(), "done").m_value.m_int != 0);
    return GM_OK;
}
int GM_CDECL gmf_ticket_getIfError(gmThread* a_thread) {
    gmTableObject* self = a_thread->ThisTable();
    a_thread->PushInt(self && self->Get(a_thread->GetMachine(), "error").m_value.m_int != 0);
    return GM_OK;
}

// A ticket exposing .getSize()/.getFilename(), for reqGetFileSize/
// reqOSFreeSpace/reqCreateSave/reqSaveScreenShot-shaped "just give the
// script something with the right methods" returns. size<0 means "file
// doesn't exist" -> real call sites (Main.gm) test getSize() as a bool, so
// stat-style tickets report 0 (falsy) either way. reqLoadFile opts into exact
// requested-size preservation because retail getSize() returns request+0x4c.
gmVariable MakeSizeTicket(gmMachine& a_machine, const std::string& a_filename, long long a_size,
                          bool a_done = true, bool a_error = false,
                          bool a_preserveSize = false) {
    gmTableObject* t = a_machine.AllocTableObject();
    t->Set(&a_machine, "filename", gmVariable(a_machine.AllocStringObject(a_filename.c_str())));
    const long long reportedSize = a_preserveSize ? a_size : (a_size > 0 ? a_size : 0);
    t->Set(&a_machine, "size", gmVariable((gmint)reportedSize));
    t->Set(&a_machine, "done", gmVariable((gmint)(a_done ? 1 : 0)));
    t->Set(&a_machine, "error", gmVariable((gmint)(a_error ? 1 : 0)));
    t->Set(&a_machine, "getSize", g_getSizeFn);
    t->Set(&a_machine, "getFilename", g_getFilenameFn);
    t->Set(&a_machine, "getIfDone", g_getIfDoneFn);
    t->Set(&a_machine, "getIfError", g_getIfErrorFn);
    return gmVariable(t);
}

// SaveData.c @ 0xad234). Real call sites (Main.gm, EditScreenInfoPopupBase.gm,
// MainMenu5*/EditScreen2*.gm) all immediately call .getSize() on the result
// and treat 0/falsy as "file doesn't exist, skip loading it".
int GM_CDECL gmf_SAVEDATA_reqGetFileSize(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), path, SaveData::FileSize(path)));
    return GM_OK;
}

// SaveData.c @ 0xad440), returns native SAVEREQUEST type 52. Retail stores
// the second argument's integer size directly at request+0x4c; when passed a
// SAVEREQUEST it reads that request's +0x4c field instead. `getSize()` at
// 0xae1a0 returns this stored value. Preserve it in the desktop table ticket;
// check file existence separately for the local error flag.
int GM_CDECL gmf_SAVEDATA_reqLoadFile(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(path, 0, "");
    gmMachine* machine = a_thread->GetMachine();
    const gmVariable sizeArg = a_thread->Param(1);
    long long requestedSize = 0;
    if (sizeArg.m_type == GM_INT) {
        requestedSize = sizeArg.m_value.m_int;
    } else if (sizeArg.m_type == GM_TABLE) {
        gmTableObject* sourceTicket = (gmTableObject*)machine->GetObject(sizeArg.m_value.m_ref);
        if (sourceTicket) {
            const gmVariable storedSize = sourceTicket->Get(machine, "size");
            if (storedSize.m_type == GM_INT) requestedSize = storedSize.m_value.m_int;
        }
    }
    const long long fileSize = SaveData::FileSize(path);
    a_thread->Push(MakeSizeTicket(*machine, path, requestedSize, true, fileSize < 0, true));
    return GM_OK;
}

// SaveData.c @ 0xac618). Real call site (MyLevelsTOC.gm:256-258) never
// reads the return value, so it only needs to not crash.
int GM_CDECL gmf_SAVEDATA_reqDelete(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    SaveData::DeleteFile(path);
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), path, 0));
    return GM_OK;
}

// SAVEDATA_reqSaveFromLoadRequest(dstPath, loadTicket) -- confirmed real
// (EditScreenInfoPopupBase.gm) copy a level's data from one TOC slot to
// another (level/image/loop file each copied separately). `loadTicket` is
// whatever SAVEDATA_reqLoadFile returned for the source. Resolve either the
// table-backed desktop request or a legacy string ticket, copy the bytes,
// and return a size ticket for dstPath.
int GM_CDECL gmf_SAVEDATA_reqSaveFromLoadRequest(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(dstPath, 0, "");
    gmVariable srcVar = a_thread->Param(1);
    std::string srcPath;
    if (GetTicketFilename(*a_thread->GetMachine(), srcVar, srcPath)) {
        std::string content;
        if (SaveData::ReadFile(srcPath, content)) {
            SaveData::WriteFile(dstPath, content);
        }
    }
    a_thread->PushNewString(dstPath);
    return GM_OK;
}

// SAVEDATA_reqSaveString(filename, content, [requestMode]) -- retail wrapper
// 0xacb00 converts the first two arguments to strings and returns a typed
// SAVEREQUEST. A third non-null argument selects an alternate request
// constructor; it does not replace either string payload. The desktop save
// API completes requests synchronously, so persist the content now and
// expose the same filename/byte-count/status methods as other tickets.
// Retail's native object and asynchronous request-manager lifecycle remain
// outside this adapter.
int GM_CDECL gmf_SAVEDATA_reqSaveString(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 2) return GM_EXCEPTION;
    GM_STRING_PARAM(filename, 0, "");
    GM_STRING_PARAM(content, 1, "");
    const bool saved = SaveData::WriteFile(filename, content);
    const long long byteCount = static_cast<long long>(
        std::char_traits<char>::length(content));
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), filename,
                                  byteCount, true, !saved, true));
    return GM_OK;
}

// classes/SaveData.c @ 0xac588). Real call sites (DebugMenuNew.gm, Main.gm's
// `saveSuccessful = SAVEDATA_SaveAll()`) treat the return as success/fail;
// this engine's writes (via AddString/AddCompressedString below, and
// GameMonkeyScript.saveTableToJsonSaveRequest for the primary save.json
// path) already happened synchronously by the time this is called, so
// always true.
int GM_CDECL gmf_SAVEDATA_SaveAll(gmThread* a_thread) {
    a_thread->PushInt(1);
    return GM_OK;
}

// SAVEDATA_AddString(filename, content, fileArea=1) / AddCompressedString --
// is a plain int, not unwrapped through a String -- unlike arg0/arg1 --
// confirming it's a numeric "area" flag, not a string; defaults to 1 when
// omitted). Real call sites (Main.gm's level-save flow) always pass
// already-serialized JSON text. `fileArea` and the "compressed" distinction
// both have no local-file equivalent (see save_data.h's header comment --
// this is plain text on disk, nothing to compress into/segregate by area
// for); both write the content as-is. Return is bool success, matching the
// confirmed int return tag.
int GM_CDECL gmf_SAVEDATA_AddString(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    GM_STRING_PARAM(filename, 0, "");
    GM_STRING_PARAM(content, 1, "");
    a_thread->PushInt(SaveData::WriteFile(filename, content) ? 1 : 0);
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_AddCompressedString(gmThread* a_thread) {
    return gmf_SAVEDATA_AddString(a_thread);
}

// SAVEDATA_AddScreenShot(filename, fileArea, sizeDivisor) -- confirmed real
// return value at all (unlike AddString/AddCompressedString above), so
// retail scripts never check its result -- matches Main.gm's own call site,
// which doesn't use one either. No screenshot pixel buffer is available at
// this binding layer (that's the renderer's -- out of this scope's files),
// so this is a documented no-op rather than a guess at capturing one.
int GM_CDECL gmf_SAVEDATA_AddScreenShot(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// SAVEDATA_reqSaveScreenShot(filename, sizeDivisor) -- confirmed real
// here" situation as AddScreenShot above. Real call site (Main.gm:3839)
// only stores the ticket to poll later via the shared doRequest/
// isRequestDone flow, never reads a field off it directly, so an empty
// ticket satisfies the contract.
int GM_CDECL gmf_SAVEDATA_reqSaveScreenShot(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), path, 0));
    return GM_OK;
}

// classes/SaveData.c @ 0xad704/0xad7c4), both real corpus call sites
// (SaveErrors.gm, all commented-out/dead code in this corpus -- see that
// file) never read the returned ticket's fields, so an empty ticket is
// enough.
int GM_CDECL gmf_SAVEDATA_reqCreateSave(gmThread* a_thread) {
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), "", 0));
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_reqOSFreeSpace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), "", 0));
    return GM_OK;
}

// SAVEDATA_addTicket (decompiled name: SAVEDATA_addTicketReq) -- confirmed
// pending-request list Sony's real API tracks internally. This engine's
// save path is synchronous (every ticket above already did its real work
// immediately), so there's nothing to queue.
int GM_CDECL gmf_SAVEDATA_addTicket(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// classes/SaveData.c @ 0xad9e8). No corpus call site is live (SaveErrors.gm's
// only reference is commented out) -- 0 matches this engine's "always
// plenty of space" stance (see kFreeSpaceKb above).
int GM_CDECL gmf_SAVEDATA_calcNeededSpace(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}
// SAVEDATA_resetLastRequests() -- confirmed real 0-arg, no return value
// SaveErrors.gm) just clear retail's internal pending-request list before
// starting a fresh batch; nothing queued here to clear.
int GM_CDECL gmf_SAVEDATA_resetLastRequests(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
// SAVEDATA_showNoSpaceOnMediaDialog(msg)/showOsMsgDialog(msgType) --
// with no arg-count dispatch, 0 params, and never even reads gmThread's arg
// slots). Both are real PS3 system-UI dialogs (LoginPopup.gm's real call
// site passes an SCE_MSG_DIALOG_SYSMSG_TYPE_* constant) this engine has no
// equivalent on-screen system for -- matching this codebase's existing
// stance on other unmodeled OS dialogs, a no-op rather than a guessed UI.
int GM_CDECL gmf_SAVEDATA_showNoSpaceOnMediaDialog(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_showOsMsgDialog(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

// DIALOG_SaveDialogUsrMsg(msg, ...) -- the retail wrapper runs a value
// formatter, then calls FUN_0017dda0, whose complete PPC body is `li r3,0;
// blr`. It always returns GM_INT(0); the located game-data calls are all
// commented out. Keep the global callable with the observed return tag
// without inventing a desktop dialog. See Dialog.c @ 0xac17c.
int GM_CDECL gmf_Dialog_SaveDialogUsrMsg(gmThread* a_thread) {
    if (a_thread->GetNumParams() < 1) return GM_EXCEPTION;
    a_thread->PushInt(0);
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

    // Shared, stateless ticket-method function objects (see MakeSizeTicket).
    g_getSizeFn = gmVariable(a_machine.AllocFunctionObject(gmf_ticket_getSize));
    g_getFilenameFn = gmVariable(a_machine.AllocFunctionObject(gmf_ticket_getFilename));
    g_getIfDoneFn = gmVariable(a_machine.AllocFunctionObject(gmf_ticket_getIfDone));
    g_getIfErrorFn = gmVariable(a_machine.AllocFunctionObject(gmf_ticket_getIfError));

    a_machine.RegisterLibraryFunction("SAVEDATA_reqGetFileSize", gmf_SAVEDATA_reqGetFileSize);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqLoadFile", gmf_SAVEDATA_reqLoadFile);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqDelete", gmf_SAVEDATA_reqDelete);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqSaveFromLoadRequest", gmf_SAVEDATA_reqSaveFromLoadRequest);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqSaveString", gmf_SAVEDATA_reqSaveString);
    a_machine.RegisterLibraryFunction("SAVEDATA_SaveAll", gmf_SAVEDATA_SaveAll);
    a_machine.RegisterLibraryFunction("SAVEDATA_AddString", gmf_SAVEDATA_AddString);
    a_machine.RegisterLibraryFunction("SAVEDATA_AddCompressedString", gmf_SAVEDATA_AddCompressedString);
    a_machine.RegisterLibraryFunction("SAVEDATA_AddScreenShot", gmf_SAVEDATA_AddScreenShot);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqSaveScreenShot", gmf_SAVEDATA_reqSaveScreenShot);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqCreateSave", gmf_SAVEDATA_reqCreateSave);
    a_machine.RegisterLibraryFunction("SAVEDATA_reqOSFreeSpace", gmf_SAVEDATA_reqOSFreeSpace);
    a_machine.RegisterLibraryFunction("SAVEDATA_addTicket", gmf_SAVEDATA_addTicket);
    a_machine.RegisterLibraryFunction("SAVEDATA_calcNeededSpace", gmf_SAVEDATA_calcNeededSpace);
    a_machine.RegisterLibraryFunction("SAVEDATA_resetLastRequests", gmf_SAVEDATA_resetLastRequests);
    a_machine.RegisterLibraryFunction("SAVEDATA_showNoSpaceOnMediaDialog", gmf_SAVEDATA_showNoSpaceOnMediaDialog);
    a_machine.RegisterLibraryFunction("SAVEDATA_showOsMsgDialog", gmf_SAVEDATA_showOsMsgDialog);
    a_machine.RegisterLibraryFunction("DIALOG_SaveDialogUsrMsg", gmf_Dialog_SaveDialogUsrMsg);
}
