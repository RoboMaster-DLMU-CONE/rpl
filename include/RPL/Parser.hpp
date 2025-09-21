#ifndef RPL_PARSER_HPP
#define RPL_PARSER_HPP

#include <optional>
#include <array>
#include <bit>
#include <algorithm>
#include <cppcrc.h>

#include <tl/expected.hpp>
#include "Containers/RingBuffer.hpp"
#include "Deserializer.hpp"
#include "Utils/Error.hpp"
#include "Utils/Def.hpp"
#include "Meta/PacketTraits.hpp"

namespace RPL
{
    template <typename... Ts>
    class Parser
    {
        static constexpr size_t max_frame_size = std::max({
            (FRAME_HEADER_SIZE + Meta::PacketTraits<Ts>::size + FRAME_TAIL_SIZE)...
        });

        // 编译期计算缓冲区大小
        static consteval size_t calculate_buffer_size()
        {
            constexpr size_t min_size = max_frame_size * 4;

            if constexpr (std::has_single_bit(min_size))
            {
                return min_size;
            }
            else
            {
                return std::bit_ceil(min_size);
            }
        }

        static constexpr size_t buffer_size = calculate_buffer_size();
        Containers::RingBuffer<buffer_size> ringbuffer;
        std::array<uint8_t, max_frame_size> parse_buffer{};

        // 直接引用 Deserializer
        Deserializer<Ts...>& deserializer;

    public:
        explicit Parser(Deserializer<Ts...>& des) : deserializer(des)
        {
        }

        tl::expected<void, Error> push_data(const uint8_t* buffer, const size_t length)
        {
            if (!ringbuffer.write(buffer, length))
            {
                return tl::unexpected(Error{ErrorCode::BufferOverflow, "Ringbuffer overflow"});
            }

            return try_parse_packets();
        }

        // 获取 Deserializer 引用
        Deserializer<Ts...>& get_deserializer() noexcept
        {
            return deserializer;
        }

        // 获取缓冲区统计信息
        size_t available_data() const noexcept
        {
            return ringbuffer.available();
        }

        size_t available_space() const noexcept
        {
            return ringbuffer.space();
        }

        bool is_buffer_full() const noexcept
        {
            return ringbuffer.full();
        }

        void clear_buffer() noexcept
        {
            ringbuffer.clear();
        }

    private:
        tl::expected<void, Error> try_parse_packets()
        {
            while (ringbuffer.available() >= FRAME_HEADER_SIZE)
            {
                // 查找帧起始位置
                const size_t start_pos = ringbuffer.find_byte(FRAME_START_BYTE);
                if (start_pos == SIZE_MAX)
                {
                    const size_t available = ringbuffer.available();
                    if (available > 0)
                    {
                        ringbuffer.discard(available - 1);
                    }
                    break;
                }

                if (start_pos > 0)
                {
                    ringbuffer.discard(start_pos);
                }

                if (ringbuffer.available() < FRAME_HEADER_SIZE)
                {
                    break;
                }

                // 读取和验证帧头
                if (!ringbuffer.peek(parse_buffer.data(), 0, FRAME_HEADER_SIZE))
                {
                    return tl::unexpected(Error{ErrorCode::InternalError, "Failed to peek header"});
                }

                auto header_result = validate_header(parse_buffer.data());
                if (!header_result)
                {
                    ringbuffer.discard(1);
                    continue;
                }

                const auto [cmd, data_length, sequence_number] = *header_result;

                // 检查数据长度是否合理
                if (data_length > max_frame_size - FRAME_HEADER_SIZE - FRAME_TAIL_SIZE)
                {
                    ringbuffer.discard(1);
                    continue;
                }

                // 检查完整帧
                const size_t complete_frame_size = FRAME_HEADER_SIZE + data_length + FRAME_TAIL_SIZE;
                if (ringbuffer.available() < complete_frame_size)
                {
                    break;
                }

                if (!ringbuffer.peek(parse_buffer.data(), 0, complete_frame_size))
                {
                    return tl::unexpected(Error{ErrorCode::InternalError, "Failed to peek frame"});
                }

                // 验证CRC16
                const size_t crc16_data_len = complete_frame_size - FRAME_TAIL_SIZE;
                const uint16_t calculated_crc16 = CRC16::CCITT_FALSE::calc(parse_buffer.data(), crc16_data_len);
                const uint16_t received_crc16 = *reinterpret_cast<const uint16_t*>(
                    parse_buffer.data() + crc16_data_len);

                if (calculated_crc16 != received_crc16)
                {
                    ringbuffer.discard(1);
                    continue;
                }

                ringbuffer.discard(complete_frame_size);

                // 直接使用 Deserializer 的方法写入数据
                uint8_t* write_ptr = deserializer.getWritePtr(cmd);
                if (write_ptr != nullptr) // 如果命令码存在
                {
                    std::memcpy(write_ptr, parse_buffer.data() + FRAME_HEADER_SIZE, data_length);
                }
            }

            return {};
        }

        std::optional<std::tuple<uint16_t, uint16_t, uint8_t>> validate_header(const uint8_t* header)
        {
            if (header[0] != FRAME_START_BYTE)
            {
                return std::nullopt;
            }

            const uint16_t cmd = *reinterpret_cast<const uint16_t*>(header + 1);
            const uint16_t data_length = *reinterpret_cast<const uint16_t*>(header + 3);
            const uint8_t sequence_number = header[5];
            const uint8_t received_crc8 = header[6];

            const uint8_t calculated_crc8 = CRC8::CRC8::calc(header, 6);
            if (calculated_crc8 != received_crc8)
            {
                return std::nullopt;
            }

            return std::make_tuple(cmd, data_length, sequence_number);
        }
    };
}

#endif //RPL_PARSER_HPP
