#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>

class PsarcArchive;

// Resolves script/data paths across loose roots and mounted PSARC layers.
// For the retail data layout, the loose game_data directory overlays
// patch_data001.psarc, which overlays game_data.psarc. A PS3_Platformer copy
// is only a fallback for missing Platformer assets; separately registered
// roots follow in registration order (game_data_min supplies shared test
// assets and scripts).
class FileHelper {
public:
    // Verified against the supplied retail archive names/layout. Adding a
    // game_data directory auto-mounts its sibling "game_data packed" folder.
    void AddRoot(const std::string& root);

    // Returns the resolved absolute path, checking roots in registration
    // order and preferring the requested path over an overlay fallback.
    std::optional<std::string> Resolve(const std::string& a_relativePath) const;

    // Reads a whole file as text via Resolve(). Returns false if not found.
    bool ReadTextFile(const std::string& a_relativePath, std::string& a_out) const;

    // The first registered root (registration order), or "" if none. Used
    // by FILEHELPER_AREA_GAMEDATA-targeted writes/listings (FILEHELPER_*
    // bindings, system_bindings.cpp) that need a concrete directory to act
    // on rather than Resolve()'s "does this exact file already exist"
    // check.
    const std::string& PrimaryRoot() const;

    // Lists the visible names in a directory across loose files and mounted
    // PSARC overlays, with the same first-visible-source precedence as Resolve.
    std::vector<std::string> ListFiles(const std::string& a_relativeDirectory) const;

private:
    struct Source {
        std::string root;
        std::shared_ptr<PsarcArchive> archive;
    };
    std::vector<Source> m_sources;
    std::string m_primaryRoot;
};
