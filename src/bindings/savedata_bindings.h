#pragma once
#include "gmMachine.h"
#include <string>

void RegisterSaveDataBindings(gmMachine& a_machine);
gmVariable MakeSaveDataSizeTicket(gmMachine& a_machine, const std::string& a_filename,
                                  long long a_size, bool a_done = true,
                                  bool a_error = false, bool a_preserveSize = false);
