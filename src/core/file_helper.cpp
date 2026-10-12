#include "file_helper.h"
#include "psarc_archive.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <sys/stat.h>

static bool FileExists(const std::string& path);

void FileHelper::AddRoot(const std::string& root) {
    namespace fs = std::filesystem;
    if (m_primaryRoot.empty()) m_primaryRoot = root;

    auto addArchive = [&](const fs::path& path) {
        if (!FileExists(path.string())) return;
        auto archive = std::make_shared<PsarcArchive>();
        if (archive->Open(path.string())) m_sources.push_back({path.string(), std::move(archive)});
        else std::fprintf(stderr, "FileHelper: could not mount PSARC '%s'\n", path.string().c_str());
    };

    fs::path rootPath(root);
    if (rootPath.extension() == ".psarc") {
        addArchive(rootPath);
        return;
    }

    // If given the normal unpacked data folder, automatically discover its
    // sibling packed folder. Source order is loose folder > patch archive >
    // base archive, so each later layer overrides the previous one.
    fs::path packed = rootPath;
    if (packed.filename() != "game_data packed")
        packed = packed.parent_path() / (packed.filename().string() + " packed");
    const bool packedExists = fs::exists(packed / "game_data.psarc") ||
                              fs::exists(packed / "patch_data001.psarc");

    // Loose content remains the top overlay and PrimaryRoot remains a real
    // writable host directory for the existing FILEHELPER area bindings.
    if (fs::is_directory(rootPath)) m_sources.push_back({root, nullptr});
    if (packedExists) {
        // Search precedence matches the mounted tree: patch archive shadows
        // base archive; a loose directory registered above shadows both.
        addArchive(packed / "patch_data001.psarc");
        addArchive(packed / "game_data.psarc");
    }
}

static bool FileExists(const std::string& path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::optional<std::string> FileHelper::Resolve(const std::string& a_relativePath) const {
    auto resolveInSource = [&](const Source& source, const std::string& requested) -> std::optional<std::string> {
        if (source.archive) {
            if (!source.archive->Contains(requested)) return std::nullopt;
            namespace fs = std::filesystem;
            std::error_code ec;
            const auto stamp = fs::last_write_time(source.root, ec).time_since_epoch().count();
            const auto size = fs::file_size(source.root, ec);
            const size_t archiveKey = std::hash<std::string>{}(source.root + ":" +
                std::to_string(stamp) + ":" + std::to_string(size));
            const fs::path cacheRelative = fs::path(requested).lexically_normal().relative_path();
            fs::path cache = fs::temp_directory_path() / "gameengine3_psarc_cache" /
                std::to_string(archiveKey) / cacheRelative;
            if (!FileExists(cache.string()) &&
                !source.archive->ExtractTo(requested, cache.string())) return std::nullopt;
            return cache.string();
        }
        // Preserve the requested resource when it exists. Video.getTexture
        // separately detects PS3_ siblings and applies the retail 0.5 logical
        // dimension factor; that check does not imply replacing this file.
        std::string candidate = source.root + "/" + requested;
        if (FileExists(candidate)) {
            return candidate;
        }
        return std::nullopt;
    };

    static const std::string kPrefix = "Platformer/";
    for (const auto& source : m_sources) {
        if (auto found = resolveInSource(source, a_relativePath)) return found;
        // Preserve the per-root overlay rule: use PS3_Platformer only when
        // the requested shared Platformer path is absent in this layer.
        if (a_relativePath.compare(0, kPrefix.size(), kPrefix) == 0) {
            const std::string overlay = "PS3_Platformer/" + a_relativePath.substr(kPrefix.size());
            if (auto found = resolveInSource(source, overlay)) return found;
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
    for (const auto& source : m_sources) {
        std::vector<std::string> current;
        if (source.archive) {
            current = source.archive->ListFiles(relativeDirectory);
        } else {
            std::error_code ec;
            for (const auto& entry : fs::directory_iterator(source.root + "/" + relativeDirectory, ec)) {
                if (ec) break;
                current.push_back(entry.path().filename().string());
            }
        }
        for (const auto& name : current) {
            if (std::find(names.begin(), names.end(), name) == names.end()) names.push_back(name);
        }
    }
    return names;
}

bool FileHelper::ReadTextFile(const std::string& a_relativePath, std::string& a_out) const {
    auto path = Resolve(a_relativePath);
    if (!path) return false;
    std::ifstream f(*path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    a_out = ss.str();
    return true;
}
