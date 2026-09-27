#include "save_data.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>

#if defined(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(_WIN32)
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace {

// Directory the running executable lives in. /proc/self/exe (Linux) /
// GetModuleFileName (Windows) rather than argv[0] or getcwd(): whatever
// launched this (a shell in some other directory, a debugger, a desktop
// shortcut) shouldn't change where save data ends up -- "next to the exe"
// has to mean the exe's real location, not "wherever the process happened
// to start".
fs::path ExeDir() {
#if defined(__linux__)
    char buf[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        return fs::path(buf).parent_path();
    }
#elif defined(_WIN32)
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        return fs::path(buf).parent_path();
    }
#endif
    // Fallback (macOS, or either lookup above failing) -- current directory
    // is at least deterministic for a given launch, if not ideal.
    std::error_code ec;
    fs::path cwd = fs::current_path(ec);
    return ec ? fs::path(".") : cwd;
}

} // namespace

namespace SaveData {

const std::string& Dir() {
    static std::string dir = [] {
        fs::path p = ExeDir() / "savedata";
        std::error_code ec;
        fs::create_directories(p, ec);
        if (ec) {
            std::fprintf(stderr, "SaveData::Dir: could not create '%s': %s\n",
                         p.string().c_str(), ec.message().c_str());
        }
        return p.string();
    }();
    return dir;
}

bool ReadFile(const std::string& a_relativePath, std::string& a_outContent) {
    fs::path full = fs::path(Dir()) / a_relativePath;
    std::ifstream in(full, std::ios::binary);
    if (!in) return false;
    std::ostringstream ss;
    ss << in.rdbuf();
    a_outContent = ss.str();
    return true;
}

bool WriteFile(const std::string& a_relativePath, const std::string& a_content) {
    fs::path full = fs::path(Dir()) / a_relativePath;
    std::error_code ec;
    fs::create_directories(full.parent_path(), ec);
    std::ofstream out(full, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(a_content.data(), (std::streamsize)a_content.size());
    return out.good();
}

bool FileExists(const std::string& a_relativePath) {
    fs::path full = fs::path(Dir()) / a_relativePath;
    std::error_code ec;
    return fs::exists(full, ec) && fs::is_regular_file(full, ec);
}

long long FileSize(const std::string& a_relativePath) {
    fs::path full = fs::path(Dir()) / a_relativePath;
    std::error_code ec;
    auto sz = fs::file_size(full, ec);
    return ec ? -1 : (long long)sz;
}

bool DeleteFile(const std::string& a_relativePath) {
    fs::path full = fs::path(Dir()) / a_relativePath;
    std::error_code ec;
    fs::remove(full, ec);
    return !ec;
}

void ClearAll() {
    std::error_code ec;
    for (auto& entry : fs::directory_iterator(Dir(), ec)) {
        fs::remove_all(entry.path(), ec);
    }
    std::fprintf(stderr, "SaveData::ClearAll: wiped '%s'\n", Dir().c_str());
}

} // namespace SaveData
