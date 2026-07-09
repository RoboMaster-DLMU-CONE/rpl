include(FindRplModules)
add_library(rpl INTERFACE)
add_library(rpl::rpl ALIAS rpl)

target_include_directories(rpl INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if (ZEPHYR_TOOLCHAIN_VARIANT AND CONFIG_RPL STREQUAL "y")
  zephyr_include_directories(${PROJECT_SOURCE_DIR}/include)

  foreach(_dep rpl_cppcrc rpl_frozen rpl_expected)
    if(TARGET ${_dep})
      get_target_property(_inc ${_dep} INTERFACE_INCLUDE_DIRECTORIES)
      if(_inc)
        zephyr_include_directories(${_inc})
      endif()
    endif()
  endforeach()
endif ()

target_link_libraries(rpl INTERFACE
        $<BUILD_INTERFACE:rpl::cppcrc>
        $<BUILD_INTERFACE:rpl::expected>
        $<BUILD_INTERFACE:rpl::frozen>
)

target_compile_features(rpl INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(rpl INTERFACE /utf-8)
endif()

# 环境检测：在 Linux 环境下默认使用 std::atomic 保证多核安全
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(RPL_USE_STD_ATOMIC_DEFAULT ON)
else()
    set(RPL_USE_STD_ATOMIC_DEFAULT OFF)
endif()

option(RPL_USE_STD_ATOMIC "Use std::atomic for SeqLock (recommended for SMP/Multi-core)" ${RPL_USE_STD_ATOMIC_DEFAULT})

if(RPL_USE_STD_ATOMIC)
    target_compile_definitions(rpl INTERFACE RPL_USE_STD_ATOMIC)
endif()

target_compile_definitions(rpl INTERFACE
        FRAME_HEADER_TAG=${FRAME_HEADER_TAG}
)

find_package(Python3 QUIET)
if (Python3_FOUND)
    add_custom_target(rpl_amalgamate
        COMMAND ${Python3_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/amalgamate.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Generating amalgamated RPL.hpp header"
    )
endif ()

if (NOT ZEPHYR_TOOLCHAIN_VARIANT)
  # Install RPL library headers
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/RPL
            DESTINATION include
            FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h"
    )

  # Install vendored third-party headers
  install(FILES ${PROJECT_SOURCE_DIR}/3rdparty/cppcrc/cppcrc.h
            DESTINATION include
    )
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/3rdparty/frozen/frozen
            DESTINATION include
            FILES_MATCHING PATTERN "*.h"
    )
  install(DIRECTORY ${PROJECT_SOURCE_DIR}/3rdparty/tl-expected/tl
            DESTINATION include
            FILES_MATCHING PATTERN "*.hpp"
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
