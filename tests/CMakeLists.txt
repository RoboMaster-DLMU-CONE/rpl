# Include test framework setup
include(CTest)

# Add each test subdirectory
add_subdirectory(serialization)
add_subdirectory(deserialization)
add_subdirectory(parser)
add_subdirectory(integration)
add_subdirectory(traits)

if (EXISTS "${PROJECT_SOURCE_DIR}/include/RPL/RPL.hpp")
    add_executable(test_amalgamation test_amalgamation.cpp)
    target_link_libraries(test_amalgamation PRIVATE rpl)
    target_compile_features(test_amalgamation PRIVATE cxx_std_20)
    add_test(NAME test_amalgamation COMMAND test_amalgamation)
endif ()