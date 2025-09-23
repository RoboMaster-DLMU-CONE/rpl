# Parser Tests CMakeLists.txt

add_executable(test_rpl_parser
    test_parser.cpp
)

target_link_libraries(test_rpl_parser PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Parser COMMAND test_rpl_parser)