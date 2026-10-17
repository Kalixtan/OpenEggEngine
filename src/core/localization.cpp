#include "localization.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
constexpr uint32_t kMoMagicLE = 0x950412de;
}

bool Localization::LoadMoFile(const std::string& a_path) {
    std::ifstream f(std::filesystem::u8path(a_path), std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "Localization::LoadMoFile: could not open '%s'\n", a_path.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string data = ss.str();
    if (data.size() < 20) return false;

    auto readU32 = [&](size_t offset) -> uint32_t {
        uint32_t v;
        std::memcpy(&v, data.data() + offset, sizeof(v));
        return v;
    };

    uint32_t magic = readU32(0);
    if (magic != kMoMagicLE) {
        std::fprintf(stderr, "Localization::LoadMoFile: '%s' bad magic 0x%08x\n", a_path.c_str(), magic);
        return false;
    }

    uint32_t numStrings = readU32(8);
    uint32_t origTableOff = readU32(12);
    uint32_t transTableOff = readU32(16);

    // Entry 0 is the PO header metadata (empty msgid), not a real string.
    for (uint32_t i = 1; i < numStrings; ++i) {
        uint32_t origLen = readU32(origTableOff + i * 8);
        uint32_t origOff = readU32(origTableOff + i * 8 + 4);
        uint32_t transLen = readU32(transTableOff + i * 8);
        uint32_t transOff = readU32(transTableOff + i * 8 + 4);
        if (origOff + origLen > data.size() || transOff + transLen > data.size()) continue;

        std::string orig = data.substr(origOff, origLen);
        std::string trans = data.substr(transOff, transLen);
        m_table[orig] = trans;
    }
    return true;
}

bool Localization::LoadExtraStringsFile(const std::string& a_path) {
    std::ifstream f(std::filesystem::u8path(a_path), std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "Localization::LoadExtraStringsFile: could not open '%s'\n", a_path.c_str());
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string text = ss.str();

    size_t added = 0;
    size_t pos = 0;
    while ((pos = text.find("_(\"", pos)) != std::string::npos) {
        size_t start = pos + 3;
        std::string literal;
        size_t i = start;
        bool closed = false;
        for (; i < text.size(); ++i) {
            char c = text[i];
            if (c == '\\' && i + 1 < text.size()) {
                literal += text[i + 1];
                ++i;
                continue;
            }
            if (c == '"') { closed = true; break; }
            literal += c;
        }
        pos = closed ? i + 1 : start;
        if (closed && !literal.empty() && m_table.find(literal) == m_table.end()) {
            m_table[literal] = literal; // identity fallback -- see localization.h
            ++added;
        }
    }
    std::fprintf(stderr, "Localization::LoadExtraStringsFile: '%s' added %zu fallback entries\n",
                 a_path.c_str(), added);
    return true;
}

const std::string& Localization::Translate(const std::string& a_key) {
    auto it = m_table.find(a_key);
    if (it != m_table.end()) return it->second;
    // Not found anywhere -- standard gettext fallback is the original
    // string. Insert so we can return a stable reference.
    return m_table.emplace(a_key, a_key).first->second;
}

std::string Localization::LocaleDirForLanguageCode(int a_languageCode) {
    switch (a_languageCode) {
        case 0: return "ja";
        case 1: return "en";
        case 2: return "fr";
        case 3: return "es";
        case 4: return "de";
        case 5: return "it";
        case 6: return "nl";
        case 7: return "pt";
        case 8: return "ru";
        case 9: return "ko";
        case 10: return "zh_hant";
        case 11: return "zh";
        case 12: return "en"; // FINNISH -- not shipped in game_data, fall back
        case 13: return "en"; // SWEDISH -- not shipped
        case 14: return "en"; // DANISH -- not shipped
        case 15: return "en"; // NORWEGIAN -- not shipped
        case 16: return "pl";
        case 17: return "pt_br";
        case 18: return "en_gb";
        case 19: return "es_mx";
        default: return "en";
    }
}
