#ifndef RPL_SAMPLEPACKET_HPP
#define RPL_SAMPLEPACKET_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#pragma pack(push, 1)

struct SampleA
{
    uint8_t a;
    int16_t b;
    float c;
    double d;
};

namespace RPL::Meta {
template <>
struct PacketTraits<SampleA> : PacketTraitsBase<PacketTraits<SampleA>>
{
    static constexpr uint16_t cmd = 0x0102;
    static constexpr size_t size = sizeof(SampleA);
};
} // namespace RPL::Meta

#pragma pack(pop)
#endif //RPL_SAMPLEPACKET_HPP
