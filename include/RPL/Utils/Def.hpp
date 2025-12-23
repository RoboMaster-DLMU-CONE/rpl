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
#include <cstdint>

namespace RPL
{
    static constexpr uint8_t FRAME_START_BYTE = 0xA5;  ///< 帧起始字节
    static constexpr size_t FRAME_HEADER_SIZE = 7;     ///< 帧头大小（字节）
    static constexpr size_t FRAME_TAIL_SIZE = 2;       ///< 帧尾大小（字节）
}

#endif //RPL_DEF_HPP
