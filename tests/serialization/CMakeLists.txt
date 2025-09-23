# Serialization Tests CMakeLists.txt

add_executable(test_rpl_serialization
    test_serialization.cpp
)

target_link_libraries(test_rpl_serialization PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Serialization COMMAND test_rpl_serialization)