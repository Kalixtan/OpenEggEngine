#pragma once
#include <string>
#include <unordered_map>

class Localization {
public:
    // Parses a standard little-endian .mo file, merging its msgid->msgstr
    // pairs into the table (skipping the first empty-msgid entry, which
    // holds PO header metadata, not a real string).
    bool LoadMoFile(const std::string& a_path);

    bool LoadExtraStringsFile(const std::string& a_path);

    // Returns the translation for a_key, or a_key itself if not found --
    // standard gettext fallback behaviour.
    const std::string& Translate(const std::string& a_key);

    size_t Size() const { return m_table.size(); }

    static std::string LocaleDirForLanguageCode(int a_languageCode);

private:
    std::unordered_map<std::string, std::string> m_table;
};
