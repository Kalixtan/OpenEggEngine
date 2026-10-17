# OpenEgg

> **Very early development. Not a release.** OpenEgg is incomplete and may not
> build or run. Full game playability has not been established.

OpenEgg is an independent reimplementation of the GameEngine3 runtime used by
*Sound Shapes*. The goal is native desktop play with retail scripts and assets.

## Game content and rights

This repository contains no game assets, retail scripts, disc images, firmware,
or extracted game code. OpenEgg needs game data supplied by the user; keep it
out of Git.

*Sound Shapes* was developed by Queasy Games and published by Sony Computer
Entertainment ([PlayStation listing](https://store.playstation.com/en-gb/product/EP9000-CUSA00040_00-SOUNDSHAPES00000)).
Thanks to the original developers and artists.
All rights to the game, GameEngine3, and related marks remain with their owners.
OpenEgg is unofficial, unaffiliated, and does not claim rights to that material.
Third-party middleware licenses are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Build

Requires CMake 3.16+, C++17, SDL2, GLEW, FreeType, OpenGL, and zlib. Other
dependencies are in `vendor/`.

On Debian or Ubuntu:

```sh
sudo apt install build-essential cmake pkg-config libsdl2-dev libglew-dev \
  libfreetype-dev libgl1-mesa-dev zlib1g-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

The executable is `build/OpenEgg`. Place your game archives beside it to run.
See [PLATFORM_BACKENDS.md](PLATFORM_BACKENDS.md) for other build options.

## Script API reference

Install Doxygen and run `./tools/build_api_docs.sh`. Open
`build/docs/api/html/script_api_index.html`. The reference covers registered
script names and recovered retail methods that are not implemented yet. The
retail list is incomplete; behavior and compatibility still need verification.

## Development status

No stable release is available. See [COMPATIBILITY_STATUS.md](COMPATIBILITY_STATUS.md)
for provisional compatibility figures and [TODO.md](TODO.md) for open work.

## Project layout

Runtime code is under `src/`; third-party dependencies are under `vendor/`.
Build and status notes are linked above.

## Contributing

Keep game data, firmware, saves, and build output out of Git. Include licenses
for any new third-party code.
