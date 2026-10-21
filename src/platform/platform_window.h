#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Host-owned window and OpenGL context boundary. The renderer receives an
// already-current context and only asks the host to resize, change display
// mode, and present. A different platform frontend can implement this
// contract without introducing its window system into game/render code.
class PlatformWindow {
public:
    virtual ~PlatformWindow() = default;

    virtual bool Create(int width, int height, const char* title, bool hidden) = 0;
    virtual void Destroy() = 0;
    virtual void Resize(int width, int height) = 0;
    virtual bool SetFullscreen(bool fullscreen) = 0;
    virtual bool IsFullscreen() const = 0;
    virtual void GetSize(int& width, int& height) const = 0;
    virtual bool MakeContextCurrent() = 0;
    virtual void Present() = 0;
};

// Frontend-to-runtime messages use engine-level numeric key/button codes.
// Platform adapters translate their native event model here; game code does
// not need to include SDL (or another windowing API) to consume them.
enum class PlatformEventType {
    None, Quit, KeyDown, KeyUp, ControllerAdded, ControllerRemoved,
    ControllerButtonDown, ControllerButtonUp, MouseButtonDown,
    MouseButtonUp, Resized, TextInput
};

struct PlatformEvent {
    PlatformEventType type = PlatformEventType::None;
    int code = 0;
    int device = 0;
    int width = 0;
    int height = 0;
    bool repeat = false;
    std::string text;
};

// Stable keyboard usage codes (USB HID keyboard page), independent of the
// particular frontend library used to obtain physical key events.
namespace PlatformKeyCode {
constexpr int Equals = 46;
constexpr int Right = 79;
constexpr int Left = 80;
constexpr int Down = 81;
constexpr int Up = 82;
}

// Platform-neutral physical key identifiers. The integer values follow USB
// HID keyboard-page usages so each frontend maps its native keys to the same
// stable engine codes.
namespace KeyCode {
constexpr int Unknown=0;
constexpr int A=4, B=5, C=6, D=7, E=8, F=9, G=10, H=11, I=12, J=13, K=14, L=15;
constexpr int M=16, N=17, O=18, P=19, Q=20, R=21, S=22, T=23, U=24, V=25, W=26, X=27, Y=28, Z=29;
constexpr int Num1=30, Num2=31, Num3=32, Num4=33, Num5=34, Num6=35, Num7=36, Num8=37, Num9=38, Num0=39;
constexpr int Return=40, Escape=41, Backspace=42, Tab=43, Space=44, Minus=45, Equals=46;
constexpr int LeftBracket=47, RightBracket=48, Backslash=49, Semicolon=51, Apostrophe=52, Grave=53;
constexpr int Comma=54, Period=55, Slash=56;
constexpr int F1=58, F2=59, F3=60, F4=61, F5=62, F6=63, F7=64, F8=65, F9=66, F10=67, F11=68, F12=69;
constexpr int Insert=73, Home=74, PageUp=75, Delete=76, End=77, PageDown=78;
constexpr int Right=79, Left=80, Down=81, Up=82;
constexpr int Keypad1=89, Keypad2=90, Keypad3=91, Keypad4=92, Keypad5=93, Keypad6=94;
constexpr int Keypad7=95, Keypad8=96, Keypad9=97, Keypad0=98, KeypadMultiply=85, KeypadMinus=86, KeypadPlus=87;
constexpr int LeftControl=224, LeftShift=225, LeftAlt=226, RightControl=228, RightShift=229, RightAlt=230;
constexpr int Count=512;
}

namespace PlatformPadButton {
constexpr int A=0, B=1, X=2, Y=3, LeftShoulder=4, RightShoulder=5, Back=6, Start=7;
constexpr int LeftStick=8, RightStick=9, DpadUp=10, DpadDown=11, DpadLeft=12, DpadRight=13;
constexpr int Count=14;
}

class PlatformFrontend : public PlatformWindow {
public:
    virtual void ShowErrorMessage(const char* title, const char* message) = 0;
    // Returns true only when the user chooses to continue with the engine's
    // null fallback after a missing retail API call. False requests shutdown.
    virtual bool ShowUnimplementedFunctionDialog(const char* title,
                                                 const char* message) = 0;
    virtual bool PollEvent(PlatformEvent& event) = 0;
    virtual uint32_t Milliseconds() const = 0;
    virtual uint64_t PerformanceCounter() const = 0;
    virtual uint64_t PerformanceFrequency() const = 0;
    virtual void Delay(uint32_t milliseconds) const = 0;
    virtual const uint8_t* KeyboardState(int& keyCount) const = 0;
    virtual int KeyCodeFromName(const char* name) const = 0;
    virtual bool IsKeyDown(int keyCode) const = 0;
    virtual bool IsMouseButtonDown(int buttonCode) const = 0;
    virtual void MousePosition(int& x, int& y) const = 0;
    virtual void RefreshControllers() = 0;
    virtual int ControllerIndexForInstance(int instance) = 0;
    virtual bool IsControllerPresent(int index) = 0;
    virtual bool IsControllerButtonDown(int index, int buttonCode) = 0;
    virtual float ControllerAxis(int index, int axis) = 0;
    virtual void SetActiveControllerSoftRumble(bool enabled) = 0;
    virtual void StartTextInput() = 0;
    virtual void StopTextInput() = 0;
    virtual bool DecodeWave(const std::string& path, int outputRate,
                           std::vector<float>& stereoSamples,
                           int& sourceChannels) = 0;
    virtual bool StartAudioOutput(int sampleRate) = 0;
    virtual void StopAudioOutput() = 0;
    virtual size_t QueuedAudioBytes() = 0;
    virtual void ClearAudioQueue() = 0;
    virtual void QueueAudioSamples(const float* samples, size_t sampleCount) = 0;
};
