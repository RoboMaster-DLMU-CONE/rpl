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
    template <typename T, typename... Ts>
    concept Serializable = (std::is_same_v<std::decay_t<T>, Ts> || ...);

    template <typename... Ts>
    class Serializer
    {
    public:
        // 序列化数据包到用户提供的缓冲区
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
                *reinterpret_cast<uint16_t*>(current_buffer + 1) = cmd;
                *reinterpret_cast<uint16_t*>(current_buffer + 3) = static_cast<uint16_t>(data_size);
                current_buffer[5] = m_Sequence;

                const uint8_t header_crc8 = CRC8::CRC8::calc(current_buffer, 6);
                current_buffer[6] = header_crc8;
                std::memcpy(current_buffer + FRAME_HEADER_SIZE, &packet, data_size);

                const uint16_t frame_crc16 = CRC16::CCITT_FALSE::calc(current_buffer, FRAME_HEADER_SIZE + data_size);
                *reinterpret_cast<uint16_t*>(current_buffer + FRAME_HEADER_SIZE + data_size) = frame_crc16;

                offset += current_frame_size;
            };
            (serialize_one(packets), ...);

            m_Sequence += 1;
            return offset;
        }

        // 计算指定类型的完整帧大小
        template <typename T>
            requires Serializable<T, Ts...>
        static constexpr size_t frame_size() noexcept
        {
            using DecayedT = std::decay_t<T>;
            return FRAME_HEADER_SIZE + Meta::PacketTraits<DecayedT>::size + FRAME_TAIL_SIZE;
        }

        // 计算指定命令码的完整帧大小
        static constexpr size_t frame_size_by_cmd(uint16_t cmd) noexcept
        {
            size_t result = 0;
            ((Meta::PacketTraits<Ts>::cmd == cmd
                  ? (result = FRAME_HEADER_SIZE + Meta::PacketTraits<Ts>::size + FRAME_TAIL_SIZE)
                  : 0), ...);
            return result;
        }

        // 获取最大帧大小
        static constexpr size_t max_frame_size() noexcept
        {
            return std::max({frame_size<Ts>()...});
        }

        // 检查命令码是否有效
        static constexpr bool is_valid_cmd(uint16_t cmd) noexcept
        {
            return ((Meta::PacketTraits<Ts>::cmd == cmd) || ...);
        }

        // 通过命令码获取对应的类型索引（用于调试）
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

        uint8_t m_Sequence{};

    public:
        [[nodiscard]] uint8_t get_sequence() const
        {
            return m_Sequence;
        }
    };
}

#endif //RPL_SERIALIZER_HPP
