#ifndef RPL_EVENTDATA_H
#define RPL_EVENTDATA_H

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
 * @brief 场地事件数据，1Hz 频率发送
 */
struct EventData
{
    uint32_t supply_station_occupy : 3; ///< bit 0-2：补给区占领状态
    uint32_t energy_mechanic_status : 4; ///< bit 3-6：能量机关状态（小/大能量机关）
    uint32_t center_gain_zone : 2; ///< bit 7-8：己方中央高地占领状态
    uint32_t trapezoid_gain_zone : 2; ///< bit 9-10：己方梯形高地占领状态
    uint32_t dart_hit_time : 9; ///< bit 11-19：对方飞镖最后一次击中时间 (0-420)
    uint32_t dart_hit_target : 3; ///< bit 20-22：对方飞镖最后一次击中目标类型
    uint32_t center_gain_status : 2; ///< bit 23-24：中心增益点占领状态 (仅RMUL)
    uint32_t fortress_gain_status : 2; ///< bit 25-26：己方堡垒增益点占领状态
    uint32_t outpost_gain_status : 2; ///< bit 27-28：己方前哨站增益点占领状态
    uint32_t base_gain_status : 1; ///< bit 29：己方基地增益点占领状态
    uint32_t reserved : 2; ///< bit 30-31：保留位
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<EventData> : PacketTraitsBase<PacketTraits<EventData>>
{
    static constexpr uint16_t cmd = 0x0101;
    static constexpr size_t size = 4;
    using BitLayout = std::tuple<
        Field<uint32_t, 3>,
        Field<uint32_t, 4>,
        Field<uint32_t, 2>,
        Field<uint32_t, 2>,
        Field<uint32_t, 9>,
        Field<uint32_t, 3>,
        Field<uint32_t, 2>,
        Field<uint32_t, 2>,
        Field<uint32_t, 2>,
        Field<uint32_t, 1>,
        Field<uint32_t, 2>
    >;
};
#endif // __cplusplus
#endif // RPL_EVENTDATA_H
