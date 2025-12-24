#ifndef RPL_MAPROBOTDATA_HPP
#define RPL_MAPROBOTDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 选手端小地图接收机器人位置，5Hz 频率上限
 */
struct MapRobotData
{
    uint16_t hero_position_x; ///< 英雄 X 坐标 (cm)
    uint16_t hero_position_y; ///< 英雄 Y 坐标 (cm)
    uint16_t engineer_position_x; ///< 工程 X 坐标 (cm)
    uint16_t engineer_position_y; ///< 工程 Y 坐标 (cm)
    uint16_t infantry_3_position_x; ///< 3号步兵 X 坐标 (cm)
    uint16_t infantry_3_position_y; ///< 3号步兵 Y 坐标 (cm)
    uint16_t infantry_4_position_x; ///< 4号步兵 X 坐标 (cm)
    uint16_t infantry_4_position_y; ///< 4号步兵 Y 坐标 (cm)
    uint16_t infantry_5_position_x; ///< 5号步兵 X 坐标 (cm)
    uint16_t infantry_5_position_y; ///< 5号步兵 Y 坐标 (cm)
    uint16_t sentry_position_x; ///< 哨兵 X 坐标 (cm)
    uint16_t sentry_position_y; ///< 哨兵 Y 坐标 (cm)
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<MapRobotData> : PacketTraitsBase<PacketTraits<MapRobotData>>
{
    static constexpr uint16_t cmd = 0x0305;
    static constexpr size_t size = sizeof(MapRobotData);
};
#endif // RPL_MAPROBOTDATA_HPP
