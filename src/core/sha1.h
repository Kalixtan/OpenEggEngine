#pragma once
#include <string>

// Lowercase SHA-1 digest, used for the retail EULA version identifier.
std::string Sha1Hex(const std::string& data);
