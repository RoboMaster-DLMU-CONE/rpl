find_package(nlohmann_json CONFIG QUIET)

if (NOT nlohmann_json_FOUND)

    # When building RPLC only, disable nlohmann_json installation
    # nlohmann_json is header-only and will be compiled into the rplc executable
    if (BUILD_RPLC AND NOT BUILD_RPL_LIBRARY)
        set(JSON_Install OFF CACHE BOOL "Disable JSON installation for RPLC" FORCE)
    endif ()

    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
        cmake_policy(SET CMP0135 NEW)
    endif ()
    FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
    FetchContent_MakeAvailable(json)

endif ()