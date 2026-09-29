cmake_minimum_required(VERSION 3.20)
project(SuperTennisExperiment C)
set(CMAKE_C_STANDARD 11)
set(SNESRECOMP_ENABLE_MODS OFF CACHE BOOL "Private headless measurement" FORCE)
set(SNESRECOMP_ROOT "${TITLE_ROOT}/snesrecomp")
include("${SNESRECOMP_ROOT}/runner/runner.cmake")
list(REMOVE_ITEM SNESRECOMP_RUNNER_SOURCES
  "${SNESRECOMP_RUNNER_ROOT}/src/keybinds.c"
  "${SNESRECOMP_RUNNER_ROOT}/src/host_report.c"
  "${SNESRECOMP_RUNNER_ROOT}/src/snes/interp_bridge.c"
  "${SNESRECOMP_RUNNER_ROOT}/src/snes/interp816.c")
file(GLOB GENERATED CONFIGURE_DEPENDS "${OVERLAY}/generated/*.c")
add_executable(experiment ${SNESRECOMP_RUNNER_SOURCES} ${GENERATED}
  "${OVERLAY}/interp_bridge.c" "${OVERLAY}/interp816.c" "${OVERLAY}/headless_main.c"
  "${TITLE_ROOT}/src/super_tennis_rtl.c" "${TITLE_ROOT}/src/super_tennis_spc_player.c")
target_include_directories(experiment PRIVATE
  ${SNESRECOMP_RUNNER_INCLUDE_DIRS} "${TITLE_ROOT}/src" "${TITLE_ROOT}/config"
  "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(experiment PRIVATE SNESRECOMP_REVERSE_DEBUG=0 SNESRECOMP_TRACE=0)
if(NOT MSVC)
  target_compile_options(experiment PRIVATE -w -Wno-implicit-function-declaration)
endif()
target_link_libraries(experiment PRIVATE ${SNESRECOMP_RUNNER_LIBRARIES})
