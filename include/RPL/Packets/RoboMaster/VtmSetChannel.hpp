#ifndef RPL_VTMSETCHANNEL_HPP
#define RPL_VTMSETCHANNEL_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 设置图传出图信道 / 反馈
 */
struct VtmSetChannel
{
    uint8_t channel_id; ///< 设置: 1-6; 反馈: 0-成功, 1-启动中, 2-错误
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<VtmSetChannel> : PacketTraitsBase<PacketTraits<VtmSetChannel>>
{
    static constexpr uint16_t cmd = 0x0F01;
    static constexpr size_t size = sizeof(VtmSetChannel);
};
#endif // RPL_VTMSETCHANNEL_HPP
