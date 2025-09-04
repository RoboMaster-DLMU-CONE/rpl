#ifndef RPL_MEMORYPOOL_HPP
#define RPL_MEMORYPOOL_HPP

namespace RPL::Containers
{
    template <typename Deserializer>
    struct MemoryPool
    {
        using Collector = Deserializer::Collector;

        static constinit std::array<std::byte, Collector::totalSize> buffer{};
    };
}

#endif //RPL_MEMORYPOOL_HPP
