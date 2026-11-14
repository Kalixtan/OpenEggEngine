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
| SDL2 | Desktop window, input, and audio frontend | Linux system dependency; Windows release is statically linked |
| GLEW | OpenGL extension loading | Linux system dependency; Windows release is statically linked |
| FreeType | Font rasterization | Linux system dependency; Windows release is statically linked |
| OpenGL | Rendering API | Platform-provided dependency |
| libcurl | Replacement-server HTTP client | Linux system dependency; Windows release is statically linked |
| zlib | Compression and archive support | Linux system dependency; Windows release is statically linked. Vendor's zlib license is also in `vendor/tinyxml2/LICENSE.txt` |
| bzip2, libpng, Brotli | FreeType transitive dependencies | Statically linked in the Windows release |

The Box2D CMake configuration disables its testbed, unit tests, and docs. On
Linux, SDL2, GLEW, FreeType, OpenGL, libcurl, and zlib are discovered from the
host. The Windows ZIP statically links the dependencies listed above; their
license texts are available from the package sources used by the build.
