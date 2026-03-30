#ifndef RPL_BITSTREAM_TRAITS_HPP
#define RPL_BITSTREAM_TRAITS_HPP

#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>

namespace RPL::Meta {

/**
 * @brief Field descriptor for bitstream parsing
 * 
 * Used within PacketTraits::BitLayout to define the wire format of bit-fields
 *
 * @tparam T The underlying integer type (uint8_t, uint16_t, etc.)
 * @tparam Bits The exact number of bits this field occupies on the wire
 */
template <typename T, std::size_t Bits = sizeof(T) * 8>
struct Field {
    static_assert(std::is_integral_v<T>, "Field type must be an integral type");
    static_assert(Bits > 0 && Bits <= sizeof(T) * 8, "Bits must fit within the specified type T");
    
    using type = T;
    static constexpr std::size_t bits = Bits;
};

/**
 * @brief Concept to check if a PacketTraits has a BitLayout defined
 * 
 * Used to conditionally enable bitstream parsing vs direct memcpy
 */
template <typename Traits>
concept HasBitLayout = requires {
    typename Traits::BitLayout;
};

} // namespace RPL::Meta

#endif // RPL_BITSTREAM_TRAITS_HPP
