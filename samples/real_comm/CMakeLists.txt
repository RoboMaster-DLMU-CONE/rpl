include(Modules/FindHySerial)

add_executable(rpl_sample_real_comm
        main.cpp
)

target_link_libraries(rpl_sample_real_comm PUBLIC
        HySerial::HySerial
        rpl::rpl
)