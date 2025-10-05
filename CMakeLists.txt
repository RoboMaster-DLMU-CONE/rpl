cmake_minimum_required(VERSION 3.20)
project(rpl VERSION 0.1.0)
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(FetchContent)

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
set(CPACK_PACKAGE_VENDOR "TeamCONE")
set(CPACK_PACKAGE_CONTACT "MoonFeather <moonfeather120@outlook.com>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

if (BUILD_RPLC)
    # RPLC executable package configuration
    set(CPACK_PACKAGE_NAME "rplc")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RPLC - RPL Command Line Tool")

    # Generators for executable (end-user tool)
    set(CPACK_GENERATOR "TGZ;ZIP;DEB;RPM;NSIS")

    # Debian settings for executable package
    set(CPACK_DEBIAN_PACKAGE_NAME "rplc")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

    # RPM settings for executable package
    set(CPACK_RPM_PACKAGE_NAME "rplc")
    set(CPACK_RPM_PACKAGE_LICENSE "ISC")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
    set(CPACK_RPM_PACKAGE_REQUIRES "")

    # NSIS installer settings for Windows
    set(CPACK_NSIS_DISPLAY_NAME "RPLC ${CPACK_PACKAGE_VERSION}")
    set(CPACK_NSIS_PACKAGE_NAME "RPLC")
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
else ()
    # RPL library package configuration
    set(CPACK_PACKAGE_NAME "rpl")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RPL - RoboMaster Packet Library")

    # Generators for library (development package)
    set(CPACK_GENERATOR "TGZ;ZIP;DEB;RPM")

    # Debian settings for development package
    set(CPACK_DEBIAN_PACKAGE_NAME "librpl-dev")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

    # RPM settings for development package
    set(CPACK_RPM_PACKAGE_NAME "rpl-devel")
    set(CPACK_RPM_PACKAGE_LICENSE "ISC")
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
endif ()

include(CPack)
