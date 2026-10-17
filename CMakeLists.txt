cmake_minimum_required(VERSION 3.16)
project(OpenEgg VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
  add_compile_options(/utf-8)
endif()
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(GE3_PLATFORM_FRONTEND_SOURCE "src/platform/sdl_frontend.cpp" CACHE FILEPATH
    "PlatformFrontend implementation that provides the executable's host services")
get_filename_component(GE3_PLATFORM_FRONTEND_SOURCE_ABS
  "${GE3_PLATFORM_FRONTEND_SOURCE}" REALPATH BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
get_filename_component(GE3_SDL_FRONTEND_SOURCE_ABS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/platform/sdl_frontend.cpp" REALPATH)
set(GE3_USE_SDL_FRONTEND OFF)
if(GE3_PLATFORM_FRONTEND_SOURCE_ABS STREQUAL GE3_SDL_FRONTEND_SOURCE_ABS)
  set(GE3_USE_SDL_FRONTEND ON)
endif()

if(WIN32)
  # Native Windows builds use package-manager CMake configs (for example
  # vcpkg) instead of requiring pkg-config/MSYS at configure time.
  if(GE3_USE_SDL_FRONTEND)
    find_package(SDL2 CONFIG REQUIRED)
    if(TARGET SDL2::SDL2)
      set(GE3_SDL2_TARGET SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
      set(GE3_SDL2_TARGET SDL2::SDL2-static)
    else()
      message(FATAL_ERROR "SDL2 config package must export SDL2::SDL2 or SDL2::SDL2-static")
    endif()
  endif()
  find_package(GLEW CONFIG REQUIRED)
  find_package(Freetype REQUIRED)
  set(GE3_GLEW_TARGET GLEW::GLEW)
  set(GE3_FREETYPE_TARGET Freetype::Freetype)
else()
  find_package(PkgConfig REQUIRED)
  if(GE3_USE_SDL_FRONTEND)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  endif()
  pkg_check_modules(GLEW REQUIRED IMPORTED_TARGET glew)
  # Retail rasterises text with FreeType2 (its own asserts name
  # FreeType2TestProcess.cpp), so this engine uses it too rather than
  # stb_truetype -- different rasterisers give different glyph coverage.
  pkg_check_modules(FREETYPE REQUIRED IMPORTED_TARGET freetype2)
  set(GE3_GLEW_TARGET PkgConfig::GLEW)
  set(GE3_FREETYPE_TARGET PkgConfig::FREETYPE)
endif()
find_package(OpenGL REQUIRED)
# Prefer static zlib where the platform package supplies it. The Windows
# vcpkg triplet already uses static third-party archives; on Linux the local
# SDK provides libz.a, while SDL2/GLEW/FreeType and OpenGL are shared-only.
set(ZLIB_USE_STATIC_LIBS ON)
find_package(ZLIB REQUIRED)
set(GE3_ZLIB_TARGET ZLIB::ZLIB)
if(UNIX AND NOT APPLE)
  # FindZLIB's static preference is not honored by every CMake version or
  # distro module. Use libz.a explicitly when the development package provides
  # it; fall back to its imported target otherwise.
  find_file(GE3_STATIC_ZLIB_ARCHIVE NAMES libz.a
    PATHS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES} /usr/lib /usr/local/lib
    PATH_SUFFIXES ${CMAKE_LIBRARY_ARCHITECTURE})
  if(GE3_STATIC_ZLIB_ARCHIVE)
    add_library(ge3_static_zlib STATIC IMPORTED GLOBAL)
    set_target_properties(ge3_static_zlib PROPERTIES
      IMPORTED_LOCATION "${GE3_STATIC_ZLIB_ARCHIVE}"
      INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIRS}")
    set(GE3_ZLIB_TARGET ge3_static_zlib)
  endif()
endif()

# --- vendored GameMonkeyScript VM core (gmsrc/src/gm only -- the rest of the
# upstream tree is tools/debugger/examples we don't need) ---
set(GM_SRC_DIR ${CMAKE_SOURCE_DIR}/vendor/gmscript/gmsrc/src/gm)
if(WIN32)
  set(GE3_GMSCRIPT_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/gmscript_platform/windows)
else()
  set(GE3_GMSCRIPT_PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/gmscript_platform)
endif()
file(GLOB GM_SOURCES ${GM_SRC_DIR}/*.cpp)
list(REMOVE_ITEM GM_SOURCES ${GM_SRC_DIR}/gmDebugger.cpp) # needs a net-debugger frontend we don't have

add_library(gmscript STATIC ${GM_SOURCES})
# gmscript_platform must come first: it supplies the gmConfig_p.h that
# gmConfig.h expects to find (upstream ships win32/ps2 variants only, ours
# is the Linux one -- see vendor/gmscript_platform/gmConfig_p.h).
target_include_directories(gmscript BEFORE PUBLIC ${GE3_GMSCRIPT_PLATFORM_DIR} ${GM_SRC_DIR})
# The generated GameMonkey scanner normally includes POSIX unistd.h. The
# Windows platform config maps its isatty/fileno usage to the CRT equivalents.
if(WIN32)
  target_compile_definitions(gmscript PRIVATE YY_NO_UNISTD_H)
endif()
# GM_DEBUG_BUILD is derived from NDEBUG inside gmConfig_p.h itself -- do not
# also force-define it here, #ifdef GM_DEBUG_BUILD would always be true.

# --- vendored Box2D (v2.4.1, the last "classic" C++ API tag before the
# v3.x full rewrite to a C API -- matches the OOP shape naturally, per
# the user's "just import the real lib, no need to overcomplicate"). Its
# own CMakeLists builds testbed/unit-tests by default; turn those off so we
# don't pull in glfw/imgui/glad we don't need. ---
set(BOX2D_BUILD_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_TESTBED OFF CACHE BOOL "" FORCE)
set(BOX2D_BUILD_DOCS OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/box2d)

# --- vendored TinyXML2 (zlib license, single .h/.cpp) -- replaces the
# hand-rolled <tableElement>/<element> parser that GameMonkeyScript.
# loadTableFromXMLFile used initially. RapidJSON (header-only, MIT,
# vendor/rapidjson/include only -- the rest of that repo is docs/tests we
# don't need) similarly replaces the hand-rolled json_lenient.h parser for
# both the bitmap font atlas JSON and GameMonkeyScript.loadTableFromJsonFile.
add_library(tinyxml2 STATIC vendor/tinyxml2/tinyxml2.cpp)
target_include_directories(tinyxml2 PUBLIC vendor/tinyxml2)

# --- platform-independent game/runtime and OpenGL backend ---
add_library(OpenEgg_backend STATIC
  src/platform/headless_harness.cpp
  src/core/gm_vm.cpp
  src/core/strict_calls.cpp
  src/core/save_data.cpp
  src/core/system_config.cpp
  src/core/file_helper.cpp
  src/core/psarc_archive.cpp
  src/core/process_manager.cpp
  src/render/renderer.cpp
  src/render/shader.cpp
  src/render/texture.cpp
  src/render/bitmap_font.cpp
  src/render/draw_list.cpp
  src/render/draw_context.cpp
  src/render/fsa_bci.cpp
  src/ported_native/ground_renderer.cpp
  src/ported_native/player_renderer.cpp
  src/ported_native/save_data_completion.cpp
  src/bindings/gamemonkeyscript_bindings.cpp
  src/bindings/gamesystem_bindings.cpp
  src/bindings/sequencer_bindings.cpp
  src/bindings/process_handle_bindings.cpp
  src/bindings/pulseperc_bindings.cpp
  src/bindings/lfo_bindings.cpp
  src/bindings/savedata_bindings.cpp
  src/bindings/stringhelper_bindings.cpp
  src/bindings/video_bindings.cpp
  src/bindings/video_draw_buffer_bindings.cpp
  src/bindings/shape_bindings.cpp
  src/bindings/colour_bindings.cpp
  src/bindings/entity_bindings.cpp
  src/bindings/animation_bindings.cpp
  src/bindings/input_bindings.cpp
  src/bindings/vector2d_bindings.cpp
  src/bindings/printinfo_bindings.cpp
  src/bindings/online_coordinator_bindings.cpp
  src/bindings/watermelon_bindings.cpp
  src/bindings/videohelper_bindings.cpp
  src/bindings/audio_bindings.cpp
  src/bindings/mathutil_bindings.cpp
  src/bindings/math_bindings.cpp
  src/bindings/texture_bindings.cpp
  src/bindings/envelope_bindings.cpp
  src/bindings/constants_bindings.cpp
  src/bindings/system_bindings.cpp
  src/bindings/physics_bindings.cpp
  src/bindings/matrix4_bindings.cpp
  src/bindings/physics_body_bindings.cpp
  src/bindings/physics_joint_bindings.cpp
  src/bindings/physics_global_bindings.cpp
  src/bindings/foe_compat_bindings.cpp
  src/core/physics_world.cpp
  src/core/localization.cpp
)

target_include_directories(OpenEgg_backend PUBLIC
  src
  vendor/stb
  vendor/gmscript/gmsrc/src/binds # header-only gmCall.h, no binds/*.cpp compiled
  vendor/box2d/include
  vendor/rapidjson/include
)
if(WIN32)
  # Avoid Windows SDK min/max macros and trim transitive Win32 declarations
  # pulled in by libraries such as minimp3_ex.h.
  target_compile_definitions(OpenEgg_backend PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

target_link_libraries(OpenEgg_backend PUBLIC
  gmscript
  box2d
  tinyxml2
  ${GE3_GLEW_TARGET}
  ${GE3_FREETYPE_TARGET}
  OpenGL::GL
  ${GE3_ZLIB_TARGET}
)

if(NOT WIN32)
  target_compile_options(OpenEgg_backend PRIVATE ${GLEW_CFLAGS_OTHER})
endif()

# --- native host frontend and executable bootstrap ---
# The frontend source implements CreatePlatformFrontend() and can be replaced
# independently of the game/runtime library above.
add_library(OpenEgg_frontend STATIC ${GE3_PLATFORM_FRONTEND_SOURCE})
target_include_directories(OpenEgg_frontend PRIVATE src)
if(GE3_USE_SDL_FRONTEND)
  if(WIN32)
    # This executable has its own ordinary main() in main.cpp; it does not
    # use SDL2main's SDL_main startup shim.
    target_compile_definitions(OpenEgg_frontend PRIVATE SDL_MAIN_HANDLED)
    target_link_libraries(OpenEgg_frontend PRIVATE ${GE3_SDL2_TARGET})
  else()
    target_link_libraries(OpenEgg_frontend PRIVATE PkgConfig::SDL2)
  endif()
endif()

add_executable(OpenEgg src/platform/main.cpp)
target_link_libraries(OpenEgg PRIVATE OpenEgg_frontend OpenEgg_backend)
if(WIN32)
  # Launch as a Windows GUI application so Explorer does not create a console
  # window for the game. main.cpp supplies a WinMain shim for MinGW/MSVC.
  set_target_properties(OpenEgg PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

# Desktop compatibility overlays are shipped as the highest supported loose
# patch layer beside the executable. They preserve the retail PSARC inputs and
# adapt script code only where the desktop runtime's object representation
# differs from the retail one.
set(GE3_DESKTOP_PATCH_OVERLAY
  "${CMAKE_CURRENT_SOURCE_DIR}/desktop_overrides/patch_data004")
if(EXISTS "${GE3_DESKTOP_PATCH_OVERLAY}")
  # Script overlays can change without relinking the executable. Copy them in
  # a build dependency so `cmake --build` refreshes the runtime data directory
  # on Linux and Windows even when all C++ targets are already up to date.
  file(GLOB_RECURSE GE3_DESKTOP_PATCH_FILES CONFIGURE_DEPENDS
    "${GE3_DESKTOP_PATCH_OVERLAY}/*")
  set_property(TARGET OpenEgg APPEND PROPERTY LINK_DEPENDS
    ${GE3_DESKTOP_PATCH_FILES})
  add_custom_command(TARGET OpenEgg POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${GE3_DESKTOP_PATCH_OVERLAY}"
      "$<TARGET_FILE_DIR:OpenEgg>/patch_data004"
    COMMENT "Copying local desktop compatibility overlay beside OpenEgg")
endif()

if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # MinGW's compiler runtimes are not Windows system DLLs. Link them into the
  # executable so users do not need matching MinGW runtime DLLs beside it.
  target_link_options(OpenEgg PRIVATE -static -static-libgcc -static-libstdc++)
  # Release-mode vcpkg archives can carry DWARF sections into the PE even when
  # this project itself was compiled without -g. Keep them out of distributable
  # release executables; they otherwise add many megabytes without affecting
  # runtime behavior.
  target_link_options(OpenEgg PRIVATE
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:MinSizeRel>>:-Wl,--strip-debug>")
endif()

# Runtime assets and scripts are supplied by the user beside the executable.
# The build does not download, copy, unpack, or distribute game content.
