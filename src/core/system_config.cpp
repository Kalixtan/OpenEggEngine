#include "system_config.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::string Trim(std::string value) {
    const auto first = std::find_if_not(value.begin(), value.end(),
        [](unsigned char c) { return std::isspace(c) != 0; });
    const auto last = std::find_if_not(value.rbegin(), value.rend(),
        [](unsigned char c) { return std::isspace(c) != 0; }).base();
    if (first >= last) return {};
    return std::string(first, last);
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ParseBool(const std::string& value, bool& out) {
    const std::string normalized = Lower(Trim(value));
    if (normalized == "true" || normalized == "yes" || normalized == "1" || normalized == "on") {
        out = true;
        return true;
    }
    if (normalized == "false" || normalized == "no" || normalized == "0" || normalized == "off") {
        out = false;
        return true;
    }
    return false;
}

bool ParseOsId(const std::string& value, OsId& out) {
    const std::string normalized = Lower(Trim(value));
    if (normalized == "windows" || normalized == "win32" || normalized == "0") out = OsId::WINDOWS;
    else if (normalized == "osx" || normalized == "macos" || normalized == "1") out = OsId::OSX;
    else if (normalized == "ps3" || normalized == "2") out = OsId::PS3;
    else if (normalized == "ps4" || normalized == "3") out = OsId::PS4;
    else if (normalized == "ngp" || normalized == "vita" || normalized == "psvita" || normalized == "4") out = OsId::NGP;
    else if (normalized == "vitatv" || normalized == "vita_tv" || normalized == "5") out = OsId::VITATV;
    else return false;
    return true;
}

bool ParseLanguage(const std::string& value, int& out) {
    const std::string normalized = Lower(Trim(value));
    if (normalized == "ja" || normalized == "japanese") out = 0;
    else if (normalized == "en" || normalized == "en_us" || normalized == "english_us") out = 1;
    else if (normalized == "fr" || normalized == "french") out = 2;
    else if (normalized == "es" || normalized == "spanish") out = 3;
    else if (normalized == "de" || normalized == "german") out = 4;
    else if (normalized == "it" || normalized == "italian") out = 5;
    else if (normalized == "nl" || normalized == "dutch") out = 6;
    else if (normalized == "pt" || normalized == "pt_pt" || normalized == "portuguese_pt") out = 7;
    else if (normalized == "ru" || normalized == "russian") out = 8;
    else if (normalized == "ko" || normalized == "korean") out = 9;
    else if (normalized == "zh_hant" || normalized == "chinese_t") out = 10;
    else if (normalized == "zh" || normalized == "zh_hans" || normalized == "chinese_s") out = 11;
    else if (normalized == "fi" || normalized == "finnish") out = 12;
    else if (normalized == "sv" || normalized == "swedish") out = 13;
    else if (normalized == "da" || normalized == "danish") out = 14;
    else if (normalized == "no" || normalized == "norwegian") out = 15;
    else if (normalized == "pl" || normalized == "polish") out = 16;
    else if (normalized == "pt_br" || normalized == "portuguese_br") out = 17;
    else if (normalized == "en_gb" || normalized == "english_gb") out = 18;
    else if (normalized == "es_mx" || normalized == "spanish_mx") out = 19;
    else {
        try {
            size_t consumed = 0;
            const int code = std::stoi(normalized, &consumed);
            if (consumed != normalized.size() || code < 0 || code > 19) return false;
            out = code;
        } catch (...) {
            return false;
        }
    }
    return true;
}

} // namespace

const char* SystemConfig::OsIdName() const {
    switch (osId) {
        case OsId::WINDOWS: return "windows";
        case OsId::OSX: return "osx";
        case OsId::PS3: return "ps3";
        case OsId::PS4: return "ps4";
        case OsId::NGP: return "ngp";
        case OsId::VITATV: return "vitatv";
    }
    return "unknown";
}

bool SystemConfig::LoadFromIni(const std::string& path) {
    std::ifstream input(std::filesystem::u8path(path));
    if (!input) return false;

    std::string line;
    std::string section;
    unsigned lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const auto comment = line.find_first_of(";#");
        if (comment != std::string::npos) line.erase(comment);
        line = Trim(std::move(line));
        if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']') {
            section = Lower(Trim(line.substr(1, line.size() - 2)));
            continue;
        }
        const auto equals = line.find('=');
        if (equals == std::string::npos) {
            std::fprintf(stderr, "system config: ignoring malformed line %u in '%s'\n", lineNumber, path.c_str());
            continue;
        }
        if (!section.empty() && section != "system") continue;
        const std::string key = Lower(Trim(line.substr(0, equals)));
        const std::string value = Trim(line.substr(equals + 1));
        bool valid = false;
        if (key == "os_id" || key == "osid" || key == "platform") {
            OsId parsed;
            valid = ParseOsId(value, parsed);
            if (valid) osId = parsed;
        } else if (key == "language" || key == "language_code") {
            int parsed = languageCode;
            valid = ParseLanguage(value, parsed);
            if (valid) languageCode = parsed;
        } else if (key == "is_retail_build" || key == "retail_build") {
            bool parsed = isRetailBuild;
            valid = ParseBool(value, parsed);
            if (valid) isRetailBuild = parsed;
        } else if (key == "debug_all_dlc_available" || key == "all_dlc_available") {
            bool parsed = debugAllDlcAvailable;
            valid = ParseBool(value, parsed);
            if (valid) debugAllDlcAvailable = parsed;
        } else if (key == "letterbox" || key == "letterboxing") {
            bool parsed = letterbox;
            valid = ParseBool(value, parsed);
            if (valid) letterbox = parsed;
        } else {
            continue;
        }
        if (!valid) {
            std::fprintf(stderr, "system config: invalid %s value on line %u in '%s'; keeping default/current value\n",
                         key.c_str(), lineNumber, path.c_str());
        }
    }
    return true;
}
