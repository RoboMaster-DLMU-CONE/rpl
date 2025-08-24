set(EXPECTED_BUILD_PACKAGE OFF)
set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE_DEB OFF)

set(CTEST_EMPTY_BINARY_DIRECTORY TRUE)

FetchContent_Declare(
        tl-expected
        GIT_REPOSITORY "https://github.com/TartanLlama/expected"
        GIT_TAG "master"
)
FetchContent_MakeAvailable(tl-expected)