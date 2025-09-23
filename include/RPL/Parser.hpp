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
        // 优化的快速解析路径
        tl::expected<void, Error> try_parse_packets()
        {
            // 获取缓冲区状态
            size_t available_bytes = ringbuffer.available();

            while (available_bytes >= FRAME_HEADER_SIZE)
            {
                const auto buffer_view = ringbuffer.get_contiguous_read_buffer();

                // 如果连续数据不足一个完整的最小帧，切换到慢速路径
                if (buffer_view.size() < FRAME_HEADER_SIZE)
                {
                    return parse_with_copy();
                }

                const uint8_t* data_ptr = buffer_view.data();
                const size_t view_size = buffer_view.size();

                // 快速扫描起始字节 - 内联优化
                const uint8_t* start_ptr = nullptr;
                size_t scan_offset = 0;

                // 使用更高效的搜索策略
                while (scan_offset < view_size)
                {
                    start_ptr = static_cast<const uint8_t*>(
                        std::memchr(data_ptr + scan_offset, FRAME_START_BYTE, view_size - scan_offset));

                    if (!start_ptr)
                    {
                        // 没找到起始字节，丢弃大部分数据但保留最后一个字节
                        const size_t discard_len = view_size > 1 ? view_size - 1 : 0;
                        if (discard_len > 0)
                        {
                            ringbuffer.discard(discard_len);
                            available_bytes -= discard_len;
                        }
                        goto next_iteration;
                    }

                    const size_t start_offset = start_ptr - data_ptr;

                    // 检查是否有足够的数据进行快速验证
                    const size_t remaining_in_view = view_size - start_offset;
                    if (remaining_in_view < FRAME_HEADER_SIZE)
                    {
                        // 丢弃到起始位置，然后切换到慢速路径
                        if (start_offset > 0)
                        {
                            ringbuffer.discard(start_offset);
                            available_bytes -= start_offset;
                        }
                        return parse_with_copy();
                    }

                    // 快速验证帧头 - 内联所有检查以减少函数调用
                    const uint8_t* header_ptr = start_ptr;

                    // 提取帧头信息
                    const uint16_t cmd = *reinterpret_cast<const uint16_t*>(header_ptr + 1);
                    const uint16_t data_length = *reinterpret_cast<const uint16_t*>(header_ptr + 3);
                    const uint8_t sequence_number = header_ptr[5];
                    const uint8_t received_crc8 = header_ptr[6];

                    // 快速CRC8验证
                    const uint8_t calculated_crc8 = CRC8::CRC8::calc(header_ptr, 6);
                    if (calculated_crc8 != received_crc8)
                    {
                        // CRC8失败，移动到下一个可能的位置
                        scan_offset = start_offset + 1;
                        continue;
                    }

                    // 数据长度合理性检查
                    if (data_length > max_frame_size - FRAME_HEADER_SIZE - FRAME_TAIL_SIZE)
                    {
                        scan_offset = start_offset + 1;
                        continue;
                    }

                    const size_t complete_frame_size = FRAME_HEADER_SIZE + data_length + FRAME_TAIL_SIZE;

                    // 检查完整帧是否在连续内存中
                    if (remaining_in_view < complete_frame_size)
                    {
                        // 丢弃到起始位置，切换到慢速路径处理跨界情况
                        if (start_offset > 0)
                        {
                            ringbuffer.discard(start_offset);
                            available_bytes -= start_offset;
                        }
                        return parse_with_copy();
                    }

                    // 快速CRC16验证
                    const size_t crc16_data_len = complete_frame_size - FRAME_TAIL_SIZE;
                    const uint16_t calculated_crc16 = CRC16::CCITT_FALSE::calc(header_ptr, crc16_data_len);
                    const uint16_t received_crc16 = *reinterpret_cast<const uint16_t*>(header_ptr + crc16_data_len);

                    if (calculated_crc16 != received_crc16)
                    {
                        // CRC16失败，继续搜索
                        scan_offset = start_offset + 1;
                        continue;
                    }

                    // 成功解析一个完整的帧
                    // 先丢弃起始位置之前的数据
                    if (start_offset > 0)
                    {
                        ringbuffer.discard(start_offset);
                        available_bytes -= start_offset;
                    }

                    // 写入数据到deserializer
                    uint8_t* write_ptr = deserializer.getWritePtr(cmd);
                    if (write_ptr != nullptr)
                    {
                        std::memcpy(write_ptr, header_ptr + FRAME_HEADER_SIZE, data_length);
                    }

                    // 丢弃已处理的完整帧
                    ringbuffer.discard(complete_frame_size);
                    available_bytes -= complete_frame_size;

                    // 继续处理下一个可能的帧
                    break; // 跳出当前搜索循环，重新获取buffer_view
                }

            next_iteration:
                // 检查是否还有足够的数据继续处理
                if (available_bytes < FRAME_HEADER_SIZE)
                {
                    break;
                }
            }

            return {};
        }

        // 慢速路径：当数据包不连续时，拷贝到临时缓冲区进行处理
        tl::expected<void, Error> parse_with_copy()
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

        static std::optional<std::tuple<uint16_t, uint16_t, uint8_t>> validate_header(const uint8_t* header)
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
