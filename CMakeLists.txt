cmake_minimum_required(VERSION 3.16)
project(gameengine3_reimpl CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLEW REQUIRED glew)
# Retail rasterises text with FreeType2 (its own asserts name
# FreeType2TestProcess.cpp), so this engine uses it too rather than
# stb_truetype -- different rasterisers give different glyph coverage.
pkg_check_modules(FREETYPE REQUIRED freetype2)
find_package(OpenGL REQUIRED)
find_package(ZLIB REQUIRED)

# --- vendored GameMonkeyScript VM core (gmsrc/src/gm only -- the rest of the
# upstream tree is tools/debugger/examples we don't need) ---
set(GM_SRC_DIR ${CMAKE_SOURCE_DIR}/vendor/gmscript/gmsrc/src/gm)
file(GLOB GM_SOURCES ${GM_SRC_DIR}/*.cpp)
list(REMOVE_ITEM GM_SOURCES ${GM_SRC_DIR}/gmDebugger.cpp) # needs a net-debugger frontend we don't have

add_library(gmscript STATIC ${GM_SOURCES})
# gmscript_platform must come first: it supplies the gmConfig_p.h that
# gmConfig.h expects to find (upstream ships win32/ps2 variants only, ours
# is the Linux one -- see vendor/gmscript_platform/gmConfig_p.h).
target_include_directories(gmscript PUBLIC ${CMAKE_SOURCE_DIR}/vendor/gmscript_platform ${GM_SRC_DIR})
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

# --- engine ---
add_executable(gameengine3
  src/platform/main.cpp
  src/platform/headless_harness.cpp
  src/core/gm_vm.cpp
  src/core/strict_calls.cpp
  src/core/save_data.cpp
  src/core/file_helper.cpp
  src/core/process_manager.cpp
  src/render/renderer.cpp
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
  src/bindings/physics_body_bindings.cpp
  src/bindings/physics_global_bindings.cpp
  src/bindings/foe_compat_bindings.cpp
  src/core/physics_world.cpp
  src/core/localization.cpp
)

target_include_directories(gameengine3 PRIVATE
  src
  vendor/stb
  vendor/gmscript/gmsrc/src/binds # header-only gmCall.h, no binds/*.cpp compiled
  vendor/box2d/include
  vendor/rapidjson/include
  ${SDL2_INCLUDE_DIRS}
  ${GLEW_INCLUDE_DIRS}
  ${FREETYPE_INCLUDE_DIRS}
)

target_link_libraries(gameengine3 PRIVATE
  gmscript
  box2d
  tinyxml2
  ${SDL2_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${FREETYPE_LIBRARIES}
  OpenGL::GL
  OpenGL::GLU
  ZLIB::ZLIB
)

target_compile_options(gameengine3 PRIVATE ${SDL2_CFLAGS_OTHER} ${GLEW_CFLAGS_OTHER})

# Data root: defaults to the minimal hello-world test scene, with the full
# retail game_data as a fallback search path for shared engine scripts
# (FontHelper.gm etc) that game_data_min doesn't carry its own copy of --
# mirrors the real Platformer/PS3_Platformer overlay pattern.
target_compile_definitions(gameengine3 PRIVATE
  GE3_PRIMARY_DATA_ROOT="${CMAKE_SOURCE_DIR}/../game_data_min"
  GE3_FALLBACK_DATA_ROOT="${CMAKE_SOURCE_DIR}/../game_data"
)
