# Traits Tests CMakeLists.txt

add_executable(test_rpl_traits
    test_traits.cpp
)

target_link_libraries(test_rpl_traits PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Traits COMMAND test_rpl_traits)