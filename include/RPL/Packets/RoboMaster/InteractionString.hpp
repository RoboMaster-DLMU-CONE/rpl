#ifndef RPL_INTERACTIONSTRING_HPP
#define RPL_INTERACTIONSTRING_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 客户端绘制字符图形 (子协议)
 */
struct InteractionString
{
    uint8_t[15] graphic_data; ///< 图形配置数据 (同 InteractionFigure 结构)
    uint8_t[30] data; ///< 字符串内容
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<InteractionString> : PacketTraitsBase<PacketTraits<InteractionString>>
{
    static constexpr uint16_t cmd = 0x0110;
    static constexpr size_t size = sizeof(InteractionString);
};
#endif // RPL_INTERACTIONSTRING_HPP
