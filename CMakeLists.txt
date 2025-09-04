cmake_minimum_required(VERSION 3.20)
project(rpl)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

set(FRAME_HEADER_TAG "0xA5")

add_subdirectory(src)
add_subdirectory(samples)