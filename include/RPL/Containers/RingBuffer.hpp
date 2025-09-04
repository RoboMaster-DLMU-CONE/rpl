#ifndef RPL_RINGBUFFER_HPP
#define RPL_RINGBUFFER_HPP
#include <atomic>

namespace RPL::Meta
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
        bool write(const uint8_t* data, size_t length)
        {
            size_t current_write = write_index;
            const size_t current_read = read_index;

            const size_t available = (current_read - current_write - 1) & MASK;
            if (length > available) return false;

            const size_t end_write = (current_write + length) & MASK;
            if (end_write > current_write)
            {
                memcpy(&buffer[current_write], data, length);
            }
            else
            {
                const size_t first_part = SIZE - current_write;
                memcpy(&buffer[current_write], data, first_part);
                memcpy(&buffer[0], data + first_part, length - first_part);
            }
            write_index = end_write;
            return true;
        }
    };
}

#endif //RPL_RINGBUFFER_HPP
