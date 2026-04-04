/**
 * @file Def.hpp
 * @brief RPL库的常量定义
 *
 * 此文件定义了RPL库中使用的帧格式常量。
 *
 * @author WindWeaver
 */

#ifndef RPL_DEF_HPP
#define RPL_DEF_HPP
#include <cppcrc.h>
#include <cstdint>

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

#endif // RPL_DEF_HPP
