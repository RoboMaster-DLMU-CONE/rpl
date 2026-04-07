#ifndef RPL_PROJECTILEALLOWANCE_H
#define RPL_PROJECTILEALLOWANCE_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 允许发弹量数据，10Hz 频率发送
 */
struct ProjectileAllowance
{
    uint16_t projectile_allowance_17mm; ///< 17mm 弹丸允许发弹量
    uint16_t projectile_allowance_42mm; ///< 42mm 弹丸允许发弹量
    uint16_t remaining_gold_coin; ///< 剩余金币数量
    uint16_t projectile_allowance_fortress; ///< 堡垒提供的 17mm 弹丸允许发弹量
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<ProjectileAllowance> : PacketTraitsBase<PacketTraits<ProjectileAllowance>>
{
    static constexpr uint16_t cmd = 0x0208;
    static constexpr size_t size = sizeof(ProjectileAllowance);
};
#endif // __cplusplus
#endif // RPL_PROJECTILEALLOWANCE_H
