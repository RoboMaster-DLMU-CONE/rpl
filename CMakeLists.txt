cmake_minimum_required(VERSION 3.20)
project(rpl)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(FetchContent)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src)