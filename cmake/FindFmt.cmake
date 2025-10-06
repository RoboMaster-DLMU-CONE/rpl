find_package(fmt CONFIG QUIET)

if (NOT fmt_FOUND)

    # When building RPLC without the RPL library, disable fmt installation
    # fmt will be statically linked into the rplc executable
    if (BUILD_RPLC AND NOT BUILD_RPL_LIBRARY)
        set(FMT_INSTALL OFF CACHE BOOL "Disable fmt installation for RPLC-only build" FORCE)
    endif ()

    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt
            GIT_TAG 11.2.0
    )
    FetchContent_MakeAvailable(fmt)

endif ()