#pragma once
#include <string>

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

bool DeleteFile(const std::string& a_relativePath);

} // namespace SaveData
