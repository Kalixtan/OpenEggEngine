#include "network_translation.h"
#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>

static void Put32(std::string& data, size_t offset, uint32_t value) {
    for (int i = 0; i < 4; ++i) data[offset + i] = static_cast<char>(value >> (8 * i));
}

int main() {
    std::unordered_map<std::string, std::string> entries{{"keep", "old"}};
    std::string catalog(44, '\0');
    Put32(catalog, 0, 0x950412de);
    Put32(catalog, 8, 1);
    Put32(catalog, 12, 28);
    Put32(catalog, 16, 36);
    Put32(catalog, 28, 4);
    Put32(catalog, 32, 44);
    Put32(catalog, 36, 3);
    Put32(catalog, 40, 49);
    catalog.append("keep\0new\0", 9);
    assert(MergeNetworkTranslations(catalog, entries));
    assert(entries.at("keep") == "new");
    Put32(catalog, 40, 0xffffffff);
    assert(!MergeNetworkTranslations(catalog, entries));
    assert(entries.at("keep") == "new");
    assert(!MergeNetworkTranslations("bad", entries));
}
