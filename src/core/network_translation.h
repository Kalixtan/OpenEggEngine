#pragma once
#include <string>
#include <unordered_map>

// Merge entries from the PS3 service's little-endian GNU MO catalog.
bool MergeNetworkTranslations(const std::string& data,
                              std::unordered_map<std::string, std::string>& translations);
