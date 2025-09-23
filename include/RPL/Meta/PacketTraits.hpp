#ifndef RPL_INFO_HPP
#define RPL_INFO_HPP

namespace RPL::Meta
{
    template <typename Derived>
    struct PacketTraitsBase
    {
        static constexpr uint16_t cmd = Derived::cmd;
        static constexpr size_t size = Derived::size;

        static void before_get(uint8_t* data)
        {
            // 使用不同的方法名避免递归
            if constexpr (requires { Derived::before_get_custom(data); })
            {
                Derived::before_get_custom(data);
            }
            // 如果没有定义 before_get_custom，则什么都不做
        }
    };

    template <typename T>
    struct PacketTraits;
}

#endif //RPL_INFO_HPP
