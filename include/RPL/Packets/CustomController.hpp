#ifndef RPL_CUSTOMCONTROLLER_HPP
#define RPL_CUSTOMCONTROLLER_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

#include "RPL/Meta/PacketInfoCollector.hpp"

struct __attribute__((packed)) CustomController
{
    uint8_t data[30];
};

template<>
struct RPL::Meta::PacketTraits<CustomController> : PacketTraitsBase<PacketTraits<CustomController>>
{
    static constexpr uint16_t cmd = 0x0302;
    static constexpr size_t size = sizeof(CustomController);
};

#endif //RPL_CUSTOMCONTROLLER_HPP