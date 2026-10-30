#include "system_config.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

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

bool ParseDlcPacks(const std::string& value, std::set<int>& out) {
    std::set<int> parsed;
    size_t start = 0;
    while (start < value.size()) {
        const size_t end = value.find(',', start);
        const std::string item = Trim(value.substr(start, end == std::string::npos
            ? std::string::npos : end - start));
        int pack = 0;
        const auto result = std::from_chars(item.data(), item.data() + item.size(), pack);
        if (item.empty() || result.ec != std::errc() || result.ptr != item.data() + item.size() ||
            pack < 1 || pack > 15)
            return false;
        parsed.insert(pack);
        if (end == std::string::npos) break;
        start = end + 1;
        if (start == value.size()) return false;
    }
    out = std::move(parsed);
    return true;
}

bool IsTicketHex(const std::string& value) {
    if (value.size() > 131072 || value.size() % 2 != 0) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return std::isxdigit(c) != 0;
    });
}

bool IsOnlineId(const std::string& value) {
    if (value.size() < 3 || value.size() > 16 ||
        !((value.front() >= 'A' && value.front() <= 'Z') ||
          (value.front() >= 'a' && value.front() <= 'z')))
        return false;
    return std::all_of(value.begin() + 1, value.end(), [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9') || c == '-' || c == '_';
    });
}

bool IsCountryCode(const std::string& value) {
    return value.size() == 2 && std::all_of(value.begin(), value.end(), [](unsigned char c) {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    });
}

bool RewriteNetworkSettings(const std::string& iniPath,
                            const std::vector<std::pair<std::string, std::optional<std::string>>>& updates) {
    const std::filesystem::path path = std::filesystem::u8path(iniPath);
    std::filesystem::path temporary = path;
    temporary += ".tmp";
    std::ifstream input(path, std::ios::binary);
    std::ostringstream retained;
    std::string line;
    std::string section;
    while (std::getline(input, line)) {
        std::string content = Trim(line);
        if (!content.empty() && (content.front() == ';' || content.front() == '#')) {
            retained << line << '\n';
            continue;
        }
        if (content.size() >= 2 && content.front() == '[' && content.back() == ']') {
            section = Lower(Trim(content.substr(1, content.size() - 2)));
            retained << line << '\n';
            continue;
        }
        if (section == "network") {
            const size_t comment = content.find_first_of(";#");
            if (comment != std::string::npos) content = Trim(content.substr(0, comment));
            const size_t equals = content.find('=');
            if (equals != std::string::npos) {
                const std::string key = Lower(Trim(content.substr(0, equals)));
                if (std::any_of(updates.begin(), updates.end(), [&](const auto& update) {
                        return update.first == key;
                    }))
                    continue;
            }
        }
        retained << line << '\n';
    }
    if (input.bad()) return false;

    std::string output = retained.str();
    const bool hasValues = std::any_of(updates.begin(), updates.end(), [](const auto& update) {
        return update.second.has_value();
    });
    if (hasValues) {
        if (!output.empty() && output.back() != '\n') output.push_back('\n');
        output += "\n[network]\n";
        for (const auto& [key, value] : updates)
            if (value) output += key + " = " + *value + "\n";
    }

    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file || !file.write(output.data(), static_cast<std::streamsize>(output.size())) || !file.flush())
            return false;
    }

    std::error_code error;
    std::filesystem::permissions(temporary,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace, error);
#if defined(_WIN32)
    if (!MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
        return false;
#else
    if (error) return false;
    std::filesystem::rename(temporary, path, error);
    if (error) return false;
#endif
    return true;
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
    m_iniPath = path;
    std::ifstream input(std::filesystem::u8path(path));
    if (!input) return false;

    std::string line;
    std::string section;
    unsigned lineNumber = 0;
    bool containsSensitiveNetworkSettings = false;
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
        if (!section.empty() && section != "system" && section != "network") continue;
        const std::string key = Lower(Trim(line.substr(0, equals)));
        const std::string value = Trim(line.substr(equals + 1));
        if (section == "network" && (key == "psn_ticket_hex" || key == "server_session_id" ||
                                      key == "session_id"))
            containsSensitiveNetworkSettings = true;
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
        } else if (section == "system" && key == "dlc_available_packs") {
            std::set<int> parsed;
            valid = value.empty() || ParseDlcPacks(value, parsed);
            if (valid) availableDlcPacks = std::move(parsed);
        } else if (key == "letterbox" || key == "letterboxing") {
            bool parsed = letterbox;
            valid = ParseBool(value, parsed);
            if (valid) letterbox = parsed;
        } else if (key == "network_base_url" || (section == "network" && key == "server_url")) {
            valid = value.empty() || value.rfind("http://", 0) == 0 || value.rfind("https://", 0) == 0;
            if (valid) networkBaseUrl = value;
        } else if (section == "network" && key == "image_base_url") {
            valid = value.rfind("http://", 0) == 0 || value.rfind("https://", 0) == 0;
            if (valid) networkImageBaseUrl = value;
        } else if (section == "network" && key == "product_region") {
            const std::string region = Lower(value);
            valid = region == "scea" || region == "scee" || region == "scej" || region == "sceh";
            if (valid) {
                networkProductRegion = region;
                std::transform(networkProductRegion.begin(), networkProductRegion.end(), networkProductRegion.begin(),
                               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            }
        } else if (key == "network_session_id" || (section == "network" && key == "session_id")) {
            networkSessionId = value;
            valid = true;
        } else if (key == "network_user_id" || (section == "network" && key == "user_id")) {
            networkUserId = value;
            valid = true;
        } else if (section == "network" && key == "server_session_id") {
            networkSavedSessionId = value;
            valid = !value.empty();
        } else if (section == "network" && key == "server_user_id") {
            networkSavedUserId = value;
            valid = !value.empty();
        } else if (section == "network" && key == "ticket_file") {
            networkTicketFile = value;
            valid = true;
        } else if (section == "network" && key == "psn_online_id") {
            valid = IsOnlineId(value);
            if (valid) networkPsnOnlineId = value;
        } else if (section == "network" && key == "psn_country") {
            valid = value.empty() || IsCountryCode(value);
            if (valid) networkPsnCountry = Lower(value);
        } else if (section == "network" && key == "psn_ticket_hex") {
            valid = IsTicketHex(value);
            if (valid) networkTicketHex = value;
        } else if (section == "network" && key == "community_allowed") {
            bool parsed = networkCommunityAllowed;
            valid = ParseBool(value, parsed);
            if (valid) networkCommunityAllowed = parsed;
        } else if (section == "network" && key == "account_restricted") {
            bool parsed = networkAccountRestricted;
            valid = ParseBool(value, parsed);
            if (valid) networkAccountRestricted = parsed;
        } else {
            continue;
        }
        if (!valid) {
            std::fprintf(stderr, "system config: invalid %s value on line %u in '%s'; keeping default/current value\n",
                         key.c_str(), lineNumber, path.c_str());
        }
    }
    if (containsSensitiveNetworkSettings) {
        std::error_code error;
        std::filesystem::permissions(std::filesystem::u8path(path),
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace, error);
        if (error)
            std::fprintf(stderr, "system config: could not restrict permissions on credential-bearing INI '%s'\n",
                         path.c_str());
    }
    return true;
}

bool SystemConfig::SaveReplacementSession(const std::string& sessionId, const std::string& userId) {
    const bool clearing = sessionId.empty() && userId.empty();
    if (m_iniPath.empty() || sessionId.empty() != userId.empty() ||
        sessionId.find_first_of("\r\n") != std::string::npos ||
        userId.find_first_of("\r\n") != std::string::npos)
        return false;
    std::vector<std::pair<std::string, std::optional<std::string>>> updates = {
        {"server_session_id", clearing ? std::nullopt : std::optional<std::string>(sessionId)},
        {"server_user_id", clearing ? std::nullopt : std::optional<std::string>(userId)},
    };
    if (!RewriteNetworkSettings(m_iniPath, updates)) return false;
    networkSavedSessionId = clearing ? std::string() : sessionId;
    networkSavedUserId = clearing ? std::string() : userId;
    return true;
}

bool SystemConfig::SavePsnCountry(const std::string& country) {
    if (m_iniPath.empty() || !IsCountryCode(country)) return false;
    const std::string normalized = Lower(country);
    if (!RewriteNetworkSettings(m_iniPath, {{"psn_country", normalized}})) return false;
    networkPsnCountry = normalized;
    return true;
}
