#pragma once

// Patches gaps between the two real, unmodified data sources the campaign
// menu depends on -- see save_data_completion.cpp's header comment for the
// full story. Safe to call every frame: no-ops instantly once SAVE_DATA
// isn't there yet or has already been patched.
void EnsureCampaignSaveDataComplete();
