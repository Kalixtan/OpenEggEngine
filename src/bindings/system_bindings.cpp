#include "gmThread.h"
#include "system_bindings.h"
#include "gmTableObject.h"
#include "../core/system_config.h"
#include "../core/engine_context.h"
#include "../core/strict_calls.h"
#include "../core/save_data.h"
#include <cstdio>
#include <ctime>
#include <string>

namespace {

// 0x000aa2dc: 1-or-2-arg, returns bool). Real call site (ListCellEntity.gm
// :36: `if(!FILE_IsExists(imagePath+imageFile)){...fallback image...}`)
// gates a real fallback-image path, so returning the wrong answer here
// silently either always shows the fallback or always tries (and fails to
// load) the constructed path. The `area` 2nd arg (FILEHELPER_AREA_* enum,
// default 1) has no distinct meaning in this engine's single-rooted
// FileHelper (see file_helper.h) -- accepted and ignored, same as every
// other FILEHELPER_AREA_* param elsewhere in this codebase.
int GM_CDECL gmf_FILE_IsExists(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    bool exists = GetEngineContext().fileHelper && GetEngineContext().fileHelper->Resolve(path).has_value();
    a_thread->PushInt(exists ? 1 : 0);
    return GM_OK;
}

// 0x000a2a18: a plain global debug flag setter, bare-global call site
// Main.gm:1453 `SetIsOccludingUpdates(true);`). An occlusion-culling debug
// toggle with no consumer this reimplementation models -- genuine no-op,
// consumed rather than left as a null-call.
int GM_CDECL gmf_SetIsOccludingUpdates(gmThread* a_thread) {
    (void)a_thread;
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_GetOSID(gmThread* a_thread) {
    a_thread->PushInt((gmint)SystemConfig::Get().osId);
    return GM_OK;
}

// Confirmed real short-code strings (NOT a human-readable description
// despite the name) -- every real call site does an EXACT string compare:
// PlayMenu.gm:44 `if (SYSTEM_GetOSDescription() == "NGP")`; Main.gm:1595-96
// `if (os=="NGP") {...} else if (os=="PS3") {...} else {...PC...}`
// (selects which TouchEditProcess/EntityMenuTop variant the editor loads);
// Main.gm:10258 `os != "NGP" && os != "PS3"`; Main.gm:10397-99
// `if (desc=="Win32") {...} else if (desc=="OS X") {...}`. The previous
// "PlayStation 3 (SCEA)"/"PlayStation Vita (NGP, SCEA)" strings never
// matched any of these comparisons, so Main.gm's `os=="PS3"` branch never
// fired on PS3 -- it silently fell all the way through to the PC branch
// (Platformer_TouchEditProcessPC/Platformer_EntityMenuTopPC), loading the
// wrong platform's editor input handler entirely. That's why the editor's
// PS3-specific enableShaking/enableBackPanel calls (TouchEditProcessPS3.gm)
// never ran, and why editor navigation/menu input generally didn't match
// this project's PS3 target: the PC variant expects a completely different
// (mouse/keyboard-driven) input scheme.
int GM_CDECL gmf_SYSTEM_GetOSDescription(gmThread* a_thread) {
    const char* desc = (SystemConfig::Get().osId == OsId::PS3) ? "PS3" : "NGP";
    a_thread->PushNewString(desc);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_IsBeta(gmThread* a_thread) {
    a_thread->PushInt(0); // confirmed real decompiled body: always false, not config-driven
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_IsRetailBuild(gmThread* a_thread) {
    a_thread->PushInt(SystemConfig::Get().isRetailBuild ? 1 : 0);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_GetLanguage(gmThread* a_thread) {
    a_thread->PushInt(SystemConfig::Get().languageCode);
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_getUNIXTimestamp(gmThread* a_thread) {
    // Confirmed real signature returns a STRING (type tag 0x25), not an int
    std::time_t t = std::time(nullptr);
    a_thread->PushNewString(std::to_string((long long)t).c_str());
    return GM_OK;
}

int GM_CDECL gmf_SYSTEM_Execute(gmThread* a_thread) {
    // Confirmed real signature: takes a command string, matches the stock
    // gmfSystem()/C system() pattern. Deliberately NOT executed -- running
    // an arbitrary OS command from script content is a real command-
    // injection risk we don't need to take on for a script-compatibility
    // milestone. Logged instead so callers can see what would have run.
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(cmd, 0, "");
    std::fprintf(stderr, "[SYSTEM_Execute] suppressed (not executed): %s\n", cmd);
    return GM_OK;
}

// Real global `_(str)` localization function -- see core/localization.h.
int GM_CDECL gmf_Translate(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(text, 0, "");
    Localization* loc = GetEngineContext().localization;
    if (!loc) {
        a_thread->PushNewString(text);
        return GM_OK;
    }
    a_thread->PushNewString(loc->Translate(text).c_str());
    return GM_OK;
}

// Confirmed real by usage only (Platformer/MainMenu5/PreTutorialScreen.gm,
// HomeScreenBase.gm: `local path=Localization_LocalizeFilename("Platformer/
// images/MainMenu/logo.png"); this.logoTexture=Video.getTexture(path,...);`)
// only 2 real call sites. Was previously entirely unregistered, so
// calling it silently returned nothing via the null-call leniency patch,
// leaving `path` undefined and crashing the very next line's
// Video.getTexture(path,...).
//
// Confirmed real behavior from the actual game_data asset layout (not
// just usage): Platformer/images/MainMenu/ has one subdirectory per
// locale (en/, fr/, de/, ja/, ...) each containing a same-named
// preTutorial.png/logo.png/winTitle.png, matching
// Localization::LocaleDirForLanguageCode's folder names exactly -- so
// this inserts the current locale directory right before the filename.
int GM_CDECL gmf_Localization_LocalizeFilename(gmThread* a_thread) {
    GM_CHECK_NUM_PARAMS(1);
    GM_STRING_PARAM(path, 0, "");
    std::string p(path);
    std::string localeDir = Localization::LocaleDirForLanguageCode(SystemConfig::Get().languageCode);
    size_t slash = p.find_last_of('/');
    std::string localized = (slash == std::string::npos)
        ? (localeDir + "/" + p)
        : (p.substr(0, slash + 1) + localeDir + "/" + p.substr(slash + 1));
    a_thread->PushNewString(localized.c_str());
    return GM_OK;
}

// game_data/Main.gm's InitGame() (~line 1039-1061) uses the returned
// table, which fully determines the real contract: it reads
// `results.savedataResult`, compares it against SAVEDATARESULT_NOSAVE/
// NEWSAVE/NOTOC/OK/OK_TOCXML (stored ON the result table itself -- an
// unusual but confirmed-by-usage pattern, `results.SAVEDATARESULT_NOSAVE`
// not a bare global), and on SAVEDATARESULT_OK/NOTOC reads
// `results.saveFileTicket`/`.tocFileTicket` and passes them straight to
// GameMonkeyScript.loadTableFromJsonSaveRequest()/GetMyLevelsTOC().
//
// Main.gm's own comment right above this call ("we can assume the engine
// has already loaded the files for us so we can just jump directly into
// setting up the toc and save data") plus its dead `else` branch (manual
// SAVEDATA_reqGetFileSize/SAVEDATA_reqLoadFile preloading, commented
// "this should never happen anymore!") together confirm retail's real
// savedataResult is unconditionally >=0 by the time this game shipped --
// the async-preload fallback path is dead code, not something this needs
// to replicate.
//
// This engine's save data is real JSON files under SaveData::Dir() (see
// core/save_data.h -- "savedata/" next to the built executable, plain text,
// the splash screen's save-data handshake" becomes simply: check what's
// already on disk. Ticket design matches GameMonkeyScript.
// {save,load}TableFromJsonSaveRequest's (gamemonkeyscript_bindings.cpp): a
// ticket is just the relative filename, since no real call site does
// anything with one beyond handing it straight to a loader.
int GM_CDECL gmf_SPLASHSCREEN_FinalizeSplashScreenAndGetResults(gmThread* a_thread) {
    gmMachine& m = *a_thread->GetMachine();
    gmTableObject* results = m.AllocTableObject();
    results->Set(&m, "SAVEDATARESULT_NOSAVE", gmVariable((gmint)0));
    results->Set(&m, "SAVEDATARESULT_NEWSAVE", gmVariable((gmint)1));
    results->Set(&m, "SAVEDATARESULT_NOTOC", gmVariable((gmint)2));
    results->Set(&m, "SAVEDATARESULT_OK", gmVariable((gmint)3));
    results->Set(&m, "SAVEDATARESULT_OK_TOCXML", gmVariable((gmint)4));

    bool haveSave = SaveData::FileExists("save.json");
    bool haveToc = SaveData::FileExists("myLevels/toc.json");

    int savedataResult;
    gmVariable saveTicket = gmVariable::s_null;
    gmVariable tocTicket = gmVariable::s_null;
    if (!haveSave) {
        // First-ever boot (or GE3_CLEAR_SAVE just wiped it, see main.cpp) --
        // NEWSAVE takes the "start from scratch" branch in Main.gm, which
        // builds a fresh SAVE_DATA and writes save.json itself, so no ticket
        // is needed here.
        savedataResult = 1; // SAVEDATARESULT_NEWSAVE
    } else if (!haveToc) {
        savedataResult = 2; // SAVEDATARESULT_NOTOC
        saveTicket = gmVariable(m.AllocStringObject("save.json"));
    } else {
        savedataResult = 3; // SAVEDATARESULT_OK
        saveTicket = gmVariable(m.AllocStringObject("save.json"));
        tocTicket = gmVariable(m.AllocStringObject("myLevels/toc.json"));
    }

    results->Set(&m, "savedataResult", gmVariable((gmint)savedataResult));
    results->Set(&m, "saveFileTicket", saveTicket);
    results->Set(&m, "tocFileTicket", tocTicket);
    a_thread->PushTable(results);
    return GM_OK;
}

gmFunctionEntry s_systemLib[] = {
    {"SYSTEM_GetOSID", gmf_SYSTEM_GetOSID, nullptr},
    {"SYSTEM_GetOSDescription", gmf_SYSTEM_GetOSDescription, nullptr},
    {"SYSTEM_IsBeta", gmf_SYSTEM_IsBeta, nullptr},
    {"SYSTEM_IsRetailBuild", gmf_SYSTEM_IsRetailBuild, nullptr},
    {"SYSTEM_GetLanguage", gmf_SYSTEM_GetLanguage, nullptr},
    {"SYSTEM_getUNIXTimestamp", gmf_SYSTEM_getUNIXTimestamp, nullptr},
    {"SYSTEM_Execute", gmf_SYSTEM_Execute, nullptr},
    {"SPLASHSCREEN_FinalizeSplashScreenAndGetResults", gmf_SPLASHSCREEN_FinalizeSplashScreenAndGetResults, nullptr},
    {"_", gmf_Translate, nullptr},
    {"Localization_LocalizeFilename", gmf_Localization_LocalizeFilename, nullptr},
    {"FILE_IsExists", gmf_FILE_IsExists, nullptr},
    {"SetIsOccludingUpdates", gmf_SetIsOccludingUpdates, nullptr},
};

} // namespace

void RegisterSystemBindings(gmMachine& a_machine) {
    // Confirmed real: SYSTEM_* are bare globals (SYSTEM_GetOSID(...), not
    // in Platformer/EntityMenuContents.gm. `_` is likewise a bare global,
    // confirmed by its 2500+ real call sites across game_data/*.gm.
    a_machine.RegisterLibrary(s_systemLib, sizeof(s_systemLib) / sizeof(s_systemLib[0]), nullptr);

    // OsId enum
    gmTableObject* g = a_machine.GetGlobals();
    g->Set(&a_machine, "OSID_WIN32", gmVariable((gmint)OsId::WIN32));
    g->Set(&a_machine, "OSID_OSX", gmVariable((gmint)OsId::OSX));
    g->Set(&a_machine, "OSID_PS3", gmVariable((gmint)OsId::PS3));
    g->Set(&a_machine, "OSID_PS4", gmVariable((gmint)OsId::PS4));
    g->Set(&a_machine, "OSID_NGP", gmVariable((gmint)OsId::NGP));
    g->Set(&a_machine, "OSID_VITATV", gmVariable((gmint)OsId::VITATV));
}
