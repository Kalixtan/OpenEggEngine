#include "gmThread.h"
#include "online_coordinator_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include "../core/system_config.h"
#include <cstdio>
#include <cstdlib>

namespace {

int GM_CDECL gmf_IsOnline(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_IsOffline(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_IsDisabled(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_IsConnecting(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_IsReadyGoOffline(gmThread* a_thread) { a_thread->PushInt(1); return GM_OK; }
int GM_CDECL gmf_IsLoggedIntoPlatform(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_HasChatPermissions(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
int GM_CDECL gmf_IsSendQueueFull(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }
// Script-only compatibility shim used by LoginPopup.gm and the offline
// Community launcher. It is not exported by retail's OnlineCoordinator API;
// no desktop EULA handshake exists, so the wait state is always false.
int GM_CDECL gmf_IsWaitingForEulaAcceptance(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}
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


// Network_Store* -- desktop has no PlayStation Store service. These preserve
// the retail wrapper return types so archived store/DLC scripts can cleanly
// take their unavailable/error path if one is reached. RunStoreProcess.gm
// treats nonzero GetLastError as a failed store request and checkout result 2
// as an error; product enumeration is empty and checkout/download/install
// state remains false. The stock Music Shop follows its offline login failure
// UI; PopupProcess's native configure boundary supplies a no-op only for the
// empty LoginPopup error callback. No store request runs.
int GM_CDECL gmf_NetworkStoreUnavailable(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_NetworkStoreNoOp(gmThread*) { return GM_OK; }
int GM_CDECL gmf_NetworkStoreSetIconPosition(gmThread*) { return GM_OK; }
int GM_CDECL gmf_NetworkStoreGetLastError(gmThread* a_thread) {
    a_thread->PushInt(1);
    return GM_OK;
}
int GM_CDECL gmf_NetworkStoreGetNumProducts(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_NetworkStoreGetCheckoutResult(gmThread* a_thread) {
    a_thread->PushInt(2);
    return GM_OK;
}
int GM_CDECL gmf_NetworkStoreGetLongDescription(gmThread* a_thread) {
    a_thread->PushString(a_thread->GetMachine()->AllocStringObject(""));
    return GM_OK;
}
int GM_CDECL gmf_NetworkStoreGetProductTable(gmThread* a_thread) {
    gmMachine& machine = *a_thread->GetMachine();
    gmTableObject* product = machine.AllocTableObject();
    product->Set(&machine, "productId", gmVariable(machine.AllocStringObject("")));
    product->Set(&machine, "skuId", gmVariable(machine.AllocStringObject("")));
    product->Set(&machine, "name", gmVariable(machine.AllocStringObject("")));
    product->Set(&machine, "price", gmVariable(machine.AllocStringObject("")));
    product->Set(&machine, "shortDesc", gmVariable(machine.AllocStringObject("")));
    product->Set(&machine, "purchased", gmVariable((gmint)0));
    a_thread->PushTable(product);
    return GM_OK;
}

// Complete the retail Network wrapper table even when the current archive
// does not call a platform-specific method. These APIs depend on PSN, the
// PS3 browser, Vita Near, Facebook, or platform SNS. No desktop backend is
// available, so getters/request actions report unavailable and fire-and-
// forget actions are inert. Keeping the symbols registered lets archived
// platform branches fail closed rather than raising an unknown-native error.
int GM_CDECL gmf_NetworkUnavailable(gmThread* a_thread) {
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_NetworkNoOp(gmThread*) { return GM_OK; }

int GM_CDECL gmf_Network_TrophyUnlock(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(trophyId, 0, 0);
    std::fprintf(stderr, "[Network_TrophyUnlock] trophy %d (no-op, no trophy backend)\n", trophyId);
    a_thread->PushInt(0);
    return GM_OK;
}
int GM_CDECL gmf_Network_TrophySetup(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Network_GetIsTrophyQueueFull(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }

int GM_CDECL gmf_Network_ActivityStreamPost(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    std::fprintf(stderr, "[Network_ActivityStreamPost] (no-op, no activity feed backend)\n");
    return GM_OK;
}

int GM_CDECL gmf_Network_WebBrowserLaunch(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(url, 0, "");
    std::fprintf(stderr, "[Network_WebBrowserLaunch] %s (no-op, no in-game browser)\n", url);
    a_thread->PushInt(0);
    return GM_OK;
}

int GM_CDECL gmf_Network_NearDeleteGift(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Network_NearGetExternalGiftDetails(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* out = m.AllocTableObject();
    out->Set(&m, "id", gmVariable(m.AllocStringObject("")));
    out->Set(&m, "description", gmVariable(m.AllocStringObject("")));
    out->Set(&m, "message", gmVariable(m.AllocStringObject("")));
    a_thread->PushTable(out);
    return GM_OK;
}

int GM_CDECL gmf_Network_LiveAreaReplaceFrame(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(1);
    return GM_OK;
}

int GM_CDECL gmf_Network_GetNetworkLanguageCode(gmThread* a_thread) {
    a_thread->PushNewString("en");
    return GM_OK;
}
int GM_CDECL gmf_Network_SetNetworkLanguageFont(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Network_ClearNetworkLanguageFont(gmThread* a_thread) { (void)a_thread; return GM_OK; }

// The retail DLC helper uses these during normal boot and in entity-menu
// construction. Retail checks PSN EDAT entitlements; this desktop build has
// no PSN DRM backend. A local INI debug switch can report the known pack IDs
// as available for development, but does not mount or supply DLC assets.
int GM_CDECL gmf_Network_RunDLCCheck(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(0);
    return GM_OK;
}
int GM_CDECL gmf_Network_IsDLCCheckComplete(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(0);
    a_thread->PushInt(1);
    return GM_OK;
}
int GM_CDECL gmf_Network_IsDLCAvailable(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(dlcPack, 0, "");
    // The opt-in local development setting emulates the entitlement result
    // for the retail pack IDs (DLCHelper.gm: "1" through "15"). It is off
    // by default and does not fetch, mount, or supply DLC assets.
    bool available = false;
    if (SystemConfig::Get().debugAllDlcAvailable && dlcPack[0] != '\0') {
        char* end = nullptr;
        const long pack = std::strtol(dlcPack, &end, 10);
        available = end != dlcPack && *end == '\0' && pack >= 1 && pack <= 15;
    }
    a_thread->PushInt(available ? 1 : 0);
    return GM_OK;
}

} // namespace

void RegisterOnlineCoordinatorBindings(gmMachine& a_machine) {
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsOnline", gmf_IsOnline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsOffline", gmf_IsOffline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsDisabled", gmf_IsDisabled);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsConnecting", gmf_IsConnecting);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsWaitingForEulaAcceptance",
                                      gmf_IsWaitingForEulaAcceptance);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsReadyGoOffline", gmf_IsReadyGoOffline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsLoggedIntoPlatform", gmf_IsLoggedIntoPlatform);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_HasChatPermissions", gmf_HasChatPermissions);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_IsSendQueueFull", gmf_IsSendQueueFull);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GetLastError", gmf_GetLastError);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GetOnlineID", gmf_GetOnlineID);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GoOnline", gmf_GoOnline);
    a_machine.RegisterLibraryFunction("OnlineCoordinator_GoOffline", gmf_GoOffline);

    a_machine.RegisterLibraryFunction("Network_TrophyUnlock", gmf_Network_TrophyUnlock);
    a_machine.RegisterLibraryFunction("Network_TrophySetup", gmf_Network_TrophySetup);
    a_machine.RegisterLibraryFunction("Network_GetIsTrophyQueueFull", gmf_Network_GetIsTrophyQueueFull);
    a_machine.RegisterLibraryFunction("Network_IsSignedIn", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_IsOnline", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_IsAccountRestricted", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_IsChatRestricted", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_ActivityStreamPost", gmf_Network_ActivityStreamPost);
    a_machine.RegisterLibraryFunction("Network_WebBrowserLaunch", gmf_Network_WebBrowserLaunch);
    a_machine.RegisterLibraryFunction("Network_WebBrowserIsAvailable", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_WebBrowserIsRunning", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_WebBrowserFinish", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_NearLaunchApp", gmf_NetworkNoOp);
    a_machine.RegisterLibraryFunction("Network_NearSendGift", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_NearDeleteGift", gmf_Network_NearDeleteGift);
    a_machine.RegisterLibraryFunction("Network_NearGetExternalGiftDetails", gmf_Network_NearGetExternalGiftDetails);
    a_machine.RegisterLibraryFunction("Network_LiveAreaReplaceFrame", gmf_Network_LiveAreaReplaceFrame);
    a_machine.RegisterLibraryFunction("Network_GetNetworkLanguageCode", gmf_Network_GetNetworkLanguageCode);
    a_machine.RegisterLibraryFunction("Network_SetNetworkLanguageFont", gmf_Network_SetNetworkLanguageFont);
    a_machine.RegisterLibraryFunction("Network_ClearNetworkLanguageFont", gmf_Network_ClearNetworkLanguageFont);
    a_machine.RegisterLibraryFunction("Network_PostLevelPublishedToFb", gmf_NetworkNoOp);
    a_machine.RegisterLibraryFunction("Network_GetServiceSnsState", gmf_NetworkUnavailable);
    a_machine.RegisterLibraryFunction("Network_RunDLCCheck", gmf_Network_RunDLCCheck);
    a_machine.RegisterLibraryFunction("Network_IsDLCCheckComplete", gmf_Network_IsDLCCheckComplete);
    a_machine.RegisterLibraryFunction("Network_IsDLCAvailable", gmf_Network_IsDLCAvailable);

    a_machine.RegisterLibraryFunction("Network_StoreIsAvailable", gmf_NetworkStoreUnavailable);
    a_machine.RegisterLibraryFunction("Network_StoreIsRunning", gmf_NetworkStoreUnavailable);
    a_machine.RegisterLibraryFunction("Network_StoreLaunch", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreFinish", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreIsInitialized", gmf_NetworkStoreUnavailable);
    a_machine.RegisterLibraryFunction("Network_StoreGetNumProducts", gmf_NetworkStoreGetNumProducts);
    a_machine.RegisterLibraryFunction("Network_StoreGetLastError", gmf_NetworkStoreGetLastError);
    a_machine.RegisterLibraryFunction("Network_StoreGetLongDescription", gmf_NetworkStoreGetLongDescription);
    a_machine.RegisterLibraryFunction("Network_StoreBeginCheckout", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreIsCheckoutRunning", gmf_NetworkStoreUnavailable);
    a_machine.RegisterLibraryFunction("Network_StoreGetCheckoutResult", gmf_NetworkStoreGetCheckoutResult);
    a_machine.RegisterLibraryFunction("Network_StoreEndCheckout", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreBeginDownload", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreEndDownload", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreGetProductTable", gmf_NetworkStoreGetProductTable);
    a_machine.RegisterLibraryFunction("Network_StoreSetIconPosition", gmf_NetworkStoreSetIconPosition);
    a_machine.RegisterLibraryFunction("Network_StoreGetIsDownloadFinished", gmf_NetworkStoreUnavailable);
    a_machine.RegisterLibraryFunction("Network_StoreGetHasContentToInstall", gmf_NetworkStoreUnavailable);
    a_machine.RegisterLibraryFunction("Network_StoreBeginInstallContent", gmf_NetworkStoreNoOp);
    a_machine.RegisterLibraryFunction("Network_StoreEndInstallContent", gmf_NetworkStoreNoOp);
    // One patched script contains this misspelling; keep it harmless too.
    a_machine.RegisterLibraryFunction("NetNetwork_StoreIsCheckoutRunning", gmf_NetworkStoreUnavailable);
}
