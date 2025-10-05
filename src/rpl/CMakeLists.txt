include(FindTl-expected)
include(FindCppcrc)
include(FindFrozen)

add_library(rpl INTERFACE)

target_include_directories(rpl INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${ringbuffer_SOURCE_DIR}>
)

target_link_libraries(rpl INTERFACE
        cppcrc
        tl::expected
        frozen
)

target_compile_definitions(rpl INTERFACE
        FRAME_HEADER_TAG=${FRAME_HEADER_TAG}
)

# Install RPL library headers only
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/RPL
        DESTINATION include
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
)

# Create and install CMake package config files
include(CMakePackageConfigHelpers)

# Write a custom config file that doesn't export complex dependencies
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/rplConfig.cmake "
# RPL Package Configuration
# This is a header-only library

# Include directories
get_filename_component(RPL_CMAKE_DIR \"\${CMAKE_CURRENT_LIST_FILE}\" PATH)
set(RPL_INCLUDE_DIRS \"\${RPL_CMAKE_DIR}/../../../include\")

# Users need to manually find these dependencies:
# - tl::expected
# - cppcrc
# - frozen
# - ringbuffer


# Create interface library
if(NOT TARGET rpl::rpl)
    add_library(rpl::rpl INTERFACE IMPORTED)
    set_target_properties(rpl::rpl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES \"\${RPL_INCLUDE_DIRS}\"
        INTERFACE_COMPILE_DEFINITIONS \"FRAME_HEADER_TAG=0xA5\"
    )
endif()
")

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/rplConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/rplConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/rplConfigVersion.cmake
        DESTINATION lib/cmake/rpl
)
