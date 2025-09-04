#ifndef RPL_INFO_HPP
#define RPL_INFO_HPP

namespace RPL::Meta
{
    template <typename Derived>
    struct PacketTraitsBase
    {
        static constexpr std::uint16_t cmd = Derived::cmd;
        static constexpr std::size_t size = Derived::size;

        static_assert(requires { Derived::cmd; }, "Derived class must define cmd");
        static_assert(requires { Derived::size; }, "Derived class must define size");

        static void before_get(std::uint8_t* data)
        {
            if constexpr (requires { Derived::before_get(data); })
            {
                Derived::before_get(data);
            }
        }
    };

    template <typename T>
    struct PacketTraits;
}

#endif //RPL_INFO_HPP
