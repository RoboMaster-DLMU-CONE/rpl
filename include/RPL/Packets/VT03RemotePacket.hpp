#ifndef RPL_PACKETS_VT03_REMOTE_PACKET_HPP
#define RPL_PACKETS_VT03_REMOTE_PACKET_HPP

#include "RPL/Meta/PacketTraits.hpp"
#include "RPL/Utils/Def.hpp"

struct VT03RemoteProtocol {
  // --- 帧头识别 ---
  static constexpr uint8_t start_byte = 0xA9;
  static constexpr bool has_second_byte = true;
  static constexpr uint8_t second_byte = 0x53;

  // --- 结构大小 ---
  static constexpr size_t header_size = 2;
  static constexpr size_t tail_size = 2; // CRC16

  // --- 校验 ---
  static constexpr bool has_header_crc = false;
  using RPL_CRC = RPL::ProtocolCRC16; // MCRF4XX

  // --- 策略 ---
  static constexpr bool has_length_field = false; // 固定长度
  static constexpr size_t length_offset = 0;
  static constexpr size_t length_field_bytes = 0;

  static constexpr bool has_cmd_field = false; // 固定类型
  static constexpr size_t cmd_offset = 0;
  static constexpr size_t cmd_field_bytes = 0;
};

/**
 * @brief VT03/VT13 图传模块遥控数据包
 *
 * 包含遥控器摇杆、按键、鼠标、键盘数据。
 * 数据位压缩存储，总载荷长度 17 字节。
 *
 * 字节布局（按协议比特序号，小端序）：
 * - 字节 0-7: 摇杆 (4x11bit) + 按键 (8bit)
 * - 字节 8-13: 鼠标 (3x16bit)
 * - 字节 14: 鼠标按键
 * - 字节 15-16: 键盘 (16bit)
 */
struct VT03RemotePacket {
  static constexpr uint16_t cmd = 0xA953; // 使用帧头作为命令码
  static constexpr size_t size = 17;      // 载荷大小

  // 原始字节缓冲区 - 使用字节数组确保跨平台一致性
  uint8_t buffer[17];

  // ========== 访问器方法 - 从原始字节解析 ==========

  // --- 摇杆 (11-bit, 范围 364-1684, 中位 1024) ---
  // 小端序位域布局：
  // right_stick_x: bit 0-10  = 字节 0[0-7] + 字节 1[0-2]
  // right_stick_y: bit 11-21 = 字节 1[3-7] + 字节 2[0-5]
  // left_stick_y:  bit 22-32 = 字节 2[6-7] + 字节 3[0-7] + 字节 4[0]
  // left_stick_x:  bit 33-43 = 字节 4[1-7] + 字节 5[0-3]
  uint16_t right_stick_x() const {
    return static_cast<uint16_t>(buffer[0] | ((buffer[1] & 0x07) << 8));
  }
  uint16_t right_stick_y() const {
    return static_cast<uint16_t>((buffer[1] >> 3) | ((buffer[2] & 0x3F) << 5));
  }
  uint16_t left_stick_y() const {
    return static_cast<uint16_t>((buffer[2] >> 6) | (buffer[3] << 2) | ((buffer[4] & 0x01) << 10));
  }
  uint16_t left_stick_x() const {
    return static_cast<uint16_t>((buffer[4] >> 1) | ((buffer[5] & 0x0F) << 7));
  }

  // --- 按键状态 ---
  uint8_t switch_state() const {
    return static_cast<uint8_t>((buffer[5] >> 4) & 0x03);
  }
  bool pause_btn() const {
    return (buffer[5] >> 6) & 0x01;
  }
  bool custom_left() const {
    return (buffer[5] >> 7) & 0x01;
  }
  bool custom_right() const {
    return buffer[6] & 0x01;
  }
  uint16_t wheel() const {
    // bit 49-59: 字节 6[1-7] + 字节 7[0-3]
    return static_cast<uint16_t>((buffer[6] >> 1) | ((buffer[7] & 0x0F) << 7));
  }
  bool trigger() const {
    // bit 60: 字节 7[4]
    return (buffer[7] >> 4) & 0x01;
  }

  // --- 鼠标 (16-bit 有符号) ---
  int16_t mouse_x() const {
    return static_cast<int16_t>(buffer[8] | (buffer[9] << 8));
  }
  int16_t mouse_y() const {
    return static_cast<int16_t>(buffer[10] | (buffer[11] << 8));
  }
  int16_t mouse_z() const {
    return static_cast<int16_t>(buffer[12] | (buffer[13] << 8));
  }

  // --- 鼠标按键 ---
  bool mouse_left() const {
    return buffer[14] & 0x01;
  }
  bool mouse_right() const {
    return (buffer[14] >> 1) & 0x01;
  }
  bool mouse_middle() const {
    return (buffer[14] >> 2) & 0x01;
  }

  // --- 键盘按键 (字节 15-16) ---
  bool key_w() const { return buffer[15] & 0x01; }
  bool key_s() const { return (buffer[15] >> 1) & 0x01; }
  bool key_a() const { return (buffer[15] >> 2) & 0x01; }
  bool key_d() const { return (buffer[15] >> 3) & 0x01; }
  bool key_shift() const { return (buffer[15] >> 4) & 0x01; }
  bool key_ctrl() const { return (buffer[15] >> 5) & 0x01; }
  bool key_q() const { return (buffer[15] >> 6) & 0x01; }
  bool key_e() const { return (buffer[15] >> 7) & 0x01; }
  bool key_r() const { return buffer[16] & 0x01; }
  bool key_f() const { return (buffer[16] >> 1) & 0x01; }
  bool key_g() const { return (buffer[16] >> 2) & 0x01; }
  bool key_z() const { return (buffer[16] >> 3) & 0x01; }
  bool key_x() const { return (buffer[16] >> 4) & 0x01; }
  bool key_c() const { return (buffer[16] >> 5) & 0x01; }
  bool key_v() const { return (buffer[16] >> 6) & 0x01; }
  bool key_b() const { return (buffer[16] >> 7) & 0x01; }
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<VT03RemotePacket>
    : PacketTraitsBase<VT03RemotePacket> {
  using Protocol = VT03RemoteProtocol;
};

#endif // RPL_PACKETS_VT03_REMOTE_PACKET_HPP
