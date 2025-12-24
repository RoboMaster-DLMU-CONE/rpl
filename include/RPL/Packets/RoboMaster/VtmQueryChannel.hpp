#ifndef RPL_VTMQUERYCHANNEL_HPP
#define RPL_VTMQUERYCHANNEL_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 查询图传出图信道 / 反馈
 */
struct VtmQueryChannel
{
    uint8_t query_byte; ///< 查询: 发送0; 反馈: 0-未设置, 1-6-信道
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<VtmQueryChannel> : PacketTraitsBase<PacketTraits<VtmQueryChannel>>
{
    static constexpr uint16_t cmd = 0x0F02;
    static constexpr size_t size = sizeof(VtmQueryChannel);
};
#endif // RPL_VTMQUERYCHANNEL_HPP
