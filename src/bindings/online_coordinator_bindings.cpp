#include "gmThread.h"
#include "online_coordinator_bindings.h"

namespace {

int GM_CDECL gmf_IsOnline(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_IsOffline(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_IsDisabled(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_IsConnecting(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_IsReadyGoOffline(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_IsLoggedIntoPlatform(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_HasChatPermissions(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_IsSendQueueFull(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
// LoginPopup.gm/CloudSaveHelper.gm read this as a signed error code
// (`OnlineCoordinator_GetLastError() < 0` means "there was an error");
// 0 is the genuine "no error" value, matching an always-offline-but-not-
// erroring state (this reimplementation never attempts a PSN connection in
// the first place, so there's never a connection error to surface).
int GM_CDECL gmf_GetLastError(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
// CloudSaveHelper.gm treats this as a display/log string ("My Online Id is
// X"); empty string is the honest answer for a player who was never
// actually signed into PSN here.
int GM_CDECL gmf_GetOnlineID(gmThread* a_thread) { a_thread->PushString(a_thread->GetMachine()->AllocStringObject("")); return GM_OK; }
int GM_CDECL gmf_GoOnline(gmThread*) { return GM_OK; }
int GM_CDECL gmf_GoOffline(gmThread*) { return GM_OK; }

} // namespace

void RegisterOnlineCoordinatorBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsOnline", gmf_IsOnline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsOffline", gmf_IsOffline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsDisabled", gmf_IsDisabled);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsConnecting", gmf_IsConnecting);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsReadyGoOffline", gmf_IsReadyGoOffline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsLoggedIntoPlatform", gmf_IsLoggedIntoPlatform);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_HasChatPermissions", gmf_HasChatPermissions);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsSendQueueFull", gmf_IsSendQueueFull);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GetLastError", gmf_GetLastError);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GetOnlineID", gmf_GetOnlineID);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GoOnline", gmf_GoOnline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GoOffline", gmf_GoOffline);
}
