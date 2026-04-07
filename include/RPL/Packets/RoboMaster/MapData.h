#ifndef RPL_MAPDATA_H
#define RPL_MAPDATA_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 哨兵/半自动机器人路径数据，1Hz 频率上限
 */
struct MapData
{
    uint8_t intention; ///< 意图：1-攻击, 2-防守, 3-移动
    uint16_t start_position_x; ///< 路径起点 X (dm)
    uint16_t start_position_y; ///< 路径起点 Y (dm)
    int8_t delta_x[49]; ///< 路径点 X 轴增量数组 (dm)
    int8_t delta_y[49]; ///< 路径点 Y 轴增量数组 (dm)
    uint16_t sender_id; ///< 发送者 ID
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<MapData> : PacketTraitsBase<PacketTraits<MapData>>
{
    static constexpr uint16_t cmd = 0x0307;
    static constexpr size_t size = sizeof(MapData);
};
#endif // __cplusplus
#endif // RPL_MAPDATA_H
