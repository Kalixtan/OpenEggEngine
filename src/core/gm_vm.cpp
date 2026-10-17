#include "gm_vm.h"
#include "gmMachineLib.h"
#include "gmFunctionObject.h"
#include "gmTableObject.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

float ReadNumber(gmMachine* machine, gmTableObject* table, const char* key) {
    if (!machine || !table) return 0.0f;
    const gmVariable value = table->Get(machine, key);
    if (value.m_type == GM_FLOAT) return value.m_value.m_float;
    if (value.m_type == GM_INT) return static_cast<float>(value.m_value.m_int);
    return 0.0f;
}

bool GM_CDECL TraceEditorLimitLine(gmThread* thread) {
    static unsigned int traceCount = 0;
    if (!thread || traceCount >= 5000) return false;
    const gmFunctionObject* function = thread->GetFunctionObject();
    if (!function) return false;

    const char* source = nullptr;
    const char* filename = nullptr;
    gmMachine* machine = thread->GetMachine();
    if (!machine || !machine->GetSourceCode(function->GetSourceId(), source, filename) ||
        !filename || !source) return false;

    const bool isBase = std::strstr(filename, "TouchEditProcessBase.gm") != nullptr;
    const bool isPs3 = std::strstr(filename, "TouchEditProcessPS3.gm") != nullptr;
    if (!isBase && !isPs3) return false;

    const int line = function->GetLine(thread->GetInstruction());
    const char* debugName = function->GetDebugName();
    const bool isLimitFunction = debugName &&
        (std::strstr(debugName, "enforceLimits") ||
         std::strstr(debugName, "getLevelLimitStatus"));
    const bool isKnownCallSite = isBase &&
        (line == 1741 || line == 1812 || line == 1953 ||
         line == 2614 || line == 2912);
    const bool isPs3StampSite = isPs3 && line >= 900 && line <= 920;
    if (!isLimitFunction && !isKnownCallSite && !isPs3StampSite) return false;

    const char* lineStart = source;
    for (int current = 1; current < line && *lineStart; ++current) {
        const char* newline = std::strchr(lineStart, '\n');
        if (!newline) { lineStart += std::strlen(lineStart); break; }
        lineStart = newline + 1;
    }
    const char* lineEnd = std::strchr(lineStart, '\n');
    const int lineLength = lineEnd
        ? static_cast<int>(lineEnd - lineStart)
        : static_cast<int>(std::strlen(lineStart));

    const gmVariable* thisValue = thread->GetThis();
    gmTableObject* editor = thisValue && thisValue->m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine->GetObject(thisValue->m_value.m_ref))
        : nullptr;
    const gmVariable processValue = editor
        ? editor->Get(machine, "gameProcess") : gmVariable(GM_NULL, 0);
    gmTableObject* process = processValue.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine->GetObject(processValue.m_value.m_ref))
        : nullptr;
    const gmVariable screensValue = process
        ? process->Get(machine, "screenData") : gmVariable(GM_NULL, 0);
    gmTableObject* screens = screensValue.m_type == GM_TABLE
        ? static_cast<gmTableObject*>(machine->GetObject(screensValue.m_value.m_ref))
        : nullptr;

    int screenCount = 0;
    if (screens) {
        gmTableIterator xIt;
        for (gmTableNode* xNode = screens->GetFirst(xIt); xNode;
             xNode = screens->GetNext(xIt)) {
            if (xNode->m_value.m_type != GM_TABLE) continue;
            auto* column = static_cast<gmTableObject*>(
                machine->GetObject(xNode->m_value.m_value.m_ref));
            if (column) screenCount += column->Count();
        }
    }

    std::fprintf(stderr,
        "[editor-limit-trace] file=%s line=%d function=%s globalCost=%.9g "
        "screenColumns=%d screenCount=%d source=\"%.*s\"\n",
        filename, line, debugName ? debugName : "?",
        ReadNumber(machine, process, "currentGlobalEntityCost"),
        screens ? screens->Count() : 0, screenCount,
        std::min(lineLength, 240), lineStart);
    ++traceCount;
    return false;
}

} // namespace

GmVm::GmVm() {
    m_machine.SetDebugMode(true); // readable compile/runtime error messages
    if (std::getenv("GE3_TRACE_EDITOR_LIMIT"))
        m_machine.m_line = TraceEditorLimitLine;

    // Stock GameMonkeyScript machine library -- print/format/table*/sys*/gc*.
    // (gmVersion, tableCount, print, format, sysCollectGarbage, ...) almost
    // exactly, strong evidence retail registered this same stock lib as-is.
    gmMachineLib(&m_machine);
}

GmVm::~GmVm() = default;

bool GmVm::LoadScript(const std::string& a_relativePath) {
    std::string source;
    if (!m_files.ReadTextFile(a_relativePath, source)) {
        std::fprintf(stderr, "GmVm::LoadScript: could not find '%s'\n", a_relativePath.c_str());
        return false;
    }

    int errors = m_machine.ExecuteString(source.c_str(), nullptr, true, a_relativePath.c_str());
    FlushLog();
    if (errors) {
        std::fprintf(stderr, "GmVm::LoadScript: %d compile error(s) in '%s'\n", errors, a_relativePath.c_str());
        return false;
    }
    return true;
}

void GmVm::Tick(unsigned int a_deltaMs) {
    m_machine.Execute(a_deltaMs);
    FlushLog();
}

void GmVm::FlushLog() {
    bool first = true;
    const char* message;
    while ((message = m_machine.GetLog().GetEntry(first))) {
        std::fprintf(stderr, "[gm] %s\n", message);
    }
    m_machine.GetLog().Reset();
}
