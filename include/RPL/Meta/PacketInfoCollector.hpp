#ifndef RPL_PACKETINFOCOLLECTOR_HPP
#define RPL_PACKETINFOCOLLECTOR_HPP
#include "PacketTraits.hpp"
#include <algorithm>
#include <array>
#include <utility>

#include <frozen/unordered_map.h>

namespace RPL::Meta
{
    template <typename... Ts>
    struct PacketInfoCollector
    {
        static constexpr std::size_t totalSize = (PacketTraits<Ts>::size + ...) + 0;

        // 修正 frozen::make_unordered_map 的用法
        static constexpr auto cmdToIndex = []()
        {
            std::array<std::pair<uint16_t, size_t>, sizeof...(Ts)> pairs{};
            size_t index = 0;
            size_t offset = 0;

            // 使用折叠表达式填充数组
            ((pairs[index] = std::make_pair(PacketTraits<Ts>::cmd, offset),
                offset += PacketTraits<Ts>::size,
                ++index), ...);

            return frozen::make_unordered_map(pairs);
        }();

        template <typename T>
        static constexpr size_t type_index() noexcept
        {
            return cmd_index(PacketTraits<T>::cmd);
        }

        static constexpr size_t cmd_index(uint16_t cmd) noexcept
        {
            auto it = cmdToIndex.find(cmd);
            return it != cmdToIndex.end() ? it->second : static_cast<size_t>(-1);
        }
    };
}

#endif //RPL_PACKETINFOCOLLECTOR_HPP
