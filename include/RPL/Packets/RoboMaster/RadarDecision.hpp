#ifndef RPL_RADARDECISION_HPP
#define RPL_RADARDECISION_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 雷达自主决策指令 (子协议)
 */
struct RadarDecision
{
    uint8_t confirm_double_damage : 1; ///< 确认触发双倍易伤
    uint8_t cmd_type; ///< 指令类型 (见 byte1)
    uint8_t[6] key; ///< 密钥数据 (byte2-7)
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RadarDecision> : PacketTraitsBase<PacketTraits<RadarDecision>>
{
    static constexpr uint16_t cmd = 0x0121;
    static constexpr size_t size = sizeof(RadarDecision);
};
#endif // RPL_RADARDECISION_HPP
