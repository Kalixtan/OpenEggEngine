#pragma once

#include "platform_window.h"
#include <SDL2/SDL.h>
#include <vector>

// SDL2 frontend implementation of the platform contract. SDL owns native
// window, input, and context resources; gameplay receives normalized events,
// input queries, and clock services through PlatformFrontend.
class SdlFrontend final : public PlatformFrontend {
public:
    ~SdlFrontend() override { Destroy(); }

    bool Create(int width, int height, const char* title, bool hidden) override;
    void Destroy() override;
    void ShowErrorMessage(const char* title, const char* message) override;
    bool ShowUnimplementedFunctionDialog(const char* title,
                                         const char* message) override;
    void Resize(int width, int height) override;
    bool SetFullscreen(bool fullscreen) override;
    bool IsFullscreen() const override { return m_fullscreen; }
    void GetSize(int& width, int& height) const override;
    bool MakeContextCurrent() override;
    void Present() override;
    bool PollEvent(PlatformEvent& event) override;
    uint32_t Milliseconds() const override;
    uint64_t PerformanceCounter() const override;
    uint64_t PerformanceFrequency() const override;
    void Delay(uint32_t milliseconds) const override;
    const uint8_t* KeyboardState(int& keyCount) const override;
    int KeyCodeFromName(const char* name) const override;
    bool IsKeyDown(int keyCode) const override;
    bool IsMouseButtonDown(int buttonCode) const override;
    void MousePosition(int& x, int& y) const override;
    void RefreshControllers() override;
    int ControllerIndexForInstance(int instance) override;
    bool IsControllerPresent(int index) override;
    bool IsControllerButtonDown(int index, int buttonCode) override;
    float ControllerAxis(int index, int axis) override;
    void SetActiveControllerSoftRumble(bool enabled) override;
    bool DecodeWave(const std::string& path, int outputRate,
                    std::vector<float>& stereoSamples) override;
    bool StartAudioOutput(int sampleRate) override;
    void StopAudioOutput() override;
    size_t QueuedAudioBytes() override;
    void ClearAudioQueue() override;
    void QueueAudioSamples(const float* samples, size_t sampleCount) override;

private:
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_context = nullptr;
    bool m_sdlInitialized = false;
    bool m_fullscreen = false;
    std::vector<SDL_GameController*> m_controllers;
    int m_controllerDeviceCount = -1;
    bool m_controllerSubsystemInitialized = false;
    SDL_AudioDeviceID m_audioDevice = 0;
};
