#pragma once
#include <string>

// Real save data storage -- backs SAVEDATA_*/SPLASHSCREEN_FinalizeSplash-
// ScreenAndGetResults/GameMonkeyScript.*SaveRequest (see savedata_bindings.cpp,
// system_bindings.cpp, gamemonkeyscript_bindings.cpp).
//
// Retail's real save path is Sony's SaveData API: an async ticket/request
// queue (SAVEDATA_reqSaveString/reqLoadFile/doRequest/isRequestDone/...,
// block. None of that block format or protocol is public or meaningfully
// portable -- there's nothing to "match" there beyond the script-visible
// contract each function upholds (see each binding's own comment for the
// call-by-call reasoning), so this reimplementation swaps the storage itself
// for what the user actually wants to work with: plain, uncompressed JSON
// files, at a fixed real path next to the built executable rather than
// hidden inside a save-data block, directly readable/editable in any text
// editor. Script-visible filenames are unchanged (game_data/Main.gm/
// SaveDataHelper.gm/MyLevelsTOC.gm all reference "save.json"/
// "myLevels/toc.json" literally) -- only where those bytes physically live
// changed.
namespace SaveData {

// Directory containing the running executable, independent of the process
// working directory. Paths returned by the filesystem-facing API use UTF-8.
const std::string& ExecutableDirectory();

// Absolute path to the save directory (created on first use if missing).
// Resolved once, relative to the running executable (NOT the current
// working directory, which SDL/whatever launched this from may have set to
// anything) -- see save_data.cpp for how.
const std::string& Dir();

// Reads a_relativePath (e.g. "save.json", "myLevels/toc.json") from the save
// directory as raw bytes. False if it doesn't exist or can't be read.
bool ReadFile(const std::string& a_relativePath, std::string& a_outContent);

// Writes a_content to a_relativePath under the save directory, creating any
// intermediate directories (e.g. "myLevels/") as needed. True on success.
bool WriteFile(const std::string& a_relativePath, const std::string& a_content);

bool FileExists(const std::string& a_relativePath);

// Byte size of a_relativePath under the save directory, or -1 if it doesn't
// exist / can't be stat'd. Backs SAVEDATA_reqGetFileSize's ticket.
long long FileSize(const std::string& a_relativePath);

// Deletes a_relativePath under the save directory. True if it existed and
// was removed, or didn't exist to begin with (matches the real
// SAVEDATA_reqDelete contract of "not an error if already gone" -- retail's
// file being missing).
bool DeleteFile(const std::string& a_relativePath);

} // namespace SaveData
