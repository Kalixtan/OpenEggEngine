#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>

class PsarcArchive;

// Resolves script/data paths across ordered retail data layers. For the
// retail data layout, game_data.psarc is the base, loose game_data is pushed
// over it, then each patch_data001..004 slot is applied in ascending order,
// using its PSARC and/or raw directory. A PS3_Platformer copy is only a
// fallback for missing Platformer assets within each layer;
// separately registered roots follow in registration order.
class FileHelper {
public:
    // Verified against the supplied retail archive names/layout. Adding a
    // game_data directory auto-mounts its sibling "game_data packed" folder.
    void AddRoot(const std::string& root, const std::string& archiveRoot = {});

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
    // Layers are recorded in retail application order (base -> loose ->
    // patch001 archive/folder -> ... -> patch004 archive/folder). Resolution
    // walks a root's layers backwards because each newly applied overlay has
    // higher precedence. Separately registered roots retain registration order.
    struct SourceGroup {
        std::vector<Source> layers;
    };
    std::vector<SourceGroup> m_sourceGroups;
    std::string m_primaryRoot;
};
