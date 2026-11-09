cmake_minimum_required(VERSION 3.16)

project(RetroDiscOS VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(RETRODISC_ENABLE_SANITIZERS "Enable ASan/UBSan in Debug builds" ON)
option(RETRODISC_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

# ---------------------------------------------------------------------------
# Dependencies (see AGENTS.md section 2: this list must not grow silently)
# ---------------------------------------------------------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
pkg_check_modules(LIBCDIO REQUIRED IMPORTED_TARGET libcdio)
pkg_check_modules(CDIO_PARANOIA REQUIRED IMPORTED_TARGET libcdio_paranoia)
pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)

# ---------------------------------------------------------------------------
# Shared compile options
# ---------------------------------------------------------------------------
add_library(retrodisc_options INTERFACE)
target_compile_options(retrodisc_options INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wcast-align -Wpointer-arith
)
if(RETRODISC_WARNINGS_AS_ERRORS)
  target_compile_options(retrodisc_options INTERFACE -Werror)
endif()

if(RETRODISC_ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined)
endif()

# ---------------------------------------------------------------------------
# Core library: everything that must be testable without a display or a drive
# ---------------------------------------------------------------------------
add_library(retrodisc_core STATIC
  src/core/version.cpp
  src/core/player_state.cpp
  src/core/event_bus.cpp
  src/cd/cd_interface.cpp
  src/cd/mock_cd.cpp
  src/audio/audio_interface.cpp
  src/audio/mock_audio.cpp
  src/audio/alsa_audio.cpp
  src/core/config.cpp
  src/core/player_core.cpp
  src/core/state_store.cpp
  src/core/play_mode.cpp
  src/core/resume.cpp
  src/core/spectrum.cpp
  src/input/key_mapping.cpp
  src/input/key_names.cpp
  src/input/evdev_keyboard.cpp
  src/input/power_key_state.cpp
  src/input/power_key.cpp
  src/display/font5x7.cpp
  src/display/text_renderer.cpp
  src/display/theme.cpp
  src/display/theme_manager.cpp
  src/display/seven_segment.cpp
  src/display/vu_meter.cpp
  src/display/brightness.cpp
  src/display/backlight.cpp
  src/ui/main_screen.cpp
  src/ui/help_screen.cpp
  src/ui/service_screen.cpp
  src/system/diagnostics.cpp
)
target_include_directories(retrodisc_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(retrodisc_core PUBLIC retrodisc_options PkgConfig::SDL2 PkgConfig::ALSA PkgConfig::LIBEVDEV)
target_compile_definitions(retrodisc_core PUBLIC RETRODISC_VERSION="${PROJECT_VERSION}")

# ---------------------------------------------------------------------------
# Executable
# ---------------------------------------------------------------------------
add_executable(retrodisc src/main.cpp)
target_link_libraries(retrodisc PRIVATE retrodisc_core)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
