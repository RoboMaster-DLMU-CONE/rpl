#ifndef RPL_SENTRYDECISION_HPP
#define RPL_SENTRYDECISION_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 哨兵自主决策指令 (子协议)
 */
struct SentryDecision
{
    uint32_t confirm_revive : 1; ///< 确认复活
    uint32_t confirm_immediate_revive : 1; ///< 确认立即复活
    uint32_t exchange_ammo : 11; ///< 兑换发弹量
    uint32_t exchange_ammo_times : 4; ///< 远程兑换发弹量请求次数
    uint32_t exchange_hp_times : 4; ///< 远程兑换血量请求次数
    uint32_t posture_cmd : 2; ///< 姿态指令：1-进攻, 2-防御, 3-移动
    uint32_t confirm_energy_activation : 1; ///< 确认激活能量机关
    uint32_t reserved : 8; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<SentryDecision> : PacketTraitsBase<PacketTraits<SentryDecision>>
{
    static constexpr uint16_t cmd = 0x0120;
    static constexpr size_t size = sizeof(SentryDecision);
};
#endif // RPL_SENTRYDECISION_HPP
