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

namespace RPL
{
    template <typename T, typename... Ts>
    concept Serializable = (std::is_same_v<std::decay_t<T>, Ts> || ...);

    template <typename... Ts>
    class Serializer
    {
    public:
        // 定义数据包变体类型
        using PacketVariant = std::variant<Ts...>;

        // 序列化数据包到用户提供的缓冲区
        template <typename T>
            requires Serializable<T, Ts...>
        tl::expected<size_t, Error> serialize(const T& packet, uint8_t* buffer, const uint8_t sequence_number = 0) const
        {
            using DecayedT = std::decay_t<T>;
            constexpr uint16_t cmd = Meta::PacketTraits<DecayedT>::cmd;
            constexpr size_t data_size = Meta::PacketTraits<DecayedT>::size;

            size_t offset = 0;

            // 构建帧头：起始字节 + 命令码 + 数据长度 + 序号
            buffer[0] = FRAME_START_BYTE;
            *reinterpret_cast<uint16_t*>(buffer + 1) = cmd;
            *reinterpret_cast<uint16_t*>(buffer + 3) = static_cast<uint16_t>(data_size);
            buffer[5] = sequence_number;

            // 计算帧头CRC8
            const uint8_t header_crc8 = CRC8::CRC8::calc(buffer, 6);
            buffer[6] = header_crc8;
            offset = FRAME_HEADER_SIZE;

            // 数据区：直接复制数据包内容
            std::memcpy(buffer + offset, &packet, data_size);
            offset += data_size;

            // 帧尾CRC16：计算整个帧（除了CRC16本身）的校验和
            const uint16_t frame_crc16 = CRC16::CCITT_FALSE::calc(buffer, offset);
            *reinterpret_cast<uint16_t*>(buffer + offset) = frame_crc16;
            offset += FRAME_TAIL_SIZE;

            return offset; // 返回总帧长度
        }

        // 使用 variant 的类型安全序列化
        tl::expected<size_t, Error> serialize_variant(const PacketVariant& packet_variant, uint8_t* buffer,
                                                      const uint8_t sequence_number = 0) const
        {
            return std::visit([this, buffer, sequence_number](const auto& packet) -> tl::expected<size_t, Error>
            {
                return serialize(packet, buffer, sequence_number);
            }, packet_variant);
        }

        // 通过命令码序列化（编译期分发）- 使用完美转发
        template <uint16_t cmd, typename T>
            requires Serializable<T, Ts...>
        tl::expected<size_t, Error> serialize_by_cmd(T&& packet, uint8_t* buffer,
                                                     const uint8_t sequence_number = 0) const
        {
            static_assert(Meta::PacketTraits<std::decay_t<T>>::cmd == cmd,
                          "Command code mismatch with packet type");
            return serialize(std::forward<T>(packet), buffer, sequence_number);
        }

        // 运行时通过命令码序列化 - 使用 variant
        tl::expected<size_t, Error> serialize_by_cmd_runtime(uint16_t cmd, const PacketVariant& packet_variant,
                                                             uint8_t* buffer, uint8_t sequence_number = 0) const
        {
            return std::visit([this, cmd, buffer, sequence_number](const auto& packet) -> tl::expected<size_t, Error>
            {
                using PacketType = std::decay_t<decltype(packet)>;
                if (Meta::PacketTraits<PacketType>::cmd == cmd)
                {
                    return serialize(packet, buffer, sequence_number);
                }
                return tl::unexpected(Error{ErrorCode::InvalidCommand, "Command mismatch with packet type"});
            }, packet_variant);
        }

        // 从原始数据创建 PacketVariant 的工厂方法
        template <typename T>
            requires Serializable<T, Ts...>
        static PacketVariant create_packet_variant(T&& packet)
        {
            return PacketVariant{std::forward<T>(packet)};
        }

        // 通过命令码从原始数据创建对应的 PacketVariant
        template <uint16_t cmd>
        static auto create_packet_variant_by_cmd()
        {
            return create_packet_by_cmd_impl<cmd, Ts...>();
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
    };
}

#endif //RPL_SERIALIZER_HPP
