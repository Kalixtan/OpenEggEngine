#include "headless_harness.h"
#include "../bindings/input_bindings.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace {

int MouseCodeFromName(const std::string& a_name) {
    if (a_name == "left") return 0;
    if (a_name == "right") return 1;
    if (a_name == "middle") return 2;
    return -1;
}

} // namespace

bool HeadlessHarness::Init(PlatformFrontend& frontend) {
    const char* path = std::getenv("GE3_HARNESS");
    if (!path || !*path) return false;

    std::ifstream in(std::filesystem::u8path(path));
    if (!in) {
        std::fprintf(stderr, "[harness] cannot open script '%s'\n", path);
        return false;
    }

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        // Strip comments and surrounding whitespace.
        size_t hash = line.find('#');
        if (hash != std::string::npos) line.erase(hash);
        std::istringstream ls(line);
        std::string op;
        if (!(ls >> op)) continue;

        Cmd c;
        if (op == "wait") {
            c.kind = Cmd::Wait;
            if (!(ls >> c.frames)) { std::fprintf(stderr, "[harness] line %d: wait needs a frame count\n", lineNo); continue; }
        } else if (op == "key" || op == "keydown" || op == "keyup") {
            std::string name;
            if (!(ls >> name)) { std::fprintf(stderr, "[harness] line %d: %s needs a key name\n", lineNo, op.c_str()); continue; }
            const int keyCode = frontend.KeyCodeFromName(name.c_str());
            if (keyCode == KeyCode::Unknown) { std::fprintf(stderr, "[harness] line %d: unknown key '%s'\n", lineNo, name.c_str()); continue; }
            c.code = keyCode;
            if (op == "key") {
                c.kind = Cmd::KeyHold;
                if (!(ls >> c.frames)) c.frames = 1;
            } else {
                c.kind = (op == "keydown") ? Cmd::KeyDown : Cmd::KeyUp;
            }
        } else if (op == "mouse") {
            std::string name;
            if (!(ls >> name)) { std::fprintf(stderr, "[harness] line %d: mouse needs a button name\n", lineNo); continue; }
            c.code = MouseCodeFromName(name);
            if (c.code < 0) { std::fprintf(stderr, "[harness] line %d: unknown mouse button '%s'\n", lineNo, name.c_str()); continue; }
            c.kind = Cmd::MouseHold;
            if (!(ls >> c.frames)) c.frames = 1;
        } else if (op == "move") {
            c.kind = Cmd::MouseMove;
            if (!(ls >> c.code >> c.frames)) {
                std::fprintf(stderr, "[harness] line %d: move needs x and y pixels\n", lineNo);
                continue;
            }
        } else if (op == "shot") {
            c.kind = Cmd::Shot;
            if (!(ls >> c.arg)) { std::fprintf(stderr, "[harness] line %d: shot needs a path\n", lineNo); continue; }
        } else if (op == "quit") {
            c.kind = Cmd::Quit;
        } else {
            std::fprintf(stderr, "[harness] line %d: unknown command '%s'\n", lineNo, op.c_str());
            continue;
        }
        m_cmds.push_back(c);
    }

    m_enabled = true;
    std::fprintf(stderr, "[harness] loaded %zu commands from %s\n", m_cmds.size(), path);
    return true;
}

void HeadlessHarness::BeginFrame(std::vector<int>& a_keyDown, std::vector<int>& a_keyUp,
                                 std::vector<int>& a_mouseDown, std::vector<int>& a_mouseUp) {
    m_pendingShot.clear();
    if (!m_enabled || m_quit) return;

    // A hold in progress: count it down, releasing on the frame it expires.
    if (m_waitFrames > 0) {
        if (--m_waitFrames == 0 && m_releaseCode >= 0) {
            if (m_releaseIsMouse) {
                a_mouseUp.push_back(m_releaseCode);
                SetVirtualMouseButtonDown(m_releaseCode, false);
            } else {
                a_keyUp.push_back(m_releaseCode);
                m_held.erase(std::remove(m_held.begin(), m_held.end(), m_releaseCode), m_held.end());
            }
            m_releaseCode = -1;
        }
        if (m_waitFrames > 0) return;
    }

    // Run non-blocking commands until the next one that occupies frames.
    while (m_pc < m_cmds.size()) {
        const Cmd& c = m_cmds[m_pc++];
        switch (c.kind) {
        case Cmd::Wait:
            m_waitFrames = c.frames > 0 ? c.frames : 0;
            m_releaseCode = -1;
            if (m_waitFrames > 0) return;
            break;
        case Cmd::KeyHold:
            a_keyDown.push_back(c.code);
            if (std::find(m_held.begin(), m_held.end(), c.code) == m_held.end()) m_held.push_back(c.code);
            m_waitFrames = c.frames > 0 ? c.frames : 1;
            m_releaseCode = c.code;
            m_releaseIsMouse = false;
            return;
        case Cmd::MouseHold:
            a_mouseDown.push_back(c.code);
            SetVirtualMouseButtonDown(c.code, true);
            m_waitFrames = c.frames > 0 ? c.frames : 1;
            m_releaseCode = c.code;
            m_releaseIsMouse = true;
            return;
        case Cmd::MouseMove:
            SetVirtualMousePosition(c.code, c.frames);
            break;
        case Cmd::KeyDown:
            a_keyDown.push_back(c.code);
            if (std::find(m_held.begin(), m_held.end(), c.code) == m_held.end()) m_held.push_back(c.code);
            break;
        case Cmd::KeyUp:
            a_keyUp.push_back(c.code);
            m_held.erase(std::remove(m_held.begin(), m_held.end(), c.code), m_held.end());
            break;
        case Cmd::Shot:
            m_pendingShot = c.arg;
            // The capture happens after this frame renders, so the command
            // consumes the frame -- otherwise two `shot`s in a row would
            // both try to claim the same frame and the first would be lost.
            m_waitFrames = 1;
            m_releaseCode = -1;
            return;
        case Cmd::Quit:
            m_quit = true;
            return;
        }
    }

    // Script exhausted -- shut down rather than hanging in a window nobody
    // is watching.
    if (m_pc >= m_cmds.size()) {
        std::fprintf(stderr, "[harness] script complete, quitting\n");
        m_quit = true;
    }
}
