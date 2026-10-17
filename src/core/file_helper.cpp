#include "file_helper.h"
#include "psarc_archive.h"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>

static bool FileExists(const std::string& path);

static std::vector<std::filesystem::path> FindPatchArchives(const std::filesystem::path& directory) {
    namespace fs = std::filesystem;
    std::vector<std::pair<uint64_t, fs::path>> numbered;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(directory, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;
        const std::string name = entry.path().filename().string();
        constexpr char prefix[] = "patch_data";
        constexpr char suffix[] = ".psarc";
        if (name.size() != sizeof(prefix) - 1 + 3 + sizeof(suffix) - 1 ||
            name.compare(0, sizeof(prefix) - 1, prefix) != 0 ||
            name.compare(name.size() - (sizeof(suffix) - 1), sizeof(suffix) - 1, suffix) != 0)
            continue;
        const size_t digitsBegin = sizeof(prefix) - 1;
        const size_t digitsCount = name.size() - digitsBegin - (sizeof(suffix) - 1);
        const std::string digits = name.substr(digitsBegin, digitsCount);
        if (digits.size() != 3 ||
            !std::all_of(digits.begin(), digits.end(), [](unsigned char c) { return std::isdigit(c) != 0; }))
            continue;
        try {
            const auto patchNumber = std::stoull(digits);
            // Retail's startup loop probes patch_data001.psarc through
            // patch_data004.psarc, in ascending order, and never checks 000
            // or any later patch slot.
            if (patchNumber >= 1 && patchNumber <= 4)
                numbered.emplace_back(patchNumber, entry.path());
        } catch (...) {
            // Ignore patch names whose numeric suffix does not fit uint64_t.
        }
    }
    std::sort(numbered.begin(), numbered.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second.filename().string() < b.second.filename().string();
    });
    std::vector<fs::path> result;
    result.reserve(numbered.size());
    for (auto& patch : numbered) result.push_back(std::move(patch.second));
    return result;
}

static bool HasArchiveLayers(const std::filesystem::path& directory) {
    namespace fs = std::filesystem;
    if (FileExists((directory / "game_data.psarc").u8string()) ||
        !FindPatchArchives(directory).empty()) return true;
    for (unsigned patch = 1; patch <= 4; ++patch) {
        char name[32];
        std::snprintf(name, sizeof(name), "patch_data%03u", patch);
        std::error_code ec;
        if (fs::is_directory(directory / name, ec)) return true;
    }
    return false;
}

void FileHelper::AddRoot(const std::string& root, const std::string& archiveRoot) {
    namespace fs = std::filesystem;
    if (m_primaryRoot.empty()) m_primaryRoot = root;

    SourceGroup group;
    auto addArchive = [&](const fs::path& path) {
        if (!FileExists(path.u8string())) return;
        auto archive = std::make_shared<PsarcArchive>();
        const std::string pathUtf8 = path.u8string();
        if (archive->Open(pathUtf8)) group.layers.push_back({pathUtf8, std::move(archive)});
        else std::fprintf(stderr, "FileHelper: could not mount PSARC '%s'\n", pathUtf8.c_str());
    };

    fs::path rootPath = fs::u8path(root);
    const fs::path archiveRootPath = fs::u8path(archiveRoot);
    if (rootPath.extension() == ".psarc") {
        addArchive(rootPath);
        if (!group.layers.empty()) m_sourceGroups.push_back(std::move(group));
        return;
    }

    // Retail attaches game_data.psarc as the base, pushes the loose
    // /game_data directory overlay, then applies patch_data001..004 in order.
    // New overlays take precedence, so patches override loose files and each
    // later patch overrides earlier ones. Prefer the configured build/archive directory;
    // keep the adjacent "game_data packed" layout as a fallback for custom
    // roots and existing installations.
    fs::path archives;
    const bool rootIsArchiveRoot = HasArchiveLayers(rootPath);
    if (rootIsArchiveRoot) {
        archives = rootPath;
    } else if (!archiveRoot.empty() && HasArchiveLayers(archiveRootPath)) {
        archives = archiveRootPath;
    } else {
        archives = rootPath.parent_path() / (rootPath.filename().u8string() + " packed");
    }

    addArchive(archives / "game_data.psarc");
    if (fs::is_directory(rootPath) && rootPath.filename() != "game_data packed" && !rootIsArchiveRoot) {
        // PrimaryRoot remains the directory used by FILEHELPER area bindings.
        // Retail pushes it before the numbered patch archives.
        group.layers.push_back({root, nullptr});
    }
    for (unsigned patch = 1; patch <= 4; ++patch) {
        char archiveName[40];
        char directoryName[32];
        std::snprintf(archiveName, sizeof(archiveName), "patch_data%03u.psarc", patch);
        std::snprintf(directoryName, sizeof(directoryName), "patch_data%03u", patch);
        addArchive(archives / archiveName);

        // Retail's patch helper still pushes the patch path if its PSARC
        // is absent, treating the path as a raw directory overlay. Use a
        // build-local folder first, then the application's data sibling.
        fs::path patchDirectory = archives / directoryName;
        std::error_code ec;
        if (!fs::is_directory(patchDirectory, ec))
            patchDirectory = rootPath.parent_path() / directoryName;
        ec.clear();
        if (fs::is_directory(patchDirectory, ec))
            group.layers.push_back({patchDirectory.u8string(), nullptr});
    }
    if (!group.layers.empty()) m_sourceGroups.push_back(std::move(group));
}

static bool FileExists(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_regular_file(std::filesystem::u8path(path), ec) && !ec;
}

std::optional<std::string> FileHelper::Resolve(const std::string& a_relativePath) const {
    auto resolveInSource = [&](const Source& source, const std::string& requested) -> std::optional<std::string> {
        if (source.archive) {
            if (!source.archive->Contains(requested)) return std::nullopt;
            namespace fs = std::filesystem;
            std::error_code ec;
            const fs::path archivePath = fs::u8path(source.root);
            const auto stamp = fs::last_write_time(archivePath, ec).time_since_epoch().count();
            const auto size = fs::file_size(archivePath, ec);
            const size_t archiveKey = std::hash<std::string>{}(source.root + ":" +
                std::to_string(stamp) + ":" + std::to_string(size));
            const fs::path cacheRelative = fs::u8path(requested).lexically_normal().relative_path();
            fs::path cache = fs::temp_directory_path() / "OpenEgg_psarc_cache" /
                std::to_string(archiveKey) / cacheRelative;
            // ExtractTo validates the cached size and atomically repairs a
            // partial asset left by an interrupted launch. Always consult it
            // rather than trusting existence alone.
            if (!source.archive->ExtractTo(requested, cache.u8string())) return std::nullopt;
            return cache.u8string();
        }
        // Preserve the requested resource when it exists. Video.getTexture
        // separately detects PS3_ siblings and applies the retail 0.5 logical
        // dimension factor; that check does not imply replacing this file.
        const std::filesystem::path candidatePath =
            std::filesystem::u8path(source.root) / std::filesystem::u8path(requested);
        const std::string candidate = candidatePath.u8string();
        if (FileExists(candidate)) {
            return candidate;
        }
        return std::nullopt;
    };

    static const std::string kPrefix = "Platformer/";
    for (const auto& group : m_sourceGroups) {
        for (auto layer = group.layers.rbegin(); layer != group.layers.rend(); ++layer) {
            if (auto found = resolveInSource(*layer, a_relativePath)) return found;
            // Preserve the per-layer overlay rule: use PS3_Platformer only
            // when the requested shared path is absent in this same layer.
            if (a_relativePath.compare(0, kPrefix.size(), kPrefix) == 0) {
                const std::string overlay = "PS3_Platformer/" + a_relativePath.substr(kPrefix.size());
                if (auto found = resolveInSource(*layer, overlay)) return found;
            }
        }
    }
    return std::nullopt;
}

const std::string& FileHelper::PrimaryRoot() const {
    static const std::string kEmpty;
    return m_primaryRoot.empty() ? kEmpty : m_primaryRoot;
}

std::vector<std::string> FileHelper::ListFiles(const std::string& relativeDirectory) const {
    namespace fs = std::filesystem;
    std::vector<std::string> names;
    for (const auto& group : m_sourceGroups) {
        for (auto layer = group.layers.rbegin(); layer != group.layers.rend(); ++layer) {
            std::vector<std::string> current;
            if (layer->archive) {
                current = layer->archive->ListFiles(relativeDirectory);
            } else {
                std::error_code ec;
                const fs::path directory = fs::u8path(layer->root) / fs::u8path(relativeDirectory);
                for (const auto& entry : fs::directory_iterator(directory, ec)) {
                    if (ec) break;
                    current.push_back(entry.path().filename().u8string());
                }
            }
            for (const auto& name : current) {
                if (std::find(names.begin(), names.end(), name) == names.end()) names.push_back(name);
            }
        }
    }
    return names;
}

bool FileHelper::ReadTextFile(const std::string& a_relativePath, std::string& a_out) const {
    auto path = Resolve(a_relativePath);
    if (!path) return false;
    std::ifstream f(std::filesystem::u8path(*path), std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    a_out = ss.str();
    return true;
}
