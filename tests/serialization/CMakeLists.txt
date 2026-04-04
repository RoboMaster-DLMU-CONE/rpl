# Serialization Tests CMakeLists.txt

add_executable(test_rpl_bitstream_serializer
    test_bitstream_serializer.cpp
)
target_link_libraries(test_rpl_bitstream_serializer PRIVATE rpl)
add_test(NAME RPL_Bitstream_Serializer COMMAND test_rpl_bitstream_serializer)

add_executable(test_rpl_serialization
    test_serialization.cpp
)

add_executable(test_rpl_serialization_mixed
    test_mixed_protocols.cpp
)

target_link_libraries(test_rpl_serialization PRIVATE rpl)
target_link_libraries(test_rpl_serialization_mixed PRIVATE rpl)

# Add test to CTest
add_test(NAME RPL_Serialization COMMAND test_rpl_serialization)
add_test(NAME RPL_Serialization_Mixed COMMAND test_rpl_serialization_mixed)