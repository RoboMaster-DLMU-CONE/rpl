/**
 * @file Serializer.hpp
 * @brief RPL库的序列化器实现
 *
 * 此文件包含Serializer类的定义，该类用于将数据包结构序列化为字节数组。
 * 序列化过程包括添加帧头、计算CRC校验和等。
 *
 * @author WindWeaver
 */

#ifndef RPL_SERIALIZER_HPP
#define RPL_SERIALIZER_HPP

#include "Meta/BitstreamSerializer.hpp"
#include "Meta/PacketTraits.hpp"
#include "Utils/Def.hpp"
#include "Utils/Error.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <tl/expected.hpp>
#include <type_traits>
#include <variant>

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

#endif // RPL_SERIALIZER_HPP
