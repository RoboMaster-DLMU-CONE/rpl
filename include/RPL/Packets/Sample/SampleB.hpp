#ifndef RPL_SAMPLEB_HPP
#define RPL_SAMPLEB_HPP

#include <RPL/Meta/PacketTraits.hpp>

struct SampleB
{
    int x;
    double y;
};

template <>
struct RPL::Meta::PacketTraits<SampleB> : PacketTraitsBase<PacketTraits<SampleB>>
{
    static constexpr uint16_t cmd = 0x0103;
    static constexpr size_t size = sizeof(SampleB);
};

#endif //RPL_SAMPLEB_HPP

