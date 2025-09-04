#ifndef RPL_SAMPLEPACKET_HPP
#define RPL_SAMPLEPACKET_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

struct __attribute__((packed)) SamplePacket
{
    uint8_t a;
    int16_t b;
    float c;
    double d;
};

template <>
struct RPL::Meta::PacketTraits<SamplePacket> : PacketTraitsBase<PacketTraits<SamplePacket>>
{
    static constexpr std::uint16_t cmd = 0x0102;
    static constexpr std::size_t size = sizeof(SamplePacket);
};

#endif //RPL_SAMPLEPACKET_HPP
