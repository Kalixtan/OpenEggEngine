#pragma once
#include "gmMachine.h"

// addresses 0x001b1ca8-0x001b2218), a family of 12 bare global functions
// (no dot -- called as `OnlineCoordinator_IsOnline()`, not
// `OnlineCoordinator.IsOnline()`) wrapping the PS3's PSN/Network Platform
// session state. Every decompiled body just marshals into an opaque PSN SDK
// call (FUN_0019e454 and friends, reading a global session struct at
// DAT_00f87b70) -- there is no portable 1:1 body here, and nothing to port
// it TO: this reimplementation has no PSN backend, no matchmaking server,
// no leaderboard service. Implemented as permanently-offline stubs instead
// of a GE3_REMOVED trap, matching retail's own real fallback behavior when
// PSN is unreachable (Main.gm's ONLINESTATE_OFFLINE state, which every one
// of these calls gates, is built to run forever in that branch already).
// confirmed PSN/PS Store/PS Vita Near functions) with real corpus call
// sites -- same "no online backend, permanently-offline stub" stance, see
// online_coordinator_bindings.cpp for the per-function reasoning.
void RegisterOnlineCoordinatorBindings(gmMachine& a_machine);
