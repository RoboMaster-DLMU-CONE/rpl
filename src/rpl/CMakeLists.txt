include(FindRplModules)
add_library(rpl INTERFACE)
add_library(rpl::rpl ALIAS rpl)

target_include_directories(rpl INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${cppcrc_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

if (ZEPHYR_TOOLCHAIN_VARIANT AND CONFIG_RPL STREQUAL "y")
    zephyr_include_directories(${PROJECT_SOURCE_DIR}/include)

    # Handle cppcrc include directory
    if(DEFINED cppcrc_SOURCE_DIR)
        zephyr_include_directories(${cppcrc_SOURCE_DIR})
    endif()

    if(DEFINED frozen_SOURCE_DIR)
        zephyr_include_directories(${frozen_SOURCE_DIR}/include)
    elseif(TARGET frozen::frozen)
        get_target_property(frozen_includes frozen::frozen INTERFACE_INCLUDE_DIRECTORIES)
        if(frozen_includes)
            zephyr_include_directories(${frozen_includes})
        endif()
    endif()

    # Handle tl-expected include directory
    if(DEFINED tl-expected_SOURCE_DIR)
        zephyr_include_directories(${tl-expected_SOURCE_DIR}/include)
    elseif(TARGET tl::expected)
        get_target_property(expected_includes tl::expected INTERFACE_INCLUDE_DIRECTORIES)
        if(expected_includes)
            zephyr_include_directories(${expected_includes})
        endif()
    endif()
endif ()

target_link_libraries(rpl INTERFACE
        tl::expected
        frozen
)

target_compile_definitions(rpl INTERFACE
        FRAME_HEADER_TAG=${FRAME_HEADER_TAG}
)

if (NOT ZEPHYR_TOOLCHAIN_VARIANT)
    # Install RPL library headers
    install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/RPL
            DESTINATION include
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    )

    # Install cppcrc header (it's a header-only library without install support)
    install(FILES ${cppcrc_SOURCE_DIR}/cppcrc.h
            DESTINATION include
    )

    # Install the rpl target
    install(TARGETS rpl
            EXPORT rplTargets
            INCLUDES DESTINATION include
    )

    # Create and install CMake package config files
    include(CMakePackageConfigHelpers)

    # Configure the package config file from template
    configure_package_config_file(
            ${PROJECT_SOURCE_DIR}/cmake/rplConfig.cmake.in
            ${CMAKE_CURRENT_BINARY_DIR}/rplConfig.cmake
            INSTALL_DESTINATION lib/cmake/rpl
    )

    write_basic_package_version_file(
            ${CMAKE_CURRENT_BINARY_DIR}/rplConfigVersion.cmake
            VERSION ${PROJECT_VERSION}
            COMPATIBILITY SameMajorVersion
    )

    # Install the export set
    install(EXPORT rplTargets
            FILE rplTargets.cmake
            NAMESPACE rpl::
            DESTINATION lib/cmake/rpl
    )

    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/rplConfig.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/rplConfigVersion.cmake
            DESTINATION lib/cmake/rpl
    )
endif ()
