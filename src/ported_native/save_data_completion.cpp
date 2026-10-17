#include "save_data_completion.h"
#include "../core/engine_context.h"
#include "gmMachine.h"
#include "gmTableObject.h"
#include "rapidjson/document.h"
#include <cstdio>

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
            entry->Set(machine, "completion", gmVariable((gmint)1));
            recordAsides->Set(machine, baseFilename, gmVariable(entry));
        }
    }

    g_done = true;
}
