#include "gmThread.h"
#include "online_coordinator_bindings.h"
#include "gmTableObject.h"
#include "gmStringObject.h"
#include <cstdio>

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

// ---------------------------------------------------------------------
// were registered anywhere. Every one of them is a PSN/PS Store/PS Vita
// Near online-platform feature with no real backend to talk to here, same
// "always offline" stance as OnlineCoordinator above -- only the ~10 with
// real corpus call sites are implemented (the rest, mostly PS Store/DLC/
// WebBrowser-is-running-state queries, have zero real call sites and would
// be pure speculation). All are safe no-ops / honest "nothing happened"
// values; none of their real call sites check a return value beyond what's
// noted per-function below.
// ---------------------------------------------------------------------

// Network_TrophyUnlock(trophyId) -- confirmed real 1-arg, int return
// WinScreenProcessBase.gm, TrophyWinProcess.gm, SaveDataHelper.gm), none
// check the return value. No real trophy backend here; 0 (matching this
// codebase's "no error" convention elsewhere, e.g. OnlineCoordinator_
// GetLastError above) is a safe stand-in for "unlock succeeded".
int GM_CDECL gmf_Network_TrophyUnlock(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_INT_PARAM(trophyId, 0, 0);
    std::fprintf(stderr, "[Network_TrophyUnlock] trophy %d (no-op, no trophy backend)\n", trophyId);
    a_thread->PushInt(0);
    return GM_OK;
}
// Network_TrophySetup() -- confirmed real 0-arg action, no return value.
int GM_CDECL gmf_Network_TrophySetup(gmThread* a_thread) { (void)a_thread; return GM_OK; }
// Network_GetIsTrophyQueueFull() -- confirmed real bool getter; never full
// since nothing is ever actually queued.
int GM_CDECL gmf_Network_GetIsTrophyQueueFull(gmThread* a_thread) { a_thread->PushInt(0); return GM_OK; }

// Network_ActivityStreamPost(messageTable, url[, icon1, icon2, icon3]) --
// confirmed real 2-6 arg action, no return value read by any real call
// site (10 call sites: CommunityLevelInfoPopupTableBase.gm, WinScreen*.gm,
// PauseProcessBase.gm, EditScreenSidePanel.gm, CommunityScreenSidePanel.gm
// -- all "share this to the activity feed" fire-and-forget calls). No real
// PSN activity feed here; logged and dropped.
int GM_CDECL gmf_Network_ActivityStreamPost(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(2);
    std::fprintf(stderr, "[Network_ActivityStreamPost] (no-op, no activity feed backend)\n");
    return GM_OK;
}

// Network_WebBrowserLaunch(url) -- confirmed real 1-arg, bool return
// SidePanel.gm), return value not checked. Logged; no in-game browser here.
int GM_CDECL gmf_Network_WebBrowserLaunch(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(url, 0, "");
    std::fprintf(stderr, "[Network_WebBrowserLaunch] %s (no-op, no in-game browser)\n", url);
    a_thread->PushInt(0);
    return GM_OK;
}

// Network_NearDeleteGift(giftId) -- confirmed real 1-arg action, no return
// value. Network_NearGetExternalGiftDetails(giftId, resultsTable) --
// confirmed real 2-arg, returns a table with "id"/"description" and a 3rd
// field (name unconfirmed exactly, DAT_ label only) populated from a real
// PS Vita Near gift lookup. No real Near backend here (WatermelonHelperNew.gm's
// 1 real call site for each is part of a PS Vita local-social feature this
// PC/PS3 reimplementation has nothing to back); returns empty strings.
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

// Network_LiveAreaReplaceFrame(frameTable, target) -- confirmed real
// 2-arg, and confirmed to ALWAYS return true (1) unconditionally in retail
// decompiled body). A PS Vita LiveArea (home-screen widget) feature with
// no equivalent here; 1 real call site (Main.gm), return value not
// checked either way.
int GM_CDECL gmf_Network_LiveAreaReplaceFrame(gmThread* a_thread) {
    (void)a_thread;
    a_thread->PushInt(1);
    return GM_OK;
}

// Network_GetNetworkLanguageCode() -- confirmed real 0-arg, STRING return
// (a locale name like "en-US", not literally a numeric "code" despite the
// (localeName="")/ClearNetworkLanguageFont() -- confirmed real, no return
// value, set/reset a system font by locale for network-UI text. No system
// font subsystem here; Get returns "en", Set/Clear are no-ops.
int GM_CDECL gmf_Network_GetNetworkLanguageCode(gmThread* a_thread) {
    a_thread->PushNewString("en");
    return GM_OK;
}
int GM_CDECL gmf_Network_SetNetworkLanguageFont(gmThread* a_thread) { (void)a_thread; return GM_OK; }
int GM_CDECL gmf_Network_ClearNetworkLanguageFont(gmThread* a_thread) { (void)a_thread; return GM_OK; }

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

    a_machine.RegisterLibraryFunction("Network_TrophyUnlock", gmf_Network_TrophyUnlock);
    a_machine.RegisterLibraryFunction("Network_TrophySetup", gmf_Network_TrophySetup);
    a_machine.RegisterLibraryFunction("Network_GetIsTrophyQueueFull", gmf_Network_GetIsTrophyQueueFull);
    a_machine.RegisterLibraryFunction("Network_ActivityStreamPost", gmf_Network_ActivityStreamPost);
    a_machine.RegisterLibraryFunction("Network_WebBrowserLaunch", gmf_Network_WebBrowserLaunch);
    a_machine.RegisterLibraryFunction("Network_NearDeleteGift", gmf_Network_NearDeleteGift);
    a_machine.RegisterLibraryFunction("Network_NearGetExternalGiftDetails", gmf_Network_NearGetExternalGiftDetails);
    a_machine.RegisterLibraryFunction("Network_LiveAreaReplaceFrame", gmf_Network_LiveAreaReplaceFrame);
    a_machine.RegisterLibraryFunction("Network_GetNetworkLanguageCode", gmf_Network_GetNetworkLanguageCode);
    a_machine.RegisterLibraryFunction("Network_SetNetworkLanguageFont", gmf_Network_SetNetworkLanguageFont);
    a_machine.RegisterLibraryFunction("Network_ClearNetworkLanguageFont", gmf_Network_ClearNetworkLanguageFont);
}
