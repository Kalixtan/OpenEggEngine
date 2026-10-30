#pragma once
#include <set>
#include <string>

enum class OsId {
    WINDOWS = 0,
    OSX = 1, // Unused/unreleased
    PS3 = 2,
    PS4 = 3, // Is this used?
    NGP = 4, // PS Vita -- "NGP" is the pre-launch codename
    VITATV = 5, // Is this used?
};

struct SystemConfig {
    OsId osId = OsId::PS3;

    int languageCode = 1;

    bool isBetaBuild = false;
    bool isRetailBuild = true;
    // Development convenience: report the fifteen known DLC pack IDs as
    // available to scripts. This does not add content or emulate Sony DRM.
    bool debugAllDlcAvailable = false;
    // Optional locally enabled DLC pack IDs (1 through 15), independent of PSN.
    std::set<int> availableDlcPacks;

    // Desktop presentation option. When enabled, preserve the configured
    // game aspect ratio and draw black bars in the unused window area.
    // Disable to use the full host window, matching the pre-letterbox port.
    bool letterbox = true;

    // Optional replacement OTG server root. Empty keeps online requests disabled.
    std::string networkBaseUrl;
    // Retail's separate global image manager uses this service root.
    std::string networkImageBaseUrl = "https://s3.amazonaws.com";
    // Desktop substitute for the retail product ID's publisher prefix.
    std::string networkProductRegion = "SCEA";
    // Optional OTG session cookie for local server testing.
    std::string networkSessionId;
    // Optional identity ID paired with a manually supplied session ID.
    std::string networkUserId;
    // Server-issued session remembered after ticket login.
    std::string networkSavedSessionId;
    std::string networkSavedUserId;
    // PSN Online ID used to bind server login to the same visible identity.
    std::string networkPsnOnlineId;
    // Country from the accepted ticket, retained for later server sessions.
    std::string networkPsnCountry;
    // Optional binary ticket, either inline hex or in a separate file.
    std::string networkTicketHex;
    std::string networkTicketFile;
    // Desktop fallback for PSN's chat restriction query.
    bool networkCommunityAllowed = true;
    // Desktop stand-in for PSN's account content-rating query.
    bool networkAccountRestricted = false;

    // Reads optional settings from OpenEgg.ini beside the executable.
    // Missing or invalid values leave the retail-compatible defaults above.
    bool LoadFromIni(const std::string& path);
    bool SaveReplacementSession(const std::string& sessionId, const std::string& userId);
    bool SavePsnCountry(const std::string& country);

    const char* OsIdName() const;

    // Desktop uses the configured language in place of PSN's account region.
    const char* NetworkLanguage() const {
        static const char* locales[] = {"ja", "en", "fr", "es", "de", "it", "nl", "pt", "ru", "ko",
                                        "zh_hant", "zh", "fi", "sv", "da", "no", "pl", "pt_br", "en_gb", "es_mx"};
        if (networkProductRegion == "SCEJ") return languageCode == 0 ? "ja" : "en";
        if (networkProductRegion == "SCEE") {
            if ((languageCode >= 2 && languageCode <= 8) || languageCode == 16) return locales[languageCode];
            return "en_gb";
        }
        if (networkProductRegion == "SCEH") {
            if (languageCode == 9 || languageCode == 10) return locales[languageCode];
            return "en";
        }
        return languageCode == 2 ? "fr" : languageCode == 3 ? "es_mx" : "en";
    }

    static SystemConfig& Get() {
        static SystemConfig instance;
        return instance;
    }

private:
    std::string m_iniPath;
};
