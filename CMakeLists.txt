cmake_minimum_required(VERSION 3.20)
project(rpl VERSION 0.2.0)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

set(RPL_VERSION 0.2.0)
option(BUILD_RPL_BENCHMARK "Enable building RPL Benchmark" OFF)
option(BUILD_RPL_TESTS "Enable building RPL Tests" OFF)
option(BUILD_RPL_LIBRARY "Enable building RPL Library" ON)
option(BUILD_RPL_CPACK "Enable building RPL Packages" OFF)
option(BUILD_RPL_SAMPLES "Enable building RPL Samples" OFF)
option(BUILD_RPL_DOC "Enable building RPL Document" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 20)
set(FRAME_HEADER_TAG "0xA5")

add_subdirectory(src)

if (BUILD_RPL_SAMPLES)
  add_subdirectory(samples)
endif ()

if (BUILD_RPL_BENCHMARK)
  include(Modules/FindGoogleBenchmark)
  add_subdirectory(${PROJECT_SOURCE_DIR}/benchmark)
endif ()

if (BUILD_RPL_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif ()

if(BUILD_RPL_CPACK)
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VENDOR "C-One-Studio")
set(CPACK_PACKAGE_CONTACT "MoonFeather <moonfeather120@outlook.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(RplPackaging)

include(CPack)
endif()

if(BUILD_RPL_DOC)
include(RplGenerateDoc)
endif()
