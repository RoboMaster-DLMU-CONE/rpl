cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(rpl_zephyr_benchmark)

include(FetchContent)

FetchContent_Declare(
    cppcrc
    GIT_REPOSITORY https://gitee.com/dlmu-cone/cppcrc
    GIT_TAG main
)
FetchContent_MakeAvailable(cppcrc)

set(EXPECTED_BUILD_PACKAGE OFF)
set(EXPECTED_BUILD_TESTS OFF)
FetchContent_Declare(
    tl_expected
    GIT_REPOSITORY https://gitee.com/dlmu-cone/expected.git
    GIT_TAG master
)
FetchContent_MakeAvailable(tl_expected)

FetchContent_Declare(
    frozen
    GIT_REPOSITORY https://gitee.com/dlmu-cone/frozen
    GIT_TAG master
)
FetchContent_MakeAvailable(frozen)

target_sources(app PRIVATE src/main.cpp)

# 包含 RPL 的核心头文件
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
# 包含测试用的 Packet 定义
target_include_directories(app PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include/RPL/Packets/Sample)

# 手动包含第三方库
target_include_directories(app PRIVATE ${cppcrc_SOURCE_DIR})
target_include_directories(app PRIVATE ${tl_expected_SOURCE_DIR}/include)
target_include_directories(app PRIVATE ${frozen_SOURCE_DIR}/include)
