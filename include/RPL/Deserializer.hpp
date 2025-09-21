#ifndef RPL_DESERIALIZER_HPP
#define RPL_DESERIALIZER_HPP

#include "Containers/MemoryPool.hpp"
#include "Meta/PacketInfoCollector.hpp"

namespace RPL
{
    template <typename T, typename... Ts>
    concept Deserializable = (std::is_same_v<T, Ts> || ...);

    template <typename... Ts>
    class Deserializer
    {
        using Collector = Meta::PacketInfoCollector<Ts...>;
        Containers::MemoryPool<Collector> pool{};

    public:
        template <typename T>
            requires Deserializable<T, Ts...>
        T get() const noexcept
        {
            auto ptr = reinterpret_cast<uint8_t*>(&pool.buffer[Collector::template type_index<T>()]);
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

        // TODO: 条件编译线程安全
        [[nodiscard]] constexpr uint8_t* getWritePtr(uint16_t cmd) const noexcept
        {
            return reinterpret_cast<uint8_t*>(&pool.buffer[Collector::cmd_index(cmd)]);
        }
    };
}

#endif //RPL_DESERIALIZER_HPP
