#include "sdl_frontend.h"
#include "frontend_factory.h"

#include <algorithm>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <vector>
#include <filesystem>
#include <string>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace {
int NormalizePadButton(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return PlatformPadButton::A;
        case SDL_CONTROLLER_BUTTON_B: return PlatformPadButton::B;
        case SDL_CONTROLLER_BUTTON_X: return PlatformPadButton::X;
        case SDL_CONTROLLER_BUTTON_Y: return PlatformPadButton::Y;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return PlatformPadButton::LeftShoulder;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return PlatformPadButton::RightShoulder;
        case SDL_CONTROLLER_BUTTON_BACK: return PlatformPadButton::Back;
        case SDL_CONTROLLER_BUTTON_START: return PlatformPadButton::Start;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: return PlatformPadButton::LeftStick;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: return PlatformPadButton::RightStick;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return PlatformPadButton::DpadUp;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return PlatformPadButton::DpadDown;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return PlatformPadButton::DpadLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return PlatformPadButton::DpadRight;
        default: return -1;
    }
}

SDL_GameControllerButton NativePadButton(int button) {
    static const SDL_GameControllerButton map[PlatformPadButton::Count] = {
        SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_BACK, SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_LEFTSTICK, SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT
    };
    return button >= 0 && button < PlatformPadButton::Count
        ? map[button] : SDL_CONTROLLER_BUTTON_INVALID;
}

}

std::unique_ptr<PlatformFrontend> CreatePlatformFrontend() {
    return std::make_unique<SdlFrontend>();
}

bool SdlFrontend::Create(int width, int height, const char* title, bool hidden) {
    if (m_window || m_context) Destroy();
#if defined(_WIN32) && defined(SDL_MAIN_HANDLED)
    SDL_SetMainReady();
#endif
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return false;
    }
    m_sdlInitialized = true;

    // Compatibility is required by the current OpenGL 2.1 fixed-function
    // renderer. A core-profile migration is a separate renderer project.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    const Uint32 visibility = hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN;
    // Try multisampling first, then lower sample counts, and finally a plain
    // context. Some drivers cannot provide the requested format at the
    // desktop's current resolution, so AA must not prevent startup.
    constexpr int sampleCounts[] = {4, 2, 0};
    for (int samples : sampleCounts) {
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, samples > 0 ? 1 : 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, samples);

        m_window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                    width, height,
                                    SDL_WINDOW_OPENGL | visibility | SDL_WINDOW_RESIZABLE);
        if (m_window) m_context = SDL_GL_CreateContext(m_window);
        if (m_window && m_context) break;

        const char* error = SDL_GetError();
        std::fprintf(stderr, "SDL OpenGL setup with %d samples failed: %s\n",
                     samples, error ? error : "unknown SDL error");
        if (m_context) {
            SDL_GL_DeleteContext(m_context);
            m_context = nullptr;
        }
        if (m_window) {
            SDL_DestroyWindow(m_window);
            m_window = nullptr;
        }
    }
    if (!m_window || !m_context) {
        Destroy();
        return false;
    }
    SDL_GL_SetSwapInterval(1);
    return true;
}

void SdlFrontend::ShowErrorMessage(const char* title, const char* message) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, m_window);
}

bool SdlFrontend::ShowServerLoginDialog(ServerLoginDetails& details) {
    constexpr int width = 500;
    constexpr int height = 430;
    SDL_Window* window = SDL_CreateWindow("OpenEgg sign in", SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_ALWAYS_ON_TOP);
    if (!window) return false;
    if (m_window) SDL_SetWindowModalFor(window, m_window);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenEgg sign in",
            "Could not start the sign-in window.", m_window);
        SDL_DestroyWindow(window);
        return false;
    }

    FT_Library library = nullptr;
    FT_Face face = nullptr;
    if (FT_Init_FreeType(&library) == 0) {
#if defined(_WIN32)
        const char* fontPaths[] = {"C:/Windows/Fonts/segoeui.ttf", "C:/Windows/Fonts/arial.ttf"};
#elif defined(__APPLE__)
        const char* fontPaths[] = {"/System/Library/Fonts/Supplemental/Arial.ttf"};
#else
        const char* fontPaths[] = {"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                                   "/usr/share/fonts/TTF/DejaVuSans.ttf",
                                   "/usr/share/fonts/noto/NotoSans-Regular.ttf",
                                   "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
                                   "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
                                   "/usr/share/fonts/truetype/freefont/FreeSans.ttf"};
#endif
        for (const char* path : fontPaths)
            if (FT_New_Face(library, path, 0, &face) == 0) break;
        if (face) FT_Set_Pixel_Sizes(face, 0, 19);
    }
    if (!face) {
        if (library) FT_Done_FreeType(library);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenEgg sign in",
            "No supported system font was found for the sign-in window.", m_window);
        return false;
    }

    auto text = [&](const std::string& value, int x, int baseline, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        int pen = x;
        for (unsigned char character : value) {
            if (FT_Load_Char(face, character, FT_LOAD_RENDER) != 0) continue;
            const FT_Bitmap& bitmap = face->glyph->bitmap;
            const int left = pen + face->glyph->bitmap_left;
            const int top = baseline - face->glyph->bitmap_top;
            for (unsigned int row = 0; row < bitmap.rows; ++row) {
                const int pitchRow = bitmap.pitch >= 0 ? static_cast<int>(row) :
                    static_cast<int>(bitmap.rows - 1 - row);
                const unsigned char* pixels = bitmap.buffer + pitchRow * std::abs(bitmap.pitch);
                for (unsigned int column = 0; column < bitmap.width; ++column) {
                    if (pixels[column] > 96)
                        SDL_RenderDrawPoint(renderer, left + static_cast<int>(column), top + static_cast<int>(row));
                }
            }
            pen += static_cast<int>(face->glyph->advance.x >> 6);
        }
    };
    auto fill = [&](SDL_Rect rect, SDL_Color color) {
        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &rect);
    };

    bool running = true;
    bool accepted = false;
    int activeField = details.createAccount ? 0 : 1;
    SDL_StartTextInput();
    while (running) {
        SDL_Event event;
        while (SDL_WaitEventTimeout(&event, 16)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE) {
                running = false;
                break;
            }
            if (event.type == SDL_TEXTINPUT) {
                std::string* fields[] = {&details.username, &details.email,
                                          &details.password, &details.passwordConfirm};
                std::string& field = *fields[activeField];
                const size_t limit = activeField >= 2 ? 128 : 96;
                if (field.size() + std::strlen(event.text.text) <= limit) field += event.text.text;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) running = false;
                else if (event.key.keysym.sym == SDLK_TAB) {
                    const int first = details.createAccount ? 0 : 1;
                    activeField = first + (activeField - first + 1) % (details.createAccount ? 4 : 2);
                } else if (event.key.keysym.sym == SDLK_BACKSPACE) {
                    std::string* fields[] = {&details.username, &details.email,
                                              &details.password, &details.passwordConfirm};
                    std::string& field = *fields[activeField];
                    if (!field.empty()) {
                        size_t start = field.size() - 1;
                        while (start > 0 && (static_cast<unsigned char>(field[start]) & 0xc0) == 0x80) --start;
                        field.erase(start);
                    }
                } else if (event.key.keysym.sym == SDLK_RETURN || event.key.keysym.sym == SDLK_KP_ENTER) {
                    if (!details.email.empty() && !details.password.empty() &&
                        (!details.createAccount || (!details.username.empty() &&
                         details.password == details.passwordConfirm))) {
                        accepted = true;
                        running = false;
                    }
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                const int x = event.button.x, y = event.button.y;
                if (details.createAccount && y >= 78 && y < 119) activeField = 0;
                else if (y >= (details.createAccount ? 138 : 89) &&
                         y < (details.createAccount ? 179 : 130)) activeField = 1;
                else if (y >= (details.createAccount ? 198 : 149) &&
                         y < (details.createAccount ? 239 : 190)) activeField = 2;
                else if (details.createAccount && y >= 258 && y < 299) activeField = 3;
                else if (y >= (details.createAccount ? 328 : 298) &&
                         y < (details.createAccount ? 354 : 326) && x >= 36 && x < 250)
                    details.remember = !details.remember;
                else if (y >= 370 && y < 414) {
                    if (x < 220) {
                        if (!details.createAccount && !details.email.empty() && !details.password.empty()) {
                            accepted = true;
                            running = false;
                        } else if (details.createAccount && !details.username.empty() &&
                                   !details.email.empty() && !details.password.empty() &&
                                   details.password == details.passwordConfirm) {
                            accepted = true;
                            running = false;
                        }
                    } else if (x < 370) {
                        details.createAccount = !details.createAccount;
                        activeField = details.createAccount ? 0 : 1;
                    } else running = false;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 30, 34, 42, 255);
        SDL_RenderClear(renderer);
        text(details.createAccount ? "Create an OpenEgg account" : "Sign in to the server",
             28, 42, {245, 245, 245, 255});
        auto drawField = [&](const char* label, const std::string& value, int y, int index, bool secret) {
            text(label, 30, y, {215, 219, 226, 255});
            SDL_Rect box{28, y + 7, 444, 39};
            fill(box, index == activeField ? SDL_Color{71, 83, 103, 255} : SDL_Color{52, 58, 69, 255});
            SDL_Rect border{box.x, box.y, box.w, 2};
            fill(border, index == activeField ? SDL_Color{113, 164, 232, 255} : SDL_Color{90, 98, 112, 255});
            std::string display = secret ? std::string(value.size(), '*') : value;
            if (display.size() > 48) display = display.substr(display.size() - 48);
            text(display, 40, y + 33, {255, 255, 255, 255});
        };
        int y = details.createAccount ? 65 : 82;
        if (details.createAccount) {
            drawField("Username", details.username, y, 0, false);
            y += 60;
        }
        drawField("Email", details.email, y, 1, false);
        y += 60;
        drawField("Password", details.password, y, 2, true);
        if (details.createAccount) {
            y += 60;
            drawField("Confirm password", details.passwordConfirm, y, 3, true);
        }
        const int rememberY = details.createAccount ? 348 : 320;
        text(details.remember ? "[x] Remember me on this computer" : "[ ] Remember me on this computer",
             35, rememberY, {215, 219, 226, 255});
        fill({28, 370, 190, 42}, {78, 130, 91, 255});
        text(details.createAccount ? "Create account" : "Sign in", 49, 397, {255, 255, 255, 255});
        fill({232, 370, 130, 42}, {71, 83, 103, 255});
        text(details.createAccount ? "Sign in" : "Register", 244, 397, {255, 255, 255, 255});
        fill({372, 370, 100, 42}, {71, 83, 103, 255});
        text("Cancel", 394, 397, {255, 255, 255, 255});
        SDL_RenderPresent(renderer);
    }
    SDL_StopTextInput();
    if (face) FT_Done_Face(face);
    if (library) FT_Done_FreeType(library);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    if (m_window && m_context) SDL_GL_MakeCurrent(m_window, m_context);
    return accepted;
}

bool SdlFrontend::ShowUnimplementedFunctionDialog(const char* title,
                                                  const char* message) {
    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Continue"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "Quit"},
    };
    const SDL_MessageBoxData dialog = {
        SDL_MESSAGEBOX_WARNING,
        m_window,
        title,
        message,
        static_cast<int>(std::size(buttons)),
        buttons,
        nullptr,
    };
    int selectedButton = 0;
    if (SDL_ShowMessageBox(&dialog, &selectedButton) != 0) return false;
    return selectedButton == 1;
}

void SdlFrontend::Destroy() {
    StopAudioOutput();
    for (SDL_GameController* controller : m_controllers)
        if (controller) SDL_GameControllerClose(controller);
    m_controllers.clear();
    m_controllerDeviceCount = -1;
    if (m_context) {
        SDL_GL_DeleteContext(m_context);
        m_context = nullptr;
    }
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    if (m_sdlInitialized) {
        SDL_Quit();
        m_sdlInitialized = false;
    }
    m_fullscreen = false;
}

void SdlFrontend::Resize(int width, int height) {
    if (m_window && width > 0 && height > 0) SDL_SetWindowSize(m_window, width, height);
}

bool SdlFrontend::SetFullscreen(bool fullscreen) {
    if (!m_window) return false;
    const Uint32 flags = fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    if (SDL_SetWindowFullscreen(m_window, flags) != 0) {
        std::fprintf(stderr, "SDL_SetWindowFullscreen failed: %s\n", SDL_GetError());
        return false;
    }
    m_fullscreen = fullscreen;
    return true;
}

void SdlFrontend::GetSize(int& width, int& height) const {
    width = height = 0;
    if (m_window) SDL_GetWindowSize(m_window, &width, &height);
}

bool SdlFrontend::MakeContextCurrent() {
    if (!m_window || !m_context) return false;
    if (SDL_GL_MakeCurrent(m_window, m_context) != 0) {
        std::fprintf(stderr, "SDL_GL_MakeCurrent failed: %s\n", SDL_GetError());
        return false;
    }
    return true;
}

void SdlFrontend::Present() {
    if (m_window && m_context) SDL_GL_SwapWindow(m_window);
}

bool SdlFrontend::PollEvent(PlatformEvent& output) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      output = {};
      switch (event.type) {
        case SDL_QUIT:
            output.type = PlatformEventType::Quit;
            return true;
        case SDL_KEYDOWN:
            output.type = PlatformEventType::KeyDown;
            output.code = static_cast<int>(event.key.keysym.scancode);
            output.repeat = event.key.repeat != 0;
            return true;
        case SDL_KEYUP:
            output.type = PlatformEventType::KeyUp;
            output.code = static_cast<int>(event.key.keysym.scancode);
            return true;
        case SDL_TEXTINPUT:
            output.type = PlatformEventType::TextInput;
            output.text = event.text.text;
            return true;
        case SDL_CONTROLLERDEVICEADDED:
            output.type = PlatformEventType::ControllerAdded;
            output.device = event.cdevice.which;
            return true;
        case SDL_CONTROLLERDEVICEREMOVED:
            output.type = PlatformEventType::ControllerRemoved;
            output.device = event.cdevice.which;
            return true;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            output.type = event.type == SDL_CONTROLLERBUTTONDOWN
                ? PlatformEventType::ControllerButtonDown
                : PlatformEventType::ControllerButtonUp;
            output.device = event.cbutton.which;
            output.code = NormalizePadButton(event.cbutton.button);
            return true;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            output.type = event.type == SDL_MOUSEBUTTONDOWN
                ? PlatformEventType::MouseButtonDown
                : PlatformEventType::MouseButtonUp;
            output.code = event.button.button == SDL_BUTTON_LEFT ? 0
                : event.button.button == SDL_BUTTON_RIGHT ? 1
                : event.button.button == SDL_BUTTON_MIDDLE ? 2 : -1;
            return true;
        case SDL_WINDOWEVENT:
            if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                output.type = PlatformEventType::Resized;
                output.width = event.window.data1;
                output.height = event.window.data2;
                return true;
            }
            break;
        default:
            break;
      }
    }
    output = {};
    return false;
}

uint32_t SdlFrontend::Milliseconds() const { return SDL_GetTicks(); }
uint64_t SdlFrontend::PerformanceCounter() const { return SDL_GetPerformanceCounter(); }
uint64_t SdlFrontend::PerformanceFrequency() const { return SDL_GetPerformanceFrequency(); }
void SdlFrontend::Delay(uint32_t milliseconds) const { SDL_Delay(milliseconds); }

const uint8_t* SdlFrontend::KeyboardState(int& keyCount) const {
    return SDL_GetKeyboardState(&keyCount);
}

int SdlFrontend::KeyCodeFromName(const char* name) const {
    const SDL_Scancode code = SDL_GetScancodeFromName(name ? name : "");
    return code == SDL_SCANCODE_UNKNOWN ? KeyCode::Unknown : static_cast<int>(code);
}

bool SdlFrontend::IsKeyDown(int keyCode) const {
    if (keyCode <= KeyCode::Unknown || keyCode >= KeyCode::Count) return false;
    int count = 0;
    const Uint8* state = SDL_GetKeyboardState(&count);
    return state && keyCode < count && state[keyCode] != 0;
}

bool SdlFrontend::IsMouseButtonDown(int buttonCode) const {
    const Uint32 buttons = SDL_GetMouseState(nullptr, nullptr);
    const Uint32 mask = buttonCode == 0 ? SDL_BUTTON_LMASK
        : buttonCode == 1 ? SDL_BUTTON_RMASK
        : buttonCode == 2 ? SDL_BUTTON_MMASK : 0;
    return mask != 0 && (buttons & mask) != 0;
}

void SdlFrontend::MousePosition(int& x, int& y) const { SDL_GetMouseState(&x, &y); }

void SdlFrontend::RefreshControllers() {
    if (!m_controllerSubsystemInitialized) {
        if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0) return;
        SDL_GameControllerEventState(SDL_ENABLE);
        m_controllerSubsystemInitialized = true;
    }
    const int count = SDL_NumJoysticks();
    if (count < 0) return;
    if (count != m_controllerDeviceCount) {
        for (SDL_GameController* controller : m_controllers)
            if (controller) SDL_GameControllerClose(controller);
        m_controllers.assign(static_cast<size_t>(count), nullptr);
        m_controllerDeviceCount = count;
    }
    for (int i = 0; i < count; ++i) {
        SDL_GameController*& controller = m_controllers[static_cast<size_t>(i)];
        if (controller && !SDL_GameControllerGetAttached(controller)) {
            SDL_GameControllerClose(controller);
            controller = nullptr;
        }
        if (!controller && SDL_IsGameController(i)) controller = SDL_GameControllerOpen(i);
    }
}

int SdlFrontend::ControllerIndexForInstance(int instance) {
    RefreshControllers();
    for (size_t i = 0; i < m_controllers.size(); ++i) {
        SDL_GameController* controller = m_controllers[i];
        if (controller && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) == instance)
            return static_cast<int>(i);
    }
    return -1;
}

bool SdlFrontend::IsControllerPresent(int index) {
    RefreshControllers();
    return index >= 0 && index < static_cast<int>(m_controllers.size()) &&
           m_controllers[static_cast<size_t>(index)] != nullptr;
}

bool SdlFrontend::IsControllerButtonDown(int index, int buttonCode) {
    if (!IsControllerPresent(index)) return false;
    const SDL_GameControllerButton button = NativePadButton(buttonCode);
    return button != SDL_CONTROLLER_BUTTON_INVALID &&
           SDL_GameControllerGetButton(m_controllers[static_cast<size_t>(index)], button) != 0;
}

float SdlFrontend::ControllerAxis(int index, int axis) {
    static const SDL_GameControllerAxis axes[] = {
        SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY,
        SDL_CONTROLLER_AXIS_RIGHTX, SDL_CONTROLLER_AXIS_RIGHTY
    };
    if (!IsControllerPresent(index) || axis < 0 || axis >= 4) return 0.0f;
    float value = static_cast<float>(SDL_GameControllerGetAxis(
        m_controllers[static_cast<size_t>(index)], axes[axis])) / 32767.0f;
    if (axis == 1 || axis == 3) value = -value;
    return value < -1.0f ? -1.0f : value;
}

void SdlFrontend::SetActiveControllerSoftRumble(bool enabled) {
    RefreshControllers();
    for (SDL_GameController* controller : m_controllers) {
        if (!controller || !SDL_GameControllerGetAttached(controller)) continue;

        // Retail sets CellPadActParam.motor[0] to 0/1 for one input update.
        // That byte is the PS3 small motor; SDL represents it as the high-
        // frequency channel. The game input update is 60 Hz (16.67 ms), so
        // use a 17 ms SDL pulse and stop immediately when disabled.
        SDL_GameControllerRumble(controller, 0,
            enabled ? 0xffff : 0, enabled ? 17u : 0u);
        break;
    }
}

bool SdlFrontend::DecodeWave(const std::string& path, int outputRate,
                             std::vector<float>& stereoSamples,
                             int& sourceChannels) {
    stereoSamples.clear();
    SDL_AudioSpec source{};
    Uint8* bytes = nullptr;
    Uint32 byteCount = 0;
    if (!SDL_LoadWAV(path.c_str(), &source, &bytes, &byteCount)) return false;
    if (!bytes || byteCount == 0 || source.freq <= 0 || source.channels == 0 ||
        source.channels > 8) {
        SDL_FreeWAV(bytes);
        return false;
    }
    sourceChannels = source.channels;
    SDL_AudioCVT cvt{};
    if (SDL_BuildAudioCVT(&cvt, source.format, source.channels, source.freq,
                          AUDIO_F32SYS, 2, outputRate) < 0) {
        SDL_FreeWAV(bytes);
        return false;
    }
    if (cvt.len_mult <= 0 || byteCount >
        static_cast<size_t>(std::numeric_limits<int>::max()) / static_cast<size_t>(cvt.len_mult)) {
        SDL_FreeWAV(bytes);
        return false;
    }
    cvt.len = static_cast<int>(byteCount);
    const size_t convertedCapacity = static_cast<size_t>(cvt.len) *
                                     static_cast<size_t>(cvt.len_mult);
    cvt.buf = static_cast<Uint8*>(SDL_malloc(convertedCapacity));
    if (!cvt.buf) {
        SDL_FreeWAV(bytes);
        return false;
    }
    std::memcpy(cvt.buf, bytes, byteCount);
    SDL_FreeWAV(bytes);
    if (SDL_ConvertAudio(&cvt) != 0 || cvt.len_cvt <= 0 ||
        static_cast<size_t>(cvt.len_cvt) > convertedCapacity ||
        cvt.len_cvt % static_cast<int>(sizeof(float) * 2) != 0) {
        SDL_free(cvt.buf);
        return false;
    }
    const size_t floatCount = static_cast<size_t>(cvt.len_cvt) / sizeof(float);
    const float* decoded = reinterpret_cast<const float*>(cvt.buf);
    for (size_t i = 0; i < floatCount; ++i) {
        if (!std::isfinite(decoded[i])) {
            SDL_free(cvt.buf);
            return false;
        }
    }
    stereoSamples.assign(decoded, decoded + floatCount);
    SDL_free(cvt.buf);
    return true;
}

bool SdlFrontend::StartAudioOutput(int sampleRate) {
    if (m_audioDevice) return true;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::fprintf(stderr, "Audio output unavailable: %s\n", SDL_GetError());
        return false;
    }
    SDL_AudioSpec wanted{};
    wanted.freq = sampleRate;
    wanted.format = AUDIO_F32SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    SDL_AudioSpec obtained{};
    const int allowedChanges = SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                               SDL_AUDIO_ALLOW_FORMAT_CHANGE |
                               SDL_AUDIO_ALLOW_CHANNELS_CHANGE;
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &wanted, &obtained,
                                        allowedChanges);
    if (!m_audioDevice) {
        std::fprintf(stderr, "Audio output unavailable: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }

    const int bitsPerSample = SDL_AUDIO_BITSIZE(obtained.format);
    if (obtained.freq <= 0 || obtained.channels == 0 || bitsPerSample <= 0 ||
        bitsPerSample % 8 != 0 || obtained.samples == 0) {
        std::fprintf(stderr, "Audio output unavailable: invalid device format\n");
        StopAudioOutput();
        return false;
    }

    m_audioStream = SDL_NewAudioStream(
        wanted.format, wanted.channels, wanted.freq,
        obtained.format, obtained.channels, obtained.freq);
    if (!m_audioStream) {
        std::fprintf(stderr, "Audio output unavailable: %s\n", SDL_GetError());
        StopAudioOutput();
        return false;
    }
    m_audioFrameBytes = static_cast<size_t>(bitsPerSample / 8) * obtained.channels;
    m_audioQueueLimitBytes = static_cast<size_t>(obtained.freq) *
                             m_audioFrameBytes / 4;

    const size_t prebufferFrames = static_cast<size_t>(obtained.samples) * 2;
    std::vector<Uint8> silence(prebufferFrames * m_audioFrameBytes,
                               obtained.silence);
    if (SDL_QueueAudio(m_audioDevice, silence.data(),
                       static_cast<Uint32>(silence.size())) != 0) {
        std::fprintf(stderr, "Audio output unavailable: %s\n", SDL_GetError());
        StopAudioOutput();
        return false;
    }

    std::fprintf(stderr,
                 "Audio output: %d Hz, format=0x%04x, channels=%u, callback=%u\n",
                 obtained.freq, obtained.format, obtained.channels,
                 obtained.samples);
    SDL_PauseAudioDevice(m_audioDevice, 0);
    return true;
}

void SdlFrontend::StopAudioOutput() {
    if (!m_audioDevice) return;
    SDL_PauseAudioDevice(m_audioDevice, 1);
    SDL_ClearQueuedAudio(m_audioDevice);
    if (m_audioStream) {
        SDL_FreeAudioStream(m_audioStream);
        m_audioStream = nullptr;
    }
    SDL_CloseAudioDevice(m_audioDevice);
    m_audioDevice = 0;
    m_audioFrameBytes = 0;
    m_audioQueueLimitBytes = 0;
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

size_t SdlFrontend::QueuedAudioBytes() {
    return m_audioDevice ? SDL_GetQueuedAudioSize(m_audioDevice) : 0;
}

void SdlFrontend::ClearAudioQueue() {
    if (m_audioDevice) SDL_ClearQueuedAudio(m_audioDevice);
}

void SdlFrontend::QueueAudioSamples(const float* samples, size_t sampleCount) {
    if (!m_audioDevice || !m_audioStream || !samples || sampleCount < 2 ||
        sampleCount % 2 != 0) return;
    if (sampleCount > static_cast<size_t>(std::numeric_limits<int>::max()) /
                          sizeof(float)) return;

    const int inputBytes = static_cast<int>(sampleCount * sizeof(float));
    std::vector<float> sanitized;
    const float* input = samples;
    if (!std::all_of(samples, samples + sampleCount,
                     [](float value) { return std::isfinite(value); })) {
        sanitized.assign(samples, samples + sampleCount);
        for (float& value : sanitized) {
            if (!std::isfinite(value)) value = 0.0f;
        }
        input = sanitized.data();
    }

    if (SDL_AudioStreamPut(m_audioStream, input, inputBytes) != 0) {
        std::fprintf(stderr, "Audio conversion failed: %s\n", SDL_GetError());
        return;
    }
    const int available = SDL_AudioStreamAvailable(m_audioStream);
    if (available < 0) {
        std::fprintf(stderr, "Audio conversion failed: %s\n", SDL_GetError());
        return;
    }
    if (available == 0) return;

    std::vector<Uint8> converted(static_cast<size_t>(available));
    const int convertedBytes = SDL_AudioStreamGet(
        m_audioStream, converted.data(), available);
    if (convertedBytes < 0 ||
        static_cast<size_t>(convertedBytes) % m_audioFrameBytes != 0) {
        std::fprintf(stderr, "Audio conversion failed: %s\n", SDL_GetError());
        return;
    }
    if (convertedBytes == 0) return;

    if (m_audioQueueLimitBytes > 0 &&
        QueuedAudioBytes() > m_audioQueueLimitBytes) {
        ClearAudioQueue();
    }
    if (SDL_QueueAudio(m_audioDevice, converted.data(),
                       static_cast<Uint32>(convertedBytes)) != 0) {
        std::fprintf(stderr, "Audio queue failed: %s\n", SDL_GetError());
    }
}
