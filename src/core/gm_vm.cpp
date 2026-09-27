#include "gm_vm.h"
#include "gmMachineLib.h"
#include <cstdio>

GmVm::GmVm() {
    m_machine.SetDebugMode(true); // readable compile/runtime error messages

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
