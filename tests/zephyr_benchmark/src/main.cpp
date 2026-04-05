#pragma GCC diagnostic ignored "-Wunused-result"

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>

// 防止编译器优化掉没有副作用的代码
static inline void escape(void* p) {
    __asm__ volatile("" : : "g"(p) : "memory");
}

template <typename Func>
void run_benchmark(const char* name, Func&& f, uint32_t iterations) {
    // 预热 (Warmup)
    f();

    uint32_t start_cycles = k_cycle_get_32();
    for (uint32_t i = 0; i < iterations; ++i) {
        f();
    }
    uint32_t end_cycles = k_cycle_get_32();

    uint32_t total_cycles = end_cycles - start_cycles;
    uint32_t cycles_per_iter = total_cycles / iterations;

    // 获取当前系统的硬件时钟频率 (Hz)
    uint32_t freq = sys_clock_hw_cycles_per_sec();
    
    // 计算每迭代的纳秒数
    uint32_t ns_per_iter = (uint32_t)(((uint64_t)total_cycles * 1000000000ULL) / ((uint64_t)freq * iterations));
    
    printk("[BENCHMARK] %-30s : %u cycles/iter (%u ns/iter)\n", name, cycles_per_iter, ns_per_iter);
}

ZTEST(rpl_benchmarks, test_serialization_single) {
    RPL::Serializer<SampleA, SampleB> serializer;
    SampleA packet_a{42, -1234, 3.14f, 2.718};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    uint8_t buffer[frame_size];

    run_benchmark("BM_Serialization_Single", [&]() {
        auto result = serializer.serialize(buffer, sizeof(buffer), packet_a);
        escape(&result);
    }, 100000);
}

ZTEST(rpl_benchmarks, test_parser_small_full) {
    RPL::Serializer<SampleA> serializer;
    SampleA packet_a{1, 2, 3.0f, 4.0};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    uint8_t buffer[frame_size];
    serializer.serialize(buffer, sizeof(buffer), packet_a);

    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser(deserializer);

    run_benchmark("BM_Parser_Small_FullWrite", [&]() {
        auto result = parser.push_data(buffer, sizeof(buffer));
        escape(&result);
    }, 100000);
}

ZTEST(rpl_benchmarks, test_parser_small_frag) {
    RPL::Serializer<SampleA> serializer;
    SampleA packet_a{1, 2, 3.0f, 4.0};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    uint8_t buffer[frame_size];
    serializer.serialize(buffer, sizeof(buffer), packet_a);

    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser(deserializer);

    run_benchmark("BM_Parser_Small_1B_Frag", [&]() {
        for(size_t i = 0; i < sizeof(buffer); i++) {
            auto result = parser.push_data(&buffer[i], 1);
            escape(&result);
        }
    }, 10000);
}

ZTEST(rpl_benchmarks, test_deserialization_get) {
    RPL::Deserializer<SampleA> deserializer;
    auto &raw_ref = deserializer.getRawRef<SampleA>();
    raw_ref = {42, -1234, 3.14f, 2.718};

    run_benchmark("BM_Deserialization_Get", [&]() {
        auto packet = deserializer.get<SampleA>();
        escape(&packet);
    }, 1000000);
}

ZTEST_SUITE(rpl_benchmarks, NULL, NULL, NULL, NULL, NULL);
