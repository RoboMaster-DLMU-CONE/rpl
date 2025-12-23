# Parser Tests CMakeLists.txt

add_executable(test_rpl_parser
    test_parser.cpp
)

add_executable(test_rpl_parser_advanced
    test_parser_advanced.cpp
)

target_link_libraries(test_rpl_parser PRIVATE rpl)
target_link_libraries(test_rpl_parser_advanced PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Parser COMMAND test_rpl_parser)
add_test(NAME RPL_Parser_Advanced COMMAND test_rpl_parser_advanced)