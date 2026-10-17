#include "gmThread.h"
#include "savedata_bindings.h"
#include "../core/save_data.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <cstdio>

namespace {

constexpr int kFreeSpaceKb = 1000000;

int GM_CDECL gmf_SAVEDATA_getSystemFreeSpaceQuota(gmThread* a_thread) {
    a_thread->PushInt(kFreeSpaceKb);
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_getSystemFreeSpaceMedia(gmThread* a_thread) {
    a_thread->PushInt(kFreeSpaceKb);
    return GM_OK;
}


bool g_saveSystemSafe = true;

int GM_CDECL gmf_SAVEDATA_setSaveSystemSafe(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(safe, 0, 0);
    g_saveSystemSafe = (safe != 0);
    return GM_OK;
}

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

int GM_CDECL gmf_SAVEDATA_clearRequest(gmThread* a_thread) {
    (void)a_thread; // no-op, see header comment
    return GM_OK;
}

int GM_CDECL gmf_SAVEDATA_abortLoad(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}


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

int GM_CDECL gmf_SAVEDATA_reqGetFileSize(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), path, SaveData::FileSize(path)));
    return GM_OK;
}

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

int GM_CDECL gmf_SAVEDATA_reqDelete(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    SaveData::DeleteFile(path);
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), path, 0));
    return GM_OK;
}

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

int GM_CDECL gmf_SAVEDATA_SaveAll(gmThread* a_thread) {
    a_thread->PushInt(1);
    return GM_OK;
}

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

int GM_CDECL gmf_SAVEDATA_AddScreenShot(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_reqSaveScreenShot(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), path, 0));
    return GM_OK;
}

int GM_CDECL gmf_SAVEDATA_reqCreateSave(gmThread* a_thread) {
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), "", 0));
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_reqOSFreeSpace(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    a_thread->Push(MakeSizeTicket(*a_thread->GetMachine(), "", 0));
    return GM_OK;
}

int GM_CDECL gmf_SAVEDATA_addTicket(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_calcNeededSpace(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_resetLastRequests(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_showNoSpaceOnMediaDialog(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}
int GM_CDECL gmf_SAVEDATA_showOsMsgDialog(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

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
