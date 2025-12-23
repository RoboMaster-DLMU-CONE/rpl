/**
 * @file RingBuffer.hpp
 * @brief RPL库的环形缓冲区实现
 *
 * 此文件包含RingBuffer类的定义，用于高效的数据流处理。
 * 环形缓冲区支持无锁的多线程访问，适用于数据包解析场景。
 *
 * @author WindWeaver
 */

#ifndef RPL_RINGBUFFER_HPP
#define RPL_RINGBUFFER_HPP

#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>

namespace RPL::Containers
{
    /**
     * @brief 环形缓冲区类
     *
     * 提供高效的环形缓冲区实现，支持多线程无锁访问。
     * 缓冲区大小必须是2的幂，以优化性能。
     *
     * @tparam SIZE 缓冲区大小，必须是2的幂
     */
    template <size_t SIZE>
    class RingBuffer
    {
        static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be a power of 2");  ///< 确保SIZE是2的幂
        static constexpr size_t MASK = SIZE - 1;  ///< 用于快速取模运算的掩码

        alignas(64) uint8_t buffer[SIZE]{};  ///< 存储数据的缓冲区，64字节对齐以避免缓存行伪共享
        size_t write_index{0};  ///< 写入索引
        size_t read_index{0};   ///< 读取索引

    public:
        /**
         * @brief 获取可直接写入的缓冲区视图
         *
         * 获取当前写指针指向的连续内存区域，用于零拷贝写入（如DMA或直接内存操作）。
         * 注意：此函数不会更新写指针，写入完成后必须调用 advance_write_index。
         *
         * @return 包含指针和最大可写入长度的span
         */
        std::span<uint8_t> get_write_buffer() noexcept
        {
            const size_t current_write = write_index;
            const size_t current_read = read_index;
            
            if (current_write >= current_read)
            {
                // 空闲空间分为两部分（尾部和头部），或者只有尾部
                // 我们只返回尾部的连续空间
                // 注意：如果 read_index == 0，我们不能写到 SIZE，只能写到 SIZE-1
                // 因为 read == write 表示空，read == write + 1 (mod SIZE) 表示满
                const size_t end = (current_read == 0) ? SIZE - 1 : SIZE;
                return {&buffer[current_write], end - current_write};
            }
            else
            {
                // 空闲空间在中间：[write, read - 1)
                return {&buffer[current_write], current_read - current_write - 1};
            }
        }

        /**
         * @brief 更新写索引
         *
         * 在直接写入缓冲区后调用此函数来更新写索引。
         *
         * @param length 实际写入的字节数
         * @return 如果更新成功返回true，如果长度非法返回false
         */
        bool advance_write_index(size_t length)
        {
            if (length > space()) return false;
            write_index = (write_index + length) & MASK;
            return true;
        }

        /**
         * @brief 写入数据到缓冲区
         *
         * 将指定长度的数据写入环形缓冲区
         *
         * @param data 要写入的数据指针
         * @param length 要写入的数据长度
         * @return 如果写入成功返回true，否则返回false（空间不足时）
         */
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

        /**
         * @brief 从缓冲区读取数据（并移除）
         *
         * 从环形缓冲区读取指定长度的数据，并从缓冲区中移除
         *
         * @param data 用于存储读取数据的缓冲区
         * @param length 要读取的数据长度
         * @return 如果读取成功返回true，否则返回false（数据不足时）
         */
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

        /**
         * @brief 预读数据（不移除）
         *
         * 从指定偏移位置预读数据，不从缓冲区中移除
         *
         * @param data 用于存储预读数据的缓冲区
         * @param offset 从当前读取位置开始的偏移量
         * @param length 要预读的数据长度
         * @return 如果预读成功返回true，否则返回false（数据不足时）
         */
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

        /**
         * @brief 查找字节位置
         *
         * 在缓冲区中查找指定字节的位置
         *
         * @param byte 要查找的字节
         * @return 找到字节时返回其在缓冲区中的偏移量，未找到时返回SIZE_MAX
         */
        size_t find_byte(uint8_t byte) const noexcept
        {
            const size_t available_data = available();
            if (available_data == 0) return SIZE_MAX;

            const size_t current_read = read_index;
            const size_t current_write = write_index;

            if (current_read <= current_write)
            {
                // 数据是连续的，直接使用 memchr
                const void* found = std::memchr(buffer + current_read, byte, available_data);
                return found ? static_cast<const uint8_t*>(found) - (buffer + current_read) : SIZE_MAX;
            }
            // 数据分为两段
            const size_t first_part_size = SIZE - current_read;
            const void* found = std::memchr(buffer + current_read, byte, first_part_size);
            if (found)
            {
                return static_cast<const uint8_t*>(found) - (buffer + current_read);
            }

            // 在第二段中查找
            found = std::memchr(buffer, byte, current_write);
            return found ? first_part_size + (static_cast<const uint8_t*>(found) - buffer) : SIZE_MAX;
        }

        /**
         * @brief 丢弃指定数量的字节
         *
         * 从缓冲区中丢弃指定数量的字节
         *
         * @param length 要丢弃的字节数
         * @return 如果丢弃成功返回true，否则返回false（数据不足时）
         */
        bool discard(const size_t length)
        {
            if (length > available()) return false;
            read_index = (read_index + length) & MASK;
            return true;
        }


        /**
         * @brief 获取连续可读数据的视图
         *
         * 获取当前可读的连续数据块的视图，用于高效读取
         *
         * @return 指向连续可读数据的span
         */
        [[nodiscard]] std::span<const uint8_t> get_contiguous_read_buffer() const noexcept
        {
            const size_t current_read_pos = read_index;
            const size_t current_write_pos = write_index;

            if (current_read_pos <= current_write_pos)
            {
                return {buffer + current_read_pos, current_write_pos - current_read_pos};
            }
            return {buffer + current_read_pos, SIZE - current_read_pos};
        }

        /**
         * @brief 获取可用数据量
         *
         * 获取缓冲区中当前可用的数据量
         *
         * @return 可用数据量（字节）
         */
        size_t available() const
        {
            return (write_index - read_index) & MASK;
        }

        /**
         * @brief 获取可写入空间
         *
         * 获取缓冲区中当前可写入的空间
         *
         * @return 可写入空间（字节）
         */
        size_t space() const
        {
            return (read_index - write_index - 1) & MASK;
        }

        /**
         * @brief 检查缓冲区是否为空
         *
         * 检查缓冲区中是否没有任何数据
         *
         * @return 如果缓冲区为空返回true，否则返回false
         */
        bool empty() const
        {
            return read_index == write_index;
        }

        /**
         * @brief 检查缓冲区是否已满
         *
         * 检查缓冲区是否已达到最大容量
         *
         * @return 如果缓冲区已满返回true，否则返回false
         */
        bool full() const
        {
            return space() == 0;
        }

        /**
         * @brief 清空缓冲区
         *
         * 清空缓冲区中的所有数据
         */
        void clear()
        {
            read_index = write_index = 0;
        }

        /**
         * @brief 获取缓冲区总大小
         *
         * 获取环形缓冲区的总容量
         *
         * @return 缓冲区总大小（字节）
         */
        static constexpr size_t size()
        {
            return SIZE;
        }
    };
}
#endif //RPL_RINGBUFFER_HPP
