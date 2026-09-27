#pragma once
#include "gmMachine.h"

// are OUR OWN self-consistent scheme (real retail integer values for these
// aren't recovered), matching the four platforms real scripts actually
// compare against (OSID_PS3/NGP/OSX/WIN32, see game_data/*.gm) plus a new
// OSID_LINUX for this port.
void RegisterSystemBindings(gmMachine& a_machine);
