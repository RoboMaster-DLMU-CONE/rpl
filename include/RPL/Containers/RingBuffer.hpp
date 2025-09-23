#ifndef RPL_RINGBUFFER_HPP
#define RPL_RINGBUFFER_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace RPL::Containers
{
    template <size_t SIZE>
    class RingBuffer
    {
        static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be a power of 2");
        static constexpr size_t MASK = SIZE - 1;

        alignas(64) uint8_t buffer[SIZE]{};
        size_t write_index{0};
        size_t read_index{0};

    public:
        // 写入数据
        bool write(const uint8_t* data, size_t length)
        {
            if (length > space()) return false;

            const size_t current_write = write_index;
            const size_t end_write = (current_write + length) & MASK;

            if (end_write >= current_write)
            {
                // 数据不会绕回
                std::memcpy(&buffer[current_write], data, length);
            }
            else
            {
                // 数据会绕回
                const size_t first_part = SIZE - current_write;
                std::memcpy(&buffer[current_write], data, first_part);
                std::memcpy(&buffer[0], data + first_part, length - first_part);
            }

            write_index = end_write;
            return true;
        }

        // 读取数据（并移除）
        bool read(uint8_t* data, size_t length)
        {
            if (length > available()) return false;

            const size_t current_read = read_index;
            const size_t end_read = (current_read + length) & MASK;

            if (end_read >= current_read)
            {
                // 数据不会绕回
                std::memcpy(data, &buffer[current_read], length);
            }
            else
            {
                // 数据会绕回
                const size_t first_part = SIZE - current_read;
                std::memcpy(data, &buffer[current_read], first_part);
                std::memcpy(data + first_part, &buffer[0], length - first_part);
            }

            read_index = end_read;
            return true;
        }

        // 预读数据（不移除）
        bool peek(uint8_t* data, size_t offset, size_t length) const
        {
            if (offset + length > available()) return false;

            const size_t start_pos = (read_index + offset) & MASK;
            const size_t end_pos = (start_pos + length) & MASK;

            if (end_pos >= start_pos)
            {
                // 数据不会绕回
                std::memcpy(data, &buffer[start_pos], length);
            }
            else
            {
                // 数据会绕回
                const size_t first_part = SIZE - start_pos;
                std::memcpy(data, &buffer[start_pos], first_part);
                std::memcpy(data + first_part, &buffer[0], length - first_part);
            }

            return true;
        }

        // 查找字节位置
        size_t find_byte(uint8_t byte) const
        {
            const size_t available_data = available();
            if (available_data == 0) return SIZE_MAX;

            const size_t current_read = read_index;

            for (size_t i = 0; i < available_data; ++i)
            {
                const size_t pos = (current_read + i) & MASK;
                if (buffer[pos] == byte)
                {
                    return i; // 返回相对于读取位置的偏移
                }
            }

            return SIZE_MAX; // 未找到
        }

        // 丢弃指定数量的字节
        bool discard(const size_t length)
        {
            if (length > available()) return false;
            read_index = (read_index + length) & MASK;
            return true;
        }


        [[nodiscard]] std::span<const uint8_t> get_contiguous_read_buffer() const noexcept
        {
            const size_t current_read_pos = read_index;
            const size_t current_write_pos = write_index;

            if (current_read_pos <= current_write_pos)
            {
                return {buffer + current_read_pos, current_write_pos - current_read_pos};
            }
            else
            {
                return {buffer + current_read_pos, SIZE - current_read_pos};
            }
        }

        // 获取可用数据量
        size_t available() const
        {
            return (write_index - read_index) & MASK;
        }

        // 获取可写入空间
        size_t space() const
        {
            return (read_index - write_index - 1) & MASK;
        }

        // 检查是否为空
        bool empty() const
        {
            return read_index == write_index;
        }

        // 检查是否已满
        bool full() const
        {
            return space() == 0;
        }

        // 清空缓冲区
        void clear()
        {
            read_index = write_index = 0;
        }

        // 获取缓冲区总大小
        static constexpr size_t size()
        {
            return SIZE;
        }
    };
}
#endif //RPL_RINGBUFFER_HPP
