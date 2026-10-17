#pragma once
#include "gmMachine.h"
#include <string>

// / VM control functions" cluster; loadScript's signature independently
// confirmed by diagnostic scene Main.gm's own comment: 'matches
// GameMonkeyScript.loadScript("FontHelper.gm") in TESTPROG exactly').
// Most stock VM-introspection functions (print, format, table*, sys*, gc*)
// come for free from the real gmMachineLib() call in GmVm's constructor --
// this file only adds the engine-specific loadScript.
void RegisterGameMonkeyScriptBindings(gmMachine& a_machine);

// Services one pending GameMonkeyScript.clean() pass at the native frame
// update point. Retail consumes one requested pass per frame.
void ServiceGameMonkeyScriptCleanPass(gmMachine& a_machine);

// Native manager equivalents for GameMonkeyScript.clean/isCollectingGarbage.
void RequestGameMonkeyScriptCleanPasses(int a_passes);
int GetGameMonkeyScriptCleanPassesRemaining();

// Exposed for Debug.GameMonkey2JSON/JSON2GameMonkey (system_bindings.cpp) --
// see GitHub issue #7. Shares the same JSON<->gmVariable mapping
// saveTableToJsonSaveRequest/loadTableFromJsonSaveRequest already use
// internally, so the two paths can't silently drift apart.
//
// JsonStringToGmVariable(json): parses json and returns the equivalent
// gmVariable (table/number/string/bool/null), matching JsonToGmVariable's
// array-vs-keyed-table handling.
// JsonStringToGmVariable(json, extraTable): confirmed real 2-arg overload
// of JSON2GameMonkey -- merges the decoded object's keys into the
// caller-supplied table instead of allocating a fresh one.
gmVariable JsonStringToGmVariable(gmMachine& a_machine, const std::string& a_json, const gmVariable* a_extra = nullptr);
std::string GmVariableToJsonString(gmMachine& a_machine, const gmVariable& a_var);
