# OpenEgg

OpenEgg is an independent reimplementation of the GameEngine3 runtime used by
*Sound Shapes*. The project studies the engine's observable behavior and aims
to run the original game natively on desktop systems, without a console
emulator, while maintaining compatibility with retail scripts and assets.
This is a work in progress; compatibility and playability are goals, not a
claim that every feature currently works.

## Game content and rights

This repository does not distribute *Sound Shapes* game assets, retail scripts,
disc images, console firmware, or extracted proprietary game code. OpenEgg's
runtime requires a user's own legally obtained game data at launch; none is
downloaded or copied into the build by CMake. Keep local game data out of Git.

*Sound Shapes* was developed by Queasy Games and published by Sony Computer
Entertainment ([official PlayStation listing](https://store.playstation.com/en-gb/product/EP9000-CUSA00040_00-SOUNDSHAPES00000)).
We acknowledge and thank the original developers and artists for their work.
Copyright in the game and its materials, and rights to *Sound Shapes*,
GameEngine3, and related marks, remain with their respective owners. OpenEgg is
an independent, unofficial project and is not affiliated with or endorsed by
them. This notice does not claim ownership of or grant rights to their work.

OpenEgg's source code is an independent implementation. The repository does
include separately licensed third-party middleware; see
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and each component's included
license. Those notices apply to the middleware only.

## Build

Requirements: CMake 3.16+, a C++17 compiler, SDL2, GLEW, FreeType, OpenGL, and
zlib. GameMonkey Script, Box2D, TinyXML2, RapidJSON, stb, and minimp3 are
included under `vendor/` and wired into the CMake build.

On Debian or Ubuntu:

```sh
sudo apt install build-essential cmake pkg-config libsdl2-dev libglew-dev \
  libfreetype-dev libgl1-mesa-dev zlib1g-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The executable is `build/OpenEgg`. Build output and local game data are
ignored by Git. To run, place user-provided game archives next to the
executable; see [PLATFORM_BACKENDS.md](PLATFORM_BACKENDS.md) for Windows,
cross-compilation, and frontend details.

## Project layout

- `src/core`, `src/render`, `src/bindings`: runtime, renderer, and native API
- `src/platform`: desktop frontend and executable entry point
- `src/ported_native`: native runtime components implemented for OpenEgg
- `vendor`: third-party middleware and its notices
- `*.md`: build, backend, and runtime configuration notes

## Contributing

Do not commit game dumps, retail scripts, extracted assets, firmware, local
save data, or generated builds. Keep compatibility work in OpenEgg's own
implementation and document third-party changes with their applicable
licenses.
