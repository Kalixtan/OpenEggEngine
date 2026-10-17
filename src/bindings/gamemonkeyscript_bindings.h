#pragma once
#include "gmMachine.h"
#include <string>

void RegisterGameMonkeyScriptBindings(gmMachine& a_machine);

// Services one pending GameMonkeyScript.clean() pass at the native frame
// update point. Retail consumes one requested pass per frame.
void ServiceGameMonkeyScriptCleanPass(gmMachine& a_machine);

// Native manager equivalents for GameMonkeyScript.clean/isCollectingGarbage.
void RequestGameMonkeyScriptCleanPasses(int a_passes);
int GetGameMonkeyScriptCleanPassesRemaining();

gmVariable JsonStringToGmVariable(gmMachine& a_machine, const std::string& a_json, const gmVariable* a_extra = nullptr);
std::string GmVariableToJsonString(gmMachine& a_machine, const gmVariable& a_var);
