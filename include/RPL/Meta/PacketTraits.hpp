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
     * @brief 默认 RoboMaster 协议定义
     * 
     * 定义标准 RoboMaster 裁判系统协议的特征
     */
    struct DefaultProtocol {
        // --- 帧头识别 ---
        static constexpr uint8_t start_byte = 0xA5;      ///< 起始字节
        static constexpr bool has_second_byte = false;   ///< 是否有第二个固定字节
        static constexpr uint8_t second_byte = 0x00;     ///< 第二个固定字节（如果 has_second_byte 为 false 则忽略）

        // --- 结构大小 ---
        static constexpr size_t header_size = 7;         ///< 帧头总长度
        static constexpr size_t tail_size = 2;           ///< 帧尾长度（通常为CRC16）

        // --- 校验 ---
        static constexpr bool has_header_crc = true;     ///< 是否有帧头校验（CRC8）

        // --- 长度获取策略 ---
        /**
         * @brief 是否在头部包含数据长度字段
         * 
         * - true: 解析器从头部 length_offset 处读取长度 (RoboMaster 协议)
         * - false: 解析器直接使用 PacketTraits::size 作为固定长度 (新遥控器协议)
         */
        static constexpr bool has_length_field = true;
        static constexpr size_t length_offset = 1;       ///< 长度字段在帧头的偏移量
        static constexpr size_t length_field_bytes = 2;  ///< 长度字段占用的字节数

        // --- 命令码获取策略 ---
        /**
         * @brief 是否在头部包含命令码字段
         * 
         * - true: 解析器从头部 cmd_offset 处读取命令码，用于分发
         * - false: 解析器使用 PacketTraits::cmd 作为隐式命令码 (单包协议或固定映射)
         */
        static constexpr bool has_cmd_field = true;
        static constexpr size_t cmd_offset = 5;          ///< 命令码在帧头的偏移量
        static constexpr size_t cmd_field_bytes = 2;     ///< 命令码字段占用的字节数
    };

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

        /// 默认使用 RoboMaster 协议，派生类可通过 `using Protocol = MyProtocol;` 覆盖
        using Protocol = DefaultProtocol;

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
