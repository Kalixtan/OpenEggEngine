# Platform frontend boundary

The runtime and renderer use `PlatformFrontend`/`PlatformWindow` contracts in
`src/platform/platform_window.h`. The selected frontend creates a current
OpenGL context, translates native input into `PlatformEvent` and USB HID key
codes, supplies controller/mouse state and monotonic clocks, and presents the
frame. The renderer owns OpenGL resources and does not include SDL headers or
call SDL APIs. Input bindings query the injected frontend interface rather
than native devices.

The game and OpenGL backend is built as the `gameengine3_backend` static
library. The native host adapter is a separate `gameengine3_frontend` static
library, and the executable bootstrap links both. This keeps SDL out of the
backend target and lets another host link the same game/render implementation.

The executable selects its frontend source through the CMake cache variable
`GE3_PLATFORM_FRONTEND_SOURCE`; the source provides
`CreatePlatformFrontend()`. The SDL2 implementation is
`src/platform/sdl_frontend.cpp`. For example, a separate build can use:

```sh
cmake -S engine -B build-custom -DGE3_PLATFORM_FRONTEND_SOURCE=/path/to/frontend.cpp
cmake --build build-custom --target gameengine3_backend
```

Audio-device creation, WAV decoding, queue management, and shutdown are also
frontend services. The game-side voice mixer supplies normalized stereo float
samples and does not depend on SDL. Selecting another frontend source in CMake
omits the SDL2 package and link dependency.
