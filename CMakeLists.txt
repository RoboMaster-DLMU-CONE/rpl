cmake_minimum_required(VERSION 3.20)
project(rpl VERSION 0.1.1)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(FetchContent)

set(RPL_VERSION 0.1.1)
set(RPLC_VERSION 0.1.2)
option(BUILD_RPL_BENCHMARK "Enable building RPL Benchmark" OFF)
option(BUILD_RPL_TESTS "Enable building RPL Tests" OFF)
option(BUILD_RPLC "Enable build RPLC" OFF)
option(BUILD_RPL_LIBRARY "Enable build RPL Library" ON)
option(BUILD_RPL_SAMPLES "Enable building RPL Samples" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(FRAME_HEADER_TAG "0xA5")

add_subdirectory(src)

if (BUILD_RPL_SAMPLES)
    add_subdirectory(samples)
endif ()

if (BUILD_RPL_BENCHMARK)
    include(FindGoogleBenchmark)
    add_subdirectory(${PROJECT_SOURCE_DIR}/benchmark)
endif ()

if (BUILD_RPL_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# CPack configuration based on what is being built
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VENDOR "C-One-Studio")
set(CPACK_PACKAGE_CONTACT "MoonFeather <moonfeather120@outlook.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if (BUILD_RPLC)
    include(RplcPackaging)
else ()
    include(RplPackaging)
endif ()

include(CPack)
