#pragma once
#include <string>
#include <unordered_map>

// Backs the real retail `_(str)` global function (confirmed usage:
// Platformer/recordList.gm and 2500+ other call sites across game_data).
//
// Two real sources feed the string table:
//  1. Standard GNU gettext .mo catalogs (localization/<lang>/strings.mo --
//     confirmed real format: magic 0x950412de, verified by hand-parsing
//     localization/en/strings.mo, matches spec exactly, e.g.
//     "Colours" -> "Colors").
//  2. localization/ExtraStrings.gm -- per its own header comment, this is
//     "NOT a real gm file... just used to collect strings that come from
//     external sources (xml, c code, sfo files etc)... this code is not
//     used in game". It's read here as plain text (regex-scanned for
//     `_("...")` literals), NOT compiled as GameMonkeyScript, and any
//     string found that isn't already a translated .mo entry gets an
//     identity fallback entry -- so `_()` still returns a sensible value
//     for strings that only ever come from non-script sources.
class Localization {
public:
    // Parses a standard little-endian .mo file, merging its msgid->msgstr
    // pairs into the table (skipping the first empty-msgid entry, which
    // holds PO header metadata, not a real string).
    bool LoadMoFile(const std::string& a_path);

    // Plain-text scan of ExtraStrings.gm (or any file) for `_("...")`
    // literals; each one not already present gets an identity entry.
    bool LoadExtraStringsFile(const std::string& a_path);

    // Returns the translation for a_key, or a_key itself if not found --
    // standard gettext fallback behaviour.
    const std::string& Translate(const std::string& a_key);

    size_t Size() const { return m_table.size(); }

    // ELanguageCode_* (confirmed real enum, game_data/FontHelper.gm:
    // JAPANESE=0..SPANISH_MEXICAN=19) -> localization/<dir> mapping.
    // FINNISH/SWEDISH/DANISH/NORWEGIAN (12-15) have no matching folder in
    // game_data at all -- the PS3 build just didn't ship them -- so those
    // fall back to "en".
    static std::string LocaleDirForLanguageCode(int a_languageCode);

private:
    std::unordered_map<std::string, std::string> m_table;
};
