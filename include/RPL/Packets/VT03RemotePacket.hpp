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
 */
struct VT03RemotePacket {
  static constexpr uint16_t cmd = 0xA953; // 使用帧头作为命令码
  static constexpr size_t size = 17;      // 载荷大小

  // --- 字节 0-7 (64 bits) ---
  // 摇杆通道 (11 bits each)
  uint64_t right_stick_x : 11;   ///< bit 0-10: 右摇杆水平 (364-1684)
  uint64_t right_stick_y : 11;   ///< bit 11-21: 右摇杆竖直
  uint64_t left_stick_y : 11;    ///< bit 22-32: 左摇杆竖直
  uint64_t left_stick_x : 11;    ///< bit 33-43: 左摇杆水平
  uint64_t switch_state : 2;     ///< bit 44-45: 挡位开关 (0,1,2)
  uint64_t pause_btn : 1;        ///< bit 46: 暂停键
  uint64_t custom_left : 1;      ///< bit 47: 左自定义键
  uint64_t custom_right : 1;     ///< bit 48: 右自定义键
  uint64_t wheel : 11;           ///< bit 49-59: 拨轮
  uint64_t trigger : 1;          ///< bit 60: 扳机
  uint64_t reserved_padding : 3; ///< bit 61-63: 填充

  // --- 字节 8-13 (48 bits) ---
  // Mouse X: Offset 80, Len 16. (Local 64-79).
  int16_t mouse_x; ///< bit 64-79
  int16_t mouse_y; ///< bit 80-95
  int16_t mouse_z; ///< bit 96-111

  // --- 字节 14 (8 bits) ---
  // Mouse Left: Offset 128, Len 2. (Local 112-113).
  // Mouse Right: Offset 130, Len 2. (Local 114-115).
  // Mouse Middle: Offset 132, Len 2. (Local 116-117).
  // Gap: 134-135 (2 bits).
  uint8_t mouse_left : 1;       ///< bit 112 (value bit)
  uint8_t mouse_left_pad : 1;   ///< bit 113 (padding)
  uint8_t mouse_right : 1;      ///< bit 114
  uint8_t mouse_right_pad : 1;  ///< bit 115
  uint8_t mouse_middle : 1;     ///< bit 116
  uint8_t mouse_middle_pad : 1; ///< bit 117
  uint8_t mouse_reserved : 2;   ///< bit 118-119 (padding to 120/136)

  // --- 字节 15-16 (16 bits) ---
  // Keyboard: Offset 136, Len 16. (Local 120-135).
  uint16_t key_w : 1;     ///< bit 0
  uint16_t key_s : 1;     ///< bit 1
  uint16_t key_a : 1;     ///< bit 2
  uint16_t key_d : 1;     ///< bit 3
  uint16_t key_shift : 1; ///< bit 4
  uint16_t key_ctrl : 1;  ///< bit 5
  uint16_t key_q : 1;     ///< bit 6
  uint16_t key_e : 1;     ///< bit 7
  uint16_t key_r : 1;     ///< bit 8
  uint16_t key_f : 1;     ///< bit 9
  uint16_t key_g : 1;     ///< bit 10
  uint16_t key_z : 1;     ///< bit 11
  uint16_t key_x : 1;     ///< bit 12
  uint16_t key_c : 1;     ///< bit 13
  uint16_t key_v : 1;     ///< bit 14
  uint16_t key_b : 1;     ///< bit 15
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<VT03RemotePacket>
    : PacketTraitsBase<VT03RemotePacket> {
  using Protocol = VT03RemoteProtocol;
};

#endif // RPL_PACKETS_VT03_REMOTE_PACKET_HPP
