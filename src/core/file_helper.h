#pragma once
#include <string>
#include <vector>
#include <optional>

// Resolves script/data paths the way the retail engine's FILEHELPER_AREA_*
// system does: scripts and assets are looked up by relative path against
// one or more data roots, first match wins.
// Retail ships a shared root plus a platform-specific overlay
// (Platformer/ + PS3_Platformer/); we mirror that same idea to let the
// game_data_min hello-world test find shared engine scripts (FontHelper.gm
// etc.) that only exist in the full game_data tree.
class FileHelper {
public:
    void AddRoot(const std::string& root);

    // Returns the first existing absolute path for a_relativePath across
    // all registered roots, in registration order.
    std::optional<std::string> Resolve(const std::string& a_relativePath) const;

    // Reads a whole file as text via Resolve(). Returns false if not found.
    bool ReadTextFile(const std::string& a_relativePath, std::string& a_out) const;

    // The first registered root (registration order), or "" if none. Used
    // by FILEHELPER_AREA_GAMEDATA-targeted writes/listings (FILEHELPER_*
    // bindings, system_bindings.cpp) that need a concrete directory to act
    // on rather than Resolve()'s "does this exact file already exist"
    // check.
    const std::string& PrimaryRoot() const;

private:
    std::vector<std::string> m_roots;
};
