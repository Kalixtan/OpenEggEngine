#include "file_helper.h"
#include <fstream>
#include <sstream>
#include <sys/stat.h>

void FileHelper::AddRoot(const std::string& root) {
    m_roots.push_back(root);
}

static bool FileExists(const std::string& path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::optional<std::string> FileHelper::Resolve(const std::string& a_relativePath) const {
    for (const auto& root : m_roots) {
        std::string candidate = root + "/" + a_relativePath;
        if (FileExists(candidate)) {
            return candidate;
        }

        // Platformer/ + PS3_Platformer/ overlay (see header comment): real
        // scripts only ever reference paths under "Platformer/" (there is
        // no script or Video.getTexture call anywhere in the corpus that
        // spells out "PS3_Platformer/" directly), but some real assets
        // (confirmed: Platformer/images/MainMenu/*/preTutorial.png,
        // logo.png, winTitle.png -- entirely absent from Platformer/,
        // present only under PS3_Platformer/images/MainMenu/*/) only
        // physically exist in the PS3-specific overlay tree. Tried as a
        // fallback, not first, since most Platformer/ paths DO resolve
        // directly and the overlay is the exception, not the rule.
        static const std::string kPrefix = "Platformer/";
        if (a_relativePath.compare(0, kPrefix.size(), kPrefix) == 0) {
            std::string overlayCandidate = root + "/PS3_Platformer/" + a_relativePath.substr(kPrefix.size());
            if (FileExists(overlayCandidate)) {
                return overlayCandidate;
            }
        }
    }
    return std::nullopt;
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
