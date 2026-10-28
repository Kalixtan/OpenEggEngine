#pragma once
#include "gmVariable.h"
#include <string>
class gmMachine;

// Inflate a network level resource and convert its XML or JSON table.
gmVariable ParseNetworkLevelData(gmMachine& machine, const std::string& compressed);
