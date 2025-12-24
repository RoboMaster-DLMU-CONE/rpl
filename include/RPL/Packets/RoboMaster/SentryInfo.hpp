#ifndef RPL_SENTRYINFO_HPP
#define RPL_SENTRYINFO_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 哨兵自主决策信息，1Hz 频率发送
 */
struct SentryInfo
{
    uint32_t allowed_ammo_exchange : 11; ///< 成功兑换的允许发弹量
    uint32_t remote_exchange_ammo_times : 4; ///< 成功远程兑换发弹量次数
    uint32_t remote_exchange_hp_times : 4; ///< 成功远程兑换血量次数
    uint32_t confirm_free_revive : 1; ///< 是否确认免费复活 (1=是)
    uint32_t confirm_immediate_revive : 1; ///< 是否确认立即复活 (1=是)
    uint32_t immediate_revive_cost : 10; ///< 兑换立即复活花费的金币数
    uint32_t reserved_0 : 1; ///< 保留位
    uint16_t reserved_1 : 12; ///< 保留位 (bit 0-11)
    uint16_t sentry_posture : 2; ///< 哨兵姿态：1-进攻, 2-防御, 3-移动
    uint16_t energy_activation_status : 1; ///< 能量机关是否可激活
    uint16_t reserved_2 : 1; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<SentryInfo> : PacketTraitsBase<PacketTraits<SentryInfo>>
{
    static constexpr uint16_t cmd = 0x020D;
    static constexpr size_t size = sizeof(SentryInfo);
};
#endif // RPL_SENTRYINFO_HPP
