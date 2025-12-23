/**
 * @file Serializer.hpp
 * @brief RPL库的序列化器实现
 *
 * 此文件包含Serializer类的定义，该类用于将数据包结构序列化为字节数组。
 * 序列化过程包括添加帧头、计算CRC校验和等。
 *
 * @author WindWeaver
 */

#ifndef RPL_SERIALIZER_HPP
#define RPL_SERIALIZER_HPP

#include "Meta/PacketTraits.hpp"
#include "Utils/Error.hpp"
#include "Utils/Def.hpp"
#include <tl/expected.hpp>
#include <cppcrc.h>
#include <cstring>
#include <cstdint>
#include <variant>
#include <type_traits>
#include <optional>
#include <algorithm>

namespace RPL
{
    /**
     * @brief 可序列化概念
     *
     * 用于检查类型T是否为可序列化类型之一
     *
     * @tparam T 要检查的类型
     * @tparam Ts 可序列化类型列表
     */
    template <typename T, typename... Ts>
    concept Serializable = (std::is_same_v<std::decay_t<T>, Ts> || ...);

    /**
     * @brief 序列化器类
     *
     * 用于将数据包结构序列化为字节数组，包含帧头、命令码、数据长度、序列号和CRC校验
     *
     * @tparam Ts 可序列化的数据包类型列表
     */
    template <typename... Ts>
    class Serializer
    {
    public:
        /**
         * @brief 将数据包序列化到用户提供的缓冲区
         *
         * 将多个数据包序列化为字节数组，每个数据包包含帧头、数据和帧尾
         *
         * @tparam Packets 要序列化的数据包类型列表
         * @param buffer 用户提供的输出缓冲区
         * @param size 缓冲区大小
         * @param packets 要序列化的数据包
         * @return 序列化成功时返回写入的字节数，失败时返回错误信息
         */
        template <typename... Packets>
            requires (Serializable<Packets, Ts...> && ...)
        tl::expected<size_t, Error> serialize(uint8_t* buffer, const size_t size,
                                              const Packets&... packets)
        {
            size_t offset = 0;

            static constexpr size_t total_size = (frame_size<Packets>() + ...);
            if (size < total_size)
            {
                return tl::make_unexpected(Error{ErrorCode::BufferOverflow, "Expecting a larger size buffer"});
            }

            auto serialize_one = [&]<typename T>(const T& packet)
            {
                using DecayedT = std::decay_t<T>;
                constexpr uint16_t cmd = Meta::PacketTraits<DecayedT>::cmd;
                constexpr size_t data_size = Meta::PacketTraits<DecayedT>::size;
                constexpr size_t current_frame_size = frame_size<DecayedT>();

                uint8_t* current_buffer = buffer + offset;
                current_buffer[0] = FRAME_START_BYTE;

                // cmd
                current_buffer[1] = static_cast<uint8_t>(cmd & 0xFF);
                current_buffer[2] = static_cast<uint8_t>((cmd >> 8) & 0xFF);

                // data size
                const auto data_size_u16 = static_cast<uint16_t>(data_size);
                current_buffer[3] = static_cast<uint8_t>(data_size_u16 & 0xFF);
                current_buffer[4] = static_cast<uint8_t>((data_size_u16 >> 8) & 0xFF);

                // seq
                current_buffer[5] = m_Sequence;

                const uint8_t header_crc8 = CRC8::CRC8::calc(current_buffer, 6);
                current_buffer[6] = header_crc8;
                std::memcpy(current_buffer + FRAME_HEADER_SIZE, &packet, data_size);

                const uint16_t frame_crc16 = CRC16::CCITT_FALSE::calc(current_buffer, FRAME_HEADER_SIZE + data_size);
                current_buffer[FRAME_HEADER_SIZE + data_size] = static_cast<uint8_t>(frame_crc16 & 0xFF);
                current_buffer[FRAME_HEADER_SIZE + data_size + 1] = static_cast<uint8_t>((frame_crc16 >> 8) & 0xFF);

                offset += current_frame_size;
            };
            (serialize_one(packets), ...);

            m_Sequence += 1;
            return offset;
        }

        /**
         * @brief 计算指定类型的完整帧大小
         *
         * 计算包含帧头、数据和帧尾的完整帧大小
         *
         * @tparam T 数据包类型
         * @return 完整帧大小（字节）
         */
        template <typename T>
            requires Serializable<T, Ts...>
        static constexpr size_t frame_size() noexcept
        {
            using DecayedT = std::decay_t<T>;
            return FRAME_HEADER_SIZE + Meta::PacketTraits<DecayedT>::size + FRAME_TAIL_SIZE;
        }

        /**
         * @brief 计算指定命令码的完整帧大小
         *
         * 根据命令码计算对应的完整帧大小
         *
         * @param cmd 命令码
         * @return 对应的完整帧大小（字节），如果命令码无效则返回0
         */
        static constexpr size_t frame_size_by_cmd(uint16_t cmd) noexcept
        {
            size_t result = 0;
            ((Meta::PacketTraits<Ts>::cmd == cmd
                  ? (result = FRAME_HEADER_SIZE + Meta::PacketTraits<Ts>::size + FRAME_TAIL_SIZE)
                  : 0), ...);
            return result;
        }

        /**
         * @brief 获取最大帧大小
         *
         * 获取所有可序列化类型中的最大帧大小
         *
         * @return 最大帧大小（字节）
         */
        static constexpr size_t max_frame_size() noexcept
        {
            return std::max({frame_size<Ts>()...});
        }

        /**
         * @brief 检查命令码是否有效
         *
         * 检查给定的命令码是否对应于任何可序列化类型
         *
         * @param cmd 要检查的命令码
         * @return 如果命令码有效返回true，否则返回false
         */
        static constexpr bool is_valid_cmd(uint16_t cmd) noexcept
        {
            return ((Meta::PacketTraits<Ts>::cmd == cmd) || ...);
        }

        /**
         * @brief 通过命令码获取对应的类型索引
         *
         * 获取与指定命令码关联的类型在模板参数列表中的索引（用于调试）
         *
         * @param cmd 命令码
         * @return 类型索引，如果命令码无效则返回SIZE_MAX
         */
        static constexpr size_t get_type_index_by_cmd(uint16_t cmd) noexcept
        {
            size_t index = 0;
            bool found = false;
            (([&]()
            {
                if (Meta::PacketTraits<Ts>::cmd == cmd)
                {
                    found = true;
                }
                else if (!found)
                {
                    ++index;
                }
            }()), ...);
            return found ? index : SIZE_MAX;
        }

    private:
        // 编译期命令码到类型映射的辅助函数
        template <uint16_t cmd, typename T, typename... Rest>
        static constexpr auto create_packet_by_cmd_impl()
        {
            if constexpr (Meta::PacketTraits<T>::cmd == cmd)
            {
                return std::optional<T>{};
            }
            else
            {
                if constexpr (sizeof...(Rest) > 0)
                {
                    return create_packet_by_cmd_impl<cmd, Rest...>();
                }
                else
                {
                    return std::nullopt;
                }
            }
        }

        uint8_t m_Sequence{};  ///< 序列号，每次序列化后递增

    public:
        /**
         * @brief 获取当前序列号
         *
         * 获取当前的序列号值
         *
         * @return 当前序列号
         */
        [[nodiscard]] uint8_t get_sequence() const
        {
            return m_Sequence;
        }
    };
}

#endif //RPL_SERIALIZER_HPP
