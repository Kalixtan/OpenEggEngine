#pragma once
#include "gmMachine.h"

// SAVEDATA_* -- confirmed real bare-global native family (game_data/
// Platformer/SaveDataHelper.gm: `SAVEDATA_getSystemFreeSpaceQuota()`,
// `SAVEDATA_getSystemFreeSpaceMedia()`, both compared in kb against
// `REQUIRED_QUOTA_TO_EDIT`/`REQUIRED_QUOTA_TO_PLAY`, game_data/Main.gm:14
// `REQUIRED_QUOTA_TO_EDIT = 512`) -- none of this family was registered at
// all, so every call silently returned null/0 via gmThread's undefined-
// global-call leniency (same mechanism noted in process_base_bindings.h),
// making every `getSystemFreeSpace*() < REQUIRED_QUOTA_*` check a false
// positive on literally every save attempt (confirmed real-world impact:
// the resulting "no virtual space!!" GenericOkDialogProcess popup fired on
// every boot, e.g. saving the calibration screen's scale setting, and only
// became VISIBLE once ProcessManager::PostDrawAll was wired up -- before
// that this whole codepath was already running every time, just silently,
// since popups draw in postDraw() too). This reimplementation has no real
// PS3 memory-card storage to query and no plausible way to run out of it
// on a PC filesystem, so both getters return a large, fixed generous value
// -- enough to clear every real REQUIRED_QUOTA_* comparison in the corpus
// -- rather than modelling actual free-space accounting.
//
// Also registers the SAVEDATA_setSaveSystemSafe/doRequest/isRequestDone/
// processRequest state-machine quartet game_data/Main.gm's InitGame()
// drives directly (see savedata_bindings.cpp for the call-by-call
// evidence) -- real save reads/writes go through GameMonkeyScript.
// {save,load}TableFromJsonSaveRequest (gamemonkeyscript_bindings.cpp),
// backed by real JSON files under core/save_data.h's SaveData::Dir().
// The rest of the SAVEDATA_* family (reqOSFreeSpace/addTicketReq/
// reqSaveScreenShot/etc) stays unregistered: none of it is reached by any
// real call site once the no-space error path above is prevented from
// triggering and the live (non-dead) save/load path is implemented.
void RegisterSaveDataBindings(gmMachine& a_machine);
