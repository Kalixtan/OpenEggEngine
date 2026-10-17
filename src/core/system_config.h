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

    // Reads optional settings from OpenEgg.ini beside the executable.
    // Missing or invalid values leave the retail-compatible defaults above.
    bool LoadFromIni(const std::string& path);

    const char* OsIdName() const;

    static SystemConfig& Get() {
        static SystemConfig instance;
        return instance;
    }
};
