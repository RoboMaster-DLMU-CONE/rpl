add_executable(test_rpl_usb_transport
    test_usb_transport.cpp
)

target_link_libraries(test_rpl_usb_transport PRIVATE rpl)
add_test(NAME RPL_USB_Transport COMMAND test_rpl_usb_transport)
