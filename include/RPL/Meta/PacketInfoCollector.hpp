#ifndef RPL_PACKETINFOCOLLECTOR_HPP
#define RPL_PACKETINFOCOLLECTOR_HPP
#include "PacketTraits.hpp"
#include <algorithm>

#include <frozen/unordered_map.h>

namespace RPL::Meta
{
    template <typename... Ts>
    struct PacketInfoCollector
    {
        static constexpr std::size_t totalSize = (PacketTraits<Ts>::size + ...) + 0;

        static constexpr auto cmdToIndex = frozen::make_unordered_map<uint16_t, size_t>([](auto& out)
        {
            size_t offset = 0;
            ([&]
            {
                out.emplace_back(PacketTraits<Ts>::cmd, offset);
                offset += PacketTraits<Ts>::size;
            }(), ...);
        });

        template <typename T>
        static constexpr size_t type_index() noexcept
        {
            return get_index(PacketTraits<T>::cmd);
        }

        static constexpr size_t get_index(uint16_t cmd) noexcept
        {
            return cmdToIndex.find(cmd)->second;
        }
    };
}

#endif //RPL_PACKETINFOCOLLECTOR_HPP
