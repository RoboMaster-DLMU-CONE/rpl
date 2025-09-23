# Deserialization Tests CMakeLists.txt

add_executable(test_rpl_deserialization
    test_deserialization.cpp
)

target_link_libraries(test_rpl_deserialization PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Deserialization COMMAND test_rpl_deserialization)