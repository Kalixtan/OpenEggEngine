#include "fsa_bci.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <utility>

namespace {

uint32_t ReadU32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
uint16_t ReadU16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

bool DecodeBci(const uint8_t* a_data, size_t a_size, BciFrame& a_out) {
    if (a_size < 16 || std::memcmp(a_data, "BCI ", 4) != 0) return false;
    a_out.width = (int)ReadU16(a_data + 4) * 2;
    a_out.height = (int)ReadU16(a_data + 6);
    if (a_out.width <= 0 || a_out.height <= 0) return false;

    size_t total = (size_t)a_out.width * (size_t)a_out.height;
    a_out.indices.assign(total, 0);

    size_t offset = 16;
    size_t i = 0;
    while (offset < a_size && i < total) {
        uint8_t col = a_data[offset++];
        if (offset >= a_size) break;
        int count = a_data[offset++];
        if (count == 0xFF) {
            if (offset >= a_size) break;
            count = a_data[offset++] * 0xFF;
        }
        for (int n = 0; n < count && i < total; ++n) {
            a_out.indices[i++] = col & 0xF;
            if (i >= total) break;
            a_out.indices[i++] = (col >> 4) & 0xF;
        }
    }
    // Short decodes (confirmed real, e.g. the degenerate 2x2 stub frame
    // documented in FSA_BCI.md) are left zero-padded rather than treated
    // as an error -- matches the Python reference's own behavior.
    return true;
}

} // namespace

bool FsaAsset::LoadFromFile(const std::string& a_path) {
    frames.clear();
    std::ifstream f(a_path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string buf = ss.str();
    const uint8_t* data = (const uint8_t*)buf.data();
    size_t size = buf.size();

    if (size < 0x40) return false;
    uint32_t magic = ReadU32(data);
    if (magic != 0x00BFF5A7u) return false;

    int fileCount = (int)ReadU16(data + 8);
    size_t endOfHead = 0x40 + (size_t)fileCount * 0x80;
    if (endOfHead > size) return false;

    // (name, frame) pairs, sorted by name below -- directory order matches
    // filename order in every file checked so far (sequential
    // NNNN.bci naming), but sorting explicitly is a cheap safety net
    // against a container where it doesn't.
    std::vector<std::pair<std::string, BciFrame>> named;
    for (int i = 0; i < fileCount; ++i) {
        size_t entryOff = 0x40 + (size_t)i * 0x80;
        if (entryOff + 0x80 > size) break;
        uint32_t relOff = ReadU32(data + entryOff);
        uint32_t fileSize = ReadU32(data + entryOff + 4);
        const char* nameBytes = (const char*)(data + entryOff + 12);
        size_t nameMax = 0x38;
        size_t nameLen = 0;
        while (nameLen < nameMax && nameBytes[nameLen] != '\0') ++nameLen;
        std::string name(nameBytes, nameLen);

        size_t fileOff = endOfHead + relOff;
        if (fileOff + fileSize > size || fileOff > size) continue;

        // Confirmed real: some frames are plain .png instead of .bci
        // (216 seen across the corpus) -- not decoded here (no real usage
        // in the entities this pass targets), skipped rather than
        // mis-parsed as BCI.
        if (name.size() < 4 || name.compare(name.size() - 4, 4, ".bci") != 0) continue;

        BciFrame frame;
        if (DecodeBci(data + fileOff, fileSize, frame)) {
            named.emplace_back(std::move(name), std::move(frame));
        }
    }
    std::sort(named.begin(), named.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    frames.reserve(named.size());
    for (auto& n : named) frames.push_back(std::move(n.second));
    return true;
}
