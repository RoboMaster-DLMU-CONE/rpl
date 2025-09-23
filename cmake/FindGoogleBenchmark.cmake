set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
set(BENCHMARK_ENABLE_TESTING OFF)

find_package(benchmark QUIET)

if (NOT benchmark_FOUND)
    message(STATUS "Google Benchmark can't be found locally, try fetching from remote...")
    include(FetchContent)
    execute_process(
            COMMAND git ls-remote --heads https://github.com/google/benchmark
            RESULT_VARIABLE benchmark_primary_repo_ok
            OUTPUT_QUIET
            ERROR_QUIET
    )

    if (NOT benchmark_primary_repo_ok EQUAL 0)
        message(STATUS "Primary fetch failed, falling back to Gitee mirror")
        set(benchmark_repo https://gitee.com/moonfeather/benchmark)
    else ()
        set(benchmark_repo https://github.com/google/benchmark)
    endif ()

    FetchContent_Declare(
            benchmark_fetched
            GIT_REPOSITORY ${benchmark_repo}
            GIT_TAG "main"
    )
    FetchContent_MakeAvailable(benchmark_fetched)
else ()
    message(STATUS "Found Google Benchmark v${benchmark_VERSION}")
endif ()