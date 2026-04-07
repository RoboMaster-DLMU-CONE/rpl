#ifndef RPL_INTERACTIONSTRING_H
#define RPL_INTERACTIONSTRING_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 客户端绘制字符图形 (子协议)
 */
struct InteractionString
{
    uint8_t graphic_data[15]; ///< 图形配置数据 (同 InteractionFigure 结构)
    uint8_t data[30]; ///< 字符串内容
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<InteractionString> : PacketTraitsBase<PacketTraits<InteractionString>>
{
    static constexpr uint16_t cmd = 0x0110;
    static constexpr size_t size = sizeof(InteractionString);
};
#endif // __cplusplus
#endif // RPL_INTERACTIONSTRING_H
