#ifndef RPL_RPL_HPP
#define RPL_RPL_HPP

#include <concepts>
#include <cstdint>
#include <optional>
#include <array>
#include <vector>

#include <tl/expected.hpp>
#include <ringbuffer.hpp>
#include <cppcrc.h>

#include "Error.hpp"
#include "Packet.hpp"

namespace rpl
{
    // 新的帧格式常量
    static constexpr uint8_t FRAME_START_BYTE = 0xA5;
    static constexpr size_t FRAME_HEADER_SIZE = 5; // 1字节起始 + 2字节长度 + 1字节序号 + 1字节CRC8
    static constexpr size_t FRAME_TAIL_SIZE = 2; // 2字节CRC16

    template <typename T>
    concept RPLPacket = std::derived_from<T, Packet>;

    template <RPLPacket T>
    class Parser
    {
    public:
        Parser() = default;

        // 序列化数据包到缓冲区
        tl::expected<size_t, Error> serialize(const T& packet, uint8_t* buffer)
        {
            const size_t data_len = packet.data_size();

            size_t offset = 0;

            // 帧头：起始字节(1) + 数据长度(2) + 包序号(1) + 帧头CRC8(1)
            buffer[offset++] = FRAME_START_BYTE;

            // 数据长度（2字节，小端序）
            *reinterpret_cast<uint16_t*>(buffer + offset) = static_cast<uint16_t>(data_len);
            offset += sizeof(uint16_t);

            // 包序号
            buffer[offset++] = packet.get_sequence_number();

            // 计算帧头CRC8（前4字节）
            const uint8_t header_crc8 = CRC8::CRC8::calc(buffer, 4);
            buffer[offset++] = header_crc8;

            // 数据区
            packet.serialize_data(buffer + offset);
            offset += data_len;

            // 帧尾：CRC16校验（覆盖帧头+数据区）
            const uint16_t frame_crc16 = CRC16::CCITT_FALSE::calc(buffer, offset);
            *reinterpret_cast<uint16_t*>(buffer + offset) = frame_crc16;
            offset += sizeof(uint16_t);

            return offset;
        }

        // 反序列化缓冲区数据
        tl::expected<T, Error> deserialize(const uint8_t* buffer, const size_t length)
        {
            for (size_t i = 0; i < length; ++i)
            {
                if (!ringbuffer.insert(buffer[i]))
                {
                    return tl::unexpected(Error{ErrorCode::BufferOverflow, "Ringbuffer overflow"});
                }
            }

            return try_parse_packet();
        }

    private:
        jnk0le::Ringbuffer<uint8_t, 512> ringbuffer;
        static constexpr size_t expected_data_size = T{}.data_size();
        static constexpr size_t expected_frame_size = T{}.frame_size();

        tl::expected<T, Error> try_parse_packet()
        {
            // 检查是否有足够的数据解析帧头
            if (ringbuffer.readAvailable() < FRAME_HEADER_SIZE)
            {
                return tl::unexpected(Error{ErrorCode::InsufficientData, "Not enough data for header"});
            }

            // 查找帧起始字节
            const auto start_pos = find_frame_start();
            if (!start_pos)
            {
                clear_invalid_data();
                return tl::unexpected(Error{ErrorCode::NoFrameHeader, "Frame start byte not found"});
            }

            // 移除起始位置之前的无效数据
            for (size_t i = 0; i < *start_pos; ++i)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
            }

            // 再次检查是否有足够的帧头数据
            if (ringbuffer.readAvailable() < FRAME_HEADER_SIZE)
            {
                return tl::unexpected(Error{ErrorCode::InsufficientData, "Not enough data for complete header"});
            }

            // 解析帧头
            uint8_t start_byte;
            uint16_t data_length;
            uint8_t sequence_number;
            uint8_t header_crc8;

            peek_data(0, &start_byte, 1);
            peek_data(1, reinterpret_cast<uint8_t*>(&data_length), 2);
            peek_data(3, &sequence_number, 1);
            peek_data(4, &header_crc8, 1);

            // 验证起始字节
            if (start_byte != FRAME_START_BYTE)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
                return tl::unexpected(Error{ErrorCode::InvalidFrameHeader, "Invalid frame start byte"});
            }

            // 验证数据长度
            if (data_length != expected_data_size)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
                return tl::unexpected(Error{ErrorCode::InvalidFrameHeader, "Invalid data length"});
            }

            // 验证帧头CRC8
            std::array<uint8_t, 4> header_for_crc{};
            peek_data(0, header_for_crc.data(), 4);
            const uint8_t calculated_header_crc8 = CRC8::CRC8::calc(header_for_crc.data(), 4);
            if (calculated_header_crc8 != header_crc8)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
                return tl::unexpected(Error{ErrorCode::CrcMismatch, "Header CRC8 verification failed"});
            }

            // 检查是否有完整的帧
            const size_t complete_frame_size = FRAME_HEADER_SIZE + data_length + FRAME_TAIL_SIZE;
            if (ringbuffer.readAvailable() < complete_frame_size)
            {
                return tl::unexpected(Error{ErrorCode::InsufficientData, "Incomplete frame"});
            }

            // 提取完整帧进行CRC16验证
            std::vector<uint8_t> frame_buffer(complete_frame_size);
            for (size_t i = 0; i < complete_frame_size; ++i)
            {
                peek_data(i, &frame_buffer[i], 1);
            }

            // 验证帧尾CRC16
            const size_t crc16_data_len = complete_frame_size - FRAME_TAIL_SIZE;
            const uint16_t calculated_crc16 = CRC16::CCITT_FALSE::calc(frame_buffer.data(), crc16_data_len);
            const uint16_t received_crc16 = *reinterpret_cast<uint16_t*>(frame_buffer.data() + crc16_data_len);

            if (calculated_crc16 != received_crc16)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
                return tl::unexpected(Error{ErrorCode::CrcMismatch, "Frame CRC16 verification failed"});
            }

            // CRC验证通过，移除已处理的数据
            for (size_t i = 0; i < complete_frame_size; ++i)
            {
                uint8_t dummy;
                ringbuffer.remove(dummy);
            }

            // 反序列化数据包
            T packet;
            packet.set_sequence_number(sequence_number);
            packet.deserialize_data(frame_buffer.data() + FRAME_HEADER_SIZE);

            return packet;
        }

        std::optional<size_t> find_frame_start()
        {
            const size_t available = ringbuffer.readAvailable();
            for (size_t i = 0; i < available; ++i)
            {
                uint8_t byte;
                peek_data(i, &byte, 1);
                if (byte == FRAME_START_BYTE)
                {
                    return i;
                }
            }
            return std::nullopt;
        }

        void peek_data(const size_t offset, uint8_t* buffer, const size_t len)
        {
            if (const auto readable = ringbuffer.readAvailable(); offset + len > readable) return;
            for (size_t i = 0; i < len; ++i)
            {
                buffer[i] = ringbuffer[offset + i];
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
