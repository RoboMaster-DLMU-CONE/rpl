/**
 * @file PacketTraits.hpp
 * @brief RPL库的数据包特性定义
 *
 * 此文件定义了数据包特性的基类和模板特化结构。
 * 用于描述数据包的命令码、大小等元信息。
 *
 * @author WindWeaver
 */

#ifndef RPL_INFO_HPP
#define RPL_INFO_HPP

#include <cstddef>
#include <cstdint>

namespace RPL::Meta
{
    /**
     * @brief 数据包特性基类
     *
     * 提供数据包特性的基础实现，包括命令码、大小和获取前的处理
     *
     * @tparam Derived 派生类类型
     */
    template <typename Derived>
    struct PacketTraitsBase
    {
        static constexpr uint16_t cmd = Derived::cmd;  ///< 命令码
        static constexpr size_t size = Derived::size;  ///< 数据包大小

        /**
         * @brief 获取数据包前的处理
         *
         * 在获取数据包之前执行的处理函数，如果派生类定义了before_get_custom则调用它
         *
         * @param data 指向数据的指针
         */
        static void before_get(uint8_t* data)
        {
            // 使用不同的方法名避免递归
            if constexpr (requires { Derived::before_get_custom(data); })
            {
                Derived::before_get_custom(data);
            }
            // 如果没有定义 before_get_custom，则什么都不做
        }
    };

    /**
     * @brief 数据包特性模板
     *
     * 用于特化各种数据包类型的特性，包括命令码和大小
     *
     * @tparam T 数据包类型
     */
    template <typename T>
    struct PacketTraits;
}

#endif //RPL_INFO_HPP
