#pragma once
#include "gmMachine.h"

// Watermelon_* / WatermelonGlobals_* -- confirmed real (Ghidra,
// "Watermelon" community-level web service client (queasy server: publish/
// browse/rate/heart user levels, leaderboards, cloud saves, news feed).
// 69 bare global functions.
//
// Split into two groups here:
//  - LOCAL helpers (string/score/query-string manipulation) -- these have
//    fully or mostly recoverable bodies and real single-player call sites
//    (WinScreenProcessBase.gm packs scores on every level completion), so
//    they are implemented with real semantics.
//  - SERVER requests -- every body just formats an HTTP request against the
//    long-dead Watermelon backend and schedules an async GM callback. There
//    is no backend to talk to; these are permanently-offline stubs that
//    return 0 ("request not started") and never invoke the callback,
//    matching retail's own behavior with no network connection. Each stub's
//    comment documents the real request shape for whoever revives the
//    online layer later.
//
// Watermelon_Prepare_Screenshot / Watermelon_Clear_Prepared_Screenshot are
// registered by RegisterOnlineCoordinatorBindings (predates this file).
void RegisterWatermelonBindings(gmMachine& a_machine);
