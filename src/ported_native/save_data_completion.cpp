#include "save_data_completion.h"
#include "../core/engine_context.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "rapidjson/document.h"
#include <cstdio>

// This engine's boot sequence and Platformer/SaveDataHelper.gm are both
// unmodified, real retail code -- but they draw from two DIFFERENT real
// data sources for the campaign album list, and those two sources disagree:
//
//   - Platformer/recordList.gm's global RECORD_LIST drives
//     SAVEDATAHELPER_UpdateRecords(), which is what actually seeds
//     saveData.campaign.asides[recordName][baseFilename] the first time a
//     save is created (SAVEDATAHELPER_GetSaveData() -> MakeNewSave() ->
//     UpdateRecords()). It only knows about 6 records (VicLand,
//     superbrothers, Colin, pixeljam, Beck, Tutorials) -- "Credits" is
//     present but explicitly commented out, and "Trophy Challenges" isn't
//     in the file at all.
//
//   - Platformer/MainMenu5/recordList.json is what
//     CampaignScreenAlbumSelect.gm's loadRecords() actually reads to build
//     the on-screen album list (the file's own commented-out "NEW SHIT"
//     block shows retail deliberately chose the JSON source over
//     RECORD_LIST.gm for display). It has 8 records -- the same 6 plus
//     "Trophy Challenges" and "Credits" -- and its Tutorials entry uses the
//     bare basename "editorTutorial", while RECORD_LIST.gm computes
//     "editorTutorial"+RECORDHELPER_GetTutorialPostfix(), which is
//     "editorTutorialPS3" on this engine's simulated platform (OSID_PS3).
//
// loadRecords() does `saveData.campaign.asides[recordList[i].recordName]
// [recordList[i].tracks[j].baseFilename].completion` with no `?` guard, so
// for any of these 3 gaps (Trophy Challenges, Credits, editorTutorial) the
// lookup hits GM_NULL and BC_GETIND kills the thread (confirmed real GM
// behavior, unpatched -- see vendor/GMSCRIPT_PATCHES.md #1: only `?`-guarded
// chains survive a null intermediate). That aborts
// CampaignScreenAlbumSelect's constructor before it ever assigns
// this.parent.albumSelectWidget, which then makes every subsequent frame's
// CampaignScreenRecordPlayer.gm preDraw() (`this.parent.albumSelectWidget.
// isVisible()`) hit the same fate -- and since drawing is one flat
// painter's-algorithm list, one draw call throwing mid-frame takes the
// whole main menu down with it (the observed all-white screen).
//
// Not a native-vs-script gap to "reimplement" -- both real files already
// exist unmodified. This just applies UpdateRecords' own defensive pattern
// (`if (!?saveData.campaign.asides[x]) { ...create stub... }`, its comment
// literally reads "these should def already exist at this point buuuuut
// script errors are doing crazy things to my globals?") using the SAME
// data source loadRecords() itself reads, so every key it will ever index
// is guaranteed to exist by the time it runs. Runs once, right after
// SAVE_DATA first exists, from the main loop (see main.cpp) -- there's no
// native call site inside the .gm boot sequence to hook this off of
// instead.
namespace {

bool g_done = false;

gmTableObject* GetOrCreateSubtable(gmMachine& a_machine, gmTableObject* a_parent, const char* a_key) {
    gmVariable existing = a_parent->Get(&a_machine, a_key);
    if (existing.m_type == GM_TABLE) {
        return (gmTableObject*)a_machine.GetObject(existing.m_value.m_ref);
    }
    gmTableObject* fresh = a_machine.AllocTableObject();
    a_parent->Set(&a_machine, a_key, gmVariable(fresh));
    return fresh;
}

} // namespace

void EnsureCampaignSaveDataComplete() {
    if (g_done) return;

    gmMachine* machine = GetEngineContext().machine;
    if (!machine) return;

    gmVariable saveDataVar = machine->GetGlobals()->Get(machine, "SAVE_DATA");
    if (saveDataVar.m_type != GM_TABLE) return; // GM hasn't created it yet -- try again next frame

    gmTableObject* saveData = (gmTableObject*)machine->GetObject(saveDataVar.m_value.m_ref);
    gmVariable campaignVar = saveData->Get(machine, "campaign");
    if (campaignVar.m_type != GM_TABLE) return; // MakeNewSave() hasn't run yet either

    std::string jsonText;
    if (!GetEngineContext().fileHelper->ReadTextFile("Platformer/MainMenu5/recordList.json", jsonText)) {
        g_done = true; // nothing to patch against -- don't retry every frame forever
        return;
    }

    rapidjson::Document doc;
    if (doc.Parse(jsonText.c_str()).HasParseError() || !doc.IsObject() || !doc.HasMember("campaignList")) {
        g_done = true;
        return;
    }

    gmTableObject* campaign = (gmTableObject*)machine->GetObject(campaignVar.m_value.m_ref);
    gmTableObject* asides = GetOrCreateSubtable(*machine, campaign, "asides");

    for (const auto& record : doc["campaignList"].GetArray()) {
        if (!record.IsObject() || !record.HasMember("recordName") || !record["recordName"].IsString()) continue;
        const char* recordName = record["recordName"].GetString();
        gmTableObject* recordAsides = GetOrCreateSubtable(*machine, asides, recordName);

        if (!record.HasMember("tracks") || !record["tracks"].IsArray()) continue;
        for (const auto& track : record["tracks"].GetArray()) {
            if (!track.IsObject() || !track.HasMember("baseFilename") || !track["baseFilename"].IsString()) continue;
            const char* baseFilename = track["baseFilename"].GetString();

            gmVariable existingEntry = recordAsides->Get(machine, baseFilename);
            if (existingEntry.m_type == GM_TABLE) continue; // already seeded (by UpdateRecords or a prior run)

            gmTableObject* entry = machine->AllocTableObject();
            gmTableObject* stats = machine->AllocTableObject();
            stats->Set(machine, "totalJumps", gmVariable((gmint)0));
            stats->Set(machine, "totalDeaths", gmVariable((gmint)0));
            stats->Set(machine, "totalPlayTime", gmVariable((gmint)0));
            entry->Set(machine, "stats", gmVariable(stats));
            // Matches UpdateRecords' own convention for a record's first
            // track (completion=1, i.e. unlocked-not-yet-played) -- the
            // safest default for a stub this project has no decompiled or
            // corpus evidence to pick more precisely for these specific
            // gap-filled categories.
            entry->Set(machine, "completion", gmVariable((gmint)1));
            recordAsides->Set(machine, baseFilename, gmVariable(entry));
        }
    }

    g_done = true;
}
