# Integration Tests CMakeLists.txt

add_executable(test_rpl_integration
    test_integration.cpp
)

target_link_libraries(test_rpl_integration PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Integration COMMAND test_rpl_integration)