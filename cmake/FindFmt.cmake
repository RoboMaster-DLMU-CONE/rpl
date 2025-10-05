find_package(fmt CONFIG QUIET)

if (NOT fmt_FOUND)

    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt
            GIT_TAG 11.2.0
    )
    FetchContent_MakeAvailable(fmt)

endif ()