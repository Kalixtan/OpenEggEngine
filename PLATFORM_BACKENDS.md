# Platform frontend boundary

The runtime and renderer use `PlatformFrontend`/`PlatformWindow` contracts in
`src/platform/platform_window.h`. The selected frontend creates a current
OpenGL context, translates native input into `PlatformEvent` and USB HID key
codes, supplies controller/mouse state and monotonic clocks, and presents the
frame. The renderer owns OpenGL resources and does not include SDL headers or
call SDL APIs. Input bindings query the injected frontend interface rather
than native devices.

The game and OpenGL backend is built as the `OpenEgg_backend` static
library. The native host adapter is a separate `OpenEgg_frontend` static
library, and the executable bootstrap links both. This keeps SDL out of the
backend target and lets another host link the same game/render implementation.

The executable selects its frontend source through the CMake cache variable
`GE3_PLATFORM_FRONTEND_SOURCE`; the source provides
`CreatePlatformFrontend()`. The SDL2 implementation is
`src/platform/sdl_frontend.cpp`. For example, a separate build can use:

```sh
cmake -S . -B build-custom -DGE3_PLATFORM_FRONTEND_SOURCE=/path/to/frontend.cpp
cmake --build build-custom --target OpenEgg_backend
```

Audio-device creation, WAV decoding, queue management, and shutdown are also
frontend services. The game-side voice mixer supplies normalized stereo float
samples and does not depend on SDL. Selecting another frontend source in CMake
omits the SDL2 package and link dependency.

## Building on Linux and Windows

The default SDL2 frontend and OpenGL backend build on Linux with the system
development packages:

```sh
sudo apt install build-essential cmake pkg-config libsdl2-dev libglew-dev \
  libfreetype-dev libgl1-mesa-dev zlib1g-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On Windows, use Visual Studio 2022 with the x64 C++ workload and vcpkg. Install
the native dependencies, then configure with vcpkg's toolchain file:

```powershell
vcpkg install sdl2 glew freetype zlib --triplet x64-windows
cmake -S . -B build-windows -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-windows --config Release
```

The Windows CMake path uses package config targets and does not require
pkg-config. OpenGL is provided by the Windows SDK/driver; the other listed
libraries come from vcpkg. `SDL_MAIN_HANDLED` keeps the project's
standard `main()` entry point and the frontend calls `SDL_SetMainReady()` before
initializing SDL. Put the required runtime DLLs beside `OpenEgg.exe` when
distributing a dynamic vcpkg build, or use a vcpkg static triplet.

To cross-compile a Windows executable from Linux with MinGW, install the
`mingw-w64` toolchain, clone/bootstrap vcpkg, and install the Windows packages
with its community MinGW triplet:

```sh
vcpkg install sdl2 glew freetype zlib \
  --triplet x64-mingw-static \
  --overlay-triplets="$VCPKG_ROOT/triplets/community"
cmake -S . -B build-windows \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
  -DVCPKG_INSTALLED_DIR="$VCPKG_ROOT/installed"
cmake --build build-windows --config Release -j2
```

The resulting executable is `build-windows/OpenEgg.exe`. The
community triplet is supplied by vcpkg, so keep the overlay argument when
installing packages and configuring CMake. The MinGW build links the compiler
runtime and vcpkg libraries statically; only Windows system DLLs remain as
runtime dependencies. Place `game_data.psarc` and any
`patch_data001.psarc` through `patch_data004.psarc` files beside the
executable. OpenEgg does not bundle, extract, or copy retail scripts or assets;
provide your own legally obtained game data separately.
