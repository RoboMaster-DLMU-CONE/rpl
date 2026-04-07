#ifndef RPL_RADARINFO_H
#define RPL_RADARINFO_H

#ifdef __cplusplus
#include <cstdint>
#include <array>
#include <tuple>
#include <RPL/Meta/BitstreamTraits.hpp>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 雷达自主决策信息，1Hz 频率发送
 */
struct RadarInfo
{
    uint8_t double_damage_chance : 2; ///< 是否拥有触发双倍易伤机会
    uint8_t opponent_double_damage : 1; ///< 对方是否正在被触发双倍易伤
    uint8_t encrypt_level : 2; ///< 己方加密等级 (1-3)
    uint8_t key_editable : 1; ///< 是否可以修改密钥
    uint8_t reserved : 2; ///< 保留位
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<RadarInfo> : PacketTraitsBase<PacketTraits<RadarInfo>>
{
    static constexpr uint16_t cmd = 0x020E;
    static constexpr size_t size = 1;
    using BitLayout = std::tuple<
        Field<uint8_t, 2>,
        Field<uint8_t, 1>,
        Field<uint8_t, 2>,
        Field<uint8_t, 1>,
        Field<uint8_t, 2>
    >;
};
#endif // __cplusplus
#endif // RPL_RADARINFO_H
