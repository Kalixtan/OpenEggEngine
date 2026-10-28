#include "sha1.h"

#include <array>
#include <cstdint>
#include <vector>

namespace {
uint32_t RotateLeft(uint32_t value, unsigned bits) {
    return (value << bits) | (value >> (32 - bits));
}
}

std::string Sha1Hex(const std::string& data) {
    std::vector<uint8_t> bytes(data.begin(), data.end());
    const uint64_t bitLength = static_cast<uint64_t>(bytes.size()) * 8;
    bytes.push_back(0x80);
    while (bytes.size() % 64 != 56) bytes.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8)
        bytes.push_back(static_cast<uint8_t>(bitLength >> shift));

    std::array<uint32_t, 5> hash = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    for (size_t offset = 0; offset < bytes.size(); offset += 64) {
        uint32_t words[80];
        for (int i = 0; i < 16; ++i) {
            const size_t p = offset + static_cast<size_t>(i) * 4;
            words[i] = (uint32_t(bytes[p]) << 24) | (uint32_t(bytes[p + 1]) << 16) |
                       (uint32_t(bytes[p + 2]) << 8) | uint32_t(bytes[p + 3]);
        }
        for (int i = 16; i < 80; ++i)
            words[i] = RotateLeft(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
        uint32_t a = hash[0], b = hash[1], c = hash[2], d = hash[3], e = hash[4];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
            else { f = b ^ c ^ d; k = 0xca62c1d6; }
            const uint32_t next = RotateLeft(a, 5) + f + e + k + words[i];
            e = d; d = c; c = RotateLeft(b, 30); b = a; a = next;
        }
        hash[0] += a; hash[1] += b; hash[2] += c; hash[3] += d; hash[4] += e;
    }
    constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(40);
    for (uint32_t word : hash)
        for (int shift = 28; shift >= 0; shift -= 4)
            result.push_back(digits[(word >> shift) & 15]);
    return result;
}
