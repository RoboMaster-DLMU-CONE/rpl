add_executable(rpl_benchmark rpl_benchmark.cpp)

target_link_libraries(rpl_benchmark PRIVATE rpl benchmark::benchmark)
