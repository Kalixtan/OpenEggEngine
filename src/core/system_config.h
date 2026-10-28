#pragma once
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

    // Desktop presentation option. When enabled, preserve the configured
    // game aspect ratio and draw black bars in the unused window area.
    // Disable to use the full host window, matching the pre-letterbox port.
    bool letterbox = true;

    // Optional replacement OTG server root. Empty keeps online requests disabled.
    std::string networkBaseUrl;
    // Retail's separate global image manager uses this service root.
    std::string networkImageBaseUrl = "https://s3.amazonaws.com";
    // Optional OTG session cookie for local server testing.
    std::string networkSessionId;
    // Optional identity ID paired with a manually supplied session ID.
    std::string networkUserId;
    // Optional binary platform ticket for the experimental login exchange.
    std::string networkTicketFile;
    // Optional credentials for the replacement server's desktop login.
    std::string networkAccountEmail;
    std::string networkAccountPassword;
    // Desktop fallback for PSN's chat restriction query.
    bool networkCommunityAllowed = true;

    // Reads optional settings from OpenEgg.ini beside the executable.
    // Missing or invalid values leave the retail-compatible defaults above.
    bool LoadFromIni(const std::string& path);

    const char* OsIdName() const;

    // Desktop fallback for the PS3 SCEA account-region language mapping.
    const char* SceaNetworkLanguage() const {
        return languageCode == 2 ? "fr" : languageCode == 3 ? "es_mx" : "en";
    }

    static SystemConfig& Get() {
        static SystemConfig instance;
        return instance;
    }
};
