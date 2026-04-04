#ifndef RPL_BITSTREAM_SERIALIZER_HPP
#define RPL_BITSTREAM_SERIALIZER_HPP

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
 * @brief 在特定位偏移处将指定位数注入到字节序列中
 *
 * 此函数处理跨字节位注入，采用小端线格式假设。
 * 由于 BitOffset 和 BitWidth 是编译时常量，编译器会将此优化
 * 为高效的位操作。
 */
template <typename T, std::size_t BitOffset, std::size_t BitWidth>
constexpr void inject_bits(std::span<uint8_t> buffer, T value) {
    static_assert(BitWidth <= sizeof(T) * 8, "BitWidth exceeds input type capacity");

    std::size_t current_bit_offset = BitOffset;
    std::size_t bits_injected = 0;

    T masked_value = value;
    if constexpr (BitWidth < sizeof(T) * 8) {
        masked_value &= (static_cast<T>(1) << BitWidth) - 1;
    }

    while (bits_injected < BitWidth) {
        std::size_t byte_index = current_bit_offset / 8;
        std::size_t bit_in_byte = current_bit_offset % 8;

        std::size_t bits_to_put = std::min(BitWidth - bits_injected, static_cast<std::size_t>(8 - bit_in_byte));

        if (byte_index >= buffer.size()) {
            break;
        }

        uint8_t chunk = static_cast<uint8_t>((masked_value >> bits_injected) & ((1ULL << bits_to_put) - 1));
        chunk <<= bit_in_byte;
        buffer[byte_index] |= chunk;

        bits_injected += bits_to_put;
        current_bit_offset += bits_to_put;
    }
}

/**
 * @brief 使用结构化绑定将结构体解包为成员元组
 *
 * 我们使用值绑定 (const auto [ ... ]) 而不是引用绑定 (auto& [ ... ])
 * 因为在 C++ 中绑定引用到位域是非法的。
 * 这可以安全地将位域值复制到纯标准元组中。
 */
template <std::size_t N, typename T>
constexpr auto struct_to_tuple(const T& obj) {
    if constexpr (N == 1) {
        const auto [m1] = obj; return std::make_tuple(m1);
    } else if constexpr (N == 2) {
        const auto [m1, m2] = obj; return std::make_tuple(m1, m2);
    } else if constexpr (N == 3) {
        const auto [m1, m2, m3] = obj; return std::make_tuple(m1, m2, m3);
    } else if constexpr (N == 4) {
        const auto [m1, m2, m3, m4] = obj; return std::make_tuple(m1, m2, m3, m4);
    } else if constexpr (N == 5) {
        const auto [m1, m2, m3, m4, m5] = obj; return std::make_tuple(m1, m2, m3, m4, m5);
    } else if constexpr (N == 6) {
        const auto [m1, m2, m3, m4, m5, m6] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6);
    } else if constexpr (N == 7) {
        const auto [m1, m2, m3, m4, m5, m6, m7] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7);
    } else if constexpr (N == 8) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8);
    } else if constexpr (N == 9) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9);
    } else if constexpr (N == 10) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10);
    } else if constexpr (N == 11) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11);
    } else if constexpr (N == 12) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12);
    } else if constexpr (N == 13) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13);
    } else if constexpr (N == 14) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14);
    } else if constexpr (N == 15) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15);
    } else if constexpr (N == 16) {
        const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16] = obj; return std::make_tuple(m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16);
    } else {
        static_assert(N <= 16, "struct_to_tuple supports up to 16 fields. Add more branches to support more!");
        return std::make_tuple();
    }
}

} // namespace RPL::Detail

namespace RPL {

/**
 * @brief 将基于位流的包序列化到预清零的缓冲区中
 *
 * 使用结构化绑定从结构中提取位域并注入
 * 到字节序列中正确的编译期偏移处。
 */
template <typename T>
requires Meta::HasBitLayout<Meta::PacketTraits<T>>
constexpr void serialize_bitstream(std::span<uint8_t> buffer, const T& packet) {
    using Layout = typename Meta::PacketTraits<T>::BitLayout;
    constexpr std::size_t N = std::tuple_size_v<Layout>;

    // 1. 将结构体解包为值元组 (对位域安全)
    auto values = Detail::struct_to_tuple<N>(packet);

    // 在编译期计算位偏移的前缀和
    constexpr auto offsets = []() {
        std::array<std::size_t, N + 1> arr{0};
        std::size_t current = 0;
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((arr[Is + 1] = current += std::tuple_element_t<Is, Layout>::bits), ...);
        }(std::make_index_sequence<N>{});
        return arr;
    }();

    // 2. 将每个元组元素注入到字节序列中编译期计算的偏移处
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (Detail::inject_bits<
            typename std::tuple_element_t<Is, Layout>::type,
            offsets[Is],
            std::tuple_element_t<Is, Layout>::bits
        >(buffer, std::get<Is>(values)), ...);
    }(std::make_index_sequence<N>{});
}

} // namespace RPL

#endif // RPL_BITSTREAM_SERIALIZER_HPP