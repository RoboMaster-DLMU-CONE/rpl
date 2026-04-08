/**
 * @file BitstreamTraits.hpp
 * @brief RPL 位流特性定义
 *
 * 此文件提供位流解析/序列化的基础类型和概念定义。
 * 包括 Field 描述器用于定义位域的线格式布局，
 * 以及 HasBitLayout 概念用于检查类型是否定义了位流布局。
 *
 * @par 设计原理
 * - Field 模板用于声明每个位域的底层类型和位数
 * - HasBitLayout concept 用于启用/禁用位流处理代码路径
 *
 * @author WindWeaver
 */

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
 *
 * @tparam T 要检查的类型
 */
template <typename T>
struct is_std_array : std::false_type {};

/**
 * @brief std::array 的特化
 * @tparam T 数组元素类型
 * @tparam N 数组大小
 */
template <typename T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

/// @brief is_std_array 的便捷别名
template <typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

/**
 * @brief 位流解析/序列化的字段描述器
 *
 * 用于 PacketTraits::BitLayout 中定义位域的线格式。
 * 每个 Field 实例描述一个字段的底层类型和占用的位数。
 *
 * @tparam T 底层整数类型 (uint8_t, uint16_t 等) 或 std::array
 * @tparam Bits 该字段在线上占用的确切位数
 *
 * @par 使用示例
 * @code
 * using BitLayout = std::tuple<
 *     Field<uint8_t, 3>,   // 3位标志字段
 *     Field<uint16_t, 11>, // 11位通道值
 *     Field<int16_t, 16>   // 16位有符号值
 * >;
 * @endcode
 *
 * @note 所有字段的位数之和必须等于数据包的总位数
 */
template <typename T, std::size_t Bits = sizeof(T) * 8>
struct Field {
    // We allow integral types or std::array
    // static_assert(std::is_integral_v<T>, "Field type must be an integral type");
    /// @brief 位数必须为正数
    static_assert(Bits > 0, "Bits must be positive");

    /// @brief 字段的基础类型
    using type = T;
    /// @brief 字段占用的位数
    static constexpr std::size_t bits = Bits;
};

/**
 * @brief 检查 PacketTraits 是否定义了 BitLayout
 *
 * 用于有条件地启用位流解析或直接 memcpy。
 * 如果 PacketTraits<T> 定义了 BitLayout 类型别名，则此概念为 true。
 *
 * @tparam Traits 要检查的 PacketTraits 特化
 */
template <typename Traits>
concept HasBitLayout = requires {
    typename Traits::BitLayout;
};

} // namespace RPL::Meta

#endif // RPL_BITSTREAM_TRAITS_HPP
