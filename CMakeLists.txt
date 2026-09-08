cmake_minimum_required(VERSION 3.20)
project(Super_TennisRecomp C CXX)
set(CMAKE_C_STANDARD 11)

set(SNESRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/snesrecomp" CACHE PATH
    "Path to the snesrecomp checkout used by this game")
include(${SNESRECOMP_ROOT}/runner/runner.cmake)

# Dear ImGui pre-boot launcher UI (mstan/recomp-ui submodule). One call wires
# the launcher into the desktop target only; the headless target stays
# SDL-free. recomp_ui.cmake reads SNESRECOMP_SDL_BACKEND (set by runner.cmake)
# and selects the matching SDL platform backend automatically.
set(RECOMP_UI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/recomp-ui" CACHE PATH
    "Path to the recomp-ui checkout used by this game")
include(${RECOMP_UI_ROOT}/recomp_ui.cmake)

# The shared list includes two desktop-only helpers that intentionally pull
# SDL key/window types. This target has no UI, so keep its runtime source set
# SDL-free.
set(SUPER_TENNIS_HEADLESS_RUNNER_SOURCES ${SNESRECOMP_RUNNER_SOURCES})
list(REMOVE_ITEM SUPER_TENNIS_HEADLESS_RUNNER_SOURCES
  "${SNESRECOMP_RUNNER_ROOT}/src/keybinds.c"
  "${SNESRECOMP_RUNNER_ROOT}/src/host_report.c")

file(GLOB GENERATED_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/generated/*.c")
if(NOT GENERATED_SOURCES)
  message(FATAL_ERROR
    "No generated C found. Run tools/regenerate.sh, or build.ps1 on Windows, with the supported ROM.")
endif()
add_library(snesrecomp_game STATIC ${GENERATED_SOURCES})
target_include_directories(snesrecomp_game PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/config"
  ${SNESRECOMP_RUNNER_INCLUDE_DIRS})
if(NOT MSVC)
  target_compile_options(snesrecomp_game PRIVATE
    -w -Wno-implicit-function-declaration)
endif()

add_executable(super_tennis_headless
  ${SUPER_TENNIS_HEADLESS_RUNNER_SOURCES}
  src/headless_main.c
  src/super_tennis_rtl.c
  src/super_tennis_spc_player.c)
target_compile_definitions(super_tennis_headless PRIVATE
  SNESRECOMP_REVERSE_DEBUG=0
  SNESRECOMP_TRACE=0)
target_include_directories(super_tennis_headless PRIVATE
  ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
  "${CMAKE_CURRENT_SOURCE_DIR}/config"
  "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(NOT MSVC)
  target_compile_options(super_tennis_headless PRIVATE
    -w -Wno-implicit-function-declaration)
endif()
target_link_libraries(super_tennis_headless PRIVATE
  snesrecomp_game
  ${SNESRECOMP_RUNNER_LIBRARIES})

add_executable(super_tennis
  ${SNESRECOMP_RUNNER_SOURCES}
  src/main.c
  src/config.c
  src/super_tennis_rtl.c
  src/super_tennis_spc_player.c)
target_compile_definitions(super_tennis PRIVATE
  SNESRECOMP_REVERSE_DEBUG=0
  SNESRECOMP_TRACE=0)
target_include_directories(super_tennis PRIVATE
  ${SNESRECOMP_RUNNER_INCLUDE_DIRS}
  "${SNESRECOMP_RUNNER_ROOT}/src/desktop"
  "${CMAKE_CURRENT_SOURCE_DIR}/config"
  "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(NOT MSVC)
  target_compile_options(super_tennis PRIVATE
    -w -Wno-implicit-function-declaration)
endif()
snesrecomp_target_sdl(super_tennis)
# CONSOLE is required by recomp-ui: it selects exactly one validated console
# asset manifest and profile. BOXART is a self-made placeholder (the real
# commercial box art stays private). The launcher is compiled into this
# desktop target and defines RECOMP_LAUNCHER for src/main.c; nothing in the
# headless target sees it.
recomp_target_launcher_ui(super_tennis
    CONSOLE snes
    BOXART ${CMAKE_CURRENT_SOURCE_DIR}/recomp/launcher/boxart.tga)

# ---- in-game runtime settings overlay (recomp-ui RecompRuntimeUi) --------
# The overlay draws through SDL_Renderer. recomp-ui's pinned Dear ImGui
# (1.91.9b) ships only the SDL2 renderer backend, so the SDL3 backend is a
# local port (src/imgui_impl_sdlrenderer3.cpp). The desktop host's render
# path is SDL3-only (ungated SDL3 calls in src/main.c), so the overlay is
# compiled only for the SDL3 backend; the SDL2 fallback keeps the
# pre-overlay host.
if(SNESRECOMP_SDL_BACKEND STREQUAL "SDL3")
  target_sources(super_tennis PRIVATE
    src/runtime_ui.c
    src/runtime_ui_imgui.cpp
    src/imgui_impl_sdlrenderer3.cpp)
endif()

target_link_libraries(super_tennis PRIVATE
  snesrecomp_game
  ${SNESRECOMP_RUNNER_LIBRARIES})

# Keep the public README beside the desktop executable so it is included when
# the build directory is packaged. It contains the PolyForm URL and required
# copyright notice used by binary releases.
set_property(TARGET super_tennis APPEND PROPERTY LINK_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
add_custom_command(TARGET super_tennis POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
          "$<TARGET_FILE_DIR:super_tennis>"
  VERBATIM)
