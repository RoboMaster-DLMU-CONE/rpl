#ifndef RPL_RADARMARKDATA_HPP
#define RPL_RADARMARKDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 雷达标记进度数据，1Hz 频率发送
 */
struct RadarMarkData
{
    uint16_t mark_progress; ///< 标记进度：bit 0-4 对方易伤, bit 5-9 己方特殊标识
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RadarMarkData> : PacketTraitsBase<PacketTraits<RadarMarkData>>
{
    static constexpr uint16_t cmd = 0x020C;
    static constexpr size_t size = sizeof(RadarMarkData);
};
#endif // RPL_RADARMARKDATA_HPP
