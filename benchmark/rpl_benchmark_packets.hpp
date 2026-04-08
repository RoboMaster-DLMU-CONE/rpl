#ifndef RPL_BENCHMARK_PACKETS_HPP
#define RPL_BENCHMARK_PACKETS_HPP

#include <RPL/Meta/PacketTraits.hpp>
#include <array>
#include <cstddef>
#include <cstdint>

struct SmallPacket {
  std::array<uint8_t, 16> payload{};
};

struct MediumPacket {
  std::array<uint8_t, 256> payload{};
};

struct LargePacket {
  std::array<uint8_t, 2048> payload{};
};

struct XLargePacket {
  std::array<uint8_t, 8192> payload{};
};

template <>
struct RPL::Meta::PacketTraits<SmallPacket>
    : PacketTraitsBase<PacketTraits<SmallPacket>> {
  static constexpr uint16_t cmd = 0x1101;
  static constexpr size_t size = sizeof(SmallPacket);
};

template <>
struct RPL::Meta::PacketTraits<MediumPacket>
    : PacketTraitsBase<PacketTraits<MediumPacket>> {
  static constexpr uint16_t cmd = 0x1102;
  static constexpr size_t size = sizeof(MediumPacket);
};

template <>
struct RPL::Meta::PacketTraits<LargePacket>
    : PacketTraitsBase<PacketTraits<LargePacket>> {
  static constexpr uint16_t cmd = 0x1103;
  static constexpr size_t size = sizeof(LargePacket);
};

template <>
struct RPL::Meta::PacketTraits<XLargePacket>
    : PacketTraitsBase<PacketTraits<XLargePacket>> {
  static constexpr uint16_t cmd = 0x1104;
  static constexpr size_t size = sizeof(XLargePacket);
};

// --- Bitfield Packets ---

struct RobotStatus {
  uint8_t is_online : 1;
  uint8_t work_mode : 3;
  uint8_t error_code : 4;
  uint16_t voltage;
};

namespace RPL::Meta {
template <>
struct PacketTraits<RobotStatus> : PacketTraitsBase<PacketTraits<RobotStatus>> {
  static constexpr uint16_t cmd = 0x1001;
  static constexpr size_t size = 3;
  using BitLayout =
      std::tuple<Field<uint8_t, 1>, Field<uint8_t, 3>, Field<uint8_t, 4>,
                 Field<uint16_t, 16>>;
};
} // namespace RPL::Meta

struct CrossByteTest {
  uint32_t val1 : 12;
  uint32_t val2 : 12;
  uint8_t val3 : 8;
};

namespace RPL::Meta {
template <>
struct PacketTraits<CrossByteTest>
    : PacketTraitsBase<PacketTraits<CrossByteTest>> {
  static constexpr uint16_t cmd = 0x1002;
  static constexpr size_t size = 4;
  using BitLayout =
      std::tuple<Field<uint32_t, 12>, Field<uint32_t, 12>, Field<uint8_t, 8>>;
};
} // namespace RPL::Meta

#endif // RPL_BENCHMARK_PACKETS_HPP
