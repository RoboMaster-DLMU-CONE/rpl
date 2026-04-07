#ifndef RPL_SHOOTDATA_H
#define RPL_SHOOTDATA_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 实时射击数据，弹丸发射后发送
 */
struct ShootData
{
    uint8_t bullet_type; ///< 弹丸类型：1-17mm, 2-42mm
    uint8_t shooter_number; ///< 发射机构ID：1-17mm, 3-42mm
    uint8_t launching_frequency; ///< 弹丸射速 (Hz)
    float initial_speed; ///< 弹丸初速度 (m/s)
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<ShootData> : PacketTraitsBase<PacketTraits<ShootData>>
{
    static constexpr uint16_t cmd = 0x0207;
    static constexpr size_t size = sizeof(ShootData);
};
#endif // __cplusplus
#endif // RPL_SHOOTDATA_H
