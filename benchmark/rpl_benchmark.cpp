#include <benchmark/benchmark.h>

#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Serializer.hpp>

#include "rpl_benchmark_packets.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <vector>

using PacketA = SampleA;
using PacketB = SampleB;

using StressSmall = SmallPacket;
using StressMedium = MediumPacket;
using StressLarge = LargePacket;
using StressXLarge = XLargePacket;

using StressSerializer =
    RPL::Serializer<PacketA, PacketB, StressSmall, StressMedium, StressLarge,
                    StressXLarge>;
using StressDeserializer =
    RPL::Deserializer<PacketA, PacketB, StressSmall, StressMedium, StressLarge,
                      StressXLarge>;
using StressParser =
    RPL::Parser<PacketA, PacketB, StressSmall, StressMedium, StressLarge,
                StressXLarge>;

template <typename Packet>
static Packet make_packet_pattern(uint8_t seed) {
  Packet packet{};
  auto *raw = reinterpret_cast<uint8_t *>(&packet);
  for (size_t i = 0; i < sizeof(Packet); ++i) {
    raw[i] = static_cast<uint8_t>(seed + static_cast<uint8_t>(i * 13U + 7U));
  }
  return packet;
}

static std::vector<uint8_t> serialize_one(StressSerializer &serializer,
                                          const auto &packet) {
  constexpr size_t frame_size = StressSerializer::template frame_size<
      std::remove_cvref_t<decltype(packet)>>();
  std::vector<uint8_t> out(frame_size);
  (void)serializer.serialize(out.data(), out.size(), packet);
  return out;
}

template <typename... Packets>
static std::vector<uint8_t> serialize_many(StressSerializer &serializer,
                                           const Packets &...packets) {
  const size_t total_size =
      (StressSerializer::template frame_size<std::remove_cvref_t<Packets>>() +
       ... + 0U);
  std::vector<uint8_t> out(total_size);
  (void)serializer.serialize(out.data(), out.size(), packets...);
  return out;
}

static std::vector<uint8_t> build_repeated_stream(const std::vector<uint8_t> &f1,
                                                  const std::vector<uint8_t> &f2,
                                                  size_t repeat) {
  std::vector<uint8_t> out;
  out.reserve((f1.size() + f2.size()) * repeat);
  for (size_t i = 0; i < repeat; ++i) {
    out.insert(out.end(), f1.begin(), f1.end());
    out.insert(out.end(), f2.begin(), f2.end());
  }
  return out;
}

static std::vector<uint8_t> build_repeated_stream(const std::vector<uint8_t> &f,
                                                  size_t repeat) {
  std::vector<uint8_t> out;
  out.reserve(f.size() * repeat);
  for (size_t i = 0; i < repeat; ++i) {
    out.insert(out.end(), f.begin(), f.end());
  }
  return out;
}

static void process_fragmented(StressParser &parser, const std::vector<uint8_t> &data,
                               size_t chunk_size) {
  size_t offset = 0;
  while (offset < data.size()) {
    const size_t n = std::min(chunk_size, data.size() - offset);
    const auto result = parser.push_data(data.data() + offset, n);
    benchmark::DoNotOptimize(result);
    offset += n;
  }
}

static void process_via_advance_write(StressParser &parser,
                                      const std::vector<uint8_t> &data,
                                      size_t chunk_size) {
  size_t offset = 0;
  while (offset < data.size()) {
    const size_t n = std::min(chunk_size, data.size() - offset);
    auto write_span = parser.get_write_buffer();
    if (write_span.size() < n) {
      parser.clear_buffer();
      write_span = parser.get_write_buffer();
      if (write_span.size() < n) {
        return;
      }
    }
    std::memcpy(write_span.data(), data.data() + offset, n);
    const auto result = parser.advance_write_index(n);
    benchmark::DoNotOptimize(result);
    offset += n;
  }
}

static void set_throughput(benchmark::State &state, int64_t bytes_per_iteration,
                           int64_t items_per_iteration) {
  state.SetBytesProcessed(bytes_per_iteration *
                          static_cast<int64_t>(state.iterations()));
  state.SetItemsProcessed(items_per_iteration *
                          static_cast<int64_t>(state.iterations()));
}

// --- Serialization Benchmarks ---

static void BM_Serialization_SinglePacket(benchmark::State &state) {
  RPL::Serializer<PacketA, PacketB> serializer;
  PacketA packet_a{42, -1234, 3.14f, 2.718};
  constexpr size_t frame_size = RPL::Serializer<PacketA>::frame_size<PacketA>();
  std::vector<uint8_t> buffer(frame_size);

  for (auto _ : state) {
    auto result = serializer.serialize(buffer.data(), buffer.size(), packet_a);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Serialization_SinglePacket);

static void BM_Serialization_MultiPacket(benchmark::State &state) {
  RPL::Serializer<PacketA, PacketB> serializer;
  PacketA packet_a{42, -1234, 3.14f, 2.718};
  PacketB packet_b{1337, 9.876};
  constexpr size_t total_size =
      RPL::Serializer<PacketA, PacketB>::frame_size<PacketA>() +
      RPL::Serializer<PacketA, PacketB>::frame_size<PacketB>();
  std::vector<uint8_t> buffer(total_size);

  for (auto _ : state) {
    auto result =
        serializer.serialize(buffer.data(), buffer.size(), packet_a, packet_b);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_Serialization_MultiPacket);

// --- Deserialization Benchmark ---

static void BM_Deserialization_GetPacket(benchmark::State &state) {
  RPL::Deserializer<PacketA, PacketB> deserializer;
  auto &raw_ref_a = deserializer.getRawRef<PacketA>();
  raw_ref_a = {42, -1234, 3.14f, 2.718};

  for (auto _ : state) {
    auto packet = deserializer.get<PacketA>();
    benchmark::DoNotOptimize(packet);
  }
}
BENCHMARK(BM_Deserialization_GetPacket);

// --- Parser Stress Benchmarks ---

class ParserStressFixture : public benchmark::Fixture {
public:
  StressSerializer serializer;
  StressDeserializer deserializer;
  StressParser parser{deserializer};

  StressSmall small_packet{};
  StressMedium medium_packet{};
  StressLarge large_packet{};
  StressXLarge xlarge_packet{};

  std::vector<uint8_t> small_frame;
  std::vector<uint8_t> medium_frame;
  std::vector<uint8_t> large_frame;
  std::vector<uint8_t> xlarge_frame;
  std::vector<uint8_t> burst_small_stream;
  std::vector<uint8_t> burst_mixed_stream;
  std::vector<uint8_t> noisy_stream;
  std::vector<uint8_t> invalid_crc_stream;
  std::vector<uint8_t> mixed_size_stream;

  static constexpr size_t burst_small_frames = 256;
  static constexpr size_t burst_mixed_frames = 16;
  static constexpr size_t mixed_repeats = 4;

  void SetUp(const ::benchmark::State &) override {
    small_packet = make_packet_pattern<StressSmall>(0x11);
    medium_packet = make_packet_pattern<StressMedium>(0x22);
    large_packet = make_packet_pattern<StressLarge>(0x33);
    xlarge_packet = make_packet_pattern<StressXLarge>(0x44);

    small_frame = serialize_one(serializer, small_packet);
    medium_frame = serialize_one(serializer, medium_packet);
    large_frame = serialize_one(serializer, large_packet);
    xlarge_frame = serialize_one(serializer, xlarge_packet);

    burst_small_stream = build_repeated_stream(small_frame, burst_small_frames);
    burst_mixed_stream =
        build_repeated_stream(small_frame, large_frame, burst_mixed_frames);

    mixed_size_stream.clear();
    for (size_t i = 0; i < mixed_repeats; ++i) {
      mixed_size_stream.insert(mixed_size_stream.end(), small_frame.begin(),
                               small_frame.end());
      mixed_size_stream.insert(mixed_size_stream.end(), medium_frame.begin(),
                               medium_frame.end());
      mixed_size_stream.insert(mixed_size_stream.end(), xlarge_frame.begin(),
                               xlarge_frame.end());
    }

    noisy_stream.assign(256, 0x5A);
    noisy_stream.insert(noisy_stream.end(), burst_mixed_stream.begin(),
                        burst_mixed_stream.end());
    noisy_stream.insert(noisy_stream.end(), 128, 0xC3);
    noisy_stream.insert(noisy_stream.end(), small_frame.begin(), small_frame.end());

    invalid_crc_stream = large_frame;
    if (invalid_crc_stream.size() >= 2) {
      invalid_crc_stream[invalid_crc_stream.size() - 1] ^= 0xFF;
    }
    invalid_crc_stream.insert(invalid_crc_stream.end(), large_frame.begin(),
                              large_frame.end());
  }

  void TearDown(const ::benchmark::State &) override { parser.clear_buffer(); }
};

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FullWrite_Small)
(benchmark::State &state) {
  for (auto _ : state) {
    auto result = parser.push_data(small_frame.data(), small_frame.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(small_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FullWrite_Large)
(benchmark::State &state) {
  for (auto _ : state) {
    auto result = parser.push_data(large_frame.data(), large_frame.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(large_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FullWrite_XLarge)
(benchmark::State &state) {
  for (auto _ : state) {
    auto result = parser.push_data(xlarge_frame.data(), xlarge_frame.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(xlarge_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FragmentedWrite_1B)
(benchmark::State &state) {
  for (auto _ : state) {
    process_fragmented(parser, large_frame, 1);
  }
  set_throughput(state, static_cast<int64_t>(large_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FragmentedWrite_4B)
(benchmark::State &state) {
  for (auto _ : state) {
    process_fragmented(parser, large_frame, 4);
  }
  set_throughput(state, static_cast<int64_t>(large_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FragmentedWrite_16B)
(benchmark::State &state) {
  for (auto _ : state) {
    process_fragmented(parser, large_frame, 16);
  }
  set_throughput(state, static_cast<int64_t>(large_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_SingleFrame_FragmentedWrite_64B)
(benchmark::State &state) {
  for (auto _ : state) {
    process_fragmented(parser, large_frame, 64);
  }
  set_throughput(state, static_cast<int64_t>(large_frame.size()), 1);
}

BENCHMARK_F(ParserStressFixture, Parser_BurstFrames_BackToBack_Small)
(benchmark::State &state) {
  for (auto _ : state) {
    auto result =
        parser.push_data(burst_small_stream.data(), burst_small_stream.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(burst_small_stream.size()),
                 static_cast<int64_t>(burst_small_frames));
}

BENCHMARK_F(ParserStressFixture, Parser_BurstFrames_BackToBack_Mixed)
(benchmark::State &state) {
  for (auto _ : state) {
    auto result =
        parser.push_data(burst_mixed_stream.data(), burst_mixed_stream.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(burst_mixed_stream.size()),
                 static_cast<int64_t>(burst_mixed_frames * 2));
}

BENCHMARK_F(ParserStressFixture, Parser_WithNoise_Resync)(benchmark::State &state) {
  for (auto _ : state) {
    auto result = parser.push_data(noisy_stream.data(), noisy_stream.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(noisy_stream.size()),
                 static_cast<int64_t>(burst_mixed_frames * 2 + 1));
}

BENCHMARK_F(ParserStressFixture, Parser_InvalidCRC_RejectCost)
(benchmark::State &state) {
  for (auto _ : state) {
    auto result =
        parser.push_data(invalid_crc_stream.data(), invalid_crc_stream.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(invalid_crc_stream.size()), 2);
}

BENCHMARK_F(ParserStressFixture, Parser_MixedSize_Stream)(benchmark::State &state) {
  for (auto _ : state) {
    auto result = parser.push_data(mixed_size_stream.data(), mixed_size_stream.size());
    benchmark::DoNotOptimize(result);
  }
  set_throughput(state, static_cast<int64_t>(mixed_size_stream.size()),
                 static_cast<int64_t>(mixed_repeats * 3));
}

BENCHMARK_F(ParserStressFixture, Parser_AdvanceWriteIndex_Path)(benchmark::State &state) {
  for (auto _ : state) {
    process_via_advance_write(parser, burst_mixed_stream, 64);
  }
  set_throughput(state, static_cast<int64_t>(burst_mixed_stream.size()),
                 static_cast<int64_t>(burst_mixed_frames * 2));
}

BENCHMARK_MAIN();
