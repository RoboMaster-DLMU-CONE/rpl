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

#ifndef RPL_INFO_HPP
#define RPL_INFO_HPP

#include "RPL/Utils/Def.hpp"
#include <cstddef>
#include <cstdint>

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

#endif // RPL_INFO_HPP
