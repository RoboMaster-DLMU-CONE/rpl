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
 * @brief Extract a specific number of bits from a byte span at a specific bit offset
 *
 * This function handles cross-byte bit extraction with Little-Endian wire format assumptions.
 * Since BitOffset and BitWidth are compile-time constants, the compiler will optimize
 * this into highly efficient bitwise operations.
 *
 * @tparam T The return type (integral)
 * @tparam BitOffset The starting bit index (0 is the LSB of the first byte)
 * @tparam BitWidth The number of bits to extract
 * @param buffer The span of bytes to read from
 * @return The extracted value cast to type T
 */
template <typename T, std::size_t BitOffset, std::size_t BitWidth>
constexpr T extract_bits(std::span<const uint8_t> buffer) {
    static_assert(BitWidth <= sizeof(T) * 8, "BitWidth exceeds return type capacity");

    T result = 0;
    std::size_t current_bit_offset = BitOffset;
    std::size_t bits_extracted = 0;

    // We process byte by byte
    while (bits_extracted < BitWidth) {
        std::size_t byte_index = current_bit_offset / 8;
        std::size_t bit_in_byte = current_bit_offset % 8;
        
        // How many bits can we take from the current byte?
        // It's either the remaining bits we need, or the remaining bits in this byte
        std::size_t bits_to_take = std::min(BitWidth - bits_extracted, 8 - bit_in_byte);

        // Safety check to avoid out-of-bounds, though typically the buffer should be large enough
        if (byte_index >= buffer.size()) {
            break;
        }

        uint8_t byte_val = buffer[byte_index];
        
        // Shift down to put our target bits at position 0
        byte_val >>= bit_in_byte;
        
        // Mask out any upper bits we don't want
        uint8_t mask = (1ULL << bits_to_take) - 1;
        byte_val &= mask;

        // Place these extracted bits into the result at the correct position
        // Since we extract starting from LSB of the wire format (assuming little-endian bit packing)
        result |= (static_cast<T>(byte_val) << bits_extracted);

        bits_extracted += bits_to_take;
        current_bit_offset += bits_to_take;
    }

    return result;
}

/**
 * @brief Core implementation for parsing a bitstream layout into a tuple
 */
template <typename Layout, std::size_t... Is>
constexpr auto parse_bitstream_impl(std::span<const uint8_t> buffer, std::index_sequence<Is...>) {
    // Calculate prefix sums for bit offsets at compile time
    constexpr auto offsets = []() {
        std::array<std::size_t, sizeof...(Is) + 1> arr{0};
        std::size_t current = 0;
        // Fold expression to calculate running sum of bits
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
 * @brief Deserialize a packet using its BitLayout definition
 * 
 * Extracts bit-fields from the buffer and safely constructs the target struct T
 * using C++20 parenthesized aggregate initialization.
 *
 * @tparam T The target structure type
 * @param buffer Byte span containing the wire format data
 * @return The constructed structure with bit-fields correctly initialized
 */
template <typename T>
requires Meta::HasBitLayout<Meta::PacketTraits<T>>
constexpr T deserialize_bitstream(std::span<const uint8_t> buffer) {
    using Layout = typename Meta::PacketTraits<T>::BitLayout;
    constexpr std::size_t N = std::tuple_size_v<Layout>;
    
    // 1. Extract values into a tuple based on the compile-time layout
    auto values_tuple = Detail::parse_bitstream_impl<Layout>(
        buffer, std::make_index_sequence<N>{}
    );
    
    // 2. Use C++20 aggregate initialization to perfectly assign to native bit-fields
    return std::make_from_tuple<T>(values_tuple);
}

} // namespace RPL

#endif // RPL_BITSTREAM_PARSER_HPP