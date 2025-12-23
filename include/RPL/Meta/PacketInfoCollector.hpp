/**
 * @file PacketInfoCollector.hpp
 * @brief RPL库的数据包信息收集器
 *
 * 此文件定义了数据包信息收集器，用于管理数据包的元信息，
 * 包括总大小、命令码到索引的映射等。
 *
 * @author WindWeaver
 */

#ifndef RPL_PACKETINFOCOLLECTOR_HPP
#define RPL_PACKETINFOCOLLECTOR_HPP
#include "PacketTraits.hpp"
#include <algorithm>
#include <array>
#include <utility>

#include <frozen/unordered_map.h>

namespace RPL::Meta
{
    /**
     * @brief 数据包信息收集器
     *
     * 用于收集和管理数据包的元信息，包括总大小、命令码到索引的映射等
     *
     * @tparam Ts 数据包类型列表
     */
    template <typename... Ts>
    struct PacketInfoCollector
    {
        static constexpr std::size_t totalSize = (PacketTraits<Ts>::size + ...) + 0;  ///< 所有数据包类型的总大小

        /**
         * @brief 命令码到索引的映射
         *
         * 静态常量映射，将命令码映射到在内存池中的偏移量
         */
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

        /**
         * @brief 获取指定类型的索引
         *
         * 获取指定数据包类型在内存池中的索引（偏移量）
         *
         * @tparam T 数据包类型
         * @return 该类型的索引（偏移量）
         */
        template <typename T>
        static constexpr size_t type_index() noexcept
        {
            return cmd_index(PacketTraits<T>::cmd);
        }

        /**
         * @brief 根据命令码获取索引
         *
         * 根据命令码获取对应数据包类型在内存池中的索引（偏移量）
         *
         * @param cmd 命令码
         * @return 对应的索引（偏移量），如果命令码不存在则返回-1
         */
        static constexpr size_t cmd_index(uint16_t cmd) noexcept
        {
            auto it = cmdToIndex.find(cmd);
            return it != cmdToIndex.end() ? it->second : static_cast<size_t>(-1);
        }
    };
}

#endif //RPL_PACKETINFOCOLLECTOR_HPP
