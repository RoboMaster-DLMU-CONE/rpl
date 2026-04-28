/**
 * RPL (RoboMaster Packet Library) - Single Header Version
 * Generated automatically. Do not edit directly.
 */

#ifndef RPL_SINGLE_HEADER_HPP
#define RPL_SINGLE_HEADER_HPP

#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <stddef.h>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

/**
 * @file Def.hpp
 * @brief RPL库的常量定义
 *
 * 此文件定义了RPL库中使用的帧格式常量。
 *
 * @author WindWeaver
 */

/// @file cppcrc.h
/// @author Darren V Levine (DarrenVLevine@gmail.com)
/// @brief A very small, fast, header-only, C++ library for generating CRCs
/// @version 1.3
/// @date 2022-11-17
///
/// @copyright (c) 2022 Darren V Levine. This code is licensed under MIT license (see LICENSE file for details).

//
// Backend implementation:
//

namespace crc_utils
{
    inline constexpr uint8_t reverse_bits(uint8_t x)
    {
        constexpr uint8_t lookup[16] = {0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe, 0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
        return (lookup[x & 0x0F] << 4) | lookup[x >> 4];
    }
    inline constexpr uint16_t reverse_bits(uint16_t x)
    {
        return uint16_t(reverse_bits(uint8_t(x))) << 8 | uint16_t(reverse_bits(uint8_t(x >> 8)));
    }
    inline constexpr uint32_t reverse_bits(uint32_t x)
    {
        return uint32_t(reverse_bits(uint16_t(x))) << 16 | uint32_t(reverse_bits(uint16_t(x >> 16)));
    }
    inline constexpr uint64_t reverse_bits(uint64_t x)
    {
        return uint64_t(reverse_bits(uint32_t(x))) << 32 | uint64_t(reverse_bits(uint32_t(x >> 32)));
    }
    template <typename out_t, out_t poly, bool refl_in, bool refl_out, size_t index>
    constexpr out_t get_crc_table_value_at_index()
    {
        constexpr size_t bit_width = sizeof(out_t) * 8;
        out_t remainder            = refl_in ? reverse_bits(static_cast<out_t>(index)) >> (bit_width - 8u) : static_cast<out_t>(index);
        constexpr out_t mask       = static_cast<out_t>(1) << (bit_width - 1u);
        for (size_t i = 0; i < bit_width; i++)
        {
            if (remainder & mask)
                remainder = (remainder << 1) ^ poly;
            else
                remainder <<= 1;
        }
        return refl_in ? reverse_bits(remainder) : remainder;
    }

    template <typename out_t, out_t poly, bool refl_in, bool refl_out, size_t size = 256, typename = std::make_index_sequence<size>>
    struct crc_lookup_table;

    template <typename out_t, out_t poly, bool refl_in, bool refl_out, size_t size, size_t... indexes>
    struct crc_lookup_table<out_t, poly, refl_in, refl_out, size, std::index_sequence<indexes...>>
    {
        static constexpr out_t value[size] = {get_crc_table_value_at_index<out_t, poly, refl_in, refl_out, indexes>()...};
    };

#if ((defined(_MSVC_LANG) && _MSVC_LANG < 201703L) || (defined(__cplusplus) && __cplusplus < 201703L)) // redeclaration is only needed before C++17
    template <typename out_t, out_t poly, bool refl_in, bool refl_out, size_t size, size_t... indexes>
    constexpr out_t crc_lookup_table<out_t, poly, refl_in, refl_out, size, std::index_sequence<indexes...>>::value[size];
#endif

    template <typename out_t, out_t poly, bool refl_in, bool refl_out, out_t x_or_out, typename std::enable_if<refl_in, int *>::type = nullptr>
    constexpr out_t calculate_crc(const uint8_t *bytes, size_t n, out_t crc)
    {
        constexpr auto &lookup = crc_lookup_table<out_t, poly, refl_in, refl_out>().value;
        crc                    = reverse_bits(crc);
        while (n--)
            crc = lookup[static_cast<uint8_t>(*bytes++ ^ crc)] ^ (crc >> 8);
        return (refl_out != refl_in ? reverse_bits(crc) : crc) ^ x_or_out; // needed since the reflections are baked into the table for speed
    }

    template <typename out_t, out_t poly, bool refl_in, bool refl_out, out_t x_or_out, typename std::enable_if<!refl_in, int *>::type = nullptr>
    constexpr out_t calculate_crc(const uint8_t *bytes, size_t n, out_t crc)
    {
        constexpr auto &lookup             = crc_lookup_table<out_t, poly, refl_in, refl_out>().value;
        constexpr size_t bit_width_minus_8 = sizeof(out_t) * 8 - 8U;
        while (n--)
            crc = lookup[static_cast<uint8_t>(*bytes++ ^ (crc >> bit_width_minus_8))] ^ (crc << 8);
        return (refl_out ? reverse_bits(crc) : crc) ^ x_or_out;
    }

    template <typename out_t, out_t poly_arg, out_t init_arg, bool refl_in_arg, bool refl_out_arg, out_t x_or_out_arg>
    struct crc
    {
        using type                      = out_t;        // base type of the crc algorithm
        static constexpr out_t poly     = poly_arg;     // polynomial of the crc algorithm
        static constexpr out_t init     = init_arg;     // initial CRC internal state, WARNING: may be different from "null_crc"
        static constexpr bool refl_in   = refl_in_arg;  // true if the bits of the crc should be reflected/reversed on input
        static constexpr bool refl_out  = refl_out_arg; // true if the bits of the crc should be reflected/reversed on output
        static constexpr out_t x_or_out = x_or_out_arg; // the value to X-OR the output with
        static constexpr out_t null_crc = (refl_out ? reverse_bits(init) : init) ^ x_or_out; // CRC value of no/null data

        /// @brief Calculate the checksum of some bytes, or continue an existing calculation by passing in the prior crc value
        static constexpr out_t calc(const uint8_t *bytes = nullptr, size_t num_bytes = 0u, out_t prior_crc_value = null_crc)
        {
            prior_crc_value = x_or_out ? prior_crc_value ^ x_or_out : prior_crc_value;
            prior_crc_value = refl_out ? reverse_bits(prior_crc_value) : prior_crc_value;
            return calculate_crc<out_t, poly, refl_in, refl_out, x_or_out>(bytes, num_bytes, prior_crc_value);
        }
        /// @brief the underlying pre-computed CRC table used for fast lookup-table-based calculations
        static constexpr auto &table()
        {
            return crc_lookup_table<out_t, poly, refl_in, refl_out>().value;
        }
    };
} // namespace crc_utils

//
// Default CRC Configurations: <type, poly, init, refl_in, refl_out, x_or_out>
//

namespace CRC8
{
    using CRC8     = crc_utils::crc<uint8_t, 0x07, 0x00, false, false, 0x00>;
    using CDMA2000 = crc_utils::crc<uint8_t, 0x9B, 0xFF, false, false, 0x00>;
    using DARC     = crc_utils::crc<uint8_t, 0x39, 0x00, true, true, 0x00>;
    using DVB_S2   = crc_utils::crc<uint8_t, 0xD5, 0x00, false, false, 0x00>;
    using EBU      = crc_utils::crc<uint8_t, 0x1D, 0xFF, true, true, 0x00>;
    using I_CODE   = crc_utils::crc<uint8_t, 0x1D, 0xFD, false, false, 0x00>;
    using ITU      = crc_utils::crc<uint8_t, 0x07, 0x00, false, false, 0x55>;
    using MAXIM    = crc_utils::crc<uint8_t, 0x31, 0x00, true, true, 0x00>;
    using ROHC     = crc_utils::crc<uint8_t, 0x07, 0xFF, true, true, 0x00>;
    using WCDMA    = crc_utils::crc<uint8_t, 0x9B, 0x00, true, true, 0x00>;
} // namespace CRC8
namespace CRC16
{
    using ARC         = crc_utils::crc<uint16_t, 0x8005, 0x0000, true, true, 0x0000>;
    using AUG_CCITT   = crc_utils::crc<uint16_t, 0x1021, 0x1D0F, false, false, 0x0000>;
    using BUYPASS     = crc_utils::crc<uint16_t, 0x8005, 0x0000, false, false, 0x0000>;
    using CCITT_FALSE = crc_utils::crc<uint16_t, 0x1021, 0xFFFF, false, false, 0x0000>;
    using CDMA2000    = crc_utils::crc<uint16_t, 0xC867, 0xFFFF, false, false, 0x0000>;
    using DDS_110     = crc_utils::crc<uint16_t, 0x8005, 0x800D, false, false, 0x0000>;
    using DECT_R      = crc_utils::crc<uint16_t, 0x0589, 0x0000, false, false, 0x0001>;
    using DECT_X      = crc_utils::crc<uint16_t, 0x0589, 0x0000, false, false, 0x0000>;
    using DNP         = crc_utils::crc<uint16_t, 0x3D65, 0x0000, true, true, 0xFFFF>;
    using EN_13757    = crc_utils::crc<uint16_t, 0x3D65, 0x0000, false, false, 0xFFFF>;
    using GENIBUS     = crc_utils::crc<uint16_t, 0x1021, 0xFFFF, false, false, 0xFFFF>;
    using KERMIT      = crc_utils::crc<uint16_t, 0x1021, 0x0000, true, true, 0x0000>;
    using MAXIM       = crc_utils::crc<uint16_t, 0x8005, 0x0000, true, true, 0xFFFF>;
    using MCRF4XX     = crc_utils::crc<uint16_t, 0x1021, 0xFFFF, true, true, 0x0000>;
    using MODBUS      = crc_utils::crc<uint16_t, 0x8005, 0xFFFF, true, true, 0x0000>;
    using RIELLO      = crc_utils::crc<uint16_t, 0x1021, 0xB2AA, true, true, 0x0000>;
    using T10_DIF     = crc_utils::crc<uint16_t, 0x8BB7, 0x0000, false, false, 0x0000>;
    using TELEDISK    = crc_utils::crc<uint16_t, 0xA097, 0x0000, false, false, 0x0000>;
    using TMS37157    = crc_utils::crc<uint16_t, 0x1021, 0x89EC, true, true, 0x0000>;
    using USB         = crc_utils::crc<uint16_t, 0x8005, 0xFFFF, true, true, 0xFFFF>;
    using X_25        = crc_utils::crc<uint16_t, 0x1021, 0xFFFF, true, true, 0xFFFF>;
    using XMODEM      = crc_utils::crc<uint16_t, 0x1021, 0x0000, false, false, 0x0000>;
    using A           = crc_utils::crc<uint16_t, 0x1021, 0xC6C6, true, true, 0x0000>;
} // namespace CRC16
namespace CRC32
{
    using CRC32  = crc_utils::crc<uint32_t, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF>;
    using BZIP2  = crc_utils::crc<uint32_t, 0x04C11DB7, 0xFFFFFFFF, false, false, 0xFFFFFFFF>;
    using JAMCRC = crc_utils::crc<uint32_t, 0x04C11DB7, 0xFFFFFFFF, true, true, 0x00000000>;
    using MPEG_2 = crc_utils::crc<uint32_t, 0x04C11DB7, 0xFFFFFFFF, false, false, 0x00000000>;
    using POSIX  = crc_utils::crc<uint32_t, 0x04C11DB7, 0x00000000, false, false, 0xFFFFFFFF>;
    using SATA   = crc_utils::crc<uint32_t, 0x04C11DB7, 0x52325032, false, false, 0x00000000>;
    using XFER   = crc_utils::crc<uint32_t, 0x000000AF, 0x00000000, false, false, 0x00000000>;
    using C      = crc_utils::crc<uint32_t, 0x1EDC6F41, 0xFFFFFFFF, true, true, 0xFFFFFFFF>;
    using D      = crc_utils::crc<uint32_t, 0xA833982B, 0xFFFFFFFF, true, true, 0xFFFFFFFF>;
    using Q      = crc_utils::crc<uint32_t, 0x814141AB, 0x00000000, false, false, 0x00000000>;
} // namespace CRC32
namespace CRC64
{
    using ECMA   = crc_utils::crc<uint64_t, 0x42F0E1EBA9EA3693, 0x0000000000000000, false, false, 0x0000000000000000>;
    using GO_ISO = crc_utils::crc<uint64_t, 0x000000000000001B, 0xFFFFFFFFFFFFFFFF, true, true, 0xFFFFFFFFFFFFFFFF>;
    using WE     = crc_utils::crc<uint64_t, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF, false, false, 0xFFFFFFFFFFFFFFFF>;
    using XY     = crc_utils::crc<uint64_t, 0x42F0E1EBA9EA3693, 0xFFFFFFFFFFFFFFFF, true, true, 0xFFFFFFFFFFFFFFFF>;
} // namespace CRC64

namespace RPL {
static constexpr uint8_t FRAME_START_BYTE = 0xA5; ///< 帧起始字节
static constexpr size_t FRAME_HEADER_SIZE = 7;    ///< 帧头大小（字节）
static constexpr size_t FRAME_TAIL_SIZE = 2;      ///< 帧尾大小（字节）

/// CRC8: poly=0x31, init=0xFF, 输入/输出反射 — 与裁判系统协议一致
using ProtocolCRC8 = crc_utils::crc<uint8_t, 0x31, 0xFF, true, true, 0x00>;
/// CRC16: CRC-16/MCRF4XX, poly=0x1021, init=0xFFFF, 输入/输出反射 —
/// 与裁判系统协议一致
using ProtocolCRC16 = CRC16::MCRF4XX;

} // namespace RPL

/**
 * @file Deserializer.hpp
 * @brief RPL库的反序列化器实现
 *
 * 此文件包含Deserializer类的定义，该类用于从字节数组中反序列化数据包结构。
 * 使用内存池来存储反序列化的数据包。
 *
 * @author WindWeaver
 */

/**
 * @file MemoryPool.hpp
 * @brief RPL库的静态内存池实现
 *
 * 此文件包含 MemoryPool 类的定义，用于管理反序列化数据的内存分配。
 * 内存池通过预分配固定大小的内存块来避免动态分配，提高性能。
 *
 * MemoryPool 是一个简单的静态内存分配器，它在编译期根据 Collector
 * 提供的 totalSize 常量预分配内存。这种设计避免了运行时动态内存分配，
 * 非常适合嵌入式系统。
 *
 * @par 使用场景
 * - Deserializer 类用于存储反序列化后的数据包
 * - 需要固定内存地址且无动态分配的场景
 *
 * @author WindWeaver
 */

namespace RPL::Containers
{
    /**
     * @brief 静态内存池结构体
     *
     * 用于管理反序列化数据的内存分配，通过预分配固定大小的内存块来避免动态分配。
     * 内存池的大小在编译期由 Collector::totalSize 确定。
     *
     * @tparam Collector 用于收集包信息的类型，必须提供 totalSize 静态常量
     *
     * @note 内存缓冲区使用 std::max_align_t 对齐，以确保任何数据类型的访问都是安全的
     */
    template <typename Collector>
    struct MemoryPool
    {
        /**
         * @brief 预分配的内存缓冲区
         *
         * 使用 std::max_align_t 对齐的静态内存缓冲区，
         * 大小由 Collector::totalSize 在编译期确定。
         */
        alignas(std::max_align_t) std::array<std::byte, Collector::totalSize> buffer{};  ///< 预分配的内存缓冲区
    };
}

/**
 * @file BitstreamParser.hpp
 * @brief RPL 位流解析器实现
 *
 * 此文件提供位流反序列化功能，可以从字节流中提取位域数据并构造成结构体。
 * 支持跨字节位提取、编译期优化和 C++20 聚合初始化。
 *
 * @par 设计原理
 * - 使用编译期位偏移计算，运行时仅执行高效的位操作
 * - 支持小端线格式 (little-endian wire format) 的位提取
 * - 利用 C++20 括号省略特性初始化含有 C 数组的结构体
 *
 * @par 使用场景
 * - 紧凑位域协议的解析（如遥控器协议）
 * - 需要节省带宽的嵌入式通信
 *
 * @author WindWeaver
 */

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

/**
 * @file PacketTraits.hpp
 * @brief RPL库的数据包特性定义
 *
 * 此文件定义了数据包特性的基类和模板特化结构，
 * 用于描述数据包的命令码、大小、协议格式等元信息。
 *
 * @par 核心组件
 * - DefaultProtocol: 默认 RoboMaster 协议定义
 * - PacketTraitsBase: 数据包特性基类
 * - PacketTraits<T>: 数据包特性模板（需用户特化）
 *
 * @par 设计原理
 * - 通过模板特化在编译期定义数据包属性
 * - 支持自定义协议覆盖默认设置
 * - 提供扩展点（如 before_get_custom）
 *
 * @author WindWeaver
 */

namespace RPL::Meta {
/**
 * @brief 默认 RoboMaster 协议定义
 *
 * 定义标准 RoboMaster 裁判系统协议的特征。
 * 用户可以通过在 PacketTraits 特化中定义 `using Protocol = MyProtocol;`
 * 来覆盖此默认设置。
 *
 * @par 协议格式
 * ```
 * | Start Byte | Length | Seq | Header CRC | Cmd ID | Data | Frame CRC16 |
 * | 1 byte     | 2 bytes| 1B  | 1 byte     | 2 bytes| N bytes | 2 bytes   |
 * ```
 *
 * @par 使用示例
 * @code
 * // 自定义协议
 * struct MyProtocol {
 *     static constexpr uint8_t start_byte = 0xAA;
 *     static constexpr bool has_second_byte = true;
 *     static constexpr uint8_t second_byte = 0x55;
 *     // ... 其他设置
 * };
 * 
 * // 在 PacketTraits 特化中使用
 * template <> struct PacketTraits<MyPacket> : PacketTraitsBase<MyPacket> {
 *     using Protocol = MyProtocol;
 *     // ...
 * };
 * @endcode
 */
struct DefaultProtocol {
  // --- 帧头识别 ---
  static constexpr uint8_t start_byte = 0xA5;    ///< 起始字节（帧同步标识）
  static constexpr bool has_second_byte = false; ///< 是否有第二个固定字节
  static constexpr uint8_t second_byte =
      0x00; ///< 第二个固定字节（如果 has_second_byte 为 false 则忽略）

  // --- 结构大小 ---
  static constexpr size_t header_size = 7; ///< 帧头总长度（字节）
  static constexpr size_t tail_size = 2;   ///< 帧尾长度（通常为CRC16）

  // --- 校验 ---
  static constexpr bool has_header_crc = true;   ///< 是否有帧头校验（CRC8）
  static constexpr size_t header_crc_offset = 4; ///< 帧头校验字节偏移量

  // --- 序列号 ---
  static constexpr bool has_seq_field = true;    ///< 是否有序列号字段
  static constexpr size_t seq_offset = 3;        ///< 序列号偏移量（字节）

  // --- CRC类型定义 ---
  /**
   * @brief 整包校验使用的CRC算法类型
   *
   * 必须提供静态 calc 函数: static uint16_t calc(const void* data, size_t len,
   * uint16_t init = ...); 默认使用 RoboMaster 标准 CRC16
   */
  using RPL_CRC = RPL::ProtocolCRC16;

  // --- 长度获取策略 ---
  /**
   * @brief 是否在头部包含数据长度字段
   *
   * - true: 解析器从头部 length_offset 处读取长度 (RoboMaster 协议)
   * - false: 解析器直接使用 PacketTraits::size 作为固定长度 (新遥控器协议)
   */
  static constexpr bool has_length_field = true;
  static constexpr size_t length_offset = 1;      ///< 长度字段在帧头的偏移量
  static constexpr size_t length_field_bytes = 2; ///< 长度字段占用的字节数

  // --- 命令码获取策略 ---
  /**
   * @brief 是否在头部包含命令码字段
   *
   * - true: 解析器从头部 cmd_offset 处读取命令码，用于分发
   * - false: 解析器使用 PacketTraits::cmd 作为隐式命令码 (单包协议或固定映射)
   */
  static constexpr bool has_cmd_field = true;
  static constexpr size_t cmd_offset = 5;      ///< 命令码在帧头的偏移量
  static constexpr size_t cmd_field_bytes = 2; ///< 命令码字段占用的字节数
};

/**
 * @brief 数据包特性基类
 *
 * 提供数据包特性的基础实现，包括命令码、大小和获取前的处理。
 * 用户的数据包类型应继承此类并特化相应的模板参数。
 *
 * @tparam Derived 派生类类型（CRTP 模式）
 *
 * @par 使用示例
 * @code
 * struct MyPacket {
 *     uint8_t flags;
 *     int16_t value;
 * };
 * 
 * template <>
 * struct PacketTraits<MyPacket> : PacketTraitsBase<MyPacket> {
 *     static constexpr uint16_t cmd = 0x0102;
 *     static constexpr size_t size = sizeof(MyPacket);
 * };
 * @endcode
 */
template <typename Derived> struct PacketTraitsBase {
  /// @brief 命令码（由派生类定义）
  static constexpr uint16_t cmd = Derived::cmd;
  /// @brief 数据包大小（由派生类定义）
  static constexpr size_t size = Derived::size;

  /// @brief 默认使用 RoboMaster 协议，派生类可通过 `using Protocol = MyProtocol;` 覆盖
  using Protocol = DefaultProtocol;

  /// @brief 是否跳过写入 Deserializer 的 MemoryPool（默认 false）
  /// @note 若设为 true，Parser 解析成功后不会将该包拷贝到 MemoryPool，
  ///       适合已通过 after_parse 回调直接消费数据的场景。
  static constexpr bool skip_memory_pool = false;

  /**
   * @brief 获取数据包前的处理
   *
   * 在获取数据包之前执行的处理函数。
   * 如果派生类定义了 before_get_custom 则调用它，可用于数据修复或转换。
   *
   * @param data 指向数据的指针
   */
  static void before_get(uint8_t *data) {
    // 使用不同的方法名避免递归
    if constexpr (requires { Derived::before_get_custom(data); }) {
      Derived::before_get_custom(data);
    }
    // 如果没有定义 before_get_custom，则什么都不做
  }
};

/**
 * @brief 数据包特性模板
 *
 * 用于特化各种数据包类型的特性，包括命令码和大小。
 * 用户需要为每个数据包类型提供此模板的特化。
 *
 * @tparam T 数据包类型
 *
 * @par 特化要求
 * - 必须定义 `cmd` 静态常量（命令码）
 * - 必须定义 `size` 静态常量（数据包大小）
 * - 可选定义 `BitLayout` 类型（用于位流序列化/反序列化）
 * - 可选定义 `after_parse` 函数（解析完成后回调，接收 const T&）
 * - 可选定义 `skip_memory_pool` 静态常量（跳过写入 MemoryPool）
 * - 可选定义 `before_get_custom` 函数（获取前处理）
 *
 * @par 完整特化示例
 * @code
 * struct MyPacket {
 *     uint8_t flags;
 *     int16_t channels[8];
 * };
 * 
 * template <>
 * struct PacketTraits<MyPacket> : PacketTraitsBase<MyPacket> {
 *     static constexpr uint16_t cmd = 0x0102;
 *     static constexpr size_t size = sizeof(MyPacket);
 *     
 *     // 可选：定义位流布局
 *     using BitLayout = std::tuple<
 *         Field<uint8_t, 3>,
 *         Field<int16_t, 16, 8>
 *     >;
 * };
 * @endcode
 */
template <typename T> struct PacketTraits;
} // namespace RPL::Meta

namespace RPL::Detail {

/**
 * @brief 在特定位偏移处从字节序列中提取指定位数
 *
 * 此函数处理跨字节位提取，采用小端线格式假设。
 * 由于 BitOffset 和 BitWidth 是编译时常量，编译器会将此优化
 * 为高效的位操作。
 *
 * @tparam T 返回类型 (整数或 std::array)
 * @tparam BitOffset 起始位索引 (0 是第一个字节的 LSB)
 * @tparam BitWidth 要提取的位数
 * @param buffer 要读取的字节序列
 * @return 提取的值并转换为类型 T
 *
 * @note 此函数是位流解析的核心，支持跨越字节边界的位提取
 * @warning 如果 BitWidth 超过 T 的容量，将触发 static_assert
 */
template <typename T, std::size_t BitOffset, std::size_t BitWidth>
constexpr T extract_bits(std::span<const uint8_t> buffer) {
    if constexpr (Meta::is_std_array_v<T>) {
        T result{};
        using ElementType = typename T::value_type;
        constexpr std::size_t N = std::tuple_size_v<T>;
        constexpr std::size_t bits_per_element = BitWidth / N;
        static_assert(bits_per_element * N == BitWidth, "BitWidth must be a multiple of array size");
        
        [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            ((result[Is] = extract_bits<ElementType, BitOffset + Is * bits_per_element, bits_per_element>(buffer)), ...);
        }(std::make_index_sequence<N>{});
        return result;
    } else {
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
            std::size_t bits_to_take = std::min(BitWidth - bits_extracted, static_cast<std::size_t>(8 - bit_in_byte));

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
}

/**
 * @brief 将位流布局解析为元组的核心实现
 *
 * 根据编译期的位布局定义，从字节缓冲区中提取所有位域并返回元组。
 * 使用前缀和算法在编译期计算每个字段的位偏移。
 *
 * @tparam Layout 位布局定义（元组 Field 类型）
 * @tparam Is 索引序列（用于展开折叠表达式）
 * @param buffer 包含线格式数据的字节序列
 * @return 包含所有提取值的元组
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

/**
 * @brief 辅助函数：将单个元素包装成 tuple，如果是 std::array 则展开
 *
 * @tparam T 元素类型
 * @param val 要包装的值
 * @return 包含元素的 tuple，如果是 std::array 则展开为多个元素
 */
template <typename T>
constexpr auto flatten_element(T&& val) {
    return std::make_tuple(std::forward<T>(val));
}

/**
 * @brief 辅助函数：将 std::array 元素展开为 tuple
 *
 * @tparam T 数组元素类型
 * @tparam N 数组大小
 * @param arr 要展开的数组
 * @return 包含数组所有元素的 tuple
 */
template <typename T, std::size_t N>
constexpr auto flatten_element(const std::array<T, N>& arr) {
    return [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        return std::make_tuple(arr[Is]...);
    }(std::make_index_sequence<N>{});
}

/**
 * @brief 将包含 std::array 的 tuple 打平为全标量 tuple
 *
 * 这样可以利用 C++20 的大括号省略特性初始化含有 C 数组的结构体。
 * 例如，tuple<int, std::array<float, 3>> 会被打平为 tuple<int, float, float, float>。
 *
 * @tparam Ts 元组元素类型
 * @param t 要打平的元组
 * @return 打平后的元组（所有 std::array 被展开）
 */
template <typename... Ts>
constexpr auto flatten_tuple(const std::tuple<Ts...>& t) {
    return std::apply([](const auto&... args) {
        return std::tuple_cat(flatten_element(args)...);
    }, t);
}

} // namespace RPL::Detail

namespace RPL {

/**
 * @brief 使用位流布局定义反序列化数据包
 *
 * 从缓冲区中提取位域并安全地构造目标结构 T。
 * 使用 C++20 括号聚合初始化，支持含有 C 数组的结构体。
 *
 * @tparam T 目标结构类型（必须有 BitLayout 特化）
 * @param buffer 包含线格式数据的字节序列
 * @return 正确初始化位域的结构
 *
 * @par 使用示例
 * @code
 * struct MyPacket {
 *     uint8_t flags;
 *     int16_t channels[8];
 * };
 * 
 * // 在 PacketTraits 特化中定义 BitLayout...
 * 
 * auto packet = RPL::deserialize_bitstream<MyPacket>(buffer);
 * @endcode
 *
 * @note 此函数要求 Meta::HasBitLayout<Meta::PacketTraits<T>> 为 true
 */
template <typename T>
requires Meta::HasBitLayout<Meta::PacketTraits<T>>
constexpr T deserialize_bitstream(std::span<const uint8_t> buffer) {
    using Layout = typename Meta::PacketTraits<T>::BitLayout;
    constexpr std::size_t N = std::tuple_size_v<Layout>;

    // 1. 根据编译期布局将值提取到元组中 (如果是数组字段，这里就是一个 std::array)
    auto values_tuple = Detail::parse_bitstream_impl<Layout>(
        buffer, std::make_index_sequence<N>{}
    );

    // 2. 直接使用 C++20 聚合初始化赋值
    // 如果 T 的成员是 std::array，它能完美接收元组中的 std::array 元素
    return std::make_from_tuple<T>(values_tuple);
}

} // namespace RPL

/**
 * @file PacketInfoCollector.hpp
 * @brief RPL库的数据包信息收集器
 *
 * 此文件定义了数据包信息收集器，用于管理数据包的元信息，
 * 包括总大小、命令码到索引的映射等。
 *
 * PacketInfoCollector 在编译期计算所有数据包的内存布局，
 * 考虑对齐要求，并生成高效的查找表（使用 frozen 库）。
 *
 * @par 设计原理
 * - 使用编译期计算避免运行时开销
 * - 考虑内存对齐以确保安全访问
 * - 使用 frozen::unordered_map 提供高效的编译期查找表
 *
 * @author WindWeaver
 */

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#if defined(FROZEN_NO_EXCEPTIONS) || (defined(_MSC_VER) && !defined(_CPPUNWIND)) || (!defined(_MSC_VER) && !defined(__cpp_exceptions))

#define FROZEN_THROW_OR_ABORT(_) std::abort()

#else

#define FROZEN_THROW_OR_ABORT(err) throw err

#endif

namespace frozen {

namespace bits {

// used as a fake argument for frozen::make_set and frozen::make_map in the case of N=0
struct ignored_arg {};

template <class T, std::size_t N>
class cvector {
  T data [N] = {}; // zero-initialization for scalar type T, default-initialized otherwise
  std::size_t dsize = 0;

public:
  // Container typdefs
  using value_type = T;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // Constructors
  constexpr cvector(void) = default;
  constexpr cvector(size_type count, const T& value) : dsize(count) {
    for (std::size_t i = 0; i < N; ++i)
      data[i] = value;
  }

  // Iterators
  constexpr       iterator begin() noexcept { return data; }
  constexpr       iterator end() noexcept { return data + dsize; }
  constexpr const_iterator begin() const noexcept { return data; }
  constexpr const_iterator end() const noexcept { return data + dsize; }

  // Capacity
  constexpr size_type size() const { return dsize; }

  // Element access
  constexpr       reference operator[](std::size_t index) { return data[index]; }
  constexpr const_reference operator[](std::size_t index) const { return data[index]; }

  constexpr       reference back() { return data[dsize - 1]; }
  constexpr const_reference back() const { return data[dsize - 1]; }

  // Modifiers
  constexpr void push_back(const T & a) { data[dsize++] = a; }
  constexpr void push_back(T && a) { data[dsize++] = std::move(a); }
  constexpr void pop_back() { --dsize; }

  constexpr void clear() { dsize = 0; }
};

template <class T, std::size_t N>
class carray {
  T data_ [N] = {}; // zero-initialization for scalar type T, default-initialized otherwise

  template <class Iter, std::size_t... I>
  constexpr carray(Iter iter, std::index_sequence<I...>)
      : data_{((void)I, *iter++)...} {}
  template <std::size_t... I>
  constexpr carray(const T& value, std::index_sequence<I...>)
      : data_{((void)I, value)...} {}

public:
  // Container typdefs
  using value_type = T;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // Constructors
  constexpr carray() = default;
  constexpr carray(const value_type& val)
    : carray(val, std::make_index_sequence<N>()) {}
  template <typename U, std::enable_if_t<std::is_convertible<U, T>::value, std::size_t> M>
  constexpr carray(U const (&init)[M])
    : carray(init, std::make_index_sequence<N>())
  {
    static_assert(M >= N, "Cannot initialize a carray with an smaller array");
  }
  template <typename U, std::enable_if_t<std::is_convertible<U, T>::value, std::size_t> M>
  constexpr carray(std::array<U, M> const &init)
    : carray(init.begin(), std::make_index_sequence<N>())
  {
    static_assert(M >= N, "Cannot initialize a carray with an smaller array");
  }
  template <typename U, std::enable_if_t<std::is_convertible<U, T>::value>* = nullptr>
  constexpr carray(std::initializer_list<U> init)
    : carray(init.begin(), std::make_index_sequence<N>())
  {
    // clang & gcc doesn't recognize init.size() as a constexpr
    // static_assert(init.size() >= N, "Cannot initialize a carray with an smaller initializer list");
  }
  template <typename U, std::enable_if_t<std::is_convertible<U, T>::value>* = nullptr>
  constexpr carray(const carray<U, N>& rhs)
    : carray(rhs.begin(), std::make_index_sequence<N>())
  {
  }

  // Iterators
  constexpr iterator begin() noexcept { return data_; }
  constexpr const_iterator begin() const noexcept { return data_; }
  constexpr iterator end() noexcept { return data_ + N; }
  constexpr const_iterator end() const noexcept { return data_ + N; }

  // Capacity
  constexpr size_type size() const { return N; }
  constexpr size_type max_size() const { return N; }

  // Element access
  constexpr       reference operator[](std::size_t index) { return data_[index]; }
  constexpr const_reference operator[](std::size_t index) const { return data_[index]; }

  constexpr       reference at(std::size_t index) {
    if (index > N)
      FROZEN_THROW_OR_ABORT(std::out_of_range("Index (" + std::to_string(index) + ") out of bound (" + std::to_string(N) + ')'));
    return data_[index];
  }
  constexpr const_reference at(std::size_t index) const {
    if (index > N)
      FROZEN_THROW_OR_ABORT(std::out_of_range("Index (" + std::to_string(index) + ") out of bound (" + std::to_string(N) + ')'));
    return data_[index];
  }

  constexpr       reference front() { return data_[0]; }
  constexpr const_reference front() const { return data_[0]; }

  constexpr       reference back() { return data_[N - 1]; }
  constexpr const_reference back() const { return data_[N - 1]; }

  constexpr       value_type* data() noexcept { return data_; }
  constexpr const value_type* data() const noexcept { return data_; }
};
template <class T>
class carray<T, 0> {

public:
  // Container typdefs
  using value_type = T;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  // Constructors
  constexpr carray(void) = default;

};

} // namespace bits

} // namespace frozen

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifdef _MSC_VER

// FIXME: find a way to implement that correctly for msvc
#define constexpr_assert(cond, msg)

#else

#define constexpr_assert(cond, msg)\
    assert(cond && msg);
#endif

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

namespace frozen {

template <class T = void> struct elsa {
  static_assert(std::is_integral<T>::value || std::is_enum<T>::value,
                "only supports integral types, specialize for other types");

  constexpr std::size_t operator()(T const &value, std::size_t seed) const {
    std::size_t key = seed ^ static_cast<std::size_t>(value);
    key = (~key) + (key << 21); // key = (key << 21) - key - 1;
    key = key ^ (key >> 24);
    key = (key + (key << 3)) + (key << 8); // key * 265
    key = key ^ (key >> 14);
    key = (key + (key << 2)) + (key << 4); // key * 21
    key = key ^ (key >> 28);
    key = key + (key << 31);
    return key;
  }
};

template <> struct elsa<void> {
  template<class T>
  constexpr std::size_t operator()(T const &value, std::size_t seed) const {
    return elsa<T>{}(value, seed);
  }
};

template <class T=void> using anna = elsa<T>;
} // namespace frozen

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

// inspired from http://stevehanov.ca/blog/index.php?id=119

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

namespace frozen {

namespace bits {

auto constexpr next_highest_power_of_two(std::size_t v) {
  // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  constexpr auto trip_count = std::numeric_limits<decltype(v)>::digits;
  v--;
  for(std::size_t i = 1; i < trip_count; i <<= 1)
    v |= v >> i;
  v++;
  return v;
}

template<class T>
auto constexpr log(T v) {
  std::size_t n = 0;
  while (v > 1) {
    n += 1;
    v >>= 1;
  }
  return n;
}

constexpr std::size_t bit_weight(std::size_t n) {
  return (n <= 8*sizeof(unsigned int))
    + (n <= 8*sizeof(unsigned long))
    + (n <= 8*sizeof(unsigned long long))
    + (n <= 128);
}

unsigned int select_uint_least(std::integral_constant<std::size_t, 4>);
unsigned long select_uint_least(std::integral_constant<std::size_t, 3>);
unsigned long long select_uint_least(std::integral_constant<std::size_t, 2>);
template<std::size_t N>
unsigned long long select_uint_least(std::integral_constant<std::size_t, N>) {
  static_assert(N < 2, "unsupported type size");
  return {};
}

template<std::size_t N>
using select_uint_least_t = decltype(select_uint_least(std::integral_constant<std::size_t, bit_weight(N)>()));

template <typename Iter, typename Compare>
constexpr auto min_element(Iter begin, const Iter end,
                           Compare const &compare) {
  auto result = begin;
  while (begin != end) {
    if (compare(*begin, *result)) {
      result = begin;
    }
    ++begin;
  }
  return result;
}

template <class T>
constexpr void cswap(T &a, T &b) {
  auto tmp = a;
  a = b;
  b = tmp;
}

template <class T, class U>
constexpr void cswap(std::pair<T, U> & a, std::pair<T, U> & b) {
  cswap(a.first, b.first);
  cswap(a.second, b.second);
}

template <class... Tys, std::size_t... Is>
constexpr void cswap(std::tuple<Tys...> &a, std::tuple<Tys...> &b, std::index_sequence<Is...>) {
  using swallow = int[];
  (void) swallow{(cswap(std::get<Is>(a), std::get<Is>(b)), 0)...};
}

template <class... Tys>
constexpr void cswap(std::tuple<Tys...> &a, std::tuple<Tys...> &b) {
  cswap(a, b, std::make_index_sequence<sizeof...(Tys)>());
}

template <typename Iter>
constexpr void iter_swap(Iter a, Iter b) {
  cswap(*a, *b);
}

template <typename Iterator, class Compare>
constexpr Iterator partition(Iterator left, Iterator right, Compare const &compare) {
  auto pivot = left + (right - left) / 2;
  iter_swap(right, pivot);
  pivot = right;
  for (auto it = left; 0 < right - it; ++it) {
    if (compare(*it, *pivot)) {
      iter_swap(it, left);
      left++;
    }
  }
  iter_swap(pivot, left);
  pivot = left;
  return pivot;
}

template <typename Iterator, class Compare>
constexpr void quicksort(Iterator left, Iterator right, Compare const &compare) {
  while (0 < right - left) {
    auto new_pivot = bits::partition(left, right, compare);
    quicksort(left, new_pivot, compare);
    left = new_pivot + 1;
  }
}

template <typename Container, class Compare>
constexpr Container quicksort(Container const &array,
                                     Compare const &compare) {
  Container res = array;
  quicksort(res.begin(), res.end() - 1, compare);
  return res;
}

template <class T, class Compare> struct LowerBound {
  T const &value_;
  Compare const &compare_;
  constexpr LowerBound(T const &value, Compare const &compare)
      : value_(value), compare_(compare) {}

  template <class ForwardIt>
  inline constexpr ForwardIt doit_fast(ForwardIt first,
                                  std::integral_constant<std::size_t, 0>) {
    return first;
  }

  template <class ForwardIt, std::size_t N>
  inline constexpr ForwardIt doit_fast(ForwardIt first,
                                  std::integral_constant<std::size_t, N>) {
    auto constexpr step = N / 2;
    static_assert(N/2 == N - N / 2 - 1, "power of two minus 1");
    auto it = first + step;
    auto next_it = compare_(*it, value_) ? it + 1 : first;
    return doit_fast(next_it, std::integral_constant<std::size_t, N / 2>{});
  }

  template <class ForwardIt, std::size_t N>
  inline constexpr ForwardIt doitfirst(ForwardIt first, std::integral_constant<std::size_t, N>, std::integral_constant<bool, true>) {
    return doit_fast(first, std::integral_constant<std::size_t, N>{});
  }

  template <class ForwardIt, std::size_t N>
  inline constexpr ForwardIt doitfirst(ForwardIt first, std::integral_constant<std::size_t, N>, std::integral_constant<bool, false>) {
    auto constexpr next_power = next_highest_power_of_two(N);
    auto constexpr next_start = next_power / 2 - 1;
    auto it = first + next_start;
    if (compare_(*it, value_)) {
      auto constexpr next = N - next_start - 1;
      return doitfirst(it + 1, std::integral_constant<std::size_t, next>{}, std::integral_constant<bool, next_highest_power_of_two(next) - 1 == next>{});
    }
    else
      return doit_fast(first, std::integral_constant<std::size_t, next_start>{});
  }

  template <class ForwardIt>
  inline constexpr ForwardIt doitfirst(ForwardIt first, std::integral_constant<std::size_t, 1>, std::integral_constant<bool, false>) {
    return doit_fast(first, std::integral_constant<std::size_t, 1>{});
  }
};

template <std::size_t N, class ForwardIt, class T, class Compare>
constexpr ForwardIt lower_bound(ForwardIt first, const T &value, Compare const &compare) {
  return LowerBound<T, Compare>{value, compare}.doitfirst(first, std::integral_constant<std::size_t, N>{}, std::integral_constant<bool, next_highest_power_of_two(N) - 1 == N>{});
}

template <std::size_t N, class Compare, class ForwardIt, class T>
constexpr bool binary_search(ForwardIt first, const T &value,
                             Compare const &compare) {
  ForwardIt where = lower_bound<N>(first, value, compare);
  return (!(where == first + N) && !(compare(value, *where)));
}

template<class InputIt1, class InputIt2>
constexpr bool equal(InputIt1 first1, InputIt1 last1, InputIt2 first2)
{
  for (; first1 != last1; ++first1, ++first2) {
    if (!(*first1 == *first2)) {
      return false;
    }
  }
  return true;
}

template<class InputIt1, class InputIt2>
constexpr bool lexicographical_compare(InputIt1 first1, InputIt1 last1, InputIt2 first2, InputIt2 last2)
{
  for (; (first1 != last1) && (first2 != last2); ++first1, ++first2) {
    if (*first1 < *first2)
      return true;
    if (*first2 < *first1)
      return false;
  }
  return (first1 == last1) && (first2 != last2);
}

} // namespace bits
} // namespace frozen

namespace frozen {

namespace bits {

// Function object for sorting buckets in decreasing order of size
struct bucket_size_compare {
  template <typename B>
  bool constexpr operator()(B const &b0,
                            B const &b1) const {
    return b0.size() > b1.size();
  }
};

// Step One in pmh routine is to take all items and hash them into buckets,
// with some collisions. Then process those buckets further to build a perfect
// hash function.
// pmh_buckets represents the initial placement into buckets.

template <std::size_t M>
struct pmh_buckets {
  // Step 0: Bucket max is 2 * sqrt M
  // TODO: Come up with justification for this, should it not be O(log M)?
  static constexpr auto bucket_max = 2 * (1u << (log(M) / 2));

  using bucket_t = cvector<std::size_t, bucket_max>;
  carray<bucket_t, M> buckets;
  std::uint64_t seed;

  // Represents a reference to a bucket. This is used because the buckets
  // have to be sorted, but buckets are big, making it slower than sorting refs
  struct bucket_ref {
    unsigned hash;
    const bucket_t * ptr;

    // Forward some interface of bucket
    using value_type = typename bucket_t::value_type;
    using const_iterator = typename bucket_t::const_iterator;

    constexpr auto size() const { return ptr->size(); }
    constexpr const auto & operator[](std::size_t idx) const { return (*ptr)[idx]; }
    constexpr auto begin() const { return ptr->begin(); }
    constexpr auto end() const { return ptr->end(); }
  };

  // Make a bucket_ref for each bucket
  template <std::size_t... Is>
  carray<bucket_ref, M> constexpr make_bucket_refs(std::index_sequence<Is...>) const {
    return {{ bucket_ref{Is, &buckets[Is]}... }};
  }

  // Makes a bucket_ref for each bucket and sorts them by size
  carray<bucket_ref, M> constexpr get_sorted_buckets() const {
    carray<bucket_ref, M> result{this->make_bucket_refs(std::make_index_sequence<M>())};
    bits::quicksort(result.begin(), result.end() - 1, bucket_size_compare{});
    return result;
  }
};

template <std::size_t M, class Item, std::size_t N, class Hash, class Key, class PRG>
pmh_buckets<M> constexpr make_pmh_buckets(const carray<Item, N> & items,
                                Hash const & hash,
                                Key const & key,
                                PRG & prg) {
  using result_t = pmh_buckets<M>;
  // Continue until all items are placed without exceeding bucket_max
  while (1) {
    result_t result{};
    result.seed = prg();
    bool rejected = false;
    for (std::size_t i = 0; i < items.size(); ++i) {
      auto & bucket = result.buckets[hash(key(items[i]), static_cast<std::size_t>(result.seed)) % M];
      if (bucket.size() >= result_t::bucket_max) {
        rejected = true;
        break;
      }
      bucket.push_back(i);
    }
    if (!rejected) { return result; }
  }
}

// Check if an item appears in a cvector
template<class T, std::size_t N>
constexpr bool all_different_from(cvector<T, N> & data, T & a) {
  for (std::size_t i = 0; i < data.size(); ++i)
    if (data[i] == a)
      return false;

  return true;
}

// Represents either an index to a data item array, or a seed to be used with
// a hasher. Seed must have high bit of 1, value has high bit of zero.
struct seed_or_index {
  using value_type = std::uint64_t;

private:
  static constexpr value_type MINUS_ONE = std::numeric_limits<value_type>::max();
  static constexpr value_type HIGH_BIT = ~(MINUS_ONE >> 1);

  value_type value_ = 0;

public:
  constexpr value_type value() const { return value_; }
  constexpr bool is_seed() const { return value_ & HIGH_BIT; }

  constexpr seed_or_index(bool is_seed, value_type value)
    : value_(is_seed ? (value | HIGH_BIT) : (value & ~HIGH_BIT)) {}

  constexpr seed_or_index() = default;
  constexpr seed_or_index(const seed_or_index &) = default;
  constexpr seed_or_index & operator =(const seed_or_index &) = default;
};

// Represents the perfect hash function created by pmh algorithm
template <std::size_t M, class Hasher>
struct pmh_tables : private Hasher {
  std::uint64_t first_seed_;
  carray<seed_or_index, M> first_table_;
  carray<std::size_t, M> second_table_;

  constexpr pmh_tables(
      std::uint64_t first_seed,
      carray<seed_or_index, M> first_table,
      carray<std::size_t, M> second_table,
      Hasher hash) noexcept
    : Hasher(hash)
    , first_seed_(first_seed)
    , first_table_(first_table)
    , second_table_(second_table)
  {}

  constexpr Hasher const& hash_function() const noexcept {
    return static_cast<Hasher const&>(*this);
  }

  template <typename KeyType>
  constexpr std::size_t lookup(const KeyType & key) const {
    return lookup(key, hash_function());
  }

  // Looks up a given key, to find its expected index in carray<Item, N>
  // Always returns a valid index, must use KeyEqual test after to confirm.
  template <typename KeyType, typename HasherType>
  constexpr std::size_t lookup(const KeyType & key, const HasherType& hasher) const {
    auto const d = first_table_[hasher(key, static_cast<std::size_t>(first_seed_)) % M];
    if (!d.is_seed()) { return static_cast<std::size_t>(d.value()); } // this is narrowing std::uint64 -> std::size_t but should be fine
    else { return second_table_[hasher(key, static_cast<std::size_t>(d.value())) % M]; }
  }
};

// Make pmh tables for given items, hash function, prg, etc.
template <std::size_t M, class Item, std::size_t N, class Hash, class Key, class PRG>
pmh_tables<M, Hash> constexpr make_pmh_tables(const carray<Item, N> &
                                                               items,
                                                           Hash const &hash,
                                                           Key const &key,
                                                           PRG prg) {
  // Step 1: Place all of the keys into buckets
  auto step_one = make_pmh_buckets<M>(items, hash, key, prg);

  // Step 2: Sort the buckets to process the ones with the most items first.
  auto buckets = step_one.get_sorted_buckets();

  // Special value for unused slots. This is purposefully the index
  // one-past-the-end of 'items' to function as a sentinel value. Both to avoid
  // the need to apply the KeyEqual predicate and to be easily convertible to
  // end().
  // Unused entries in both hash tables (G and H) have to contain this value.
  const auto UNUSED = items.size();

  // G becomes the first hash table in the resulting pmh function
  carray<seed_or_index, M> G({false, UNUSED});

  // H becomes the second hash table in the resulting pmh function
  carray<std::size_t, M> H(UNUSED);

  // Step 3: Map the items in buckets into hash tables.
  for (const auto & bucket : buckets) {
    auto const bsize = bucket.size();

    if (bsize == 1) {
      // Store index to the (single) item in G
      // assert(bucket.hash == hash(key(items[bucket[0]]), step_one.seed) % M);
      G[bucket.hash] = {false, static_cast<std::uint64_t>(bucket[0])};
    } else if (bsize > 1) {

      // Repeatedly try different H of d until we find a hash function
      // that places all items in the bucket into free slots
      seed_or_index d{true, prg()};
      cvector<std::size_t, decltype(step_one)::bucket_max> bucket_slots;

      while (bucket_slots.size() < bsize) {
        auto slot = hash(key(items[bucket[bucket_slots.size()]]), static_cast<std::size_t>(d.value())) % M;

        if (H[slot] != UNUSED || !all_different_from(bucket_slots, slot)) {
          bucket_slots.clear();
          d = {true, prg()};
          continue;
        }

        bucket_slots.push_back(slot);
      }

      // Put successful seed in G, and put indices to items in their slots
      // assert(bucket.hash == hash(key(items[bucket[0]]), step_one.seed) % M);
      G[bucket.hash] = d;
      for (std::size_t i = 0; i < bsize; ++i)
        H[bucket_slots[i]] = bucket[i];
    }
  }

  return {step_one.seed, G, H, hash};
}

} // namespace bits

} // namespace frozen

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#define FROZEN_MAJOR_VERSION 1
#define FROZEN_MINOR_VERSION 1
#define FROZEN_PATCH_VERSION 1

/*
 * Frozen
 * Copyright 2016 QuarksLab
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

namespace frozen {
template <class UIntType, UIntType a, UIntType c, UIntType m>
class linear_congruential_engine {

  static_assert(std::is_unsigned<UIntType>::value,
                "UIntType must be an unsigned integral type");

  template<class T>
  static constexpr UIntType modulo(T val, std::integral_constant<UIntType, 0>) {
    return static_cast<UIntType>(val);
  }

  template<class T, UIntType M>
  static constexpr UIntType modulo(T val, std::integral_constant<UIntType, M>) {
    // the static cast below may end up doing a truncation
    return static_cast<UIntType>(val % M);
  }

public:
  using result_type = UIntType;
  static constexpr result_type multiplier = a;
  static constexpr result_type increment = c;
  static constexpr result_type modulus = m;
  static constexpr result_type default_seed = 1u;

  linear_congruential_engine() = default;
  constexpr linear_congruential_engine(result_type s) { seed(s); }

  void seed(result_type s = default_seed) { state_ = s; }
  constexpr result_type operator()() {
	  using uint_least_t = bits::select_uint_least_t<bits::log(a) + bits::log(m) + 4>;
    uint_least_t tmp = static_cast<uint_least_t>(multiplier) * state_ + increment;

    state_ = modulo(tmp, std::integral_constant<UIntType, modulus>());
    return state_;
  }
  constexpr void discard(unsigned long long n) {
    while (n--)
      operator()();
  }
  static constexpr result_type min() { return increment == 0u ? 1u : 0u; }
  static constexpr result_type max() { return modulus - 1u; }
  friend constexpr bool operator==(linear_congruential_engine const &self,
                                   linear_congruential_engine const &other) {
    return self.state_ == other.state_;
  }
  friend constexpr bool operator!=(linear_congruential_engine const &self,
                                   linear_congruential_engine const &other) {
    return !(self == other);
  }

private:
  result_type state_ = default_seed;
};

using minstd_rand0 =
    linear_congruential_engine<std::uint_fast32_t, 16807, 0, 2147483647>;
using minstd_rand =
    linear_congruential_engine<std::uint_fast32_t, 48271, 0, 2147483647>;

// This generator is used by default in unordered frozen containers
using default_prg_t = minstd_rand;

} // namespace frozen

namespace frozen {

namespace bits {

struct GetKey {
  template <class KV> constexpr auto const &operator()(KV const &kv) const {
    return kv.first;
  }
};

} // namespace bits

template <class Key, class Value, std::size_t N, typename Hash = anna<Key>,
          class KeyEqual = std::equal_to<Key>>
class unordered_map : private KeyEqual {
  static constexpr std::size_t storage_size =
      bits::next_highest_power_of_two(N) * (N < 32 ? 2 : 1); // size adjustment to prevent high collision rate for small sets
  using container_type = bits::carray<std::pair<const Key, Value>, N>;
  using tables_type = bits::pmh_tables<storage_size, Hash>;

  container_type items_;
  tables_type tables_;

public:
  /* typedefs */
  using Self = unordered_map<Key, Value, N, Hash, KeyEqual>;
  using key_type = Key;
  using mapped_type = Value;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;
  using difference_type = typename container_type::difference_type;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using reference = typename container_type::reference;
  using const_reference = typename container_type::const_reference;
  using pointer = typename container_type::pointer;
  using const_pointer = typename container_type::const_pointer;
  using iterator = typename container_type::iterator;
  using const_iterator = typename container_type::const_iterator;

public:
  /* constructors */
  unordered_map(unordered_map const &) = default;
  constexpr unordered_map(container_type items,
                          Hash const &hash, KeyEqual const &equal)
      : KeyEqual{equal}
      , items_{items}
      , tables_{
            bits::make_pmh_tables<storage_size>(
                items_, hash, bits::GetKey{}, default_prg_t{})} {}
  explicit constexpr unordered_map(container_type items)
      : unordered_map{items, Hash{}, KeyEqual{}} {}

  constexpr unordered_map(std::initializer_list<value_type> items,
                          Hash const & hash, KeyEqual const & equal)
      : unordered_map{container_type{items}, hash, equal} {
        constexpr_assert(items.size() == N, "Inconsistent initializer_list size and type size argument");
      }

  constexpr unordered_map(std::initializer_list<value_type> items)
      : unordered_map{items, Hash{}, KeyEqual{}} {}

  /* iterators */
  constexpr iterator begin() { return items_.begin(); }
  constexpr iterator end() { return items_.end(); }
  constexpr const_iterator begin() const { return items_.begin(); }
  constexpr const_iterator end() const { return items_.end(); }
  constexpr const_iterator cbegin() const { return items_.begin(); }
  constexpr const_iterator cend() const { return items_.end(); }

  /* capacity */
  constexpr bool empty() const { return !N; }
  constexpr size_type size() const { return N; }
  constexpr size_type max_size() const { return N; }

  /* lookup */
  template <class KeyType>
  constexpr std::size_t count(KeyType const &key) const {
    return find(key) != end();
  }

  template <class KeyType>
  constexpr Value const &at(KeyType const &key) const {
    return at_impl(*this, key);
  }
  template <class KeyType>
  constexpr Value &at(KeyType const &key) {
    return at_impl(*this, key);
  }

  template <class KeyType>
  constexpr const_iterator find(KeyType const &key) const {
    return find_impl(*this, key, hash_function(), key_eq());
  }
  template <class KeyType>
  constexpr iterator find(KeyType const &key) {
    return find_impl(*this, key, hash_function(), key_eq());
  }

  template <class KeyType>
  constexpr bool contains(KeyType const &key) const {
    return this->find(key) != this->end();
  }

  template <class KeyType>
  constexpr std::pair<const_iterator, const_iterator> equal_range(KeyType const &key) const {
    return equal_range_impl(*this, key);
  }
  template <class KeyType>
  constexpr std::pair<iterator, iterator> equal_range(KeyType const &key) {
    return equal_range_impl(*this, key);
  }

  /* bucket interface */
  constexpr std::size_t bucket_count() const { return storage_size; }
  constexpr std::size_t max_bucket_count() const { return storage_size; }

  /* observers*/
  constexpr const hasher& hash_function() const { return tables_.hash_function(); }
  constexpr const key_equal& key_eq() const { return static_cast<KeyEqual const&>(*this); }

private:
  template <class This, class KeyType>
  static inline constexpr auto& at_impl(This&& self, KeyType const &key) {
    auto it = self.find(key);
    if (it != self.end())
      return it->second;
    else
      FROZEN_THROW_OR_ABORT(std::out_of_range("unknown key"));
  }

  template <class This, class KeyType, class Hasher, class Equal>
  static inline constexpr auto find_impl(This&& self, KeyType const &key, Hasher const &hash, Equal const &equal) {
    auto const pos = self.tables_.lookup(key, hash);
    auto it = self.items_.begin() + pos;
    if (it != self.items_.end() && equal(it->first, key))
      return it;
    else
      return self.items_.end();
  }

  template <class This, class KeyType>
  static inline constexpr auto equal_range_impl(This&& self, KeyType const &key) {
    auto const it = self.find(key);
    if (it != self.end())
      return std::make_pair(it, it + 1);
    else
      return std::make_pair(self.end(), self.end());
  }
};

template <typename T, typename U, std::size_t N>
constexpr auto make_unordered_map(std::pair<T, U> const (&items)[N]) {
  return unordered_map<T, U, N>{items};
}

template <typename T, typename U, std::size_t N, typename Hasher, typename Equal>
constexpr auto make_unordered_map(
        std::pair<T, U> const (&items)[N],
        Hasher const &hash = elsa<T>{},
        Equal const &equal = std::equal_to<T>{}) {
  return unordered_map<T, U, N, Hasher, Equal>{items, hash, equal};
}

template <typename T, typename U, std::size_t N>
constexpr auto make_unordered_map(std::array<std::pair<T, U>, N> const &items) {
  return unordered_map<T, U, N>{items};
}

template <typename T, typename U, std::size_t N, typename Hasher, typename Equal>
constexpr auto make_unordered_map(
        std::array<std::pair<T, U>, N> const &items,
        Hasher const &hash = elsa<T>{},
        Equal const &equal = std::equal_to<T>{}) {
  return unordered_map<T, U, N, Hasher, Equal>{items, hash, equal};
}

} // namespace frozen

namespace RPL::Meta {
/**
 * @brief 编译期对齐计算辅助函数
 *
 * 将 offset 向上对齐到 alignment 的倍数。
 *
 * @param offset 当前偏移量
 * @param alignment 对齐要求（必须是 2 的幂）
 * @return 对齐后的偏移量
 */
constexpr size_t align_up(size_t offset, size_t alignment) {
  return (offset + alignment - 1) & ~(alignment - 1);
}

/**
 * @brief 数据包信息收集器
 *
 * 用于收集和管理数据包的元信息，包括总大小、命令码到索引的映射等。
 * 所有计算都在编译期完成，运行时零开销。
 *
 * @tparam Ts 数据包类型列表
 *
 * @par 主要功能
 * - 计算所有数据包在内存池中的布局（考虑对齐）
 * - 生成命令码到内存偏移量的映射表
 * - 生成命令码到序列索引的映射表（用于 SeqLock）
 *
 * @code
 * using Collector = RPL::Meta::PacketInfoCollector<PacketA, PacketB>;
 * constexpr size_t total_size = Collector::totalSize;
 * constexpr size_t offset = Collector::cmd_index(0x0102);
 * @endcode
 */
template <typename... Ts> struct PacketInfoCollector {
  /**
   * @brief 递归计算偏移量的辅助函数
   *
   * 遍历所有类型，计算每个类型在内存池中的对齐后偏移量。
   *
   * @tparam T 当前处理的类型
   * @tparam Rest 剩余类型列表
   * @param offsets 存储所有偏移量的数组
   * @param current_offset 当前偏移量（会更新）
   * @param index 当前类型的索引
   */
  template <typename T, typename... Rest>
  static constexpr void calculate_offsets(
      std::array<size_t, sizeof...(Ts)> &offsets, size_t &current_offset,
      size_t index) {
    current_offset = align_up(current_offset, alignof(T));
    offsets[index] = current_offset;
    current_offset += sizeof(T);

    if constexpr (sizeof...(Rest) > 0) {
      calculate_offsets<Rest...>(offsets, current_offset, index + 1);
    }
  }

  /**
   * @brief 计算所有数据包在内存池中的布局（考虑对齐）
   *
   * 此静态常量在编译期计算所有数据包的内存布局，
   * 包括每个数据包的偏移量和总大小。
   */
  static constexpr auto layout = []() {
    struct LayoutInfo {
      std::array<size_t, sizeof...(Ts)> offsets;
      size_t total_size;
    } info{};

    size_t current_offset = 0;
    calculate_offsets<Ts...>(info.offsets, current_offset, 0);

    info.total_size = current_offset;
    return info;
  }();

  static constexpr std::size_t totalSize =
      layout.total_size; ///< 所有数据包类型的总大小（含对齐填充）

  /**
   * @brief 命令码到索引的映射
   *
   * 静态常量映射，将命令码映射到在内存池中的偏移量。
   * 使用 frozen::unordered_map 实现编译期查找表。
   */
  static constexpr auto cmdToIndex = []() {
    std::array<std::pair<uint16_t, size_t>, sizeof...(Ts)> pairs{};
    size_t index = 0;

    // 使用折叠表达式填充数组
    ((pairs[index] =
          std::make_pair(PacketTraits<Ts>::cmd, layout.offsets[index]),
      ++index),
     ...);

    return frozen::make_unordered_map(pairs);
  }();

  /**
   * @brief 获取指定类型的索引
   *
   * 获取指定数据包类型在内存池中的索引（偏移量）
   *
   * @tparam T 数据包类型
   * @return 该类型的索引（偏移量）
   */
  template <typename T> static constexpr size_t type_index() noexcept {
    return cmd_index(PacketTraits<T>::cmd);
  }

  /**
   * @brief 根据命令码获取索引
   *
   * 根据命令码获取对应数据包类型在内存池中的索引（偏移量）
   *
   * @param cmd 命令码
   * @return 对应的索引（偏移量），如果命令码不存在则返回-1
   */
  static constexpr size_t cmd_index(uint16_t cmd) noexcept {
    auto it = cmdToIndex.find(cmd);
    return it != cmdToIndex.end() ? it->second : static_cast<size_t>(-1);
  }

  /**
   * @brief 命令码到序列索引的映射（0-based 类型序号，用于 SeqLock version 数组）
   *
   * 此映射将命令码映射到其在模板参数列表中的序号（0, 1, 2, ...）。
   * 用于 SeqLock 机制中定位对应的 version 计数器。
   */
  static constexpr auto cmdToSeqIndex = []() {
    std::array<std::pair<uint16_t, size_t>, sizeof...(Ts)> pairs{};
    size_t index = 0;
    ((pairs[index] = std::make_pair(PacketTraits<Ts>::cmd, index), ++index),
     ...);
    return frozen::make_unordered_map(pairs);
  }();

  /**
   * @brief 根据命令码获取序列索引
   *
   * @param cmd 命令码
   * @return 对应的序列索引（0-based 类型序号），如果命令码不存在则返回-1
   */
  static constexpr size_t cmd_seq_index(uint16_t cmd) noexcept {
    auto it = cmdToSeqIndex.find(cmd);
    return it != cmdToSeqIndex.end() ? it->second : static_cast<size_t>(-1);
  }

  /**
   * @brief 获取指定类型的序列索引
   *
   * @tparam T 数据包类型
   * @return 该类型的序列索引（0-based 类型序号）
   */
  template <typename T> static constexpr size_t type_seq_index() noexcept {
    return cmd_seq_index(PacketTraits<T>::cmd);
  }
};
} // namespace RPL::Meta

/**
 * @file CompilerBarrier.hpp
 * @brief RPL 编译器屏障实现
 *
 * 此文件提供编译器屏障（Compiler Barrier）功能，
 * 用于阻止编译器对内存访问指令进行重排序优化。
 *
 * @par 设计原理
 * - 编译器屏障是一种内存屏障的轻量级形式
 * - 它告诉编译器不要跨越屏障重排内存访问
 * - 与硬件内存屏障不同，它不生成任何CPU指令
 *
 * @par 使用场景
 * - SeqLock 实现中的读写同步
 * - 无锁数据结构
 * - 禁用异常环境中的内存 ordering
 *
 * @author WindWeaver
 */

namespace RPL {

/**
 * @brief 编译器屏障函数
 *
 * 阻止编译器对跨越此屏障的内存访问进行重排序优化。
 * 这是一个轻量级的软件屏障，不生成任何 CPU 指令。
 *
 * @note 在 GCC/Clang 上使用 inline asm，在 MSVC 上使用 _ReadWriteBarrier
 * @warning 此函数仅提供编译器级别的屏障，不提供硬件级别的内存排序保证
 *          在需要更强保证的场景中，应使用 std::atomic 或硬件屏障
 */
#ifndef compiler_barrier
inline void compiler_barrier() noexcept {
#if defined(_MSC_VER)
  _ReadWriteBarrier();
#else
  asm volatile("" ::: "memory");
#endif
}
#endif

} // namespace RPL

#ifdef RPL_USE_STD_ATOMIC
#endif

/**
 * @namespace RPL
 * @brief RoboMaster Packet Library 的主命名空间
 *
 * RPL 是一个专为嵌入式高性能通信设计的 C++20 数据包序列化/反序列化库。
 */
namespace RPL {
/**
 * @brief 可反序列化概念
 *
 * 用于检查类型 T 是否为可反序列化类型之一。
 * 此概念用于模板约束，确保只有注册的数据包类型可以被反序列化。
 *
 * @tparam T 要检查的类型
 * @tparam Ts 可反序列化类型列表
 */
template <typename T, typename... Ts>
concept Deserializable = (std::is_same_v<T, Ts> || ...);

/**
 * @brief 反序列化器类
 *
 * 用于从字节数组中反序列化数据包结构，使用内存池来存储反序列化的数据。
 * 支持 SeqLock 机制以实现线程安全的读取。
 *
 * @tparam Ts 可反序列化的数据包类型列表
 *
 * @par 设计原理
 * - 使用静态内存池避免动态分配
 * - SeqLock 机制保证读取一致性
 * - 支持分段写入（用于 BipBuffer 边界跨越场景）
 *
 * @par 使用示例
 * @code
 * RPL::Deserializer<PacketA, PacketB> deserializer;
 * 
 * // Parser 内部调用 write() 写入数据
 * deserializer.write(PacketA::cmd, data_ptr, sizeof(PacketA));
 * 
 * // 用户获取数据包
 * auto packet_a = deserializer.get<PacketA>();
 * @endcode
 */
template <typename... Ts> class Deserializer {
  using Collector = Meta::PacketInfoCollector<Ts...>; ///< 用于收集包信息的类型
  Containers::MemoryPool<Collector> pool{}; ///< 存储反序列化数据的内存池

#ifdef RPL_USE_STD_ATOMIC
  /// @brief SeqLock version counters（原子版本）
  std::atomic<uint32_t> versions_[sizeof...(Ts)]{};
#else
  /// @brief SeqLock version counters（volatile + compiler barrier 版本）
  volatile uint32_t versions_[sizeof...(Ts)]{};
#endif

public:
  /**
   * @brief SeqLock 写入方法
   *
   * 供 Parser 调用，写入前后递增 version（odd=writing, even=done）。
   * 使用 SeqLock 机制确保读取器在读取时不会获得不一致的数据。
   *
   * @par SeqLock 工作原理
   * - 写入前：version++（变为奇数，表示正在写入）
   * - 写入数据
   * - 写入后：version++（变为偶数，表示写入完成）
   * - 读取器：检查 version 是否为偶数且前后一致
   *
   * @param cmd 命令码
   * @param src 数据源指针
   * @param len 数据长度
   */
  void write(uint16_t cmd, const uint8_t *src, size_t len) noexcept {
    const auto byte_offset = Collector::cmd_index(cmd);
    if (byte_offset == static_cast<size_t>(-1))
      return;
    const auto seq_idx = Collector::cmd_seq_index(cmd);

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    versions_[seq_idx] = versions_[seq_idx] + 1;
    compiler_barrier();
#endif

    std::memcpy(reinterpret_cast<uint8_t *>(&pool.buffer[byte_offset]), src,
                len);

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    compiler_barrier();
    versions_[seq_idx] = versions_[seq_idx] + 1;
#endif
  }

  /**
   * @brief 分段 SeqLock 写入方法
   *
   * 用于处理跨越 BipBuffer 边界的数据包，避免中间拷贝。
   * 当数据跨越缓冲区 A/B 区域边界时，数据会被分为两个 span。
   *
   * @param cmd 命令码
   * @param s1 第一段数据（可能为空）
   * @param s2 第二段数据（可能为空）
   *
   * @note 此方法用于零拷贝场景，直接从 BipBuffer 的分段视图写入
   */
  void write_segmented(uint16_t cmd, std::span<const uint8_t> s1,
                       std::span<const uint8_t> s2) noexcept {
    const auto byte_offset = Collector::cmd_index(cmd);
    if (byte_offset == static_cast<size_t>(-1))
      return;
    const auto seq_idx = Collector::cmd_seq_index(cmd);

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    versions_[seq_idx] = versions_[seq_idx] + 1;
    compiler_barrier();
#endif

    uint8_t *dest = reinterpret_cast<uint8_t *>(&pool.buffer[byte_offset]);
    if (!s1.empty()) {
      std::memcpy(dest, s1.data(), s1.size());
    }
    if (!s2.empty()) {
      std::memcpy(dest + s1.size(), s2.data(), s2.size());
    }

#ifdef RPL_USE_STD_ATOMIC
    versions_[seq_idx].fetch_add(1, std::memory_order_release);
#else
    compiler_barrier();
    versions_[seq_idx] = versions_[seq_idx] + 1;
#endif
  }

  /**
   * @brief 获取指定类型的数据包（SeqLock 读循环）
   *
   * 从内存池中获取指定类型的反序列化数据包，
   * 通过 SeqLock 保证读取一致性。
   *
   * @par 读取流程
   * 1. 读取 version（必须为偶数）
   * 2. 复制数据
   * 3. 再次读取 version
   * 4. 如果 version 改变或为奇数，重试
   *
   * @tparam T 要获取的数据包类型
   * @return 指定类型的反序列化数据包
   *
   * @note 此方法是线程安全的（与 write() 并发调用时）
   * @note 如果 RPL_USE_STD_ATOMIC 未定义，使用 volatile + compiler barrier
   */
  template <typename T>
    requires Deserializable<T, Ts...>
  T get() noexcept {
    constexpr auto seq_idx = Collector::template type_seq_index<T>();
    T result;
    uint32_t v1, v2;
    do {
#ifdef RPL_USE_STD_ATOMIC
      v1 = versions_[seq_idx].load(std::memory_order_acquire);
#else
      v1 = versions_[seq_idx];
      compiler_barrier();
#endif

      auto ptr = reinterpret_cast<uint8_t *>(
          &pool.buffer[Collector::template type_index<T>()]);
      Meta::PacketTraits<T>::before_get(ptr);
      if constexpr (Meta::HasBitLayout<Meta::PacketTraits<T>>) {
        result = deserialize_bitstream<T>(
            std::span<const uint8_t>(ptr, Meta::PacketTraits<T>::size));
      } else {
        result = *reinterpret_cast<const T *>(ptr);
      }

#ifdef RPL_USE_STD_ATOMIC
      std::atomic_thread_fence(std::memory_order_acquire);
      v2 = versions_[seq_idx].load(std::memory_order_relaxed);
#else
      compiler_barrier();
      v2 = versions_[seq_idx];
#endif
    } while (v1 != v2 || (v1 & 1));
    return result;
  };

  /**
   * @brief 获取指定类型的直接引用
   *
   * 获取内存池中指定类型的直接引用，无拷贝。
   *
   * @warning 存在竞态访问可能，不保证读取一致性
   * @warning 仅在对性能有极致要求且能确保无并发写入时使用
   * @tparam T 要获取引用的数据包类型
   * @return 指定类型的直接引用
   *
   * @note 此方法跳过 SeqLock 检查，速度更快但不安全
   */
  template <typename T>
    requires Deserializable<T, Ts...>
  constexpr T &getRawRef() noexcept {
    return reinterpret_cast<T &>(
        pool.buffer[Collector::template type_index<T>()]);
  };

  /**
   * @brief 获取指定命令码的写入指针
   *
   * @deprecated 请改用 write() 方法以获得 SeqLock 线程安全保护
   * @param cmd 命令码
   * @return 指向数据缓冲区的指针，如果命令码无效则返回nullptr
   *
   * @warning 此方法不提供 SeqLock 保护，存在竞态风险
   */
  [[deprecated("Use write() for SeqLock-protected writes")]]
  [[nodiscard]] constexpr uint8_t *getWritePtr(uint16_t cmd) noexcept {
    const auto index = Collector::cmd_index(cmd);
    if (index == static_cast<size_t>(-1))
      return nullptr;
    return reinterpret_cast<uint8_t *>(&pool.buffer[index]);
  }
};
} // namespace RPL

/**
 * @file Serializer.hpp
 * @brief RPL库的序列化器实现
 *
 * 此文件包含Serializer类的定义，该类用于将数据包结构序列化为字节数组。
 * 序列化过程包括添加帧头、计算CRC校验和等。
 *
 * @author WindWeaver
 */

/**
 * @file BitstreamSerializer.hpp
 * @brief RPL 位流序列化器实现
 *
 * 此文件提供位流序列化功能，可以将结构体数据打包为紧凑的位流字节序列。
 * 支持跨字节位注入、编译期优化和 C 数组到 std::array 的自动转换。
 *
 * @par 设计原理
 * - 使用编译期位偏移计算，运行时仅执行高效的位操作
 * - 支持小端线格式 (little-endian wire format) 的位注入
 * - 通过结构化绑定 (C++17) 自动解包结构体成员
 *
 * @par 使用场景
 * - 紧凑位域协议的序列化（如遥控器协议）
 * - 需要节省带宽的嵌入式通信
 *
 * @author WindWeaver
 */

namespace RPL::Detail {

/**
 * @brief 在特定位偏移处将指定位数注入到字节序列中
 *
 * 此函数处理跨字节位注入，采用小端线格式假设。
 * 由于 BitOffset 和 BitWidth 是编译时常量，编译器会将此优化
 * 为高效的位操作。
 *
 * @tparam T 值类型 (整数或 std::array)
 * @tparam BitOffset 起始位索引 (0 是第一个字节的 LSB)
 * @tparam BitWidth 要注入的位数
 * @param buffer 要写入的字节序列
 * @param value 要注入的值
 *
 * @note 此函数是位流序列化的核心，支持跨越字节边界的位注入
 * @warning 如果 BitWidth 超过 T 的容量，将触发 static_assert
 * @note 此函数采用 OR 操作注入位，缓冲区应预先清零
 */
template <typename T, std::size_t BitOffset, std::size_t BitWidth>
constexpr void inject_bits(std::span<uint8_t> buffer, T value) {
  if constexpr (Meta::is_std_array_v<T>) {
    using ElementType = typename T::value_type;
    constexpr std::size_t N = std::tuple_size_v<T>;
    constexpr std::size_t bits_per_element = BitWidth / N;
    static_assert(bits_per_element * N == BitWidth,
                  "BitWidth must be a multiple of array size");

    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      (inject_bits<ElementType, BitOffset + Is * bits_per_element,
                   bits_per_element>(buffer, value[Is]),
       ...);
    }(std::make_index_sequence<N>{});
  } else {
    static_assert(BitWidth <= sizeof(T) * 8,
                  "BitWidth exceeds input type capacity");

    std::size_t current_bit_offset = BitOffset;
    std::size_t bits_injected = 0;

    T masked_value = value;
    if constexpr (BitWidth < sizeof(T) * 8) {
      masked_value &= (static_cast<T>(1) << BitWidth) - 1;
    }

    while (bits_injected < BitWidth) {
      std::size_t byte_index = current_bit_offset / 8;
      std::size_t bit_in_byte = current_bit_offset % 8;

      std::size_t bits_to_put = std::min(
          BitWidth - bits_injected, static_cast<std::size_t>(8 - bit_in_byte));

      if (byte_index >= buffer.size()) {
        break;
      }

      uint8_t chunk = static_cast<uint8_t>((masked_value >> bits_injected) &
                                           ((1ULL << bits_to_put) - 1));
      chunk <<= bit_in_byte;
      buffer[byte_index] |= chunk;

      bits_injected += bits_to_put;
      current_bit_offset += bits_to_put;
    }
  }
}

/**
 * @brief 辅助函数：如果是 C 数组则转换为 std::array，否则保持原样
 *
 * 用于处理结构化绑定中解包出的 C 数组成员，将其转换为 std::array
 * 以便后续位注入操作。
 *
 * @tparam T 值类型
 * @param val 要转换的值
 * @return 如果是 C 数组则返回 std::array，否则返回转发后的值
 */
template <typename T> constexpr auto to_array_if_needed(T &&val) {
  if constexpr (std::is_array_v<std::remove_cvref_t<T>>) {
    using Elem = std::remove_extent_t<std::remove_cvref_t<T>>;
    constexpr size_t N = std::extent_v<std::remove_cvref_t<T>>;
    std::array<Elem, N> result;
    for (size_t i = 0; i < N; ++i)
      result[i] = val[i];
    return result;
  } else {
    return std::forward<T>(val);
  }
}

/**
 * @brief 使用结构化绑定将结构体解包为成员元组
 *
 * 此函数利用 C++17 的结构化绑定特性，将结构体的每个成员提取出来
 * 并打包为元组。对于 C 数组成员，会自动转换为 std::array。
 *
 * @note 此函数支持 1 到 64 个成员的结构体
 * @warning 如果结构体成员超过 64 个，需要添加更多分支或使用其他方法
 *
 * @tparam N 结构体成员数量
 * @tparam T 结构体类型
 * @param obj 要解包的结构体对象
 * @return 包含所有成员的元组（C 数组已转换为 std::array）
 */
template <std::size_t N, typename T> constexpr auto struct_to_tuple(const T &obj) {
  if constexpr (N == 1) { const auto [m1] = obj; return std::make_tuple(to_array_if_needed(m1)); }
  else if constexpr (N == 2) { const auto [m1, m2] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2)); }
  else if constexpr (N == 3) { const auto [m1, m2, m3] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3)); }
  else if constexpr (N == 4) { const auto [m1, m2, m3, m4] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4)); }
  else if constexpr (N == 5) { const auto [m1, m2, m3, m4, m5] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5)); }
  else if constexpr (N == 6) { const auto [m1, m2, m3, m4, m5, m6] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6)); }
  else if constexpr (N == 7) { const auto [m1, m2, m3, m4, m5, m6, m7] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7)); }
  else if constexpr (N == 8) { const auto [m1, m2, m3, m4, m5, m6, m7, m8] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8)); }
  else if constexpr (N == 9) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9)); }
  else if constexpr (N == 10) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10)); }
  else if constexpr (N == 11) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11)); }
  else if constexpr (N == 12) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12)); }
  else if constexpr (N == 13) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13)); }
  else if constexpr (N == 14) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14)); }
  else if constexpr (N == 15) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15)); }
  else if constexpr (N == 16) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16)); }
  else if constexpr (N == 17) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17)); }
  else if constexpr (N == 18) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18)); }
  else if constexpr (N == 19) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19)); }
  else if constexpr (N == 20) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20)); }
  else if constexpr (N == 21) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21)); }
  else if constexpr (N == 22) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22)); }
  else if constexpr (N == 23) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23)); }
  else if constexpr (N == 24) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24)); }
  else if constexpr (N == 25) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25)); }
  else if constexpr (N == 26) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26)); }
  else if constexpr (N == 27) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27)); }
  else if constexpr (N == 28) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28)); }
  else if constexpr (N == 29) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29)); }
  else if constexpr (N == 30) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30)); }
  else if constexpr (N == 31) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31)); }
  else if constexpr (N == 32) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32)); }
  else if constexpr (N == 33) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33)); }
  else if constexpr (N == 34) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34)); }
  else if constexpr (N == 35) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35)); }
  else if constexpr (N == 36) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36)); }
  else if constexpr (N == 37) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37)); }
  else if constexpr (N == 38) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38)); }
  else if constexpr (N == 39) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39)); }
  else if constexpr (N == 40) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40)); }
  else if constexpr (N == 41) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41)); }
  else if constexpr (N == 42) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42)); }
  else if constexpr (N == 43) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43)); }
  else if constexpr (N == 44) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44)); }
  else if constexpr (N == 45) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45)); }
  else if constexpr (N == 46) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46)); }
  else if constexpr (N == 47) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47)); }
  else if constexpr (N == 48) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48)); }
  else if constexpr (N == 49) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49)); }
  else if constexpr (N == 50) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50)); }
  else if constexpr (N == 51) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51)); }
  else if constexpr (N == 52) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52)); }
  else if constexpr (N == 53) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53)); }
  else if constexpr (N == 54) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54)); }
  else if constexpr (N == 55) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55)); }
  else if constexpr (N == 56) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56)); }
  else if constexpr (N == 57) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57)); }
  else if constexpr (N == 58) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58)); }
  else if constexpr (N == 59) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58), to_array_if_needed(m59)); }
  else if constexpr (N == 60) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58), to_array_if_needed(m59), to_array_if_needed(m60)); }
  else if constexpr (N == 61) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58), to_array_if_needed(m59), to_array_if_needed(m60), to_array_if_needed(m61)); }
  else if constexpr (N == 62) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58), to_array_if_needed(m59), to_array_if_needed(m60), to_array_if_needed(m61), to_array_if_needed(m62)); }
  else if constexpr (N == 63) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58), to_array_if_needed(m59), to_array_if_needed(m60), to_array_if_needed(m61), to_array_if_needed(m62), to_array_if_needed(m63)); }
  else if constexpr (N == 64) { const auto [m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15, m16, m17, m18, m19, m20, m21, m22, m23, m24, m25, m26, m27, m28, m29, m30, m31, m32, m33, m34, m35, m36, m37, m38, m39, m40, m41, m42, m43, m44, m45, m46, m47, m48, m49, m50, m51, m52, m53, m54, m55, m56, m57, m58, m59, m60, m61, m62, m63, m64] = obj; return std::make_tuple(to_array_if_needed(m1), to_array_if_needed(m2), to_array_if_needed(m3), to_array_if_needed(m4), to_array_if_needed(m5), to_array_if_needed(m6), to_array_if_needed(m7), to_array_if_needed(m8), to_array_if_needed(m9), to_array_if_needed(m10), to_array_if_needed(m11), to_array_if_needed(m12), to_array_if_needed(m13), to_array_if_needed(m14), to_array_if_needed(m15), to_array_if_needed(m16), to_array_if_needed(m17), to_array_if_needed(m18), to_array_if_needed(m19), to_array_if_needed(m20), to_array_if_needed(m21), to_array_if_needed(m22), to_array_if_needed(m23), to_array_if_needed(m24), to_array_if_needed(m25), to_array_if_needed(m26), to_array_if_needed(m27), to_array_if_needed(m28), to_array_if_needed(m29), to_array_if_needed(m30), to_array_if_needed(m31), to_array_if_needed(m32), to_array_if_needed(m33), to_array_if_needed(m34), to_array_if_needed(m35), to_array_if_needed(m36), to_array_if_needed(m37), to_array_if_needed(m38), to_array_if_needed(m39), to_array_if_needed(m40), to_array_if_needed(m41), to_array_if_needed(m42), to_array_if_needed(m43), to_array_if_needed(m44), to_array_if_needed(m45), to_array_if_needed(m46), to_array_if_needed(m47), to_array_if_needed(m48), to_array_if_needed(m49), to_array_if_needed(m50), to_array_if_needed(m51), to_array_if_needed(m52), to_array_if_needed(m53), to_array_if_needed(m54), to_array_if_needed(m55), to_array_if_needed(m56), to_array_if_needed(m57), to_array_if_needed(m58), to_array_if_needed(m59), to_array_if_needed(m60), to_array_if_needed(m61), to_array_if_needed(m62), to_array_if_needed(m63), to_array_if_needed(m64)); }
  else {
    static_assert(N <= 64, "struct_to_tuple supports up to 64 fields. Add more "
                           "branches to support more!");
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
 *
 * @tparam T 目标结构类型（必须有 BitLayout 特化）
 * @param buffer 要写入的字节序列（应预先清零）
 * @param packet 要序列化的数据包对象
 *
 * @par 使用示例
 * @code
 * MyPacket packet{...};
 * std::array<uint8_t, 16> buffer{}; // 初始化为 0
 * RPL::serialize_bitstream(buffer, packet);
 * @endcode
 *
 * @warning 缓冲区应预先清零，因为此函数使用 OR 操作注入位
 * @note 此函数要求 Meta::HasBitLayout<Meta::PacketTraits<T>> 为 true
 */
template <typename T>
  requires Meta::HasBitLayout<Meta::PacketTraits<T>>
constexpr void serialize_bitstream(std::span<uint8_t> buffer, const T &packet) {
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
    (Detail::inject_bits<typename std::tuple_element_t<Is, Layout>::type,
                         offsets[Is], std::tuple_element_t<Is, Layout>::bits>(
         buffer, std::get<Is>(values)),
     ...);
  }(std::make_index_sequence<N>{});
}

} // namespace RPL

/**
 * @file Error.hpp
 * @brief RPL库的错误处理定义
 *
 * 此文件定义了RPL库中使用的错误码和错误结构体。
 *
 * @author WindWeaver
 */

namespace RPL
{
    /**
     * @brief 错误码枚举
     *
     * 定义了RPL库中可能出现的各种错误类型
     */
    enum class ErrorCode
    {
        Again,            ///< 需要再次尝试
        InsufficientData, ///< 数据不足
        NoFrameHeader,    ///< 没有找到帧头
        InvalidFrameHeader, ///< 无效的帧头
        CrcMismatch,      ///< CRC校验不匹配
        BufferOverflow,   ///< 缓冲区溢出
        InternalError,    ///< 内部错误
        InvalidCommand,   ///< 无效命令
    };

    /**
     * @brief 错误结构体
     *
     * 包含错误码和错误消息的结构体
     */
    struct Error
    {
        /**
         * @brief 构造函数
         *
         * 使用错误码和错误消息构造错误对象
         *
         * @param c 错误码
         * @param msg 错误消息
         */
        Error(const ErrorCode c, std::string msg) : message(std::move(msg)), code(c)
        {
        }

        std::string message;  ///< 错误消息
        ErrorCode code;       ///< 错误码
    };
}

///
// expected - An implementation of std::expected with extensions
// Written in 2017 by Sy Brand (tartanllama@gmail.com, @TartanLlama)
//
// Documentation available at http://tl.tartanllama.xyz/
//
// To the extent possible under law, the author(s) have dedicated all
// copyright and related and neighboring rights to this software to the
// public domain worldwide. This software is distributed without any warranty.
//
// You should have received a copy of the CC0 Public Domain Dedication
// along with this software. If not, see
// <http://creativecommons.org/publicdomain/zero/1.0/>.
///

#define TL_EXPECTED_VERSION_MAJOR 1
#define TL_EXPECTED_VERSION_MINOR 3
#define TL_EXPECTED_VERSION_PATCH 1

#if defined(__EXCEPTIONS) || defined(_CPPUNWIND)
#define TL_EXPECTED_EXCEPTIONS_ENABLED
#endif

#if (defined(_MSC_VER) && _MSC_VER == 1900)
#define TL_EXPECTED_MSVC2015
#define TL_EXPECTED_MSVC2015_CONSTEXPR
#else
#define TL_EXPECTED_MSVC2015_CONSTEXPR constexpr
#endif

#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 &&              \
     !defined(__clang__))
#define TL_EXPECTED_GCC49
#endif

#if (defined(__GNUC__) && __GNUC__ == 5 && __GNUC_MINOR__ <= 4 &&              \
     !defined(__clang__))
#define TL_EXPECTED_GCC54
#endif

#if (defined(__GNUC__) && __GNUC__ == 5 && __GNUC_MINOR__ <= 5 &&              \
     !defined(__clang__))
#define TL_EXPECTED_GCC55
#endif

#ifdef _MSVC_LANG
#define TL_CPLUSPLUS _MSVC_LANG
#else
#define TL_CPLUSPLUS __cplusplus
#endif

#if !defined(TL_ASSERT)
//can't have assert in constexpr in C++11 and GCC 4.9 has a compiler bug
#if (TL_CPLUSPLUS > 201103L) && !defined(TL_EXPECTED_GCC49)
#define TL_ASSERT(x) assert(x)
#else 
#define TL_ASSERT(x)
#endif
#endif

#if (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ <= 9 &&              \
     !defined(__clang__))
// GCC < 5 doesn't support overloading on const&& for member functions

#define TL_EXPECTED_NO_CONSTRR
// GCC < 5 doesn't support some standard C++11 type traits
#define TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)                         \
  std::has_trivial_copy_constructor<T>
#define TL_EXPECTED_IS_TRIVIALLY_COPY_ASSIGNABLE(T)                            \
  std::has_trivial_copy_assign<T>

// This one will be different for GCC 5.7 if it's ever supported
#define TL_EXPECTED_IS_TRIVIALLY_DESTRUCTIBLE(T)                               \
  std::is_trivially_destructible<T>

// GCC 5 < v < 8 has a bug in is_trivially_copy_constructible which breaks
// std::vector for non-copyable types
#elif (defined(__GNUC__) && __GNUC__ < 8 && !defined(__clang__))
#ifndef TL_GCC_LESS_8_TRIVIALLY_COPY_CONSTRUCTIBLE_MUTEX
#define TL_GCC_LESS_8_TRIVIALLY_COPY_CONSTRUCTIBLE_MUTEX
namespace tl {
namespace detail {
template <class T>
struct is_trivially_copy_constructible
    : std::is_trivially_copy_constructible<T> {};
#ifdef _GLIBCXX_VECTOR
template <class T, class A>
struct is_trivially_copy_constructible<std::vector<T, A>> : std::false_type {};
#endif
} // namespace detail
} // namespace tl
#endif

#define TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)                         \
  tl::detail::is_trivially_copy_constructible<T>
#define TL_EXPECTED_IS_TRIVIALLY_COPY_ASSIGNABLE(T)                            \
  std::is_trivially_copy_assignable<T>
#define TL_EXPECTED_IS_TRIVIALLY_DESTRUCTIBLE(T)                               \
  std::is_trivially_destructible<T>
#else
#define TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)                         \
  std::is_trivially_copy_constructible<T>
#define TL_EXPECTED_IS_TRIVIALLY_COPY_ASSIGNABLE(T)                            \
  std::is_trivially_copy_assignable<T>
#define TL_EXPECTED_IS_TRIVIALLY_DESTRUCTIBLE(T)                               \
  std::is_trivially_destructible<T>
#endif

#if TL_CPLUSPLUS > 201103L
#define TL_EXPECTED_CXX14
#endif

#ifdef TL_EXPECTED_GCC49
#define TL_EXPECTED_GCC49_CONSTEXPR
#else
#define TL_EXPECTED_GCC49_CONSTEXPR constexpr
#endif

#if (TL_CPLUSPLUS == 201103L || defined(TL_EXPECTED_MSVC2015) ||                \
     defined(TL_EXPECTED_GCC49))
#define TL_EXPECTED_11_CONSTEXPR
#else
#define TL_EXPECTED_11_CONSTEXPR constexpr
#endif

#if TL_CPLUSPLUS >= 201703L 
#define TL_EXPECTED_NODISCARD [[nodiscard]]
#else
#define TL_EXPECTED_NODISCARD
#endif

namespace tl {
template <class T, class E> class TL_EXPECTED_NODISCARD expected;

#ifndef TL_MONOSTATE_INPLACE_MUTEX
#define TL_MONOSTATE_INPLACE_MUTEX
class monostate {};

struct in_place_t {
  explicit in_place_t() = default;
};
static constexpr in_place_t in_place{};
#endif

template <class E> class unexpected {
public:
  static_assert(!std::is_same<E, void>::value, "E must not be void");

  unexpected() = delete;
  constexpr explicit unexpected(const E &e) : m_val(e) {}

  constexpr explicit unexpected(E &&e) : m_val(std::move(e)) {}

  template <class... Args, typename std::enable_if<std::is_constructible<
                               E, Args &&...>::value>::type * = nullptr>
  constexpr explicit unexpected(Args &&...args)
      : m_val(std::forward<Args>(args)...) {}
  template <
      class U, class... Args,
      typename std::enable_if<std::is_constructible<
          E, std::initializer_list<U> &, Args &&...>::value>::type * = nullptr>
  constexpr explicit unexpected(std::initializer_list<U> l, Args &&...args)
      : m_val(l, std::forward<Args>(args)...) {}

  constexpr const E &value() const & { return m_val; }
  TL_EXPECTED_11_CONSTEXPR E &value() & { return m_val; }
  TL_EXPECTED_11_CONSTEXPR E &&value() && { return std::move(m_val); }
  constexpr const E &&value() const && { return std::move(m_val); }

private:
  E m_val;
};

#ifdef __cpp_deduction_guides
template <class E> unexpected(E) -> unexpected<E>;
#endif

template <class E>
constexpr bool operator==(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() == rhs.value();
}
template <class E>
constexpr bool operator!=(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() != rhs.value();
}
template <class E>
constexpr bool operator<(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() < rhs.value();
}
template <class E>
constexpr bool operator<=(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() <= rhs.value();
}
template <class E>
constexpr bool operator>(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() > rhs.value();
}
template <class E>
constexpr bool operator>=(const unexpected<E> &lhs, const unexpected<E> &rhs) {
  return lhs.value() >= rhs.value();
}

template <class E>
unexpected<typename std::decay<E>::type> make_unexpected(E &&e) {
  return unexpected<typename std::decay<E>::type>(std::forward<E>(e));
}

struct unexpect_t {
  unexpect_t() = default;
};
static constexpr unexpect_t unexpect{};

#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
#define TL_EXPECTED_THROW_EXCEPTION(e) throw((e));
#else
#define TL_EXPECTED_THROW_EXCEPTION(e) std::terminate();
#endif

namespace detail {
#ifndef TL_TRAITS_MUTEX
#define TL_TRAITS_MUTEX
// C++14-style aliases for brevity
template <class T> using remove_const_t = typename std::remove_const<T>::type;
template <class T>
using remove_reference_t = typename std::remove_reference<T>::type;
template <class T> using decay_t = typename std::decay<T>::type;
template <bool E, class T = void>
using enable_if_t = typename std::enable_if<E, T>::type;
template <bool B, class T, class F>
using conditional_t = typename std::conditional<B, T, F>::type;

// std::conjunction from C++17
template <class...> struct conjunction : std::true_type {};
template <class B> struct conjunction<B> : B {};
template <class B, class... Bs>
struct conjunction<B, Bs...>
    : std::conditional<bool(B::value), conjunction<Bs...>, B>::type {};

#if defined(_LIBCPP_VERSION) && __cplusplus == 201103L
#define TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
#endif

// In C++11 mode, there's an issue in libc++'s std::mem_fn
// which results in a hard-error when using it in a noexcept expression
// in some cases. This is a check to workaround the common failing case.
#ifdef TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
template <class T>
struct is_pointer_to_non_const_member_func : std::false_type {};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*)(Args...)>
    : std::true_type {};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*)(Args...) &>
    : std::true_type {};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*)(Args...) &&>
    : std::true_type {};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*)(Args...) volatile>
    : std::true_type {};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*)(Args...) volatile &>
    : std::true_type {};
template <class T, class Ret, class... Args>
struct is_pointer_to_non_const_member_func<Ret (T::*)(Args...) volatile &&>
    : std::true_type {};

template <class T> struct is_const_or_const_ref : std::false_type {};
template <class T> struct is_const_or_const_ref<T const &> : std::true_type {};
template <class T> struct is_const_or_const_ref<T const> : std::true_type {};
#endif

// std::invoke from C++17
// https://stackoverflow.com/questions/38288042/c11-14-invoke-workaround
template <
    typename Fn, typename... Args,
#ifdef TL_TRAITS_LIBCXX_MEM_FN_WORKAROUND
    typename = enable_if_t<!(is_pointer_to_non_const_member_func<Fn>::value &&
                             is_const_or_const_ref<Args...>::value)>,
#endif
    typename = enable_if_t<std::is_member_pointer<decay_t<Fn>>::value>, int = 0>
constexpr auto invoke(Fn &&f, Args &&...args) noexcept(
    noexcept(std::mem_fn(f)(std::forward<Args>(args)...)))
    -> decltype(std::mem_fn(f)(std::forward<Args>(args)...)) {
  return std::mem_fn(f)(std::forward<Args>(args)...);
}

template <typename Fn, typename... Args,
          typename = enable_if_t<!std::is_member_pointer<decay_t<Fn>>::value>>
constexpr auto invoke(Fn &&f, Args &&...args) noexcept(
    noexcept(std::forward<Fn>(f)(std::forward<Args>(args)...)))
    -> decltype(std::forward<Fn>(f)(std::forward<Args>(args)...)) {
  return std::forward<Fn>(f)(std::forward<Args>(args)...);
}

// std::invoke_result from C++17
template <class F, class, class... Us> struct invoke_result_impl;

template <class F, class... Us>
struct invoke_result_impl<
    F,
    decltype(detail::invoke(std::declval<F>(), std::declval<Us>()...), void()),
    Us...> {
  using type =
      decltype(detail::invoke(std::declval<F>(), std::declval<Us>()...));
};

template <class F, class... Us>
using invoke_result = invoke_result_impl<F, void, Us...>;

template <class F, class... Us>
using invoke_result_t = typename invoke_result<F, Us...>::type;

#if defined(_MSC_VER) && _MSC_VER <= 1900
// TODO make a version which works with MSVC 2015
template <class T, class U = T> struct is_swappable : std::true_type {};

template <class T, class U = T> struct is_nothrow_swappable : std::true_type {};
#else
// https://stackoverflow.com/questions/26744589/what-is-a-proper-way-to-implement-is-swappable-to-test-for-the-swappable-concept
namespace swap_adl_tests {
// if swap ADL finds this then it would call std::swap otherwise (same
// signature)
struct tag {};

template <class T> tag swap(T &, T &);
template <class T, std::size_t N> tag swap(T (&a)[N], T (&b)[N]);

// helper functions to test if an unqualified swap is possible, and if it
// becomes std::swap
template <class, class> std::false_type can_swap(...) noexcept(false);
template <class T, class U,
          class = decltype(swap(std::declval<T &>(), std::declval<U &>()))>
std::true_type can_swap(int) noexcept(noexcept(swap(std::declval<T &>(),
                                                    std::declval<U &>())));

template <class, class> std::false_type uses_std(...);
template <class T, class U>
std::is_same<decltype(swap(std::declval<T &>(), std::declval<U &>())), tag>
uses_std(int);

template <class T>
struct is_std_swap_noexcept
    : std::integral_constant<bool,
                             std::is_nothrow_move_constructible<T>::value &&
                                 std::is_nothrow_move_assignable<T>::value> {};

template <class T, std::size_t N>
struct is_std_swap_noexcept<T[N]> : is_std_swap_noexcept<T> {};

template <class T, class U>
struct is_adl_swap_noexcept
    : std::integral_constant<bool, noexcept(can_swap<T, U>(0))> {};
} // namespace swap_adl_tests

template <class T, class U = T>
struct is_swappable
    : std::integral_constant<
          bool,
          decltype(detail::swap_adl_tests::can_swap<T, U>(0))::value &&
              (!decltype(detail::swap_adl_tests::uses_std<T, U>(0))::value ||
               (std::is_move_assignable<T>::value &&
                std::is_move_constructible<T>::value))> {};

template <class T, std::size_t N>
struct is_swappable<T[N], T[N]>
    : std::integral_constant<
          bool,
          decltype(detail::swap_adl_tests::can_swap<T[N], T[N]>(0))::value &&
              (!decltype(detail::swap_adl_tests::uses_std<T[N], T[N]>(
                   0))::value ||
               is_swappable<T, T>::value)> {};

template <class T, class U = T>
struct is_nothrow_swappable
    : std::integral_constant<
          bool,
          is_swappable<T, U>::value &&
              ((decltype(detail::swap_adl_tests::uses_std<T, U>(0))::value &&
                detail::swap_adl_tests::is_std_swap_noexcept<T>::value) ||
               (!decltype(detail::swap_adl_tests::uses_std<T, U>(0))::value &&
                detail::swap_adl_tests::is_adl_swap_noexcept<T, U>::value))> {};
#endif
#endif

// Trait for checking if a type is a tl::expected
template <class T> struct is_expected_impl : std::false_type {};
template <class T, class E>
struct is_expected_impl<expected<T, E>> : std::true_type {};
template <class T> using is_expected = is_expected_impl<decay_t<T>>;

template <class T, class E, class U>
using expected_enable_forward_value = detail::enable_if_t<
    std::is_constructible<T, U &&>::value &&
    !std::is_same<detail::decay_t<U>, in_place_t>::value &&
    !std::is_same<expected<T, E>, detail::decay_t<U>>::value &&
    !std::is_same<unexpected<E>, detail::decay_t<U>>::value>;

template <class T, class E, class U, class G, class UR, class GR>
using expected_enable_from_other = detail::enable_if_t<
    std::is_constructible<T, UR>::value &&
    std::is_constructible<E, GR>::value &&
    !std::is_constructible<T, expected<U, G> &>::value &&
    !std::is_constructible<T, expected<U, G> &&>::value &&
    !std::is_constructible<T, const expected<U, G> &>::value &&
    !std::is_constructible<T, const expected<U, G> &&>::value &&
    !std::is_convertible<expected<U, G> &, T>::value &&
    !std::is_convertible<expected<U, G> &&, T>::value &&
    !std::is_convertible<const expected<U, G> &, T>::value &&
    !std::is_convertible<const expected<U, G> &&, T>::value>;

template <class T, class U>
using is_void_or = conditional_t<std::is_void<T>::value, std::true_type, U>;

template <class T>
using is_copy_constructible_or_void =
    is_void_or<T, std::is_copy_constructible<T>>;

template <class T>
using is_move_constructible_or_void =
    is_void_or<T, std::is_move_constructible<T>>;

template <class T>
using is_copy_assignable_or_void = is_void_or<T, std::is_copy_assignable<T>>;

template <class T>
using is_move_assignable_or_void = is_void_or<T, std::is_move_assignable<T>>;

} // namespace detail

namespace detail {
struct no_init_t {};
static constexpr no_init_t no_init{};

// Implements the storage of the values, and ensures that the destructor is
// trivial if it can be.
//
// This specialization is for where neither `T` or `E` is trivially
// destructible, so the destructors must be called on destruction of the
// `expected`
template <class T, class E, bool = std::is_trivially_destructible<T>::value,
          bool = std::is_trivially_destructible<E>::value>
struct expected_storage_base {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<T, Args &&...>::value> * =
                nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  ~expected_storage_base() {
    if (m_has_val) {
      m_val.~T();
    } else {
      m_unexpect.~unexpected<E>();
    }
  }
  union {
    T m_val;
    unexpected<E> m_unexpect;
    char m_no_init;
  };
  bool m_has_val;
};

// This specialization is for when both `T` and `E` are trivially-destructible,
// so the destructor of the `expected` can be trivial.
template <class T, class E> struct expected_storage_base<T, E, true, true> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<T, Args &&...>::value> * =
                nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  expected_storage_base(const expected_storage_base &) = default;     
  expected_storage_base(expected_storage_base &&) = default;
  expected_storage_base &operator=(const expected_storage_base &) = default;
  expected_storage_base &operator=(expected_storage_base &&) = default;
  ~expected_storage_base() = default;
  union {
    T m_val;
    unexpected<E> m_unexpect;
    char m_no_init;
  };
  bool m_has_val;
};

// T is trivial, E is not.
template <class T, class E> struct expected_storage_base<T, E, true, false> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  TL_EXPECTED_MSVC2015_CONSTEXPR expected_storage_base(no_init_t)
      : m_no_init(), m_has_val(false) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<T, Args &&...>::value> * =
                nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  expected_storage_base(const expected_storage_base &) = default;
  expected_storage_base(expected_storage_base &&) = default;
  expected_storage_base &operator=(const expected_storage_base &) = default;
  expected_storage_base &operator=(expected_storage_base &&) = default;
  ~expected_storage_base() {
    if (!m_has_val) {
      m_unexpect.~unexpected<E>();
    }
  }

  union {
    T m_val;
    unexpected<E> m_unexpect;
    char m_no_init;
  };
  bool m_has_val;
};

// E is trivial, T is not.
template <class T, class E> struct expected_storage_base<T, E, false, true> {
  constexpr expected_storage_base() : m_val(T{}), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_no_init(), m_has_val(false) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<T, Args &&...>::value> * =
                nullptr>
  constexpr expected_storage_base(in_place_t, Args &&...args)
      : m_val(std::forward<Args>(args)...), m_has_val(true) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr expected_storage_base(in_place_t, std::initializer_list<U> il,
                                  Args &&...args)
      : m_val(il, std::forward<Args>(args)...), m_has_val(true) {}
  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  expected_storage_base(const expected_storage_base &) = default;
  expected_storage_base(expected_storage_base &&) = default;
  expected_storage_base &operator=(const expected_storage_base &) = default;
  expected_storage_base &operator=(expected_storage_base &&) = default;
  ~expected_storage_base() {
    if (m_has_val) {
      m_val.~T();
    }
  }
  union {
    T m_val;
    unexpected<E> m_unexpect;
    char m_no_init;
  };
  bool m_has_val;
};

// `T` is `void`, `E` is trivially-destructible
template <class E> struct expected_storage_base<void, E, false, true> {
  #if __GNUC__ <= 5
  //no constexpr for GCC 4/5 bug
  #else
  TL_EXPECTED_MSVC2015_CONSTEXPR
  #endif 
  expected_storage_base() : m_has_val(true) {}
     
  constexpr expected_storage_base(no_init_t) : m_val(), m_has_val(false) {}

  constexpr expected_storage_base(in_place_t) : m_has_val(true) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  expected_storage_base(const expected_storage_base &) = default;
  expected_storage_base(expected_storage_base &&) = default;
  expected_storage_base &operator=(const expected_storage_base &) = default;
  expected_storage_base &operator=(expected_storage_base &&) = default;
  ~expected_storage_base() = default;
  struct dummy {};
  union {
    unexpected<E> m_unexpect;
    dummy m_val;
  };
  bool m_has_val;
};

// `T` is `void`, `E` is not trivially-destructible
template <class E> struct expected_storage_base<void, E, false, false> {
  constexpr expected_storage_base() : m_dummy(), m_has_val(true) {}
  constexpr expected_storage_base(no_init_t) : m_dummy(), m_has_val(false) {}

  constexpr expected_storage_base(in_place_t) : m_dummy(), m_has_val(true) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected_storage_base(unexpect_t, Args &&...args)
      : m_unexpect(std::forward<Args>(args)...), m_has_val(false) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected_storage_base(unexpect_t,
                                           std::initializer_list<U> il,
                                           Args &&...args)
      : m_unexpect(il, std::forward<Args>(args)...), m_has_val(false) {}

  expected_storage_base(const expected_storage_base &) = default;
  expected_storage_base(expected_storage_base &&) = default;
  expected_storage_base &operator=(const expected_storage_base &) = default;
  expected_storage_base &operator=(expected_storage_base &&) = default;
  ~expected_storage_base() {
    if (!m_has_val) {
      m_unexpect.~unexpected<E>();
    }
  }

  union {
    unexpected<E> m_unexpect;
    char m_dummy;
  };
  bool m_has_val;
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <class T, class E>
struct expected_operations_base : expected_storage_base<T, E> {
  using expected_storage_base<T, E>::expected_storage_base;

  template <class... Args> void construct(Args &&...args) noexcept {
    new (std::addressof(this->m_val)) T(std::forward<Args>(args)...);
    this->m_has_val = true;
  }

  template <class Rhs> void construct_with(Rhs &&rhs) noexcept {
    new (std::addressof(this->m_val)) T(std::forward<Rhs>(rhs).get());
    this->m_has_val = true;
  }

  template <class... Args> void construct_error(Args &&...args) noexcept {
    new (std::addressof(this->m_unexpect))
        unexpected<E>(std::forward<Args>(args)...);
    this->m_has_val = false;
  }

#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED

  // These assign overloads ensure that the most efficient assignment
  // implementation is used while maintaining the strong exception guarantee.
  // The problematic case is where rhs has a value, but *this does not.
  //
  // This overload handles the case where we can just copy-construct `T`
  // directly into place without throwing.
  template <class U = T,
            detail::enable_if_t<std::is_nothrow_copy_constructible<U>::value>
                * = nullptr>
  void assign(const expected_operations_base &rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(rhs.get());
    } else {
      assign_common(rhs);
    }
  }

  // This overload handles the case where we can attempt to create a copy of
  // `T`, then no-throw move it into place if the copy was successful.
  template <class U = T,
            detail::enable_if_t<!std::is_nothrow_copy_constructible<U>::value &&
                                std::is_nothrow_move_constructible<U>::value>
                * = nullptr>
  void assign(const expected_operations_base &rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      T tmp = rhs.get();
      geterr().~unexpected<E>();
      construct(std::move(tmp));
    } else {
      assign_common(rhs);
    }
  }

  // This overload is the worst-case, where we have to move-construct the
  // unexpected value into temporary storage, then try to copy the T into place.
  // If the construction succeeds, then everything is fine, but if it throws,
  // then we move the old unexpected value back into place before rethrowing the
  // exception.
  template <class U = T,
            detail::enable_if_t<!std::is_nothrow_copy_constructible<U>::value &&
                                !std::is_nothrow_move_constructible<U>::value>
                * = nullptr>
  void assign(const expected_operations_base &rhs) {
    if (!this->m_has_val && rhs.m_has_val) {
      auto tmp = std::move(geterr());
      geterr().~unexpected<E>();

#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
      try {
        construct(rhs.get());
      } catch (...) {
        geterr() = std::move(tmp);
        throw;
      }
#else
      construct(rhs.get());
#endif
    } else {
      assign_common(rhs);
    }
  }

  // These overloads do the same as above, but for rvalues
  template <class U = T,
            detail::enable_if_t<std::is_nothrow_move_constructible<U>::value>
                * = nullptr>
  void assign(expected_operations_base &&rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(std::move(rhs).get());
    } else {
      assign_common(std::move(rhs));
    }
  }

  template <class U = T,
            detail::enable_if_t<!std::is_nothrow_move_constructible<U>::value>
                * = nullptr>
  void assign(expected_operations_base &&rhs) {
    if (!this->m_has_val && rhs.m_has_val) {
      auto tmp = std::move(geterr());
      geterr().~unexpected<E>();
#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
      try {
        construct(std::move(rhs).get());
      } catch (...) {
        geterr() = std::move(tmp);
        throw;
      }
#else
      construct(std::move(rhs).get());
#endif
    } else {
      assign_common(std::move(rhs));
    }
  }

#else

  // If exceptions are disabled then we can just copy-construct
  void assign(const expected_operations_base &rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(rhs.get());
    } else {
      assign_common(rhs);
    }
  }

  void assign(expected_operations_base &&rhs) noexcept {
    if (!this->m_has_val && rhs.m_has_val) {
      geterr().~unexpected<E>();
      construct(std::move(rhs).get());
    } else {
      assign_common(std::move(rhs));
    }
  }

#endif

  // The common part of move/copy assigning
  template <class Rhs> void assign_common(Rhs &&rhs) {
    if (this->m_has_val) {
      if (rhs.m_has_val) {
        get() = std::forward<Rhs>(rhs).get();
      } else {
        destroy_val();
        construct_error(std::forward<Rhs>(rhs).geterr());
      }
    } else {
      if (!rhs.m_has_val) {
        geterr() = std::forward<Rhs>(rhs).geterr();
      }
    }
  }

  bool has_value() const { return this->m_has_val; }

  TL_EXPECTED_11_CONSTEXPR T &get() & { return this->m_val; }
  constexpr const T &get() const & { return this->m_val; }
  TL_EXPECTED_11_CONSTEXPR T &&get() && { return std::move(this->m_val); }
#ifndef TL_EXPECTED_NO_CONSTRR
  constexpr const T &&get() const && { return std::move(this->m_val); }
#endif

  TL_EXPECTED_11_CONSTEXPR unexpected<E> &geterr() & {
    return this->m_unexpect;
  }
  constexpr const unexpected<E> &geterr() const & { return this->m_unexpect; }
  TL_EXPECTED_11_CONSTEXPR unexpected<E> &&geterr() && {
    return std::move(this->m_unexpect);
  }
#ifndef TL_EXPECTED_NO_CONSTRR
  constexpr const unexpected<E> &&geterr() const && {
    return std::move(this->m_unexpect);
  }
#endif

  TL_EXPECTED_11_CONSTEXPR void destroy_val() { get().~T(); }
};

// This base class provides some handy member functions which can be used in
// further derived classes
template <class E>
struct expected_operations_base<void, E> : expected_storage_base<void, E> {
  using expected_storage_base<void, E>::expected_storage_base;

  template <class... Args> void construct() noexcept { this->m_has_val = true; }

  // This function doesn't use its argument, but needs it so that code in
  // levels above this can work independently of whether T is void
  template <class Rhs> void construct_with(Rhs &&) noexcept {
    this->m_has_val = true;
  }

  template <class... Args> void construct_error(Args &&...args) noexcept {
    new (std::addressof(this->m_unexpect))
        unexpected<E>(std::forward<Args>(args)...);
    this->m_has_val = false;
  }

  template <class Rhs> void assign(Rhs &&rhs) noexcept {
    if (!this->m_has_val) {
      if (rhs.m_has_val) {
        geterr().~unexpected<E>();
        construct();
      } else {
        geterr() = std::forward<Rhs>(rhs).geterr();
      }
    } else {
      if (!rhs.m_has_val) {
        construct_error(std::forward<Rhs>(rhs).geterr());
      }
    }
  }

  bool has_value() const { return this->m_has_val; }

  TL_EXPECTED_11_CONSTEXPR unexpected<E> &geterr() & {
    return this->m_unexpect;
  }
  constexpr const unexpected<E> &geterr() const & { return this->m_unexpect; }
  TL_EXPECTED_11_CONSTEXPR unexpected<E> &&geterr() && {
    return std::move(this->m_unexpect);
  }
#ifndef TL_EXPECTED_NO_CONSTRR
  constexpr const unexpected<E> &&geterr() const && {
    return std::move(this->m_unexpect);
  }
#endif

  TL_EXPECTED_11_CONSTEXPR void destroy_val() {
    // no-op
  }
};

// This class manages conditionally having a trivial copy constructor
// This specialization is for when T and E are trivially copy constructible
template <class T, class E,
          bool = is_void_or<T, TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T)>::
              value &&TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(E)::value,
          bool = (is_copy_constructible_or_void<T>::value &&
                  std::is_copy_constructible<E>::value)>
struct expected_copy_base : expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;
};

// This specialization is for when T or E are non-trivially copy constructible
template <class T, class E>
struct expected_copy_base<T, E, false, true> : expected_operations_base<T, E> {
  using expected_operations_base<T, E>::expected_operations_base;

  expected_copy_base() = default;
  expected_copy_base(const expected_copy_base &rhs)
      : expected_operations_base<T, E>(no_init) {
    if (rhs.has_value()) {
      this->construct_with(rhs);
    } else {
      this->construct_error(rhs.geterr());
    }
  }

  expected_copy_base(expected_copy_base &&rhs) = default;
  expected_copy_base &operator=(const expected_copy_base &rhs) = default;
  expected_copy_base &operator=(expected_copy_base &&rhs) = default;
};

// This class manages conditionally having a trivial move constructor
// Unfortunately there's no way to achieve this in GCC < 5 AFAIK, since it
// doesn't implement an analogue to std::is_trivially_move_constructible. We
// have to make do with a non-trivial move constructor even if T is trivially
// move constructible
#ifndef TL_EXPECTED_GCC49
template <class T, class E,
          bool = is_void_or<T, std::is_trivially_move_constructible<T>>::value
              &&std::is_trivially_move_constructible<E>::value>
struct expected_move_base : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;
};
#else
template <class T, class E, bool = false> struct expected_move_base;
#endif
template <class T, class E>
struct expected_move_base<T, E, false> : expected_copy_base<T, E> {
  using expected_copy_base<T, E>::expected_copy_base;

  expected_move_base() = default;
  expected_move_base(const expected_move_base &rhs) = default;

  expected_move_base(expected_move_base &&rhs) noexcept(
      std::is_nothrow_move_constructible<T>::value)
      : expected_copy_base<T, E>(no_init) {
    if (rhs.has_value()) {
      this->construct_with(std::move(rhs));
    } else {
      this->construct_error(std::move(rhs.geterr()));
    }
  }
  expected_move_base &operator=(const expected_move_base &rhs) = default;
  expected_move_base &operator=(expected_move_base &&rhs) = default;
};

// This class manages conditionally having a trivial copy assignment operator
template <class T, class E,
          bool = is_void_or<
              T, conjunction<TL_EXPECTED_IS_TRIVIALLY_COPY_ASSIGNABLE(T),
                             TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(T),
                             TL_EXPECTED_IS_TRIVIALLY_DESTRUCTIBLE(T)>>::value
              &&TL_EXPECTED_IS_TRIVIALLY_COPY_ASSIGNABLE(E)::value
                  &&TL_EXPECTED_IS_TRIVIALLY_COPY_CONSTRUCTIBLE(E)::value
                      &&TL_EXPECTED_IS_TRIVIALLY_DESTRUCTIBLE(E)::value,
          bool = (is_copy_constructible_or_void<T>::value &&
             std::is_copy_constructible<E>::value &&
             is_copy_assignable_or_void<T>::value &&
             std::is_copy_assignable<E>::value)>
struct expected_copy_assign_base : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;
};

template <class T, class E>
struct expected_copy_assign_base<T, E, false, true> : expected_move_base<T, E> {
  using expected_move_base<T, E>::expected_move_base;

  expected_copy_assign_base() = default;
  expected_copy_assign_base(const expected_copy_assign_base &rhs) = default;

  expected_copy_assign_base(expected_copy_assign_base &&rhs) = default;
  expected_copy_assign_base &operator=(const expected_copy_assign_base &rhs) {
    this->assign(rhs);
    return *this;
  }
  expected_copy_assign_base &
  operator=(expected_copy_assign_base &&rhs) = default;
};

// This class manages conditionally having a trivial move assignment operator
// Unfortunately there's no way to achieve this in GCC < 5 AFAIK, since it
// doesn't implement an analogue to std::is_trivially_move_assignable. We have
// to make do with a non-trivial move assignment operator even if T is trivially
// move assignable
#ifndef TL_EXPECTED_GCC49
template <class T, class E,
          bool =
              is_void_or<T, conjunction<std::is_trivially_destructible<T>,
                                        std::is_trivially_move_constructible<T>,
                                        std::is_trivially_move_assignable<T>>>::
                  value &&std::is_trivially_destructible<E>::value
                      &&std::is_trivially_move_constructible<E>::value
                          &&std::is_trivially_move_assignable<E>::value>
struct expected_move_assign_base : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;
};
#else
template <class T, class E, bool = false> struct expected_move_assign_base;
#endif

template <class T, class E>
struct expected_move_assign_base<T, E, false>
    : expected_copy_assign_base<T, E> {
  using expected_copy_assign_base<T, E>::expected_copy_assign_base;

  expected_move_assign_base() = default;
  expected_move_assign_base(const expected_move_assign_base &rhs) = default;

  expected_move_assign_base(expected_move_assign_base &&rhs) = default;

  expected_move_assign_base &
  operator=(const expected_move_assign_base &rhs) = default;

  expected_move_assign_base &
  operator=(expected_move_assign_base &&rhs) noexcept(
      std::is_nothrow_move_constructible<T>::value
          &&std::is_nothrow_move_assignable<T>::value) {
    this->assign(std::move(rhs));
    return *this;
  }
};

// expected_delete_ctor_base will conditionally delete copy and move
// constructors depending on whether T is copy/move constructible
template <class T, class E,
          bool EnableCopy = (is_copy_constructible_or_void<T>::value &&
                             std::is_copy_constructible<E>::value),
          bool EnableMove = (is_move_constructible_or_void<T>::value &&
                             std::is_move_constructible<E>::value)>
struct expected_delete_ctor_base {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = default;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_ctor_base<T, E, true, false> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = delete;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_ctor_base<T, E, false, true> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = delete;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = default;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_ctor_base<T, E, false, false> {
  expected_delete_ctor_base() = default;
  expected_delete_ctor_base(const expected_delete_ctor_base &) = delete;
  expected_delete_ctor_base(expected_delete_ctor_base &&) noexcept = delete;
  expected_delete_ctor_base &
  operator=(const expected_delete_ctor_base &) = default;
  expected_delete_ctor_base &
  operator=(expected_delete_ctor_base &&) noexcept = default;
};

// expected_delete_assign_base will conditionally delete copy and move
// constructors depending on whether T and E are copy/move constructible +
// assignable
template <class T, class E,
          bool EnableCopy = (is_copy_constructible_or_void<T>::value &&
                             std::is_copy_constructible<E>::value &&
                             is_copy_assignable_or_void<T>::value &&
                             std::is_copy_assignable<E>::value),
          bool EnableMove = (is_move_constructible_or_void<T>::value &&
                             std::is_move_constructible<E>::value &&
                             is_move_assignable_or_void<T>::value &&
                             std::is_move_assignable<E>::value)>
struct expected_delete_assign_base {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
      default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = default;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_assign_base<T, E, true, false> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
      default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = default;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = delete;
};

template <class T, class E>
struct expected_delete_assign_base<T, E, false, true> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
      default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = delete;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = default;
};

template <class T, class E>
struct expected_delete_assign_base<T, E, false, false> {
  expected_delete_assign_base() = default;
  expected_delete_assign_base(const expected_delete_assign_base &) = default;
  expected_delete_assign_base(expected_delete_assign_base &&) noexcept =
      default;
  expected_delete_assign_base &
  operator=(const expected_delete_assign_base &) = delete;
  expected_delete_assign_base &
  operator=(expected_delete_assign_base &&) noexcept = delete;
};

// This is needed to be able to construct the expected_default_ctor_base which
// follows, while still conditionally deleting the default constructor.
struct default_constructor_tag {
  explicit constexpr default_constructor_tag() = default;
};

// expected_default_ctor_base will ensure that expected has a deleted default
// constructor if T is not default constructible.
// This specialization is for when T is default constructible
template <class T, class E,
          bool Enable =
              std::is_default_constructible<T>::value || std::is_void<T>::value>
struct expected_default_ctor_base {
  constexpr expected_default_ctor_base() noexcept = default;
  constexpr expected_default_ctor_base(
      expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
      default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base const &) noexcept = default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};

// This specialization is for when T is not default constructible
template <class T, class E> struct expected_default_ctor_base<T, E, false> {
  constexpr expected_default_ctor_base() noexcept = delete;
  constexpr expected_default_ctor_base(
      expected_default_ctor_base const &) noexcept = default;
  constexpr expected_default_ctor_base(expected_default_ctor_base &&) noexcept =
      default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base const &) noexcept = default;
  expected_default_ctor_base &
  operator=(expected_default_ctor_base &&) noexcept = default;

  constexpr explicit expected_default_ctor_base(default_constructor_tag) {}
};
} // namespace detail

template <class E> class bad_expected_access : public std::exception {
public:
  explicit bad_expected_access(E e) : m_val(std::move(e)) {}

  virtual const char *what() const noexcept override {
    return "Bad expected access";
  }

  const E &error() const & { return m_val; }
  E &error() & { return m_val; }
  const E &&error() const && { return std::move(m_val); }
  E &&error() && { return std::move(m_val); }

private:
  E m_val;
};

/// An `expected<T, E>` object is an object that contains the storage for
/// another object and manages the lifetime of this contained object `T`.
/// Alternatively it could contain the storage for another unexpected object
/// `E`. The contained object may not be initialized after the expected object
/// has been initialized, and may not be destroyed before the expected object
/// has been destroyed. The initialization state of the contained object is
/// tracked by the expected object.
template <class T, class E>
class TL_EXPECTED_NODISCARD expected :
                 private detail::expected_move_assign_base<T, E>,
                 private detail::expected_delete_ctor_base<T, E>,
                 private detail::expected_delete_assign_base<T, E>,
                 private detail::expected_default_ctor_base<T, E> {
  static_assert(!std::is_reference<T>::value, "T must not be a reference");
  static_assert(!std::is_same<T, std::remove_cv<in_place_t>::type>::value,
                "T must not be in_place_t");
  static_assert(!std::is_same<T, std::remove_cv<unexpect_t>::type>::value,
                "T must not be unexpect_t");
  static_assert(
      !std::is_same<T, typename std::remove_cv<unexpected<E>>::type>::value,
      "T must not be unexpected<E>");
  static_assert(!std::is_reference<E>::value, "E must not be a reference");

  T *valptr() { return std::addressof(this->m_val); }
  const T *valptr() const { return std::addressof(this->m_val); }
  unexpected<E> *errptr() { return std::addressof(this->m_unexpect); }
  const unexpected<E> *errptr() const {
    return std::addressof(this->m_unexpect);
  }

  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR U &val() {
    return this->m_val;
  }
  TL_EXPECTED_11_CONSTEXPR unexpected<E> &err() { return this->m_unexpect; }

  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  constexpr const U &val() const {
    return this->m_val;
  }
  constexpr const unexpected<E> &err() const { return this->m_unexpect; }

  using impl_base = detail::expected_move_assign_base<T, E>;
  using ctor_base = detail::expected_default_ctor_base<T, E>;

public:
  typedef T value_type;
  typedef E error_type;
  typedef unexpected<E> unexpected_type;

#if defined(TL_EXPECTED_CXX14) && !defined(TL_EXPECTED_GCC49) &&               \
    !defined(TL_EXPECTED_GCC54) && !defined(TL_EXPECTED_GCC55)
  template <class F> TL_EXPECTED_11_CONSTEXPR auto and_then(F &&f) & {
    return and_then_impl(*this, std::forward<F>(f));
  }
  template <class F> TL_EXPECTED_11_CONSTEXPR auto and_then(F &&f) && {
    return and_then_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto and_then(F &&f) const & {
    return and_then_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F> constexpr auto and_then(F &&f) const && {
    return and_then_impl(std::move(*this), std::forward<F>(f));
  }
#endif

#else
  template <class F>
  TL_EXPECTED_11_CONSTEXPR auto
  and_then(F &&f) & -> decltype(and_then_impl(std::declval<expected &>(),
                                              std::forward<F>(f))) {
    return and_then_impl(*this, std::forward<F>(f));
  }
  template <class F>
  TL_EXPECTED_11_CONSTEXPR auto
  and_then(F &&f) && -> decltype(and_then_impl(std::declval<expected &&>(),
                                               std::forward<F>(f))) {
    return and_then_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F>
  constexpr auto and_then(F &&f) const & -> decltype(and_then_impl(
      std::declval<expected const &>(), std::forward<F>(f))) {
    return and_then_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F>
  constexpr auto and_then(F &&f) const && -> decltype(and_then_impl(
      std::declval<expected const &&>(), std::forward<F>(f))) {
    return and_then_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

#if defined(TL_EXPECTED_CXX14) && !defined(TL_EXPECTED_GCC49) &&               \
    !defined(TL_EXPECTED_GCC54) && !defined(TL_EXPECTED_GCC55)
  template <class F> TL_EXPECTED_11_CONSTEXPR auto map(F &&f) & {
    return expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F> TL_EXPECTED_11_CONSTEXPR auto map(F &&f) && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto map(F &&f) const & {
    return expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto map(F &&f) const && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
#else
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(expected_map_impl(
      std::declval<expected &>(), std::declval<F &&>()))
  map(F &&f) & {
    return expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(expected_map_impl(std::declval<expected>(),
                                                      std::declval<F &&>()))
  map(F &&f) && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F>
  constexpr decltype(expected_map_impl(std::declval<const expected &>(),
                                       std::declval<F &&>()))
  map(F &&f) const & {
    return expected_map_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F>
  constexpr decltype(expected_map_impl(std::declval<const expected &&>(),
                                       std::declval<F &&>()))
  map(F &&f) const && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

#if defined(TL_EXPECTED_CXX14) && !defined(TL_EXPECTED_GCC49) &&               \
    !defined(TL_EXPECTED_GCC54) && !defined(TL_EXPECTED_GCC55)
  template <class F> TL_EXPECTED_11_CONSTEXPR auto transform(F &&f) & {
    return expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F> TL_EXPECTED_11_CONSTEXPR auto transform(F &&f) && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto transform(F &&f) const & {
    return expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto transform(F &&f) const && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
#else
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(expected_map_impl(
      std::declval<expected &>(), std::declval<F &&>()))
  transform(F &&f) & {
    return expected_map_impl(*this, std::forward<F>(f));
  }
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(expected_map_impl(std::declval<expected>(),
                                                      std::declval<F &&>()))
  transform(F &&f) && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F>
  constexpr decltype(expected_map_impl(std::declval<const expected &>(),
                                       std::declval<F &&>()))
  transform(F &&f) const & {
    return expected_map_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F>
  constexpr decltype(expected_map_impl(std::declval<const expected &&>(),
                                       std::declval<F &&>()))
  transform(F &&f) const && {
    return expected_map_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif

#if defined(TL_EXPECTED_CXX14) && !defined(TL_EXPECTED_GCC49) &&               \
    !defined(TL_EXPECTED_GCC54) && !defined(TL_EXPECTED_GCC55)
  template <class F> TL_EXPECTED_11_CONSTEXPR auto map_error(F &&f) & {
    return map_error_impl(*this, std::forward<F>(f));
  }
  template <class F> TL_EXPECTED_11_CONSTEXPR auto map_error(F &&f) && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto map_error(F &&f) const & {
    return map_error_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto map_error(F &&f) const && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
#else
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(map_error_impl(std::declval<expected &>(),
                                                   std::declval<F &&>()))
  map_error(F &&f) & {
    return map_error_impl(*this, std::forward<F>(f));
  }
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(map_error_impl(std::declval<expected &&>(),
                                                   std::declval<F &&>()))
  map_error(F &&f) && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F>
  constexpr decltype(map_error_impl(std::declval<const expected &>(),
                                    std::declval<F &&>()))
  map_error(F &&f) const & {
    return map_error_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F>
  constexpr decltype(map_error_impl(std::declval<const expected &&>(),
                                    std::declval<F &&>()))
  map_error(F &&f) const && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif
#if defined(TL_EXPECTED_CXX14) && !defined(TL_EXPECTED_GCC49) &&               \
    !defined(TL_EXPECTED_GCC54) && !defined(TL_EXPECTED_GCC55)
  template <class F> TL_EXPECTED_11_CONSTEXPR auto transform_error(F &&f) & {
    return map_error_impl(*this, std::forward<F>(f));
  }
  template <class F> TL_EXPECTED_11_CONSTEXPR auto transform_error(F &&f) && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F> constexpr auto transform_error(F &&f) const & {
    return map_error_impl(*this, std::forward<F>(f));
  }
  template <class F> constexpr auto transform_error(F &&f) const && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
#else
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(map_error_impl(std::declval<expected &>(),
                                                   std::declval<F &&>()))
  transform_error(F &&f) & {
    return map_error_impl(*this, std::forward<F>(f));
  }
  template <class F>
  TL_EXPECTED_11_CONSTEXPR decltype(map_error_impl(std::declval<expected &&>(),
                                                   std::declval<F &&>()))
  transform_error(F &&f) && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
  template <class F>
  constexpr decltype(map_error_impl(std::declval<const expected &>(),
                                    std::declval<F &&>()))
  transform_error(F &&f) const & {
    return map_error_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F>
  constexpr decltype(map_error_impl(std::declval<const expected &&>(),
                                    std::declval<F &&>()))
  transform_error(F &&f) const && {
    return map_error_impl(std::move(*this), std::forward<F>(f));
  }
#endif
#endif
  template <class F> expected TL_EXPECTED_11_CONSTEXPR or_else(F &&f) & {
    return or_else_impl(*this, std::forward<F>(f));
  }

  template <class F> expected TL_EXPECTED_11_CONSTEXPR or_else(F &&f) && {
    return or_else_impl(std::move(*this), std::forward<F>(f));
  }

  template <class F> expected constexpr or_else(F &&f) const & {
    return or_else_impl(*this, std::forward<F>(f));
  }

#ifndef TL_EXPECTED_NO_CONSTRR
  template <class F> expected constexpr or_else(F &&f) const && {
    return or_else_impl(std::move(*this), std::forward<F>(f));
  }
#endif
  constexpr expected() = default;
  constexpr expected(const expected &rhs) = default;
  constexpr expected(expected &&rhs) = default;
  expected &operator=(const expected &rhs) = default;
  expected &operator=(expected &&rhs) = default;

  template <class... Args,
            detail::enable_if_t<std::is_constructible<T, Args &&...>::value> * =
                nullptr>
  constexpr expected(in_place_t, Args &&...args)
      : impl_base(in_place, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr expected(in_place_t, std::initializer_list<U> il, Args &&...args)
      : impl_base(in_place, il, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  template <class G = E,
            detail::enable_if_t<std::is_constructible<E, const G &>::value> * =
                nullptr,
            detail::enable_if_t<!std::is_convertible<const G &, E>::value> * =
                nullptr>
  explicit constexpr expected(const unexpected<G> &e)
      : impl_base(unexpect, e.value()),
        ctor_base(detail::default_constructor_tag{}) {}

  template <
      class G = E,
      detail::enable_if_t<std::is_constructible<E, const G &>::value> * =
          nullptr,
      detail::enable_if_t<std::is_convertible<const G &, E>::value> * = nullptr>
  constexpr expected(unexpected<G> const &e)
      : impl_base(unexpect, e.value()),
        ctor_base(detail::default_constructor_tag{}) {}

  template <
      class G = E,
      detail::enable_if_t<std::is_constructible<E, G &&>::value> * = nullptr,
      detail::enable_if_t<!std::is_convertible<G &&, E>::value> * = nullptr>
  explicit constexpr expected(unexpected<G> &&e) noexcept(
      std::is_nothrow_constructible<E, G &&>::value)
      : impl_base(unexpect, std::move(e.value())),
        ctor_base(detail::default_constructor_tag{}) {}

  template <
      class G = E,
      detail::enable_if_t<std::is_constructible<E, G &&>::value> * = nullptr,
      detail::enable_if_t<std::is_convertible<G &&, E>::value> * = nullptr>
  constexpr expected(unexpected<G> &&e) noexcept(
      std::is_nothrow_constructible<E, G &&>::value)
      : impl_base(unexpect, std::move(e.value())),
        ctor_base(detail::default_constructor_tag{}) {}

  template <class... Args,
            detail::enable_if_t<std::is_constructible<E, Args &&...>::value> * =
                nullptr>
  constexpr explicit expected(unexpect_t, Args &&...args)
      : impl_base(unexpect, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  template <class U, class... Args,
            detail::enable_if_t<std::is_constructible<
                E, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  constexpr explicit expected(unexpect_t, std::initializer_list<U> il,
                              Args &&...args)
      : impl_base(unexpect, il, std::forward<Args>(args)...),
        ctor_base(detail::default_constructor_tag{}) {}

  template <class U, class G,
            detail::enable_if_t<!(std::is_convertible<U const &, T>::value &&
                                  std::is_convertible<G const &, E>::value)> * =
                nullptr,
            detail::expected_enable_from_other<T, E, U, G, const U &, const G &>
                * = nullptr>
  explicit TL_EXPECTED_11_CONSTEXPR expected(const expected<U, G> &rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(*rhs);
    } else {
      this->construct_error(rhs.error());
    }
  }

  template <class U, class G,
            detail::enable_if_t<(std::is_convertible<U const &, T>::value &&
                                 std::is_convertible<G const &, E>::value)> * =
                nullptr,
            detail::expected_enable_from_other<T, E, U, G, const U &, const G &>
                * = nullptr>
  TL_EXPECTED_11_CONSTEXPR expected(const expected<U, G> &rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(*rhs);
    } else {
      this->construct_error(rhs.error());
    }
  }

  template <
      class U, class G,
      detail::enable_if_t<!(std::is_convertible<U &&, T>::value &&
                            std::is_convertible<G &&, E>::value)> * = nullptr,
      detail::expected_enable_from_other<T, E, U, G, U &&, G &&> * = nullptr>
  explicit TL_EXPECTED_11_CONSTEXPR expected(expected<U, G> &&rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    } else {
      this->construct_error(std::move(rhs.error()));
    }
  }

  template <
      class U, class G,
      detail::enable_if_t<(std::is_convertible<U &&, T>::value &&
                           std::is_convertible<G &&, E>::value)> * = nullptr,
      detail::expected_enable_from_other<T, E, U, G, U &&, G &&> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR expected(expected<U, G> &&rhs)
      : ctor_base(detail::default_constructor_tag{}) {
    if (rhs.has_value()) {
      this->construct(std::move(*rhs));
    } else {
      this->construct_error(std::move(rhs.error()));
    }
  }

  template <
      class U = T,
      detail::enable_if_t<!std::is_convertible<U &&, T>::value> * = nullptr,
      detail::expected_enable_forward_value<T, E, U> * = nullptr>
  explicit TL_EXPECTED_MSVC2015_CONSTEXPR expected(U &&v)
      : expected(in_place, std::forward<U>(v)) {}

  template <
      class U = T,
      detail::enable_if_t<std::is_convertible<U &&, T>::value> * = nullptr,
      detail::expected_enable_forward_value<T, E, U> * = nullptr>
  TL_EXPECTED_MSVC2015_CONSTEXPR expected(U &&v)
      : expected(in_place, std::forward<U>(v)) {}

  template <
      class U = T, class G = T,
      detail::enable_if_t<std::is_nothrow_constructible<T, U &&>::value> * =
          nullptr,
      detail::enable_if_t<!std::is_void<G>::value> * = nullptr,
      detail::enable_if_t<
          (!std::is_same<expected<T, E>, detail::decay_t<U>>::value &&
           !detail::conjunction<std::is_scalar<T>,
                                std::is_same<T, detail::decay_t<U>>>::value &&
           std::is_constructible<T, U>::value &&
           std::is_assignable<G &, U>::value &&
           std::is_nothrow_move_constructible<E>::value)> * = nullptr>
  expected &operator=(U &&v) {
    if (has_value()) {
      val() = std::forward<U>(v);
    } else {
      err().~unexpected<E>();
      ::new (valptr()) T(std::forward<U>(v));
      this->m_has_val = true;
    }

    return *this;
  }

  template <
      class U = T, class G = T,
      detail::enable_if_t<!std::is_nothrow_constructible<T, U &&>::value> * =
          nullptr,
      detail::enable_if_t<!std::is_void<U>::value> * = nullptr,
      detail::enable_if_t<
          (!std::is_same<expected<T, E>, detail::decay_t<U>>::value &&
           !detail::conjunction<std::is_scalar<T>,
                                std::is_same<T, detail::decay_t<U>>>::value &&
           std::is_constructible<T, U>::value &&
           std::is_assignable<G &, U>::value &&
           std::is_nothrow_move_constructible<E>::value)> * = nullptr>
  expected &operator=(U &&v) {
    if (has_value()) {
      val() = std::forward<U>(v);
    } else {
      auto tmp = std::move(err());
      err().~unexpected<E>();

#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
      try {
        ::new (valptr()) T(std::forward<U>(v));
        this->m_has_val = true;
      } catch (...) {
        err() = std::move(tmp);
        throw;
      }
#else
      ::new (valptr()) T(std::forward<U>(v));
      this->m_has_val = true;
#endif
    }

    return *this;
  }

  template <class G = E,
            detail::enable_if_t<std::is_nothrow_copy_constructible<G>::value &&
                                std::is_assignable<G &, G>::value> * = nullptr>
  expected &operator=(const unexpected<G> &rhs) {
    if (!has_value()) {
      err() = rhs;
    } else {
      this->destroy_val();
      ::new (errptr()) unexpected<E>(rhs);
      this->m_has_val = false;
    }

    return *this;
  }

  template <class G = E,
            detail::enable_if_t<std::is_nothrow_move_constructible<G>::value &&
                                std::is_move_assignable<G>::value> * = nullptr>
  expected &operator=(unexpected<G> &&rhs) noexcept {
    if (!has_value()) {
      err() = std::move(rhs);
    } else {
      this->destroy_val();
      ::new (errptr()) unexpected<E>(std::move(rhs));
      this->m_has_val = false;
    }

    return *this;
  }

  template <class... Args, detail::enable_if_t<std::is_nothrow_constructible<
                               T, Args &&...>::value> * = nullptr>
  void emplace(Args &&...args) {
    if (has_value()) {
      val().~T();
    } else {
      err().~unexpected<E>();
      this->m_has_val = true;
    }
    ::new (valptr()) T(std::forward<Args>(args)...);
  }

  template <class... Args, detail::enable_if_t<!std::is_nothrow_constructible<
                               T, Args &&...>::value> * = nullptr>
  void emplace(Args &&...args) {
    if (has_value()) {
      val().~T();
      ::new (valptr()) T(std::forward<Args>(args)...);
    } else {
      auto tmp = std::move(err());
      err().~unexpected<E>();

#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
      try {
        ::new (valptr()) T(std::forward<Args>(args)...);
        this->m_has_val = true;
      } catch (...) {
        err() = std::move(tmp);
        throw;
      }
#else
      ::new (valptr()) T(std::forward<Args>(args)...);
      this->m_has_val = true;
#endif
    }
  }

  template <class U, class... Args,
            detail::enable_if_t<std::is_nothrow_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  void emplace(std::initializer_list<U> il, Args &&...args) {
    if (has_value()) {
      T t(il, std::forward<Args>(args)...);
      val() = std::move(t);
    } else {
      err().~unexpected<E>();
      ::new (valptr()) T(il, std::forward<Args>(args)...);
      this->m_has_val = true;
    }
  }

  template <class U, class... Args,
            detail::enable_if_t<!std::is_nothrow_constructible<
                T, std::initializer_list<U> &, Args &&...>::value> * = nullptr>
  void emplace(std::initializer_list<U> il, Args &&...args) {
    if (has_value()) {
      T t(il, std::forward<Args>(args)...);
      val() = std::move(t);
    } else {
      auto tmp = std::move(err());
      err().~unexpected<E>();

#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
      try {
        ::new (valptr()) T(il, std::forward<Args>(args)...);
        this->m_has_val = true;
      } catch (...) {
        err() = std::move(tmp);
        throw;
      }
#else
      ::new (valptr()) T(il, std::forward<Args>(args)...);
      this->m_has_val = true;
#endif
    }
  }

private:
  using t_is_void = std::true_type;
  using t_is_not_void = std::false_type;
  using t_is_nothrow_move_constructible = std::true_type;
  using move_constructing_t_can_throw = std::false_type;
  using e_is_nothrow_move_constructible = std::true_type;
  using move_constructing_e_can_throw = std::false_type;

  void swap_where_both_have_value(expected & /*rhs*/, t_is_void) noexcept {
    // swapping void is a no-op
  }

  void swap_where_both_have_value(expected &rhs, t_is_not_void) {
    using std::swap;
    swap(val(), rhs.val());
  }

  void swap_where_only_one_has_value(expected &rhs, t_is_void) noexcept(
      std::is_nothrow_move_constructible<E>::value) {
    ::new (errptr()) unexpected_type(std::move(rhs.err()));
    rhs.err().~unexpected_type();
    std::swap(this->m_has_val, rhs.m_has_val);
  }

  void swap_where_only_one_has_value(expected &rhs, t_is_not_void) {
    swap_where_only_one_has_value_and_t_is_not_void(
        rhs, typename std::is_nothrow_move_constructible<T>::type{},
        typename std::is_nothrow_move_constructible<E>::type{});
  }

  void swap_where_only_one_has_value_and_t_is_not_void(
      expected &rhs, t_is_nothrow_move_constructible,
      e_is_nothrow_move_constructible) noexcept {
    auto temp = std::move(val());
    val().~T();
    ::new (errptr()) unexpected_type(std::move(rhs.err()));
    rhs.err().~unexpected_type();
    ::new (rhs.valptr()) T(std::move(temp));
    std::swap(this->m_has_val, rhs.m_has_val);
  }

  void swap_where_only_one_has_value_and_t_is_not_void(
      expected &rhs, t_is_nothrow_move_constructible,
      move_constructing_e_can_throw) {
    auto temp = std::move(val());
    val().~T();
#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
    try {
      ::new (errptr()) unexpected_type(std::move(rhs.err()));
      rhs.err().~unexpected_type();
      ::new (rhs.valptr()) T(std::move(temp));
      std::swap(this->m_has_val, rhs.m_has_val);
    } catch (...) {
      val() = std::move(temp);
      throw;
    }
#else
    ::new (errptr()) unexpected_type(std::move(rhs.err()));
    rhs.err().~unexpected_type();
    ::new (rhs.valptr()) T(std::move(temp));
    std::swap(this->m_has_val, rhs.m_has_val);
#endif
  }

  void swap_where_only_one_has_value_and_t_is_not_void(
      expected &rhs, move_constructing_t_can_throw,
      e_is_nothrow_move_constructible) {
    auto temp = std::move(rhs.err());
    rhs.err().~unexpected_type();
#ifdef TL_EXPECTED_EXCEPTIONS_ENABLED
    try {
      ::new (rhs.valptr()) T(std::move(val()));
      val().~T();
      ::new (errptr()) unexpected_type(std::move(temp));
      std::swap(this->m_has_val, rhs.m_has_val);
    } catch (...) {
      rhs.err() = std::move(temp);
      throw;
    }
#else
    ::new (rhs.valptr()) T(std::move(val()));
    val().~T();
    ::new (errptr()) unexpected_type(std::move(temp));
    std::swap(this->m_has_val, rhs.m_has_val);
#endif
  }

public:
  template <class OT = T, class OE = E>
  detail::enable_if_t<detail::is_swappable<OT>::value &&
                      detail::is_swappable<OE>::value &&
                      (std::is_nothrow_move_constructible<OT>::value ||
                       std::is_nothrow_move_constructible<OE>::value)>
  swap(expected &rhs) noexcept(
      std::is_nothrow_move_constructible<T>::value
          &&detail::is_nothrow_swappable<T>::value
              &&std::is_nothrow_move_constructible<E>::value
                  &&detail::is_nothrow_swappable<E>::value) {
    if (has_value() && rhs.has_value()) {
      swap_where_both_have_value(rhs, typename std::is_void<T>::type{});
    } else if (!has_value() && rhs.has_value()) {
      rhs.swap(*this);
    } else if (has_value()) {
      swap_where_only_one_has_value(rhs, typename std::is_void<T>::type{});
    } else {
      using std::swap;
      swap(err(), rhs.err());
    }
  }

  constexpr const T *operator->() const {
    TL_ASSERT(has_value());
    return valptr();
  }
  TL_EXPECTED_11_CONSTEXPR T *operator->() {
    TL_ASSERT(has_value());
    return valptr();
  }

  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  constexpr const U &operator*() const & {
    TL_ASSERT(has_value());
    return val();
  }
  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR U &operator*() & {
    TL_ASSERT(has_value());
    return val();
  }
  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  constexpr const U &&operator*() const && {
    TL_ASSERT(has_value());
    return std::move(val());
  }
  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR U &&operator*() && {
    TL_ASSERT(has_value());
    return std::move(val());
  }

  constexpr bool has_value() const noexcept { return this->m_has_val; }
  constexpr explicit operator bool() const noexcept { return this->m_has_val; }

  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR const U &value() const & {
    if (!has_value())
      TL_EXPECTED_THROW_EXCEPTION(bad_expected_access<E>(err().value()));
    return val();
  }
  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR U &value() & {
    if (!has_value())
      TL_EXPECTED_THROW_EXCEPTION(bad_expected_access<E>(err().value()));
    return val();
  }
  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR const U &&value() const && {
    if (!has_value())
      TL_EXPECTED_THROW_EXCEPTION(bad_expected_access<E>(std::move(err()).value()));
    return std::move(val());
  }
  template <class U = T,
            detail::enable_if_t<!std::is_void<U>::value> * = nullptr>
  TL_EXPECTED_11_CONSTEXPR U &&value() && {
    if (!has_value())
      TL_EXPECTED_THROW_EXCEPTION(bad_expected_access<E>(std::move(err()).value()));
    return std::move(val());
  }

  constexpr const E &error() const & {
    TL_ASSERT(!has_value());
    return err().value();
  }
  TL_EXPECTED_11_CONSTEXPR E &error() & {
    TL_ASSERT(!has_value());
    return err().value();
  }
  constexpr const E &&error() const && {
    TL_ASSERT(!has_value());
    return std::move(err().value());
  }
  TL_EXPECTED_11_CONSTEXPR E &&error() && {
    TL_ASSERT(!has_value());
    return std::move(err().value());
  }

  template <class U> constexpr T value_or(U &&v) const & {
    static_assert(std::is_copy_constructible<T>::value &&
                      std::is_convertible<U &&, T>::value,
                  "T must be copy-constructible and convertible to from U&&");
    return bool(*this) ? **this : static_cast<T>(std::forward<U>(v));
  }
  template <class U> TL_EXPECTED_11_CONSTEXPR T value_or(U &&v) && {
    static_assert(std::is_move_constructible<T>::value &&
                      std::is_convertible<U &&, T>::value,
                  "T must be move-constructible and convertible to from U&&");
    return bool(*this) ? std::move(**this) : static_cast<T>(std::forward<U>(v));
  }
};

namespace detail {
template <class Exp> using exp_t = typename detail::decay_t<Exp>::value_type;
template <class Exp> using err_t = typename detail::decay_t<Exp>::error_type;
template <class Exp, class Ret> using ret_t = expected<Ret, err_t<Exp>>;

#ifdef TL_EXPECTED_CXX14
template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Exp>()))>
constexpr auto and_then_impl(Exp &&exp, F &&f) {
  static_assert(detail::is_expected<Ret>::value, "F must return an expected");

  return exp.has_value()
             ? detail::invoke(std::forward<F>(f), *std::forward<Exp>(exp))
             : Ret(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>()))>
constexpr auto and_then_impl(Exp &&exp, F &&f) {
  static_assert(detail::is_expected<Ret>::value, "F must return an expected");

  return exp.has_value() ? detail::invoke(std::forward<F>(f))
                         : Ret(unexpect, std::forward<Exp>(exp).error());
}
#else
template <class> struct TC;
template <class Exp, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Exp>())),
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr>
auto and_then_impl(Exp &&exp, F &&f) -> Ret {
  static_assert(detail::is_expected<Ret>::value, "F must return an expected");

  return exp.has_value()
             ? detail::invoke(std::forward<F>(f), *std::forward<Exp>(exp))
             : Ret(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(detail::invoke(std::declval<F>())),
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr>
constexpr auto and_then_impl(Exp &&exp, F &&f) -> Ret {
  static_assert(detail::is_expected<Ret>::value, "F must return an expected");

  return exp.has_value() ? detail::invoke(std::forward<F>(f))
                         : Ret(unexpect, std::forward<Exp>(exp).error());
}
#endif

#ifdef TL_EXPECTED_CXX14
template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Exp>())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto expected_map_impl(Exp &&exp, F &&f) {
  using result = ret_t<Exp, detail::decay_t<Ret>>;
  return exp.has_value() ? result(detail::invoke(std::forward<F>(f),
                                                 *std::forward<Exp>(exp)))
                         : result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Exp>())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto expected_map_impl(Exp &&exp, F &&f) {
  using result = expected<void, err_t<Exp>>;
  if (exp.has_value()) {
    detail::invoke(std::forward<F>(f), *std::forward<Exp>(exp));
    return result();
  }

  return result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto expected_map_impl(Exp &&exp, F &&f) {
  using result = ret_t<Exp, detail::decay_t<Ret>>;
  return exp.has_value() ? result(detail::invoke(std::forward<F>(f)))
                         : result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto expected_map_impl(Exp &&exp, F &&f) {
  using result = expected<void, err_t<Exp>>;
  if (exp.has_value()) {
    detail::invoke(std::forward<F>(f));
    return result();
  }

  return result(unexpect, std::forward<Exp>(exp).error());
}
#else
template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Exp>())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>

constexpr auto expected_map_impl(Exp &&exp, F &&f)
    -> ret_t<Exp, detail::decay_t<Ret>> {
  using result = ret_t<Exp, detail::decay_t<Ret>>;

  return exp.has_value() ? result(detail::invoke(std::forward<F>(f),
                                                 *std::forward<Exp>(exp)))
                         : result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              *std::declval<Exp>())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>

auto expected_map_impl(Exp &&exp, F &&f) -> expected<void, err_t<Exp>> {
  if (exp.has_value()) {
    detail::invoke(std::forward<F>(f), *std::forward<Exp>(exp));
    return {};
  }

  return unexpected<err_t<Exp>>(std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>

constexpr auto expected_map_impl(Exp &&exp, F &&f)
    -> ret_t<Exp, detail::decay_t<Ret>> {
  using result = ret_t<Exp, detail::decay_t<Ret>>;

  return exp.has_value() ? result(detail::invoke(std::forward<F>(f)))
                         : result(unexpect, std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>

auto expected_map_impl(Exp &&exp, F &&f) -> expected<void, err_t<Exp>> {
  if (exp.has_value()) {
    detail::invoke(std::forward<F>(f));
    return {};
  }

  return unexpected<err_t<Exp>>(std::forward<Exp>(exp).error());
}
#endif

#if defined(TL_EXPECTED_CXX14) && !defined(TL_EXPECTED_GCC49) &&               \
    !defined(TL_EXPECTED_GCC54) && !defined(TL_EXPECTED_GCC55)
template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto map_error_impl(Exp &&exp, F &&f) {
  using result = expected<exp_t<Exp>, detail::decay_t<Ret>>;
  return exp.has_value()
             ? result(*std::forward<Exp>(exp))
             : result(unexpect, detail::invoke(std::forward<F>(f),
                                               std::forward<Exp>(exp).error()));
}
template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto map_error_impl(Exp &&exp, F &&f) {
  using result = expected<exp_t<Exp>, monostate>;
  if (exp.has_value()) {
    return result(*std::forward<Exp>(exp));
  }

  detail::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return result(unexpect, monostate{});
}
template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto map_error_impl(Exp &&exp, F &&f) {
  using result = expected<exp_t<Exp>, detail::decay_t<Ret>>;
  return exp.has_value()
             ? result()
             : result(unexpect, detail::invoke(std::forward<F>(f),
                                               std::forward<Exp>(exp).error()));
}
template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto map_error_impl(Exp &&exp, F &&f) {
  using result = expected<exp_t<Exp>, monostate>;
  if (exp.has_value()) {
    return result();
  }

  detail::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return result(unexpect, monostate{});
}
#else
template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto map_error_impl(Exp &&exp, F &&f)
    -> expected<exp_t<Exp>, detail::decay_t<Ret>> {
  using result = expected<exp_t<Exp>, detail::decay_t<Ret>>;

  return exp.has_value()
             ? result(*std::forward<Exp>(exp))
             : result(unexpect, detail::invoke(std::forward<F>(f),
                                               std::forward<Exp>(exp).error()));
}

template <class Exp, class F,
          detail::enable_if_t<!std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto map_error_impl(Exp &&exp, F &&f) -> expected<exp_t<Exp>, monostate> {
  using result = expected<exp_t<Exp>, monostate>;
  if (exp.has_value()) {
    return result(*std::forward<Exp>(exp));
  }

  detail::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return result(unexpect, monostate{});
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto map_error_impl(Exp &&exp, F &&f)
    -> expected<exp_t<Exp>, detail::decay_t<Ret>> {
  using result = expected<exp_t<Exp>, detail::decay_t<Ret>>;

  return exp.has_value()
             ? result()
             : result(unexpect, detail::invoke(std::forward<F>(f),
                                               std::forward<Exp>(exp).error()));
}

template <class Exp, class F,
          detail::enable_if_t<std::is_void<exp_t<Exp>>::value> * = nullptr,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
auto map_error_impl(Exp &&exp, F &&f) -> expected<exp_t<Exp>, monostate> {
  using result = expected<exp_t<Exp>, monostate>;
  if (exp.has_value()) {
    return result();
  }

  detail::invoke(std::forward<F>(f), std::forward<Exp>(exp).error());
  return result(unexpect, monostate{});
}
#endif

#ifdef TL_EXPECTED_CXX14
template <class Exp, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
constexpr auto or_else_impl(Exp &&exp, F &&f) {
  static_assert(detail::is_expected<Ret>::value, "F must return an expected");
  return exp.has_value() ? std::forward<Exp>(exp)
                         : detail::invoke(std::forward<F>(f),
                                          std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
detail::decay_t<Exp> or_else_impl(Exp &&exp, F &&f) {
  return exp.has_value() ? std::forward<Exp>(exp)
                         : (detail::invoke(std::forward<F>(f),
                                           std::forward<Exp>(exp).error()),
                            std::forward<Exp>(exp));
}
#else
template <class Exp, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<!std::is_void<Ret>::value> * = nullptr>
auto or_else_impl(Exp &&exp, F &&f) -> Ret {
  static_assert(detail::is_expected<Ret>::value, "F must return an expected");
  return exp.has_value() ? std::forward<Exp>(exp)
                         : detail::invoke(std::forward<F>(f),
                                          std::forward<Exp>(exp).error());
}

template <class Exp, class F,
          class Ret = decltype(detail::invoke(std::declval<F>(),
                                              std::declval<Exp>().error())),
          detail::enable_if_t<std::is_void<Ret>::value> * = nullptr>
detail::decay_t<Exp> or_else_impl(Exp &&exp, F &&f) {
  return exp.has_value() ? std::forward<Exp>(exp)
                         : (detail::invoke(std::forward<F>(f),
                                           std::forward<Exp>(exp).error()),
                            std::forward<Exp>(exp));
}
#endif
} // namespace detail

template <class T, class E, class U, class F>
constexpr bool operator==(const expected<T, E> &lhs,
                          const expected<U, F> &rhs) {
  return (lhs.has_value() != rhs.has_value())
             ? false
             : (!lhs.has_value() ? lhs.error() == rhs.error() : *lhs == *rhs);
}
template <class T, class E, class U, class F>
constexpr bool operator!=(const expected<T, E> &lhs,
                          const expected<U, F> &rhs) {
  return (lhs.has_value() != rhs.has_value())
             ? true
             : (!lhs.has_value() ? lhs.error() != rhs.error() : *lhs != *rhs);
}
template <class E, class F>
constexpr bool operator==(const expected<void, E> &lhs,
                          const expected<void, F> &rhs) {
  return (lhs.has_value() != rhs.has_value())
             ? false
             : (!lhs.has_value() ? lhs.error() == rhs.error() : true);
}
template <class E, class F>
constexpr bool operator!=(const expected<void, E> &lhs,
                          const expected<void, F> &rhs) {
  return (lhs.has_value() != rhs.has_value())
             ? true
             : (!lhs.has_value() ? lhs.error() != rhs.error() : false);
}

template <class T, class E, class U>
constexpr bool operator==(const expected<T, E> &x, const U &v) {
  return x.has_value() ? *x == v : false;
}
template <class T, class E, class U>
constexpr bool operator==(const U &v, const expected<T, E> &x) {
  return x.has_value() ? *x == v : false;
}
template <class T, class E, class U>
constexpr bool operator!=(const expected<T, E> &x, const U &v) {
  return x.has_value() ? *x != v : true;
}
template <class T, class E, class U>
constexpr bool operator!=(const U &v, const expected<T, E> &x) {
  return x.has_value() ? *x != v : true;
}

template <class T, class E>
constexpr bool operator==(const expected<T, E> &x, const unexpected<E> &e) {
  return x.has_value() ? false : x.error() == e.value();
}
template <class T, class E>
constexpr bool operator==(const unexpected<E> &e, const expected<T, E> &x) {
  return x.has_value() ? false : x.error() == e.value();
}
template <class T, class E>
constexpr bool operator!=(const expected<T, E> &x, const unexpected<E> &e) {
  return x.has_value() ? true : x.error() != e.value();
}
template <class T, class E>
constexpr bool operator!=(const unexpected<E> &e, const expected<T, E> &x) {
  return x.has_value() ? true : x.error() != e.value();
}

template <class T, class E,
          detail::enable_if_t<(std::is_void<T>::value ||
                               std::is_move_constructible<T>::value) &&
                              detail::is_swappable<T>::value &&
                              std::is_move_constructible<E>::value &&
                              detail::is_swappable<E>::value> * = nullptr>
void swap(expected<T, E> &lhs,
          expected<T, E> &rhs) noexcept(noexcept(lhs.swap(rhs))) {
  lhs.swap(rhs);
}
} // namespace tl

/**
 * @namespace RPL
 * @brief RoboMaster Packet Library 的主命名空间
 */
namespace RPL {
/**
 * @brief 可序列化概念
 *
 * 用于检查类型 T 是否为可序列化类型之一。
 * 此概念使用 std::decay_t 来忽略引用/const 限定符。
 *
 * @tparam T 要检查的类型
 * @tparam Ts 可序列化类型列表
 */
template <typename T, typename... Ts>
concept Serializable = (std::is_same_v<std::decay_t<T>, Ts> || ...);

/**
 * @brief 序列化器类
 *
 * 用于将数据包结构序列化为字节数组，包含帧头、命令码、数据长度、序列号和CRC校验。
 *
 * @tparam Ts 可序列化的数据包类型列表
 *
 * @par 使用示例
 * @code
 * RPL::Serializer<PacketA, PacketB> serializer;
 * 
 * std::array<uint8_t, 128> buffer;
 * PacketA pkt_a{...};
 * PacketB pkt_b{...};
 * 
 * auto result = serializer.serialize(buffer.data(), buffer.size(), pkt_a, pkt_b);
 * if (result) {
 *     // 发送 result.value() 字节的序列化数据
 *     uart_send(buffer.data(), *result);
 * }
 * @endcode
 *
 * @par 序列化格式
 * 根据 Protocol 定义，每个帧包含：
 * - 帧头（起始字节、长度、序列号、Header CRC8）
 * - 数据负载（命令码 + 数据）
 * - 帧尾（Frame CRC16）
 */
template <typename... Ts> class Serializer {
public:
  /**
   * @brief 将数据包序列化到用户提供的缓冲区
   *
   * 将多个数据包序列化为字节数组，每个数据包包含帧头、数据和帧尾。
   * 序列号会在每次序列化后自动递增。
   *
   * @tparam Packets 要序列化的数据包类型列表
   * @param buffer 用户提供的输出缓冲区
   * @param size 缓冲区大小
   * @param packets 要序列化的数据包
   * @return 序列化成功时返回写入的字节数，失败时返回错误信息
   *
   * @note 数据包按参数顺序序列化，每个数据包独立成帧
   * @warning 缓冲区大小必须足够容纳所有数据包的帧
   */
  template <typename... Packets>
    requires(Serializable<Packets, Ts...> && ...)
  tl::expected<size_t, Error> serialize(uint8_t *buffer, const size_t size,
                                        const Packets &...packets) {
    size_t offset = 0;

    static constexpr size_t total_size = (frame_size<Packets>() + ...);
    if (size < total_size) {
      return tl::make_unexpected(
          Error{ErrorCode::BufferOverflow, "Expecting a larger size buffer"});
    }

    auto serialize_one = [&]<typename T>(const T &packet) {
      using DecayedT = std::decay_t<T>;
      using Protocol = typename Meta::PacketTraits<DecayedT>::Protocol;
      constexpr uint16_t cmd = Meta::PacketTraits<DecayedT>::cmd;
      constexpr size_t data_size = Meta::PacketTraits<DecayedT>::size;
      constexpr size_t current_frame_size = frame_size<DecayedT>();

      uint8_t *current_buffer = buffer + offset;

      // 帧头 (起始字节)
      current_buffer[0] = Protocol::start_byte;
      if constexpr (Protocol::has_second_byte) {
        current_buffer[1] = Protocol::second_byte;
      }

      // 长度字段
      if constexpr (Protocol::has_length_field) {
        const auto data_size_u16 = static_cast<uint16_t>(data_size);
        // 长度字段采用小端格式
        if constexpr (Protocol::length_field_bytes == 1) {
          current_buffer[Protocol::length_offset] =
              static_cast<uint8_t>(data_size_u16 & 0xFF);
        } else {
          current_buffer[Protocol::length_offset] =
              static_cast<uint8_t>(data_size_u16 & 0xFF);
          current_buffer[Protocol::length_offset + 1] =
              static_cast<uint8_t>((data_size_u16 >> 8) & 0xFF);
        }
      }

      // Sequence 字段
      if constexpr (requires { Protocol::has_seq_field; }) {
        if constexpr (Protocol::has_seq_field) {
          current_buffer[Protocol::seq_offset] = m_Sequence;
        }
      }

      // 帧头 CRC
      if constexpr (Protocol::has_header_crc) {
        // CRC8 覆盖从 0 到 header_crc_offset 的字节
        const uint8_t header_crc8 =
            ProtocolCRC8::calc(current_buffer, Protocol::header_crc_offset);
        current_buffer[Protocol::header_crc_offset] = header_crc8;
      }

      // 命令 ID 字段
      if constexpr (Protocol::has_cmd_field) {
        // 命令字段采用小端格式
        if constexpr (Protocol::cmd_field_bytes == 1) {
          current_buffer[Protocol::cmd_offset] =
              static_cast<uint8_t>(cmd & 0xFF);
        } else {
          current_buffer[Protocol::cmd_offset] =
              static_cast<uint8_t>(cmd & 0xFF);
          current_buffer[Protocol::cmd_offset + 1] =
              static_cast<uint8_t>((cmd >> 8) & 0xFF);
        }
      }

      // Data Payload
      if constexpr (Meta::HasBitLayout<Meta::PacketTraits<DecayedT>>) {
        std::memset(current_buffer + Protocol::header_size, 0, data_size);
        serialize_bitstream<DecayedT>(
            std::span<uint8_t>(current_buffer + Protocol::header_size,
                               data_size),
            packet);
      } else {
        std::memcpy(current_buffer + Protocol::header_size, &packet, data_size);
      }

      // 帧尾 (CRC)
      // 使用协议特定的 CRC 算法
      using FrameCRC = typename Protocol::RPL_CRC;
      const uint16_t frame_crc16 =
          FrameCRC::calc(current_buffer, Protocol::header_size + data_size);

      // CRC16 采用小端格式
      current_buffer[Protocol::header_size + data_size] =
          static_cast<uint8_t>(frame_crc16 & 0xFF);
      current_buffer[Protocol::header_size + data_size + 1] =
          static_cast<uint8_t>((frame_crc16 >> 8) & 0xFF);

      offset += current_frame_size;
    };
    (serialize_one(packets), ...);

    m_Sequence += 1;
    return offset;
  }

  /**
   * @brief 计算指定类型的完整帧大小
   *
   * 计算包含帧头、数据和帧尾的完整帧大小。
   *
   * @tparam T 数据包类型
   * @return 完整帧大小（字节）
   */
  template <typename T>
    requires Serializable<T, Ts...>
  static constexpr size_t frame_size() noexcept {
    using DecayedT = std::decay_t<T>;
    using Protocol = typename Meta::PacketTraits<DecayedT>::Protocol;
    return Protocol::header_size + Meta::PacketTraits<DecayedT>::size +
           Protocol::tail_size;
  }

  /**
   * @brief 计算指定命令码的完整帧大小
   *
   * 根据命令码计算对应的完整帧大小。
   *
   * @param cmd 命令码
   * @return 对应的完整帧大小（字节），如果命令码无效则返回0
   */
  static constexpr size_t frame_size_by_cmd(uint16_t cmd) noexcept {
    size_t result = 0;
    ((Meta::PacketTraits<Ts>::cmd == cmd ? (result = frame_size<Ts>()) : 0),
     ...);
    return result;
  }

  /**
   * @brief 获取最大帧大小
   *
   * 获取所有可序列化类型中的最大帧大小，用于预分配缓冲区。
   *
   * @return 最大帧大小（字节）
   */
  static constexpr size_t max_frame_size() noexcept {
    return std::max({frame_size<Ts>()...});
  }

  /**
   * @brief 检查命令码是否有效
   *
   * 检查给定的命令码是否对应于任何可序列化类型。
   *
   * @param cmd 要检查的命令码
   * @return 如果命令码有效返回true，否则返回false
   */
  static constexpr bool is_valid_cmd(uint16_t cmd) noexcept {
    return ((Meta::PacketTraits<Ts>::cmd == cmd) || ...);
  }

  /**
   * @brief 通过命令码获取对应的类型索引
   *
   * 获取与指定命令码关联的类型在模板参数列表中的索引（用于调试）。
   *
   * @param cmd 命令码
   * @return 类型索引，如果命令码无效则返回 SIZE_MAX
   */
  static constexpr size_t get_type_index_by_cmd(uint16_t cmd) noexcept {
    size_t index = 0;
    bool found = false;
    (([&]() {
       if (Meta::PacketTraits<Ts>::cmd == cmd) {
         found = true;
       } else if (!found) {
         ++index;
       }
     }()),
     ...);
    return found ? index : SIZE_MAX;
  }

private:
  // 编译期命令码到类型映射的辅助函数
  template <uint16_t cmd, typename T, typename... Rest>
  static constexpr auto create_packet_by_cmd_impl() {
    if constexpr (Meta::PacketTraits<T>::cmd == cmd) {
      return std::optional<T>{};
    } else {
      if constexpr (sizeof...(Rest) > 0) {
        return create_packet_by_cmd_impl<cmd, Rest...>();
      } else {
        return std::nullopt;
      }
    }
  }

  uint8_t m_Sequence{}; ///< 序列号，每次序列化后递增

public:
  /**
   * @brief 获取当前序列号
   *
   * 获取当前的序列号值。
   * 序列号在每次成功序列化后自动递增。
   *
   * @return 当前序列号
   */
  [[nodiscard]] uint8_t get_sequence() const { return m_Sequence; }
};
} // namespace RPL

/**
 * @file Parser.hpp
 * @brief RPL库的解析器实现
 *
 * 此文件包含Parser类的定义，该类用于解析流式数据包。
 * 支持分片接收、噪声容错和并发多包处理。
 * 支持可选的连接健康检测功能。
 *
 * @author WindWeaver
 */

/**
 * @file BipBuffer.hpp
 * @brief RPL 双区缓冲区实现
 *
 * 实现一个双区环形缓冲区 (BipBuffer)，保证连续内存访问。
 * 它维护两个区域 (A 和 B)，通过在缓冲区起始处开启新区域来处理
 * 回绕 (wrap-around) 情况，避免数据分裂。
 *
 * BipBuffer 是一种特殊的环形缓冲区，它保证任何可读数据块在物理内存中
 * 是连续的。这使得解析器可以直接读取数据而无需处理回绕情况。
 *
 * @par 设计原理
 * - 区域 A: FIFO 的头部（最先读取的数据）
 * - 区域 B: FIFO 的尾部（在 A 之后读取的数据），起始位置始终为 0
 * - 写入时自动选择合适区域，避免数据跨越缓冲区边界
 *
 * @par 使用场景
 * - 流式数据包解析（如 Parser 类）
 * - DMA 直接内存访问的零拷贝写入
 * - 需要连续内存块的协议解析
 *
 * @code
 * RPL::Containers::BipBuffer<1024> buffer;
 *
 * // 获取写入缓冲区
 * auto write_span = buffer.get_write_buffer();
 * // DMA 或手动写入数据...
 * buffer.advance_write_index(received_bytes);
 *
 * // 读取数据
 * auto read_span = buffer.get_contiguous_read_buffer();
 * // 处理数据...
 * buffer.discard(processed_bytes);
 * @endcode
 *
 * @author WindWeaver
 * @date 2024
 */

namespace RPL::Containers {

/**
 * @brief 双区缓冲区类
 *
 * 保证任何可读数据块在物理内存中是连续的。
 * 消除了解析时对回绕检查的需要。
 *
 * @tparam SIZE 缓冲区大小，必须是 2 的幂
 */
template <size_t SIZE> class BipBuffer {
  static_assert((SIZE & (SIZE - 1)) == 0, "SIZE must be a power of 2");

  alignas(64) uint8_t buffer[SIZE]{};

  // 区域 A: FIFO 的头部 (最先读取的数据)
  size_t region_a_start{0};
  size_t region_a_size{0};

  // 区域 B: FIFO 的尾部 (在 A 之后读取的数据)
  // 在此实现中，如果存在区域 B，其起始位置始终为 0
  size_t region_b_size{0};

public:
  /**
   * @brief 获取连续缓冲区用于写入
   *
   * 返回可用于写入的最大连续块。
   * 如果尾部空间不足，这可能会切换到缓冲区起始处。
   *
   * @return 可写入的连续内存 span，如果缓冲区已满则返回空 span
   * @note 此方法支持零拷贝 DMA 写入
   */
  std::span<uint8_t> get_write_buffer() noexcept {
    // 情况 1: 区域 B 中有数据。只能追加到 B。
    // B 始终从 0 开始并增长。空间受限于 A 的起始位置。
    if (region_b_size > 0) {
      size_t free_space = region_a_start - region_b_size;
      return {buffer + region_b_size, free_space};
    }

    // 情况 2: 仅存在区域 A (或为空)。
    // 可以在 A 之后写入直到缓冲区末尾。
    // 或者如果 A 存在且 A 不在 0 位置，可以在 0 处开启 B。

    // 2a: 尝试追加到 A
    size_t a_end = region_a_start + region_a_size;
    size_t space_at_end = SIZE - a_end;

    // 如果尾部有足够空间，或者还不能切换到 B (A 从 0 开始)，返回尾部空间。

    if (space_at_end > 0) {
      return {buffer + a_end, space_at_end};
    }

    // 2b: 尾部无空间。尝试在 0 处开启区域 B。
    // 可用空间最多到 region_a_start。
    if (region_a_start > 0) {
      return {buffer, region_a_start};
    }

    // 缓冲区已满
    return {};

    // TODO: 接收模板参数，判断是否足够容纳数据包
  }

  /**
   * @brief 预留空间/提交写入
   *
   * 在数据写入缓冲区后调用此方法，以提交已写入的字节数。
   * 此方法会根据当前写入区域（A 或 B）更新相应的状态。
   *
   * @param length 已写入的字节数
   * @return true 如果成功提交
   * @return false 如果提交长度超出可用空间
   * @note 此方法应与 get_write_buffer() 或 get_write_buffer_force_wrap() 配合使用
   */
  bool advance_write_index(size_t length) {
    if (length == 0)
      return true;

    // 根据状态确定写入位置
    if (region_b_size > 0) {
      // 必定写入到 B
      if (region_b_size + length > region_a_start)
        return false; // 溢出
      region_b_size += length;
      return true;
    }

    size_t a_end = region_a_start + region_a_size;

    if (length <= SIZE - a_end) {
      region_a_size += length;
      return true;
    }

    if (region_a_size == 0) {
      // 空缓冲区逻辑
      region_a_size += length;
      return true;
    }

    // 如果尾部有空间，get_write_buffer 返回尾部。
    if (SIZE - a_end > 0) {
      if (length > SIZE - a_end)
        return false;
      region_a_size += length;
      return true;
    }

    // 如果尾部无空间，get_write_buffer 返回起始处。
    // 我们创建 B。
    if (length > region_a_start)
      return false;
    region_b_size = length;
    return true;
  }

  /**
   * @brief 手动切换到区域 B (从头部写入)
   *
   * 强制缓冲区跳过尾部剩余空间，从 0 位置开始写入。
   * 当尾部剩余空间太小无法容纳 incoming 数据包时很有用。
   *
   * @return 缓冲区起始处的 Span (区域 B)，如果无法切换则返回空 span
   * @note 此方法用于处理特殊情况，通常应使用 get_write_buffer()
   */
  std::span<uint8_t> get_write_buffer_force_wrap() {
    if (region_b_size > 0) {
      // 已经在 B 模式，返回 B 写入空间
      return get_write_buffer();
    }
    // 强制创建 B (如果可能)
    if (region_a_start > 0) {
      return {buffer, region_a_start};
    }
    return {};
  }

  /**
   * @brief 提交写入到强制回绕区域
   */
  bool advance_write_index_wrapped(size_t length) {
    if (region_b_size > 0) {
      return advance_write_index(length);
    }
    // 创建 B
    if (length > region_a_start)
      return false;
    region_b_size = length;
    return true;
  }

  /**
   * @brief 复制数据到缓冲区
   *
   * 将数据拷贝到内部缓冲区，自动选择合适的写入区域。
   * 如果当前区域空间不足，会尝试切换到另一区域。
   *
   * @param data 指向源数据的指针
   * @param length 要写入的字节数
   * @return true 如果成功写入
   * @return false 如果没有足够的连续空间
   * @note 此方法会执行内存拷贝，对于零拷贝场景请使用 get_write_buffer()
   */
  bool write(const uint8_t *data, size_t length) {
    if (length == 0)
      return true;

    auto span = get_write_buffer();

    // 逻辑: 尝试适配当前写入区域 (A 或 B)
    if (span.size() >= length) {
      std::memcpy(span.data(), data, length);
      return advance_write_index(length);
    }

    // 如果不行，且我们正在写入 A (B 为空)，
    // 检查是否可以切换到 B。
    if (region_b_size == 0) {
      // 检查起始处空间
      if (region_a_start >= length) {
        // 切换到 B!
        std::memcpy(buffer, data, length);
        region_b_size = length;
        return true;
      }
    }

    return false; // 无足够连续空间
  }

  /**
   * @brief 获取连续数据用于读取
   *
   * 返回当前可用的第一个连续数据块（区域 A 的内容）。
   * 读取并处理完数据后，应调用 discard() 来释放空间。
   *
   * @return 可读的连续内存 span，如果没有数据则返回空 span
   */
  [[nodiscard]] std::span<const uint8_t>
  get_contiguous_read_buffer() const noexcept {
    if (region_a_size > 0) {
      return {buffer + region_a_start, region_a_size};
    }
    return {};
  }

  /**
   * @brief 丢弃数据 (推进读取索引)
   *
   * 从缓冲区中移除已处理的数据，释放空间供后续写入使用。
   * 支持跨区域丢弃（同时丢弃区域 A 和 B 的数据）。
   *
   * @param length 要丢弃的字节数
   * @return true 如果成功丢弃
   * @return false 如果请求长度超过可用数据量
   */
  bool discard(size_t length) {
    if (length == 0)
      return true;
    if (length > available())
      return false;

    if (length <= region_a_size) {
      region_a_start += length;
      region_a_size -= length;
    } else {
      // 跨越到区域 B
      size_t remaining_from_b = length - region_a_size;
      region_a_start = 0;
      region_a_size = 0; // 临时清空 A，触发下面的 B 提升
      region_b_size -= remaining_from_b;
      // 将 B 提升为 A，起始位置移动到 B 被扣除后的新起点
      // 注意：B 始终从 0 开始增长。扣除 B 的头部后，新的 A 应该从
      // remaining_from_b 开始。
      region_a_start = remaining_from_b;
      region_a_size = region_b_size;
      region_b_size = 0;
    }

    // 如果 A 为空且 B 为空，重置到 0
    if (region_a_size == 0) {
      if (region_b_size > 0) {
        region_a_start = 0;
        region_a_size = region_b_size;
        region_b_size = 0;
      } else {
        region_a_start = 0;
      }
    }
    return true;
  }

  /**
   * @brief 获取两个连续的读视图 (用于零拷贝分段处理)
   *
   * 返回指定偏移量和长度的数据视图，可能分为两个连续的 span。
   * 如果数据是连续的，第二个 span 为空。
   *
   * @param offset 相对于可用数据的偏移量
   * @param length 要读取的长度
   * @return std::pair 包含两个 span。如果数据是连续的，第二个 span 为空。
   *         如果请求超出范围，两个 span 都为空。
   * @note 此方法用于零拷贝 CRC 校验等场景
   */
  [[nodiscard]] std::pair<std::span<const uint8_t>, std::span<const uint8_t>>
  get_read_spans(size_t offset, size_t length) const noexcept {
    if (offset + length > available())
      return {{}, {}};

    if (offset < region_a_size) {
      size_t a_readable = region_a_size - offset;
      if (length <= a_readable) {
        // 全在 A 中
        return {{buffer + region_a_start + offset, length}, {}};
      } else {
        // 跨越 A 和 B
        return {{buffer + region_a_start + offset, a_readable},
                {buffer, length - a_readable}};
      }
    } else {
      // 全在 B 中
      return {{buffer + (offset - region_a_size), length}, {}};
    }
  }

  // --- 便捷方法 ---
  
  /**
   * @brief 获取可用数据字节数
   * @return 当前缓冲区中可读数据的总字节数
   */
  size_t available() const { return region_a_size + region_b_size; }

  /**
   * @brief 获取可用写入空间
   *
   * 注意：这是总空闲字节数，不一定是连续的。
   * 实际可写入的连续空间可能小于此值。
   *
   * @return 总空闲字节数
   */
  size_t space() const {
    if (region_b_size > 0)
      return region_a_start - region_b_size;
    // 最大连续可写? 还是总空闲字节?
    // 总空闲字节:
    return (SIZE - (region_a_start + region_a_size)) + region_a_start;
  }

  /**
   * @brief 检查缓冲区是否已满
   * @return true 如果没有可用写入空间
   */
  bool full() const { return space() == 0; }
  
  /**
   * @brief 检查缓冲区是否为空
   * @return true 如果没有可读数据
   */
  bool empty() const { return available() == 0; }
  
  /**
   * @brief 清空缓冲区
   * 重置所有状态，丢弃所有数据
   */
  void clear() {
    region_a_start = region_a_size = 0;
    region_b_size = 0;
  }

  /**
   * @brief 获取缓冲区总容量
   * @return 缓冲区的总大小（编译期常量）
   */
  static constexpr size_t size() { return SIZE; }

  /**
   * @brief 窥视缓冲区数据（不丢弃）
   *
   * 将数据从缓冲区复制到外部缓冲区，但不修改读取索引。
   * 如果需要读取并丢弃数据，应使用 read() 方法。
   *
   * @param data 目标缓冲区指针
   * @param offset 相对于可用数据的偏移量
   * @param length 要读取的字节数
   * @return true 如果成功读取
   * @return false 如果请求超出可用范围
   */
  bool peek(uint8_t *data, size_t offset, size_t length) const {
    if (offset + length > available())
      return false;

    // 从 A 读取
    if (offset < region_a_size) {
      size_t read_from_a = std::min(length, region_a_size - offset);
      std::memcpy(data, buffer + region_a_start + offset, read_from_a);

      if (read_from_a < length) {
        // 剩余部分从 B 读取
        std::memcpy(data + read_from_a, buffer, length - read_from_a);
      }
    } else {
      // 仅从 B 读取
      size_t offset_in_b = offset - region_a_size;
      std::memcpy(data, buffer + offset_in_b, length);
    }
    return true;
  }

  /**
   * @brief 读取并丢弃数据
   *
   * 将数据从缓冲区复制到外部缓冲区，并丢弃已读取的数据。
   * 相当于 peek() 后调用 discard()。
   *
   * @param data 目标缓冲区指针
   * @param length 要读取的字节数
   * @return true 如果成功读取并丢弃
   * @return false 如果请求超出可用范围
   */
  bool read(uint8_t *data, size_t length) {
    if (!peek(data, 0, length))
      return false;
    discard(length);
    return true;
  }
};

} // namespace RPL::Containers

/**
 * @file ConnectionMonitor.hpp
 * @brief RPL 的连接健康检测工具
 *
 * 此文件提供连接监控策略类，用于检测通信链路是否正常工作。
 * 采用编译期策略模式，不需要监控时零开销。
 *
 * @par 设计原理
 * - 使用策略模式（Strategy Pattern）实现零开销抽象
 * - NullConnectionMonitor 在不需要监控时被完全优化掉
 * - TickConnectionMonitor 支持基于时间戳的超时检测
 * - CallbackConnectionMonitor 允许用户自定义回调逻辑
 *
 * @par 使用场景
 * - 检测通信链路是否断开
 * - 超时重连逻辑
 * - 自定义数据包接收事件处理
 *
 * @author WindWeaver
 */

namespace RPL {

/**
 * @brief 连接监控器概念
 *
 * 定义连接监控器必须满足的接口要求。
 * 任何连接监控器类型必须实现 on_packet_received() 方法。
 *
 * @tparam T 要检查的类型
 */
template <typename T>
concept ConnectionMonitorConcept = requires(T &monitor) {
  { monitor.on_packet_received() } -> std::same_as<void>;
};

/**
 * @brief 空连接监控器 (零开销默认实现)
 *
 * 当不需要连接监控功能时使用此策略。
 * 所有方法均为空实现，编译器会将其完全优化掉。
 *
 * @par 使用示例
 * @code
 * //  Parser 默认使用此监控器
 * RPL::Parser<PacketA, PacketB> parser{deserializer};
 * // 等同于:
 * RPL::Parser<RPL::NullConnectionMonitor, PacketA, PacketB> parser{deserializer};
 * @endcode
 */
struct NullConnectionMonitor {
  /**
   * @brief 数据包接收通知 (空实现)
   *
   * 此方法为空操作，编译器会将其完全优化掉。
   */
  constexpr void on_packet_received() noexcept {}
};

static_assert(ConnectionMonitorConcept<NullConnectionMonitor>,
              "NullConnectionMonitor must satisfy ConnectionMonitorConcept");

/**
 * @brief Tick 提供器概念
 *
 * 定义时间戳提供器必须满足的接口要求。
 * Tick 提供器必须定义 tick_type 类型和 now() 静态方法。
 *
 * @tparam T 要检查的类型
 */
template <typename T>
concept TickProviderConcept = requires {
  typename T::tick_type;
  { T::now() } -> std::convertible_to<typename T::tick_type>;
};

/**
 * @brief 基于时间戳的连接监控器
 *
 * 记录最后一次成功接收数据包的时间戳，
 * 支持检测连接是否在指定超时时间内活跃。
 *
 * @tparam TickProvider 时间戳提供器类型，需满足 TickProviderConcept
 *
 * @par TickProvider 实现示例
 * @code
 * // STM32 HAL 示例
 * struct HALTickProvider {
 *     using tick_type = uint32_t;
 *     static tick_type now() { return HAL_GetTick(); }
 * };
 * 
 * // Linux 时间戳示例
 * struct LinuxTickProvider {
 *     using tick_type = uint64_t;
 *     static tick_type now() { 
 *         struct timespec ts;
 *         clock_gettime(CLOCK_MONOTONIC, &ts);
 *         return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
 *     }
 * };
 * @endcode
 *
 * @par 使用示例
 * @code
 * using Monitor = RPL::TickConnectionMonitor<HALTickProvider>;
 * RPL::Parser<Monitor, PacketA, PacketB> parser{deserializer};
 *
 * // 检查连接状态
 * if (!parser.get_connection_monitor().is_connected(100)) {
 *     // 超过 100ms 未收到数据
 * }
 * @endcode
 */
template <TickProviderConcept TickProvider> class TickConnectionMonitor {
public:
  /// @brief 时间戳类型（由 TickProvider 定义）
  using tick_type = typename TickProvider::tick_type;

  /**
   * @brief 数据包接收通知
   *
   * 由 Parser 在成功解析数据包后调用，更新最后接收时间戳。
   */
  void on_packet_received() noexcept { last_tick_ = TickProvider::now(); }

  /**
   * @brief 检查连接是否活跃
   *
   * @param timeout 超时阈值（单位由 TickProvider 决定，通常为毫秒）
   * @return true 如果在超时时间内收到过数据包
   * @return false 如果超过超时时间未收到数据包
   */
  [[nodiscard]] bool is_connected(tick_type timeout) const noexcept {
    return (TickProvider::now() - last_tick_) < timeout;
  }

  /**
   * @brief 获取最后接收时间戳
   *
   * @return 最后一次成功接收数据包的时间戳
   */
  [[nodiscard]] tick_type get_last_tick() const noexcept { return last_tick_; }

  /**
   * @brief 获取自最后接收以来经过的时间
   *
   * @return 距离最后一次成功接收数据包经过的时间（与 tick_type 同单位）
   */
  [[nodiscard]] tick_type get_elapsed() const noexcept {
    return TickProvider::now() - last_tick_;
  }

  /**
   * @brief 重置监控器状态
   *
   * 将最后接收时间戳设为当前时间，相当于重新建立连接。
   */
  void reset() noexcept { last_tick_ = TickProvider::now(); }

private:
  volatile tick_type last_tick_{};
};

// 验证 TickConnectionMonitor 满足概念要求
namespace detail {
struct MockTickProvider {
  using tick_type = uint32_t;
  static tick_type now() { return 0; }
};
static_assert(
    ConnectionMonitorConcept<TickConnectionMonitor<MockTickProvider>>,
    "TickConnectionMonitor must satisfy ConnectionMonitorConcept");
} // namespace detail

/**
 * @brief 自定义回调连接监控器
 *
 * 允许用户提供自定义的回调函数，在收到数据包时执行。
 * 回调函数在编译期指定，零运行时开销。
 *
 * @tparam Callback 静态回调函数类型（必须实现 on_packet() 静态方法）
 *
 * @par 使用示例
 * @code
 * struct MyCallback {
 *     static void on_packet() {
 *         // 自定义逻辑：计数器++、设置标志位等
 *         packet_count++;
 *     }
 *     static inline uint32_t packet_count = 0;
 * };
 *
 * using Monitor = RPL::CallbackConnectionMonitor<MyCallback>;
 * RPL::Parser<Monitor, PacketA> parser{deserializer};
 * @endcode
 */
template <typename Callback>
  requires requires { Callback::on_packet(); }
struct CallbackConnectionMonitor {
  /**
   * @brief 数据包接收通知
   *
   * 调用用户提供的回调函数。
   */
  constexpr void on_packet_received() noexcept { Callback::on_packet(); }
};

} // namespace RPL

/**
 * @namespace RPL
 * @brief RoboMaster Packet Library 的主命名空间
 */
namespace RPL {

extern void RPL_ERROR_START_BYTE_COLLISION();

namespace Details {
// --- 类型去重工具 ---

/**
 * @brief 类型列表
 * @tparam Ts 类型列表
 */
template <typename... Ts> struct TypeList {};

/**
 * @brief 检查类型是否在列表中
 * @tparam T 要查找的类型
 * @tparam List 类型列表
 */
template <typename T, typename List> struct Contains;
template <typename T, typename... Ts>
struct Contains<T, TypeList<Ts...>> : std::disjunction<std::is_same<T, Ts>...> {
};

template <typename In, typename Out> struct UniqueImpl;
template <typename Out> struct UniqueImpl<TypeList<>, Out> {
  using type = Out;
};
template <typename H, typename... Ts, typename... Os>
struct UniqueImpl<TypeList<H, Ts...>, TypeList<Os...>> {
  using type = std::conditional_t<
      Contains<H, TypeList<Os...>>::value,
      typename UniqueImpl<TypeList<Ts...>, TypeList<Os...>>::type,
      typename UniqueImpl<TypeList<Ts...>, TypeList<Os..., H>>::type>;
};

template <typename... Ts>
using UniqueTypes_t = typename UniqueImpl<TypeList<Ts...>, TypeList<>>::type;

/**
 * @brief 运行时获取 Tuple 元素
 *
 * 根据索引在运行时选择 tuple 元素并调用函数。
 * 用于在运行时根据协议索引分发到编译期生成的 Worker。
 *
 * @tparam Tuple Tuple 类型
 * @tparam Fn 函数对象类型
 * @param i 索引
 * @param t Tuple 实例
 * @param f 要调用的函数
 */
template <typename Tuple, typename Fn, size_t... Is>
constexpr void tuple_switch(size_t i, Tuple &&t, Fn &&f,
                            std::index_sequence<Is...>) {
  ((i == Is ? f(std::get<Is>(t)) : void()), ...);
}

/**
 * @brief 便捷函数：运行时获取 Tuple 元素
 * @tparam Tuple Tuple 类型
 * @tparam Fn 函数对象类型
 * @param i 索引
 * @param t Tuple 实例
 * @param f 要调用的函数
 */
template <typename Tuple, typename Fn>
constexpr void runtime_get(size_t i, Tuple &&t, Fn &&f) {
  tuple_switch(i, std::forward<Tuple>(t), std::forward<Fn>(f),
               std::make_index_sequence<
                   std::tuple_size_v<std::remove_reference_t<Tuple>>>{});
}

// --- ConnectionMonitor 检测工具 ---

/**
 * @brief 检查类型是否有 PacketTraits 特化 (即是否是数据包类型)
 * @tparam T 要检查的类型
 */
template <typename T>
concept IsPacketType = requires {
  { Meta::PacketTraits<T>::cmd } -> std::convertible_to<uint16_t>;
  { Meta::PacketTraits<T>::size } -> std::convertible_to<size_t>;
};

/**
 * @brief 检查类型是否是 ConnectionMonitor (满足 concept 且不是 Packet)
 * @tparam T 要检查的类型
 */
template <typename T>
struct IsConnectionMonitor
    : std::bool_constant<ConnectionMonitorConcept<T> && !IsPacketType<T>> {};

// 从模板参数中提取 Monitor 和 Packets
template <typename... Args> struct ExtractMonitorAndPackets;

// 第一个参数是 ConnectionMonitor
template <typename M, typename... Ps>
  requires IsConnectionMonitor<M>::value
struct ExtractMonitorAndPackets<M, Ps...> {
  using Monitor = M;
  using Packets = TypeList<Ps...>;
};

// 第一个参数不是 ConnectionMonitor (全部是 Packet)
template <typename... Ps>
  requires(
      sizeof...(Ps) > 0 &&
      !IsConnectionMonitor<std::tuple_element_t<0, std::tuple<Ps...>>>::value)
struct ExtractMonitorAndPackets<Ps...> {
  using Monitor = NullConnectionMonitor;
  using Packets = TypeList<Ps...>;
};

// 空参数列表
template <> struct ExtractMonitorAndPackets<> {
  using Monitor = NullConnectionMonitor;
  using Packets = TypeList<>;
};

// --- 数据包 after_parse 分发器 ---

/**
 * @brief 根据 cmd_id 分发到对应数据包类型的 after_parse 回调
 *
 * 在 Parser 成功解析出数据包后，检测该类型是否定义了 after_parse 回调，
 * 并判断是否需要 skip_memory_pool。
 *
 * @tparam DeserializerType Deserializer 类型
 * @tparam List 数据包类型列表 (TypeList<Ts...>)
 */
template <typename DeserializerType, typename List>
struct PacketDispatcher;

template <typename DeserializerType, typename... Ts>
struct PacketDispatcher<DeserializerType, TypeList<Ts...>> {
  static bool dispatch(uint16_t cmd_id, std::span<const uint8_t> s1,
                       std::span<const uint8_t> s2,
                       DeserializerType & /*deserializer*/,
                       bool &skip_pool) {
    return (try_dispatch<Ts>(cmd_id, s1, s2, skip_pool) || ...);
  }

private:
  template <typename T>
  static bool try_dispatch(uint16_t cmd_id, std::span<const uint8_t> s1,
                           std::span<const uint8_t> s2, bool &skip_pool) {
    if (cmd_id != Meta::PacketTraits<T>::cmd)
      return false;
    using Traits = Meta::PacketTraits<T>;

    // 执行 after_parse 回调（如果定义了）
    if constexpr (requires {
                    Traits::after_parse(std::declval<const T &>());
                  }) {
      if constexpr (Meta::HasBitLayout<Traits>) {
        // BitLayout：先合并到临时缓冲区，再反序列化
        if (s2.empty()) {
          Traits::after_parse(RPL::deserialize_bitstream<T>(s1));
        } else {
          alignas(alignof(T)) std::array<uint8_t, Traits::size> temp{};
          std::memcpy(temp.data(), s1.data(), s1.size());
          std::memcpy(temp.data() + s1.size(), s2.data(), s2.size());
          Traits::after_parse(RPL::deserialize_bitstream<T>(
              std::span<const uint8_t>(temp.data(), temp.size())));
        }
      } else {
        // 普通 POD：连续则直接 reinterpret，不连续则先拷贝到对齐缓冲区
        if (s2.empty()) {
          Traits::after_parse(*reinterpret_cast<const T *>(s1.data()));
        } else {
          alignas(alignof(T)) std::array<uint8_t, Traits::size> temp{};
          std::memcpy(temp.data(), s1.data(), s1.size());
          std::memcpy(temp.data() + s1.size(), s2.data(), s2.size());
          Traits::after_parse(*reinterpret_cast<const T *>(temp.data()));
        }
      }
    }

    // 判断是否跳过 MemoryPool
    if constexpr (requires { Traits::skip_memory_pool; }) {
      skip_pool = Traits::skip_memory_pool;
    } else {
      skip_pool = false;
    }
    return true;
  }
};

} // namespace Details

/**
 * @brief 解析器类
 *
 * 用于解析流式数据包，支持多协议混合解析。
 * 根据 PacketTraits 中定义的 Protocol 自动生成查找表和解析逻辑。
 * 支持可选的连接监控功能。
 *
 * @tparam Args 模板参数列表，可以是:
 *              - 仅数据包类型: Parser<PacketA, PacketB>
 *              - ConnectionMonitor + 数据包类型: Parser<Monitor, PacketA,
 * PacketB>
 *
 * @code
 * // 方式1: 无监控 (零开销)
 * RPL::Parser<SampleA, SampleB> parser{deserializer};
 *
 * // 方式2: 使用内置 Tick 监控器
 * struct HALTickProvider {
 *     using tick_type = uint32_t;
 *     static tick_type now() { return HAL_GetTick(); }
 * };
 * using Monitor = RPL::TickConnectionMonitor<HALTickProvider>;
 * RPL::Parser<Monitor, SampleA, SampleB> parser{deserializer};
 *
 * if (!parser.get_connection_monitor().is_connected(100)) {
 *     // 超过 100ms 未收到数据
 * }
 * @endcode
 */
template <typename... Args> class Parser {
  // 提取 Monitor 和 Packet 类型
  using Extracted = Details::ExtractMonitorAndPackets<Args...>;
  using MonitorType = typename Extracted::Monitor;

  // 从 TypeList 展开 Packet 类型的辅助模板
  template <typename PacketList> struct ParserImpl;

  template <typename... Ts> struct ParserImpl<Details::TypeList<Ts...>> {
    // --- 解析 Worker ---
    template <typename P, bool IsFixed, uint16_t CmdId, size_t DataSize>
    struct ProtocolWorker {
      using Protocol = P;
      static constexpr bool is_fixed = IsFixed;
      static constexpr uint16_t fixed_cmd = CmdId;
      static constexpr size_t fixed_size = DataSize;
      static constexpr size_t min_frame_size =
          P::header_size + (IsFixed ? DataSize : 0) + P::tail_size;
    };

    // --- 为每个 T 提取 Worker 类型 ---
    template <typename T> struct GetWorker {
      using P = typename Meta::PacketTraits<T>::Protocol;
      static constexpr bool IsFixed = !P::has_cmd_field;
      static constexpr uint16_t C = IsFixed ? Meta::PacketTraits<T>::cmd : 0;
      static constexpr size_t S = IsFixed ? Meta::PacketTraits<T>::size : 0;
      using type = ProtocolWorker<P, IsFixed, C, S>;
    };

    // --- 生成去重后的 Worker 列表 ---
    template <typename TypeList> struct TupleFromList;
    template <typename... Ws> struct TupleFromList<Details::TypeList<Ws...>> {
      using type = std::tuple<Ws...>;
    };

    using AllWorkers = Details::TypeList<typename GetWorker<Ts>::type...>;
    using UniqueWorkers =
        Details::UniqueTypes_t<typename GetWorker<Ts>::type...>;
    using WorkerTuple = typename TupleFromList<UniqueWorkers>::type;

    // --- 编译期计算 Max Frame Size ---
    static constexpr size_t calculate_max_frame_size() {
      size_t max = 0;
      auto check = [&max]<typename T>() {
        using P = typename Meta::PacketTraits<T>::Protocol;
        size_t size =
            P::header_size + Meta::PacketTraits<T>::size + P::tail_size;
        if (size > max)
          max = size;
      };
      (check.template operator()<Ts>(), ...);
      return max;
    }

    static constexpr size_t max_frame_size = calculate_max_frame_size();

    // --- 计算 Buffer Size ---
    static consteval size_t calculate_buffer_size() {
      constexpr size_t min_size = max_frame_size * 4;
      if constexpr (std::has_single_bit(min_size))
        return min_size;
      else
        return std::bit_ceil(min_size);
    }

    static constexpr size_t buffer_size = calculate_buffer_size();

    // --- 构建查找表 ---
    static constexpr auto header_lut = []() {
      std::array<uint8_t, 256> table;
      table.fill(0xFF);

      auto register_worker = [&table]<typename W>(size_t index) {
        uint8_t sb = W::Protocol::start_byte;
        if (table[sb] != 0xFF && table[sb] != index) {
          RPL_ERROR_START_BYTE_COLLISION();
        }
        table[sb] = static_cast<uint8_t>(index);
      };

      auto helper =
          [&register_worker]<typename... Ws>(Details::TypeList<Ws...>) {
            size_t idx = 0;
            ((register_worker.template operator()<Ws>(idx++)), ...);
          };
      helper(UniqueWorkers{});

      return table;
    }();

    static constexpr uint8_t unique_start_byte = []() {
      uint8_t first_sb = 0xFF;
      size_t count = 0;
      for (int i = 0; i < 256; ++i) {
        if (header_lut[i] != 0xFF) {
          if (first_sb == 0xFF)
            first_sb = static_cast<uint8_t>(i);
          count++;
        }
      }
      return count == 1 ? first_sb : 0xFF;
    }();

    static constexpr bool has_multiple_start_bytes = []() {
      size_t count = 0;
      for (int i = 0; i < 256; ++i) {
        if (header_lut[i] != 0xFF)
          count++;
      }
      return count > 1;
    }();

    using DeserializerType = Deserializer<Ts...>;
  };

  using Impl = ParserImpl<typename Extracted::Packets>;
  using WorkerTuple = typename Impl::WorkerTuple;
  using UniqueWorkers = typename Impl::UniqueWorkers;

  static constexpr size_t max_frame_size = Impl::max_frame_size;
  static constexpr size_t buffer_size = Impl::buffer_size;
  static constexpr auto &header_lut = Impl::header_lut;
  static constexpr uint8_t unique_start_byte = Impl::unique_start_byte;
  static constexpr bool has_multiple_start_bytes = Impl::has_multiple_start_bytes;

  // 从 Packets TypeList 中提取 Deserializer 类型
  template <typename PacketList> struct DeserializerFromPackets;
  template <typename... Ts>
  struct DeserializerFromPackets<Details::TypeList<Ts...>> {
    using type = Deserializer<Ts...>;
  };

  using DeserializerType =
      typename DeserializerFromPackets<typename Extracted::Packets>::type;

  /**
   * @brief 解析结果枚举
   *
   * 表示单次解析尝试的结果状态。
   */
  enum class ParseResult { 
    Success,    ///< 成功解析一个完整帧
    Failure,    ///< 解析失败（校验错误等），需要丢弃数据并重试
    Incomplete  ///< 数据不完整，需要等待更多数据
  };

  // --- 成员变量 ---
  Containers::BipBuffer<buffer_size> buffer;
  DeserializerType &deserializer;
  [[no_unique_address]] MonitorType monitor_{};

public:
  explicit Parser(DeserializerType &des) : deserializer(des) {}

  /**
   * @brief 获取连接监控器引用
   *
   * @return 连接监控器的引用
   */
  MonitorType &get_connection_monitor() noexcept { return monitor_; }

  /**
   * @brief 获取连接监控器常量引用
   *
   * @return 连接监控器的常量引用
   */
  const MonitorType &get_connection_monitor() const noexcept {
    return monitor_;
  }

  /**
   * @brief 推送数据到解析器
   *
   * 将接收到的数据写入内部缓冲区，并尝试解析数据包。
   * 解析成功后，数据会自动从缓冲区移除。
   *
   * @param data 指向输入数据的指针
   * @param length 数据长度
   * @return void 或错误（缓冲区溢出）
   */
  tl::expected<void, Error> push_data(const uint8_t *data,
                                      const size_t length) {
    if (!buffer.write(data, length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Buffer overflow"});
    }
    return try_parse_packets();
  }

  /**
   * @brief 获取写入缓冲区（零拷贝）
   *
   * 返回一段连续的可写内存区域，供 DMA 或其他零拷贝写入使用。
   * 写入后必须调用 advance_write_index() 提交。
   *
   * @return 可写入的连续内存 span
   */
  std::span<uint8_t> get_write_buffer() noexcept {
    return buffer.get_write_buffer();
  }

  /**
   * @brief 提交写入缓冲区的数据
   *
   * 在使用 get_write_buffer() 获取的缓冲区写入后调用此方法。
   * 提交后会尝试解析新数据。
   *
   * @param length 已写入的字节数
   * @return void 或错误（提交长度无效）
   */
  tl::expected<void, Error> advance_write_index(size_t length) {
    if (!buffer.advance_write_index(length)) {
      return tl::unexpected(
          Error{ErrorCode::BufferOverflow, "Invalid advance length"});
    }
    return try_parse_packets();
  }

  /**
   * @brief 获取反序列化器的引用
   * @return 反序列化器引用
   */
  DeserializerType &get_deserializer() noexcept { return deserializer; }
  
  /**
   * @brief 获取可用数据字节数
   * @return 当前缓冲区中可读数据的总字节数
   */
  size_t available_data() const noexcept { return buffer.available(); }
  
  /**
   * @brief 获取可用写入空间
   * @return 总空闲字节数
   */
  size_t available_space() const noexcept { return buffer.space(); }
  
  /**
   * @brief 检查缓冲区是否已满
   * @return true 如果没有可用写入空间
   */
  bool is_buffer_full() const noexcept { return buffer.full(); }
  
  /**
   * @brief 清空缓冲区
   * 丢弃所有未处理的数据
   */
  void clear_buffer() noexcept { buffer.clear(); }

  /**
   * @brief 尝试解析缓冲区中的数据包
   *
   * 从当前读取位置开始扫描，查找并解析完整的数据帧。
   * 解析成功后，已处理的数据会自动从缓冲区移除。
   *
   * @return void 或错误（解析错误）
   * @note 此方法由 push_data() 和 advance_write_index() 自动调用
   *       也可以手动调用以在特定时间点触发解析
   */
  tl::expected<void, Error> try_parse_packets() {
    size_t available_bytes = buffer.available();

    // 只要有数据就开始扫描
    while (available_bytes > 0) {
      const auto buffer_view = buffer.get_contiguous_read_buffer();
      const uint8_t *data_ptr = buffer_view.data();
      const size_t view_size = buffer_view.size();

      size_t scan_offset = 0;
      bool frame_handled = false;

      while (scan_offset < view_size) {
        uint8_t worker_idx = 0xFF;

        if constexpr (unique_start_byte != 0xFF) {
          const uint8_t *next_sb = static_cast<const uint8_t *>(
              std::memchr(data_ptr + scan_offset, unique_start_byte,
                          view_size - scan_offset));
          if (!next_sb) {
            scan_offset = view_size;
            break;
          }
          scan_offset = static_cast<size_t>(next_sb - data_ptr);
          worker_idx = header_lut[unique_start_byte];
        } else {
          // 优化多起始字节扫描
          while (scan_offset < view_size &&
                 (worker_idx = header_lut[data_ptr[scan_offset]]) == 0xFF) {
            scan_offset++;
          }
          if (scan_offset >= view_size)
            break;
        }

        // 找到潜在帧头，丢弃之前的垃圾数据
        if (scan_offset > 0) {
          buffer.discard(scan_offset);
          available_bytes -= scan_offset;
        }

        ParseResult result = ParseResult::Incomplete;

        // 使用 tuple_switch 动态分发到编译期生成的 Worker
        Details::runtime_get(worker_idx, WorkerTuple{},
                             [&](auto worker_instance) {
                               using WorkerType = decltype(worker_instance);
                               result = this->parse_frame_impl<WorkerType>();
                             });

        if (result == ParseResult::Success) {
          monitor_.on_packet_received();
          available_bytes = buffer.available();
          frame_handled = true;
          break;
        } else if (result == ParseResult::Failure) {
          // 失败，丢弃起始字节，继续扫描
          buffer.discard(1);
          available_bytes--;
          frame_handled = true;
          break;
        } else {
          // Incomplete -> 等待更多数据
          return {};
        }
      }

      if (!frame_handled) {
        if (scan_offset == view_size) {
          buffer.discard(view_size);
          available_bytes -= view_size;
        }
        if (available_bytes == 0)
          break;
      }
    }
    return {};
  }

private:
  // --- 通用帧解析实现 ---
  template <typename Worker> ParseResult parse_frame_impl() {
    using P = typename Worker::Protocol;

    if (buffer.available() < P::header_size)
      return ParseResult::Incomplete;

    // 获取帧头指针，尽量避免拷贝
    uint8_t header_stack_copy[P::header_size];
    const uint8_t *header_ptr = nullptr;
    auto [hs1, hs2] = buffer.get_read_spans(0, P::header_size);
    if (hs2.empty()) {
      header_ptr = hs1.data();
    } else {
      buffer.peek(header_stack_copy, 0, P::header_size);
      header_ptr = header_stack_copy;
    }

    if constexpr (P::has_second_byte) {
      if (header_ptr[1] != P::second_byte)
        return ParseResult::Failure;
    }

    if constexpr (P::has_header_crc) {
      if (RPL::ProtocolCRC8::calc(header_ptr, 4) != header_ptr[4])
        return ParseResult::Failure;
    }

    size_t data_len = 0;
    uint16_t cmd_id = 0;

    if constexpr (Worker::is_fixed) {
      data_len = Worker::fixed_size;
      cmd_id = Worker::fixed_cmd;
    } else {
      if constexpr (P::length_field_bytes == 2) {
        std::memcpy(&data_len, header_ptr + P::length_offset, 2);
      } else {
        data_len = header_ptr[P::length_offset];
      }
      if constexpr (P::cmd_field_bytes == 2) {
        std::memcpy(&cmd_id, header_ptr + P::cmd_offset, 2);
      }
      if (data_len > max_frame_size - P::header_size - P::tail_size)
        return ParseResult::Failure;
    }

    size_t total_len = P::header_size + data_len + P::tail_size;
    if (buffer.available() < total_len)
      return ParseResult::Incomplete;

    // 获取分段读视图，进行分段 CRC 校验
    auto [s1, s2] = buffer.get_read_spans(0, total_len);

    size_t calc_len = total_len - P::tail_size;
    uint16_t calc_crc = 0;

    if (calc_len <= s1.size()) {
      calc_crc = P::RPL_CRC::calc(s1.data(), calc_len);
    } else {
      calc_crc = P::RPL_CRC::calc(s1.data(), s1.size());
      calc_crc = P::RPL_CRC::calc(s2.data(), calc_len - s1.size(), calc_crc);
    }

    // 验证接收到的 CRC
    uint16_t recv_crc = 0;
    if (calc_len + 2 <= s1.size()) {
      std::memcpy(&recv_crc, s1.data() + calc_len, 2);
    } else if (calc_len >= s1.size()) {
      std::memcpy(&recv_crc, s2.data() + (calc_len - s1.size()), 2);
    } else {
      // CRC 跨越了 A/B 边界
      recv_crc =
          s1.data()[calc_len] | (static_cast<uint16_t>(s2.data()[0]) << 8);
    }

    if (calc_crc != recv_crc)
      return ParseResult::Failure;

    // 反序列化 (分段拷贝)
    std::span<const uint8_t> payload_s1, payload_s2;

    if (P::header_size < s1.size()) {
      payload_s1 = s1.subspan(P::header_size);
      if (payload_s1.size() >= data_len) {
        payload_s1 = payload_s1.subspan(0, data_len);
        // payload_s2 为空
      } else {
        payload_s2 = s2.subspan(0, data_len - payload_s1.size());
      }
    } else {
      // Payload 完全在 s2 中
      payload_s2 = s2.subspan(P::header_size - s1.size(), data_len);
    }

    bool skip_pool = false;
    Details::PacketDispatcher<DeserializerType,
                              typename Extracted::Packets>::dispatch(
        cmd_id, payload_s1, payload_s2, deserializer, skip_pool);

    if (!skip_pool) {
      deserializer.write_segmented(cmd_id, payload_s1, payload_s2);
    }

    // 统一丢弃
    buffer.discard(total_len);
    return ParseResult::Success;
  }
};

} // namespace RPL


#endif // RPL_SINGLE_HEADER_HPP
