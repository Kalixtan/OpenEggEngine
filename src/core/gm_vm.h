#pragma once
#include <string>
#include "gmThread.h"
#include "gmMachine.h"
#include "file_helper.h"

class GmVm {
public:
    GmVm();
    ~GmVm();

    gmMachine& Machine() { return m_machine; }
    FileHelper& Files() { return m_files; }

    // Compiles+runs a_relativePath (resolved via FileHelper) as a new thread,
    // matching GameMonkeyScript.loadScript(path). Returns false on file-not-found
    // or compile error (details already flushed to stderr).
    bool LoadScript(const std::string& a_relativePath);

    void Tick(unsigned int a_deltaMs);

    // Flushes any pending compiler/runtime log entries to stderr.
    void FlushLog();

private:
    gmMachine m_machine;
    FileHelper m_files;
};
