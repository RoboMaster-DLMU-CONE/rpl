#ifndef RPL_RPL_HPP
#define RPL_RPL_HPP

#include <concepts>
#include <optional>
#include <array>
#include <bit>

#include <tl/expected.hpp>
#include "Containers/RingBuffer.hpp"
#include "Meta/PacketInfoCollector.hpp"
#include "Meta/PacketTraits.hpp"
#include "Deserializer.hpp"
#include <ringbuffer.hpp>
#include <cppcrc.h>

#include "Utils/Error.hpp"
#include "Packet.hpp"

namespace rpl
{
    static constexpr uint8_t FRAME_START_BYTE = 0xA5;
    static constexpr size_t FRAME_HEADER_SIZE = 5;
    static constexpr size_t FRAME_TAIL_SIZE = 2;

    template <typename T>
    concept RPLPacket = std::derived_from<T, Packet>;

    template <RPLPacket T>
    class Parser
    {
    public:
        Parser() = default;

        tl::expected<size_t, error> serialize(const T& packet, uint8_t* buffer)
        {
            const size_t data_len = packet.data_size();
            size_t offset = 0;

            // 帧头构建（优化：减少指针运算）
            buffer[0] = FRAME_START_BYTE;
            *reinterpret_cast<uint16_t*>(buffer + 1) = static_cast<uint16_t>(data_len);
            buffer[3] = packet.get_sequence_number();

            // 计算帧头CRC8
            const uint8_t header_crc8 = CRC8::CRC8::calc(buffer, 4);
            buffer[4] = header_crc8;
            offset = 5;

            // 数据区
            packet.serialize_data(buffer + offset);
            offset += data_len;

            // 帧尾CRC16
            const uint16_t frame_crc16 = CRC16::CCITT_FALSE::calc(buffer, offset);
            *reinterpret_cast<uint16_t*>(buffer + offset) = frame_crc16;
            offset += 2;

            return offset;
        }

        tl::expected<T, error> deserialize(const uint8_t* buffer, const size_t length)
        {
            // 批量插入以减少函数调用开销
            if (!ringbuffer.insertMult(buffer, length))
            {
                return tl::unexpected(error{ErrorCode::BufferOverflow, "Ringbuffer overflow"});
            }

            return try_parse_packet();
        }

    private:
        static consteval size_t calculate_buffer_size()
        {
            constexpr size_t frame_size = T{}.frame_size();
            constexpr size_t min_size = frame_size * 2;

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
        static constexpr size_t expected_data_size = T{}.data_size();
        static constexpr size_t expected_frame_size = T{}.frame_size();

        jnk0le::Ringbuffer<uint8_t, buffer_size> ringbuffer;

        // 优化：预分配解析缓冲区，避免动态分配
        std::array<uint8_t, expected_frame_size> parse_buffer{};

        tl::expected<T, error> try_parse_packet()
        {
            while (ringbuffer.readAvailable() >= FRAME_HEADER_SIZE)
            {
                // 快速查找帧起始位置
                const auto start_pos = find_frame_start_optimized();
                if (!start_pos)
                {
                    clear_invalid_data();
                    return tl::unexpected(error{ErrorCode::NoFrameHeader, "Frame start byte not found"});
                }

                // 移除无效数据
                discard_bytes(*start_pos);

                // 检查是否有完整帧头
                if (ringbuffer.readAvailable() < FRAME_HEADER_SIZE)
                {
                    return tl::unexpected(error{ErrorCode::InsufficientData, "Incomplete header"});
                }

                // 优化：一次性读取帧头
                if (!peek_bytes(0, parse_buffer.data(), FRAME_HEADER_SIZE))
                {
                    return tl::unexpected(error{ErrorCode::InternalError, "Failed to read header"});
                }

                // 快速验证帧头
                auto header_result = validate_header(parse_buffer.data());
                if (!header_result)
                {
                    discard_bytes(1); // 丢弃一个字节继续搜索
                    continue;
                }

                const auto [data_length, sequence_number] = *header_result;

                // 检查完整帧可用性
                const size_t complete_frame_size = FRAME_HEADER_SIZE + data_length + FRAME_TAIL_SIZE;
                if (ringbuffer.readAvailable() < complete_frame_size)
                {
                    return tl::unexpected(error{ErrorCode::InsufficientData, "Incomplete frame"});
                }

                // 优化：一次性读取完整帧
                if (!peek_bytes(0, parse_buffer.data(), complete_frame_size))
                {
                    return tl::unexpected(error{ErrorCode::InternalError, "Failed to read frame"});
                }

                // 验证帧尾CRC16
                const size_t crc16_data_len = complete_frame_size - FRAME_TAIL_SIZE;
                const uint16_t calculated_crc16 = CRC16::CCITT_FALSE::calc(parse_buffer.data(), crc16_data_len);
                const uint16_t received_crc16 = *reinterpret_cast<const uint16_t*>(parse_buffer.data() +
                    crc16_data_len);

                if (calculated_crc16 != received_crc16)
                {
                    discard_bytes(1);
                    continue;
                }

                // 成功解析，移除已处理数据
                discard_bytes(complete_frame_size);

                // 反序列化数据包
                T packet;
                packet.set_sequence_number(sequence_number);
                packet.deserialize_data(parse_buffer.data() + FRAME_HEADER_SIZE);

                return packet;
            }

            return tl::unexpected(error{ErrorCode::InsufficientData, "Need more data"});
        }

        // 优化的帧起始查找
        std::optional<size_t> find_frame_start_optimized()
        {
            const size_t available = ringbuffer.readAvailable();

            // 优化：批量读取数据进行搜索
            constexpr size_t search_chunk_size = 64;
            std::array<uint8_t, search_chunk_size> search_buffer{};
            for (size_t offset = 0; offset < available; offset += search_chunk_size)
            {
                const size_t chunk_size = std::min(search_chunk_size, available - offset);
                if (!peek_bytes(offset, search_buffer.data(), chunk_size))
                {
                    break;
                }

                // 在chunk中搜索起始字节
                for (size_t i = 0; i < chunk_size; ++i)
                {
                    if (search_buffer[i] == FRAME_START_BYTE)
                    {
                        return offset + i;
                    }
                }
            }

            return std::nullopt;
        }

        // 优化的数据读取函数
        bool peek_bytes(const size_t offset, uint8_t* buffer, const size_t length)
        {
            if (ringbuffer.readAvailable() < offset + length)
            {
                return false;
            }

            // 优化：减少循环开销
            for (size_t i = 0; i < length; ++i)
            {
                buffer[i] = ringbuffer[offset + i];
            }
            return true;
        }

        // 优化的帧头验证
        std::optional<std::pair<uint16_t, uint8_t>> validate_header(const uint8_t* header)
        {
            // 验证起始字节
            if (header[0] != FRAME_START_BYTE)
            {
                return std::nullopt;
            }

            // 提取数据长度和序号
            const uint16_t data_length = *reinterpret_cast<const uint16_t*>(header + 1);
            const uint8_t sequence_number = header[3];
            const uint8_t received_crc8 = header[4];

            // 验证数据长度
            if (data_length != expected_data_size)
            {
                return std::nullopt;
            }

            // 验证帧头CRC8
            const uint8_t calculated_crc8 = CRC8::CRC8::calc(header, 4);
            if (calculated_crc8 != received_crc8)
            {
                return std::nullopt;
            }

            return std::make_pair(data_length, sequence_number);
        }

        // 优化的数据丢弃
        void discard_bytes(const size_t count)
        {
            for (size_t i = 0; i < count && ringbuffer.readAvailable() > 0; ++i)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
            }
        }

        void clear_invalid_data()
        {
            while (ringbuffer.readAvailable() > 1)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
            }
        }
    };
}

#endif //RPL_RPL_HPP
