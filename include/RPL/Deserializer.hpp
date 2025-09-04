#ifndef RPL_DESERIALIZER_HPP
#define RPL_DESERIALIZER_HPP

#include "Containers/MemoryPool.hpp"

namespace RPL
{
    template <typename T, typename... Ts>
    concept Deserializable = (std::is_same_v<T, Ts> || ...);

    template <typename... Ts>
    class Deserializer
    {
        using Collector = Meta::PacketInfoCollector<Ts...>;
        Containers::MemoryPool<Collector> pool;

    public:
        template <typename T>
            requires Deserializable<T, Ts...>
        T get() const noexcept
        {
            auto ptr = &pool.buffer[Collector::template type_index<T>()];
            Meta::PacketTraits<T>::before_get(ptr);
            return *reinterpret_cast<T*>(ptr);
        };


        /**
          * @warning 非线程安全
          */
        template <typename T>
            requires Deserializable<T, Ts...>
        constexpr T& getRawRef() const noexcept
        {
            return reinterpret_cast<T&>(pool.buffer[Collector::template type_index<T>()]);
        };
    };
}

#endif //RPL_DESERIALIZER_HPP
