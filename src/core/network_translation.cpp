#include "network_translation.h"
#include <cstdint>
#include <utility>

bool MergeNetworkTranslations(const std::string& data,
                              std::unordered_map<std::string, std::string>& translations) {
    auto read = [&](size_t offset, uint32_t& value) {
        if (offset > data.size() || data.size() - offset < 4) return false;
        const auto* bytes = reinterpret_cast<const unsigned char*>(data.data() + offset);
        value = uint32_t(bytes[0]) | (uint32_t(bytes[1]) << 8) |
                (uint32_t(bytes[2]) << 16) | (uint32_t(bytes[3]) << 24);
        return true;
    };
    uint32_t magic, count, originals, values;
    if (!read(0, magic) || magic != 0x950412de || !read(8, count) ||
        !read(12, originals) || !read(16, values)) return false;
    if (originals > data.size() || values > data.size() ||
        count > (data.size() - originals) / 8 || count > (data.size() - values) / 8) return false;
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t keyLength, keyOffset, valueLength, valueOffset;
        if (!read(size_t(originals) + size_t(i) * 8, keyLength) ||
            !read(size_t(originals) + size_t(i) * 8 + 4, keyOffset) ||
            !read(size_t(values) + size_t(i) * 8, valueLength) ||
            !read(size_t(values) + size_t(i) * 8 + 4, valueOffset) ||
            keyOffset > data.size() || keyLength > data.size() - keyOffset ||
            valueOffset > data.size() || valueLength > data.size() - valueOffset) return false;
        std::string key = data.substr(keyOffset, keyLength);
        std::string value = data.substr(valueOffset, valueLength);
        const auto keyEnd = key.find('\0');
        if (keyEnd != std::string::npos) key.resize(keyEnd);
        const auto valueEnd = value.find('\0');
        if (valueEnd != std::string::npos) value.resize(valueEnd);
        if (!key.empty()) {
            auto existing = translations.find(key);
            // Retail compares only the incoming value's length; an empty or
            // shorter prefix therefore leaves an existing entry unchanged.
            if (existing != translations.end() &&
                existing->second.compare(0, value.size(), value) == 0) continue;
            translations[std::move(key)] = std::move(value);
        }
    }
    return true;
}
