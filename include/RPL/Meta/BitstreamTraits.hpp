#ifndef RPL_BITSTREAM_TRAITS_HPP
#define RPL_BITSTREAM_TRAITS_HPP

#include <cstddef>
#include <cstdint>
#include <array>
#include <tuple>
#include <type_traits>

namespace RPL::Meta {

/**
 * @brief 检查类型是否为 std::array
 */
template <typename T>
struct is_std_array : std::false_type {};
template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};
template <typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

/**
 * @brief 位流解析的字段描述器
 *
 * 用于 PacketTraits::BitLayout 中定义位域的线格式
 *
 * @tparam T 底层整数类型 (uint8_t, uint16_t 等)
 * @tparam Bits 该字段在线上占用的确切位数
 */
template <typename T, std::size_t Bits = sizeof(T) * 8>
struct Field {
    // We allow integral types or std::array
    // static_assert(std::is_integral_v<T>, "Field type must be an integral type"); 
    static_assert(Bits > 0, "Bits must be positive");

    using type = T;
    static constexpr std::size_t bits = Bits;
};

/**
 * @brief 检查 PacketTraits 是否定义了 BitLayout 的概念
 *
 * 用于有条件地启用位流解析或直接 memcpy
 */
template <typename Traits>
concept HasBitLayout = requires {
    typename Traits::BitLayout;
};

} // namespace RPL::Meta

#endif // RPL_BITSTREAM_TRAITS_HPP
