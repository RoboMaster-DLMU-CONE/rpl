cmake_minimum_required(VERSION 3.20)
project(rpl)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(FetchContent)

option(BUILD_RPL_BENCHMARK "Build RPL Benchmark" OFF)
option(BUILD_RPL_TESTS "Build RPL Tests" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


set(CMAKE_CXX_STANDARD 20)

set(FRAME_HEADER_TAG "0xA5")

add_subdirectory(src)
add_subdirectory(samples)

if (BUILD_RPL_BENCHMARK)
    include(FindGoogleBenchmark)
    add_subdirectory(${PROJECT_SOURCE_DIR}/benchmark)
endif ()

if (BUILD_RPL_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()