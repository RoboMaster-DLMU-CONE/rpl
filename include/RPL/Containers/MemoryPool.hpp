#ifndef RPL_MEMORYPOOL_HPP
#define RPL_MEMORYPOOL_HPP

#include <array>
#include <cstddef>

namespace RPL::Containers
{
    template <typename Collector>
    struct MemoryPool
    {
        static inline std::array<std::byte, Collector::totalSize> buffer{};
    };
}

#endif //RPL_MEMORYPOOL_HPP
