/**
 * @file Parser.hpp
 * @brief RPL库的解析器实现
 *
 * 此文件包含Parser类的定义，该类用于解析流式数据包。
 * 支持分片接收、噪声容错和并发多包处理。
 *
 * @author WindWeaver
 */

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
    /**
     * @brief 解析器类
     *
     * 用于解析流式数据包，支持分片接收、噪声容错和并发多包处理。
     * 使用环形缓冲区来处理流式数据，并通过CRC校验确保数据完整性。
     *
     * @tparam Ts 可解析的数据包类型列表
     */
    template <typename... Ts>
    class Parser
    {
        static constexpr size_t max_frame_size = std::max({
            (FRAME_HEADER_SIZE + Meta::PacketTraits<Ts>::size + FRAME_TAIL_SIZE)...
        });

        /**
         * @brief 编译期计算缓冲区大小
         *
         * 计算最优的环形缓冲区大小，为4倍最大帧大小且为2的幂
         *
         * @return 计算出的缓冲区大小
         */
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

        static constexpr size_t buffer_size = calculate_buffer_size();  ///< 环形缓冲区大小
        Containers::RingBuffer<buffer_size> ringbuffer;                ///< 环形缓冲区，用于存储接收的数据
        std::array<uint8_t, max_frame_size> parse_buffer{};            ///< 临时解析缓冲区

        // 直接引用 Deserializer
        Deserializer<Ts...>& deserializer;

    public:
        /**
         * @brief 构造函数
         *
         * 使用反序列化器引用构造解析器
         *
         * @param des 反序列化器引用，用于存储解析后的数据
         */
        explicit Parser(Deserializer<Ts...>& des) : deserializer(des)
        {
        }

        /**
         * @brief 推送数据到解析器
         *
         * 将接收到的字节数据推送到解析器，并尝试解析数据包
         *
         * @param buffer 包含数据的缓冲区
         * @param length 数据长度
         * @return 成功时返回void，失败时返回错误信息
         */
        tl::expected<void, Error> push_data(const uint8_t* buffer, const size_t length)
        {
            if (!ringbuffer.write(buffer, length))
            {
                return tl::unexpected(Error{ErrorCode::BufferOverflow, "Ringbuffer overflow"});
            }

            return try_parse_packets();
        }

        /**
         * @brief 获取反序列化器引用
         *
         * 获取与解析器关联的反序列化器引用
         *
         * @return 反序列化器引用
         */
        Deserializer<Ts...>& get_deserializer() noexcept
        {
            return deserializer;
        }

        /**
         * @brief 获取可用数据量
         *
         * 获取环形缓冲区中当前可用的数据量
         *
         * @return 可用数据量（字节）
         */
        size_t available_data() const noexcept
        {
            return ringbuffer.available();
        }

        /**
         * @brief 获取可用空间
         *
         * 获取环形缓冲区中当前可用的空间
         *
         * @return 可用空间（字节）
         */
        size_t available_space() const noexcept
        {
            return ringbuffer.space();
        }

        /**
         * @brief 检查缓冲区是否已满
         *
         * 检查环形缓冲区是否已满
         *
         * @return 如果缓冲区已满返回true，否则返回false
         */
        bool is_buffer_full() const noexcept
        {
            return ringbuffer.full();
        }

        /**
         * @brief 清空缓冲区
         *
         * 清空环形缓冲区中的所有数据
         */
        void clear_buffer() noexcept
        {
            ringbuffer.clear();
        }

        /**
         * @brief 尝试解析数据包
         *
         * 从环形缓冲区中尝试解析数据包，使用快速路径和慢速路径两种策略
         *
         * @return 成功时返回void，失败时返回错误信息
         */
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
                    uint16_t cmd;
                    uint16_t data_length;
#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386__) || defined(__x86_64__)
                    cmd = *reinterpret_cast<const uint16_t*>(header_ptr + 1);
                    data_length = *reinterpret_cast<const uint16_t*>(header_ptr + 3);
#else
                    std::memcpy(&cmd, header_ptr + 1, sizeof(uint16_t));
                    std::memcpy(&data_length, header_ptr + 3, sizeof(uint16_t));
#endif
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
                    
                    uint16_t received_crc16;
#if defined(__ARM_FEATURE_UNALIGNED) || defined(__i386__) || defined(__x86_64__)
                    received_crc16 = *reinterpret_cast<const uint16_t*>(header_ptr + crc16_data_len);
#else
                    std::memcpy(&received_crc16, header_ptr + crc16_data_len, sizeof(uint16_t));
#endif

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

    private:
        /**
         * @brief 使用拷贝的慢速解析路径
         *
         * 当数据包跨越环形缓冲区边界时，使用此方法进行解析
         *
         * @return 成功时返回void，失败时返回错误信息
         */
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
                // 使用 peek 读取帧头到临时缓冲区
                std::array<uint8_t, FRAME_HEADER_SIZE> header_buffer;
                if (!ringbuffer.peek(header_buffer.data(), 0, FRAME_HEADER_SIZE))
                {
                    return tl::unexpected(Error{ErrorCode::InternalError, "Failed to peek header"});
                }

                auto header_result = validate_header(header_buffer.data());
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

                // 验证CRC16 - 使用分段计算避免全帧拷贝
                const size_t crc16_data_len = complete_frame_size - FRAME_TAIL_SIZE;
                
                // 获取第一段数据（可能包含整个帧，也可能只是一部分）
                auto first_span = ringbuffer.get_contiguous_read_buffer();
                uint16_t calculated_crc16;
                
                if (first_span.size() >= crc16_data_len)
                {
                    // 数据连续，直接计算
                    calculated_crc16 = CRC16::CCITT_FALSE::calc(first_span.data(), crc16_data_len);
                }
                else
                {
                    // 数据分段，分两次计算
                    // 1. 计算第一段
                    uint16_t crc_part1 = CRC16::CCITT_FALSE::calc(first_span.data(), first_span.size());
                    
                    // 2. 计算第二段
                    // 注意：CCITT_FALSE 的 refl_out=false, x_or_out=0，所以中间结果可以直接作为下一段的初始值
                    const size_t second_part_len = crc16_data_len - first_span.size();
                    
                    // 我们需要读取第二段数据。由于 RingBuffer 保证数据只分两段，第二段一定从 buffer[0] 开始
                    // 但为了不破坏封装，我们使用 peek 读取第二段数据到 parse_buffer
                    // 注意：这里只 peek 第二段，而不是整个帧
                    if (!ringbuffer.peek(parse_buffer.data(), first_span.size(), second_part_len))
                    {
                         return tl::unexpected(Error{ErrorCode::InternalError, "Failed to peek second part"});
                    }
                    calculated_crc16 = CRC16::CCITT_FALSE::calc(parse_buffer.data(), second_part_len, crc_part1);
                }

                // 读取接收到的 CRC16
                uint16_t received_crc16;
                if (!ringbuffer.peek(reinterpret_cast<uint8_t*>(&received_crc16), crc16_data_len, sizeof(uint16_t)))
                {
                    return tl::unexpected(Error{ErrorCode::InternalError, "Failed to peek CRC16"});
                }

                if (calculated_crc16 != received_crc16)
                {
                    ringbuffer.discard(1);
                    continue;
                }

                // 校验通过，开始提取数据
                ringbuffer.discard(FRAME_HEADER_SIZE); // 丢弃帧头

                uint8_t* write_ptr = deserializer.getWritePtr(cmd);
                if (write_ptr != nullptr)
                {
                    // 读取数据段
                    if (!ringbuffer.read(write_ptr, data_length))
                    {
                        return tl::unexpected(Error{ErrorCode::InternalError, "Failed to read data"});
                    }
                }
                else
                {
                    // 未知命令或无法处理，直接丢弃数据段
                    ringbuffer.discard(data_length);
                }

                // 丢弃帧尾
                ringbuffer.discard(FRAME_TAIL_SIZE);
            }

            return {};
        }

        /**
         * @brief 验证帧头
         *
         * 验证给定缓冲区中的帧头是否有效
         *
         * @param header 指向帧头的指针
         * @return 如果帧头有效返回包含命令码、数据长度和序列号的元组，否则返回nullopt
         */
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
