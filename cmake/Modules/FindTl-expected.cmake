include(FetchContent)
find_package(tl-expected QUIET)
if (NOT tl-expected_FOUND AND NOT TARGET expected)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0135 NEW)
    set(EXPECTED_BUILD_PACKAGE OFF)
    set(EXPECTED_BUILD_TESTS OFF)
    set(EXPECTED_BUILD_PACKAGE_DEB OFF)
    execute_process(
            COMMAND git ls-remote --heads https://github.com/TartanLlama/expected master
            RESULT_VARIABLE tl_expected_primary_repo_ok
            OUTPUT_QUIET
            ERROR_QUIET
    )
    if (NOT tl_expected_primary_repo_ok EQUAL 0)
        message(STATUS "Primary fetch failed, falling back to Gitee mirror")
        set(tl_expected_repo https://gitee.com/dlmu-cone/expected.git)
    else ()
        set(tl_expected_repo https://github.com/TartanLlama/expected)
    endif ()

    FetchContent_Declare(tl_expected
            GIT_REPOSITORY ${tl_expected_repo}
            GIT_TAG master
    )
    FetchContent_MakeAvailable(tl_expected)
    cmake_policy(POP)
endif ()