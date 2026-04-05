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

#endif // RPL_BENCHMARK_PACKETS_HPP
