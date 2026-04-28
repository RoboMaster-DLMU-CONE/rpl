# Parser Tests CMakeLists.txt

add_executable(test_rpl_parser
    test_parser.cpp
)

add_executable(test_rpl_parser_advanced
    test_parser_advanced.cpp
)

add_executable(test_rpl_parser_mixed
    test_mixed_protocols.cpp
)

add_executable(test_rpl_connection_monitor
    test_connection_monitor.cpp
)

add_executable(test_rpl_parser_hooks
    test_parser_hooks.cpp
)

target_link_libraries(test_rpl_parser PRIVATE rpl)
target_link_libraries(test_rpl_parser_advanced PRIVATE rpl)
target_link_libraries(test_rpl_parser_mixed PRIVATE rpl)
target_link_libraries(test_rpl_connection_monitor PRIVATE rpl)
target_link_libraries(test_rpl_parser_hooks PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Parser COMMAND test_rpl_parser)
add_test(NAME RPL_Parser_Advanced COMMAND test_rpl_parser_advanced)
add_test(NAME RPL_Parser_Mixed COMMAND test_rpl_parser_mixed)
add_test(NAME RPL_Connection_Monitor COMMAND test_rpl_connection_monitor)
add_test(NAME RPL_Parser_Hooks COMMAND test_rpl_parser_hooks)