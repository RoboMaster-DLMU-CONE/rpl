add_library(rpl_cppcrc INTERFACE)
add_library(rpl::cppcrc ALIAS rpl_cppcrc)
target_include_directories(rpl_cppcrc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/cppcrc)

add_library(rpl_frozen INTERFACE)
add_library(rpl::frozen ALIAS rpl_frozen)
target_include_directories(rpl_frozen INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/frozen)

add_library(rpl_expected INTERFACE)
add_library(rpl::expected ALIAS rpl_expected)
target_include_directories(rpl_expected INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tl-expected)
