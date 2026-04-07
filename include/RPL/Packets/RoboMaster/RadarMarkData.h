#ifndef RPL_RADARMARKDATA_H
#define RPL_RADARMARKDATA_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 雷达标记进度数据，1Hz 频率发送
 */
struct RadarMarkData
{
    uint16_t mark_progress; ///< 标记进度：bit 0-4 对方易伤, bit 5-9 己方特殊标识
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<RadarMarkData> : PacketTraitsBase<PacketTraits<RadarMarkData>>
{
    static constexpr uint16_t cmd = 0x020C;
    static constexpr size_t size = sizeof(RadarMarkData);
};
#endif // __cplusplus
#endif // RPL_RADARMARKDATA_H
