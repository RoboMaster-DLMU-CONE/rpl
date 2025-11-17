#include <benchmark/benchmark.h>
#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <vector>

// Define the packets for benchmarking
using PacketA = SampleA;
using PacketB = SampleB;

// --- Serialization Benchmarks ---

static void BM_Serialization_SinglePacket(benchmark::State& state)
{
    RPL::Serializer<PacketA, PacketB> serializer;
    PacketA packet_a{42, -1234, 3.14f, 2.718};
    constexpr size_t frame_size = RPL::Serializer<PacketA>::frame_size<PacketA>();
    std::vector<uint8_t> buffer(frame_size);

    for (auto _ : state)
    {
        auto result = serializer.serialize(buffer.data(), buffer.size(), packet_a);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_Serialization_SinglePacket);

static void BM_Serialization_MultiPacket(benchmark::State& state)
{
    RPL::Serializer<PacketA, PacketB> serializer;
    PacketA packet_a{42, -1234, 3.14f, 2.718};
    PacketB packet_b{1337, 9.876};
    constexpr size_t total_size = RPL::Serializer<PacketA, PacketB>::frame_size<PacketA>() + RPL::Serializer<
        PacketA, PacketB>::frame_size<PacketB>();
    std::vector<uint8_t> buffer(total_size);

    for (auto _ : state)
    {
        auto result = serializer.serialize(buffer.data(), buffer.size(), packet_a, packet_b);
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK(BM_Serialization_MultiPacket);


// --- Deserialization Benchmark ---

static void BM_Deserialization_GetPacket(benchmark::State& state)
{
    RPL::Deserializer<PacketA, PacketB> deserializer;
    // Pre-fill the memory pool for a realistic get scenario
    auto& raw_ref_a = deserializer.getRawRef<PacketA>();
    raw_ref_a = {42, -1234, 3.14f, 2.718};

    for (auto _ : state)
    {
        auto packet = deserializer.get<PacketA>();
        benchmark::DoNotOptimize(packet);
    }
}

BENCHMARK(BM_Deserialization_GetPacket);


// --- Parser Benchmarks ---

class ParserFixture : public benchmark::Fixture
{
public:
    RPL::Serializer<PacketA, PacketB> serializer;
    RPL::Deserializer<PacketA, PacketB> deserializer;
    RPL::Parser<PacketA, PacketB> parser{deserializer};

    PacketA packet_a{42, -1234, 3.14f, 2.718};
    PacketB packet_b{1337, 9.876};

    std::vector<uint8_t> single_packet_buffer;
    std::vector<uint8_t> multi_packet_buffer;
    std::vector<uint8_t> noisy_buffer;

    void SetUp(const ::benchmark::State& state) override
    {
        // Prepare single packet buffer
        constexpr size_t frame_size_a = RPL::Serializer<PacketA, PacketB>::frame_size<PacketA>();
        single_packet_buffer.resize(frame_size_a);
        (void)serializer.serialize(single_packet_buffer.data(), single_packet_buffer.size(), packet_a);

        // Prepare multi-packet buffer
        constexpr size_t frame_size_b = RPL::Serializer<PacketA, PacketB>::frame_size<PacketB>();
        multi_packet_buffer.resize(frame_size_a + frame_size_b);
        (void)serializer.serialize(multi_packet_buffer.data(), multi_packet_buffer.size(), packet_a, packet_b);

        // Prepare noisy buffer (some garbage at the beginning)
        std::vector<uint8_t> garbage(50, 0xAB);
        noisy_buffer.insert(noisy_buffer.end(), garbage.begin(), garbage.end());
        noisy_buffer.insert(noisy_buffer.end(), single_packet_buffer.begin(), single_packet_buffer.end());
    }

    void TearDown(const ::benchmark::State& state) override
    {
        parser.clear_buffer();
    }
};

BENCHMARK_F(ParserFixture, BM_Parser_PushAndParse_SinglePacket)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        parser.clear_buffer();
        state.ResumeTiming();

        auto result = parser.push_data(single_packet_buffer.data(), single_packet_buffer.size());
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ParserFixture, BM_Parser_PushAndParse_MultiPacket)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        parser.clear_buffer();
        state.ResumeTiming();

        auto result = parser.push_data(multi_packet_buffer.data(), multi_packet_buffer.size());
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_F(ParserFixture, BM_Parser_PushAndParse_WithNoise)(benchmark::State& state)
{
    for (auto _ : state)
    {
        state.PauseTiming();
        parser.clear_buffer();
        state.ResumeTiming();

        auto result = parser.push_data(noisy_buffer.data(), noisy_buffer.size());
        benchmark::DoNotOptimize(result);
    }
}

BENCHMARK_MAIN();

