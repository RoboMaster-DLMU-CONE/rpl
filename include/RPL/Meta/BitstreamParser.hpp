#ifndef RPL_BITSTREAM_PARSER_HPP
#define RPL_BITSTREAM_PARSER_HPP

#include "RPL/Meta/BitstreamTraits.hpp"
#include "RPL/Meta/PacketTraits.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <tuple>
#include <utility>

namespace RPL::Detail {

/**
 * @brief 在特定位偏移处从字节序列中提取指定位数
 *
 * 此函数处理跨字节位提取，采用小端线格式假设。
 * 由于 BitOffset 和 BitWidth 是编译时常量，编译器会将此优化
 * 为高效的位操作。
 *
 * @tparam T 返回类型 (整数)
 * @tparam BitOffset 起始位索引 (0 是第一个字节的 LSB)
 * @tparam BitWidth 要提取的位数
 * @param buffer 要读取的字节序列
 * @return 提取的值并转换为类型 T
 */
template <typename T, std::size_t BitOffset, std::size_t BitWidth>
constexpr T extract_bits(std::span<const uint8_t> buffer) {
    static_assert(BitWidth <= sizeof(T) * 8, "BitWidth exceeds return type capacity");

    T result = 0;
    std::size_t current_bit_offset = BitOffset;
    std::size_t bits_extracted = 0;

    // 逐字节处理
    while (bits_extracted < BitWidth) {
        std::size_t byte_index = current_bit_offset / 8;
        std::size_t bit_in_byte = current_bit_offset % 8;

        // 我们能从当前字节取多少位?
        // 要么是我们还需要的剩余位，要么是该字节剩余的位
        std::size_t bits_to_take = std::min(BitWidth - bits_extracted, 8 - bit_in_byte);

        // 安全检查以避免越界，尽管通常缓冲区应该足够大
        if (byte_index >= buffer.size()) {
            break;
        }

        uint8_t byte_val = buffer[byte_index];

        // 下移以将目标位移到位置 0
        byte_val >>= bit_in_byte;

        // 屏蔽不需要的上位
        uint8_t mask = (1ULL << bits_to_take) - 1;
        byte_val &= mask;

        // 将这些提取的位放入结果中的正确位置
        // 由于我们从线格式的 LSB 开始提取 (假设小端位打包)
        result |= (static_cast<T>(byte_val) << bits_extracted);

        bits_extracted += bits_to_take;
        current_bit_offset += bits_to_take;
    }

    return result;
}

/**
 * @brief 将位流布局解析为元组的核心实现
 */
template <typename Layout, std::size_t... Is>
constexpr auto parse_bitstream_impl(std::span<const uint8_t> buffer, std::index_sequence<Is...>) {
    // 在编译期计算位偏移的前缀和
    constexpr auto offsets = []() {
        std::array<std::size_t, sizeof...(Is) + 1> arr{0};
        std::size_t current = 0;
        // 折叠表达式计算运行总和
        ((arr[Is + 1] = current += std::tuple_element_t<Is, Layout>::bits), ...);
        return arr;
    }();

    return std::make_tuple(
        extract_bits<
            typename std::tuple_element_t<Is, Layout>::type,
            offsets[Is],
            std::tuple_element_t<Is, Layout>::bits
        >(buffer)...
    );
}

} // namespace RPL::Detail

namespace RPL {

/**
 * @brief 使用位流布局定义反序列化数据包
 *
 * 从缓冲区中提取位域并安全地构造目标结构 T
 * 使用 C++20 括号聚合初始化。
 *
 * @tparam T 目标结构类型
 * @param buffer 包含线格式数据的字节序列
 * @return 正确初始化位域的结构
 */
template <typename T>
requires Meta::HasBitLayout<Meta::PacketTraits<T>>
constexpr T deserialize_bitstream(std::span<const uint8_t> buffer) {
    using Layout = typename Meta::PacketTraits<T>::BitLayout;
    constexpr std::size_t N = std::tuple_size_v<Layout>;

    // 1. 根据编译期布局将值提取到元组中
    auto values_tuple = Detail::parse_bitstream_impl<Layout>(
        buffer, std::make_index_sequence<N>{}
    );

    // 2. 使用 C++20 聚合初始化完美赋值给原生位域
    return std::make_from_tuple<T>(values_tuple);
}

} // namespace RPL

#endif // RPL_BITSTREAM_PARSER_HPP