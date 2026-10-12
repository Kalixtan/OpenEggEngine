#include "psarc_archive.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <limits>
#include <zlib.h>

namespace {
uint32_t ReadU32BE(const unsigned char* p) {
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8) | uint32_t(p[3]);
}
uint64_t ReadU40BE(const unsigned char* p) {
    return (uint64_t(p[0]) << 32) | (uint64_t(p[1]) << 24) |
           (uint64_t(p[2]) << 16) | (uint64_t(p[3]) << 8) | uint64_t(p[4]);
}
uint32_t ReadVariableBE(const unsigned char* p, uint32_t width) {
    uint32_t value = 0;
    for (uint32_t i = 0; i < width; ++i) value = (value << 8) | p[i];
    return value;
}
std::string NormalizeArchivePath(std::string path) {
    std::replace(path.begin(), path.end(), '\\', '/');
    while (!path.empty() && path.front() == '/') path.erase(path.begin());
    std::filesystem::path normalized = std::filesystem::path(path).lexically_normal();
    path = normalized.generic_string();
    if (path == ".") return {};
    if (path == ".." || path.rfind("../", 0) == 0 || normalized.is_absolute()) return {};
    return path;
}
}

std::string PsarcArchive::LookupKey(const std::string& path) const {
    std::string key = NormalizeArchivePath(path);
    if (m_flags & 1u) {
        std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
    }
    return key;
}

bool PsarcArchive::Open(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    unsigned char header[32]{};
    if (!file.read(reinterpret_cast<char*>(header), sizeof(header)) ||
        ReadU32BE(header) != 0x50534152u || ReadU32BE(header + 4) < 0x00010003u)
        return false;
    const uint32_t tocLength = ReadU32BE(header + 12);
    const uint32_t tocEntrySize = ReadU32BE(header + 16);
    const uint32_t entryCount = ReadU32BE(header + 20);
    m_blockSize = ReadU32BE(header + 24);
    m_flags = ReadU32BE(header + 28);
    if (std::string(reinterpret_cast<char*>(header + 8), 4) != "zlib" ||
        tocEntrySize != 30 || !entryCount || !m_blockSize ||
        uint64_t(tocLength) < 32u + uint64_t(entryCount) * 30u)
        return false;
    m_path = path;

    const uint64_t fileSize = std::filesystem::file_size(path);
    uint64_t representableBlockSize = 256;
    m_blockSizeFieldWidth = 1;
    do {
        ++m_blockSizeFieldWidth;
        representableBlockSize *= 256;
    } while (representableBlockSize < m_blockSize && m_blockSizeFieldWidth < 4);
    const uint64_t blockTableBytes = tocLength - 32u - uint64_t(entryCount) * 30u;
    if (tocLength > fileSize || blockTableBytes % m_blockSizeFieldWidth != 0) return false;
    const uint64_t blockCount = blockTableBytes / m_blockSizeFieldWidth;
    if (blockCount > std::numeric_limits<size_t>::max()) return false;
    std::vector<unsigned char> toc(tocLength - 32u);
    if (!file.read(reinterpret_cast<char*>(toc.data()), static_cast<std::streamsize>(toc.size()))) return false;
    m_entries.clear();
    m_entries.reserve(entryCount);
    size_t cursor = 0;
    for (uint32_t i = 0; i < entryCount; ++i) {
        const unsigned char* e = toc.data() + cursor;
        Entry entry;
        entry.firstBlock = ReadU32BE(e + 16);
        entry.size = ReadU40BE(e + 20);
        entry.offset = ReadU40BE(e + 25);
        m_entries.push_back(std::move(entry));
        cursor += 30;
    }
    m_blockSizes.clear();
    m_blockSizes.reserve(static_cast<size_t>(blockCount));
    for (uint64_t i = 0; i < blockCount; ++i) {
        const uint32_t blockSize = ReadVariableBE(toc.data() + cursor, m_blockSizeFieldWidth);
        m_blockSizes.push_back(blockSize);
        cursor += m_blockSizeFieldWidth;
    }

    // The first PSARC entry is the newline-delimited manifest of paths for
    // entries 1..N-1; this is the format used by both supplied retail archives.
    std::string manifest;
    if (!ReadEntry(0, manifest)) return false;
    m_pathToEntry.clear();
    size_t begin = 0;
    for (uint32_t i = 1; i < entryCount; ++i) {
        size_t end = manifest.find('\n', begin);
        if (end == std::string::npos) end = manifest.size();
        std::string name = manifest.substr(begin, end - begin);
        if (!name.empty() && name.back() == '\r') name.pop_back();
        name = NormalizeArchivePath(name);
        if (name.empty()) return false;
        m_entries[i].path = name;
        m_pathToEntry.emplace(LookupKey(name), i);
        begin = end == manifest.size() ? end : end + 1;
    }
    return true;
}

bool PsarcArchive::ReadEntry(uint32_t index, std::string& output) const {
    if (index >= m_entries.size()) return false;
    const Entry& entry = m_entries[index];
    if (entry.size > std::numeric_limits<size_t>::max()) return false;
    output.clear();
    output.reserve(static_cast<size_t>(entry.size));
    if (!entry.size) return true;

    std::ifstream file(m_path, std::ios::binary);
    if (!file) return false;
    uint64_t remaining = entry.size;
    uint64_t offset = entry.offset;
    uint64_t block = entry.firstBlock;
    while (remaining) {
        if (block >= m_blockSizes.size()) return false;
        const uint32_t expected = static_cast<uint32_t>(std::min<uint64_t>(m_blockSize, remaining));
        const uint32_t stored = m_blockSizes[static_cast<size_t>(block++)];
        const uint32_t storedLength = stored ? stored : expected;
        std::string input(storedLength, '\0');
        file.seekg(static_cast<std::streamoff>(offset));
        if (!file.read(input.data(), storedLength)) return false;
        offset += storedLength;
        if (!stored) {
            output.append(input);
        } else {
            std::string decoded(expected, '\0');
            uLongf decodedLength = expected;
            const int result = uncompress(reinterpret_cast<Bytef*>(decoded.data()), &decodedLength,
                                          reinterpret_cast<const Bytef*>(input.data()), storedLength);
            if (result == Z_OK && decodedLength == expected) output.append(decoded);
            else if (storedLength == expected) output.append(input);
            else return false;
        }
        remaining -= expected;
    }
    return output.size() == entry.size;
}

bool PsarcArchive::Contains(const std::string& relativePath) const {
    return m_pathToEntry.find(LookupKey(relativePath)) != m_pathToEntry.end();
}

bool PsarcArchive::ExtractTo(const std::string& relativePath, const std::string& destination) const {
    auto found = m_pathToEntry.find(LookupKey(relativePath));
    if (found == m_pathToEntry.end()) return false;
    std::string contents;
    if (!ReadEntry(found->second, contents)) return false;
    std::filesystem::path dest(destination);
    std::error_code ec;
    std::filesystem::create_directories(dest.parent_path(), ec);
    if (ec) return false;
    std::ofstream file(destination, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
    return bool(file);
}

std::vector<std::string> PsarcArchive::ListFiles(const std::string& directory) const {
    const std::string prefix = LookupKey(directory);
    const std::string stem = prefix.empty() ? "" : prefix + "/";
    std::vector<std::string> names;
    for (const auto& entry : m_entries) {
        if (entry.path.empty()) continue;
        const std::string key = LookupKey(entry.path);
        if (key.rfind(stem, 0) != 0) continue;
        const size_t childStart = stem.size();
        const size_t slash = entry.path.find('/', childStart);
        const std::string child = entry.path.substr(childStart,
            slash == std::string::npos ? std::string::npos : slash - childStart);
        if (!child.empty() && std::find(names.begin(), names.end(), child) == names.end())
            names.push_back(child);
    }
    return names;
}
