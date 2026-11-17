cmake_minimum_required(VERSION 3.25)

project(SeamCarving
    VERSION 0.1.0
    DESCRIPTION "Seam carving with Dynamic Programming, Greedy and Graph Cut"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SEAM_BUILD_APP   "Build the ImGui viewer"      ON)
option(SEAM_BUILD_BENCH "Build the benchmark CLI"     ON)
option(SEAM_BUILD_TESTS "Build the unit tests"        ON)

# Absolute paths baked into app/bench so they find images regardless of working directory.
set(SEAM_ASSETS_DIR  "${CMAKE_SOURCE_DIR}/assets")
set(SEAM_RESULTS_DIR "${CMAKE_SOURCE_DIR}/results")

# Shared warning/flag settings for our own targets (not third-party code).
add_library(seam_warnings INTERFACE)
if(MSVC)
    target_compile_options(seam_warnings INTERFACE /W4 /permissive- /utf-8 /Zc:__cplusplus)
else()
    target_compile_options(seam_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_subdirectory(core)

if(SEAM_BUILD_APP)
    add_subdirectory(app)
    # Visual Studio solution: make the viewer the default "Start" project.
    set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT seam_app)
endif()

if(SEAM_BUILD_BENCH)
    add_subdirectory(bench)
endif()

if(SEAM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
