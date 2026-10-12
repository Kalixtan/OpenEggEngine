#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// PSARC 1.4/zlib archive reader. Verified against standard PSARC TOC and
// manifest layout and the retail game_data.psarc / patch_data001.psarc files.
class PsarcArchive {
public:
    bool Open(const std::string& path);
    bool Contains(const std::string& relativePath) const;
    bool ExtractTo(const std::string& relativePath, const std::string& destination) const;
    std::vector<std::string> ListFiles(const std::string& directory) const;

private:
    struct Entry {
        uint32_t firstBlock = 0;
        uint64_t size = 0;
        uint64_t offset = 0;
        std::string path;
    };

    bool ReadEntry(uint32_t index, std::string& output) const;
    std::string LookupKey(const std::string& path) const;

    std::string m_path;
    uint32_t m_blockSize = 0;
    uint32_t m_blockSizeFieldWidth = 0;
    uint32_t m_flags = 0;
    std::vector<Entry> m_entries;
    std::vector<uint32_t> m_blockSizes;
    std::unordered_map<std::string, uint32_t> m_pathToEntry;
};
