# Third-party middleware

OpenEgg links or includes the following independent components. Their source
notices remain in `vendor/`; consult the listed files for complete terms.

| Component | Use in OpenEgg | License / notice |
| --- | --- | --- |
| GameMonkey Script | Embedded scripting VM | MIT-style license in `vendor/gmscript/gmsrc/src/gm/gmMachine.h` |
| Box2D 2.4.1 | Physics | MIT, `vendor/box2d/LICENSE` |
| TinyXML2 | XML parsing | zlib, `vendor/tinyxml2/LICENSE.txt` |
| RapidJSON | JSON parsing and writing | MIT notice in the headers under `vendor/rapidjson/include` |
| stb_image / stb_truetype | Image loading and font utilities | Public domain / included notices in `vendor/stb` headers |
| minimp3 | MP3 decoding | CC0 1.0, `vendor/minimp3/LICENSE` |
| SDL2 | Desktop window, input, and audio frontend | System/package dependency; obtain and retain the license from the installed distribution |
| GLEW | OpenGL extension loading | System/package dependency; retain the license from the installed distribution |
| FreeType | Font rasterization | System/package dependency; retain the license from the installed distribution |
| OpenGL | Rendering API | Platform-provided dependency |
| zlib | Compression and archive support | System/package dependency; vendor's zlib license is also in `vendor/tinyxml2/LICENSE.txt` |

The Box2D CMake configuration disables its testbed, unit tests, and docs. SDL2,
GLEW, FreeType, OpenGL, and zlib are discovered from the host or package
manager. The other listed components are built or included from `vendor/`.
