#pragma once
#include <string>
#include <vector>

// Scripted-input harness: drives the game from a text script instead of the
// host's real keyboard/mouse, and captures screenshots on demand. Nothing
// here grabs the host input devices or a host window -- synthetic key events
// are injected straight into the same dispatch path SDL events use, and the
// held-key set the harness maintains is OR'd into the real SDL keyboard
// state so level-triggered onKeyType sees held virtual keys too.
//
// Enable with GE3_HARNESS=<script path>. Pair with GE3_HEADLESS=1 to render
// into an offscreen SDL video driver (no window appears on the host) and
// GE3_FIXED_TIMESTEP_MS=17 + GE3_NO_FPS_LIMIT=1 for deterministic,
// as-fast-as-possible runs -- GE3_HEADLESS=1 sets those two itself unless
// they're already set.
//
// Script grammar (one command per line, '#' starts a comment, blank lines
// ignored). Key names are SDL scancode names as accepted by
// SDL_GetScancodeFromName -- "Space", "Return", "Escape", "Left", "A", ...
//
//   wait <frames>            advance N frames with no input change
//   key <Name> <frames>      press Name, hold it for N frames, release
//   keydown <Name>           press and keep holding
//   keyup <Name>             release
//   mouse <left|right|middle> <frames>
//                            press a mouse button, hold N frames, release
//   shot <path>              capture the frame being rendered this frame
//   quit                     request shutdown
//
// A script that runs off the end requests shutdown, so a harness run always
// terminates on its own.
class HeadlessHarness {
public:
    // Reads GE3_HARNESS. Returns false if the harness is not enabled (or the
    // script could not be read, which is reported on stderr).
    bool Init();
    bool enabled() const { return m_enabled; }

    // Called once at the top of each frame, before input dispatch. Fills
    // a_keyDown/a_keyUp with the scancodes to dispatch as press/release
    // events this frame, and a_mouseDown/a_mouseUp with mouse codes
    // (0=left, 1=right, 2=middle) in the same integer namespace script uses.
    void BeginFrame(std::vector<int>& a_keyDown, std::vector<int>& a_keyUp,
                    std::vector<int>& a_mouseDown, std::vector<int>& a_mouseUp);

    // Scancodes currently held by the script -- dispatch onKeyType for these
    // in addition to the ones SDL reports as physically down.
    const std::vector<int>& HeldKeys() const { return m_held; }

    // Non-empty if a `shot` command fired this frame: save the rendered
    // frame here (called after the scene is drawn, before the buffer swap).
    // Cleared by the next BeginFrame.
    const std::string& PendingScreenshot() const { return m_pendingShot; }

    bool QuitRequested() const { return m_quit; }

private:
    struct Cmd {
        enum Kind { Wait, KeyHold, KeyDown, KeyUp, MouseHold, Shot, Quit } kind;
        int code = 0;     // scancode, or mouse button code
        int frames = 0;
        std::string arg;  // screenshot path
    };
    std::vector<Cmd> m_cmds;
    size_t m_pc = 0;
    int m_waitFrames = 0;       // frames left in the current blocking command
    int m_releaseCode = -1;     // key/mouse to release when the hold expires
    bool m_releaseIsMouse = false;
    std::vector<int> m_held;
    std::string m_pendingShot;
    bool m_enabled = false;
    bool m_quit = false;
};
