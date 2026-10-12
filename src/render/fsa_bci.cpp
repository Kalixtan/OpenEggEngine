#include "fsa_bci.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>
#include <zlib.h>

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

uint32_t ReadU32BE(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

uint8_t PaethPredictor(uint8_t a, uint8_t b, uint8_t c) {
    const int p = (int)a + (int)b - (int)c;
    const int pa = std::abs(p - (int)a);
    const int pb = std::abs(p - (int)b);
    const int pc = std::abs(p - (int)c);
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

// Retail animation frame resources include both indexed PNGs (which use the
// same generated 16-colour Animation palette as BCI) and a small set of
// direct-colour PNGs. Keep indexed pixels as indices: expanding PLTE to RGB
// here would discard the palette-bank behavior used by the native renderer.
bool DecodePng(const uint8_t* data, size_t size, BciFrame& out) {
    static constexpr uint8_t signature[] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (size < sizeof(signature) || std::memcmp(data, signature, sizeof(signature)) != 0)
        return false;

    uint32_t width = 0, height = 0;
    uint8_t bitDepth = 0, colourType = 0, interlace = 0;
    std::vector<uint8_t> compressed;
    std::vector<uint8_t> palette;
    bool haveHeader = false, havePalette = false, haveEnd = false;
    size_t offset = sizeof(signature);
    while (offset + 12 <= size) {
        const uint32_t length = ReadU32BE(data + offset);
        const uint8_t* type = data + offset + 4;
        if ((size_t)length > size - offset - 12) return false;
        const uint8_t* chunk = data + offset + 8;
        if (std::memcmp(type, "IHDR", 4) == 0) {
            if (length != 13 || haveHeader) return false;
            width = ReadU32BE(chunk);
            height = ReadU32BE(chunk + 4);
            bitDepth = chunk[8];
            colourType = chunk[9];
            if (chunk[10] != 0 || chunk[11] != 0) return false;
            interlace = chunk[12];
            haveHeader = true;
        } else if (std::memcmp(type, "PLTE", 4) == 0) {
            if (length == 0 || length % 3 != 0 || length > 768) return false;
            palette.assign(chunk, chunk + length);
            havePalette = true;
        } else if (std::memcmp(type, "IDAT", 4) == 0) {
            if ((size_t)length > compressed.max_size() - compressed.size()) return false;
            compressed.insert(compressed.end(), chunk, chunk + length);
        } else if (std::memcmp(type, "IEND", 4) == 0) {
            haveEnd = true;
            break;
        }
        offset += (size_t)length + 12;
    }
    if (!haveHeader || !haveEnd || width == 0 || height == 0 || interlace != 0 ||
        width > (uint32_t)std::numeric_limits<int>::max() ||
        height > (uint32_t)std::numeric_limits<int>::max() ||
        bitDepth != 8 || (colourType != 2 && colourType != 3) || compressed.empty())
        return false;

    // The shipped indexed PNG frame set has a 256-entry PLTE but pixel
    // indices only use the 16 slots consumed by FUN_00047498. Preserve the
    // bytes, not PLTE's template RGB values; alpha comes from that palette.
    if ((colourType == 3 && (!havePalette || palette.size() < 16 * 3)) ||
        (size_t)width > std::numeric_limits<size_t>::max() / height)
        return false;
    const size_t bytesPerPixel = colourType == 2 ? 3 : 1;
    if ((size_t)width > std::numeric_limits<size_t>::max() / bytesPerPixel)
        return false;
    const size_t rowBytes = (size_t)width * bytesPerPixel;
    if (rowBytes == std::numeric_limits<size_t>::max() ||
        rowBytes + 1 > std::numeric_limits<size_t>::max() / height)
        return false;
    const size_t rawSize = (rowBytes + 1) * (size_t)height;
    if (rawSize > (size_t)std::numeric_limits<uLongf>::max() ||
        compressed.size() > (size_t)std::numeric_limits<uLong>::max())
        return false;
    std::vector<uint8_t> filtered(rawSize);
    uLongf actualSize = (uLongf)filtered.size();
    if (uncompress(filtered.data(), &actualSize, compressed.data(),
                   (uLong)compressed.size()) != Z_OK || actualSize != rawSize)
        return false;

    const size_t pixelCount = (size_t)width * (size_t)height;
    if (colourType == 2 && pixelCount > std::numeric_limits<size_t>::max() / 4)
        return false;
    std::vector<uint8_t> decoded(rowBytes * (size_t)height);
    for (size_t y = 0; y < height; ++y) {
        const uint8_t filter = filtered[y * (rowBytes + 1)];
        if (filter > 4) return false;
        const uint8_t* source = filtered.data() + y * (rowBytes + 1) + 1;
        uint8_t* dest = decoded.data() + y * rowBytes;
        const uint8_t* previous = y ? dest - rowBytes : nullptr;
        for (size_t x = 0; x < rowBytes; ++x) {
            const uint8_t left = x >= bytesPerPixel ? dest[x - bytesPerPixel] : 0;
            const uint8_t above = previous ? previous[x] : 0;
            const uint8_t upperLeft = previous && x >= bytesPerPixel
                ? previous[x - bytesPerPixel] : 0;
            uint8_t predictor = 0;
            switch (filter) {
                case 0: break;
                case 1: predictor = left; break;
                case 2: predictor = above; break;
                case 3: predictor = (uint8_t)(((unsigned)left + above) / 2); break;
                case 4: predictor = PaethPredictor(left, above, upperLeft); break;
            }
            dest[x] = (uint8_t)(source[x] + predictor);
        }
    }

    out.width = (int)width;
    out.height = (int)height;
    if (colourType == 3) {
        out.indices = std::move(decoded);
        for (uint8_t index : out.indices)
            if (index > 15) return false;
    } else {
        out.rgba.resize(pixelCount * 4);
        for (size_t i = 0; i < pixelCount; ++i) {
            out.rgba[i * 4 + 0] = decoded[i * 3 + 0];
            out.rgba[i * 4 + 1] = decoded[i * 3 + 1];
            out.rgba[i * 4 + 2] = decoded[i * 3 + 2];
            out.rgba[i * 4 + 3] = 255;
        }
    }
    return true;
}

} // namespace

bool FsaAsset::LoadFromFile(const std::string& a_path) {
    frames.clear();
    customSequence.clear();
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

    // Keep the relative order of decoded image entries exactly as stored in
    // the FSA directory. Retail FUN_000b8808 walks those entries in directory
    // order and appends each decoded image to the animation sequence; sorting
    // filenames here would change playback for a non-lexically ordered file.
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

        BciFrame frame;
        bool decoded = false;
        const bool isBci = name.size() >= 4 &&
            name.compare(name.size() - 4, 4, ".bci") == 0;
        const bool isPng = name.size() >= 4 &&
            name.compare(name.size() - 4, 4, ".png") == 0;
        if (isBci)
            decoded = DecodeBci(data + fileOff, fileSize, frame);
        else if (isPng)
            decoded = DecodePng(data + fileOff, fileSize, frame);
        if (decoded) {
            named.emplace_back(std::move(name), std::move(frame));
        } else if (isBci || isPng) {
            std::fprintf(stderr,
                "FsaAsset::LoadFromFile: failed to decode frame '%s' in '%s' (%u bytes)\n",
                name.c_str(), a_path.c_str(), (unsigned)fileSize);
        }
    }
    frames.reserve(named.size());
    for (auto& n : named) frames.push_back(std::move(n.second));
    return true;
}
