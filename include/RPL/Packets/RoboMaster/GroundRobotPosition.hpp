#ifndef RPL_GROUNDROBOTPOSITION_HPP
#define RPL_GROUNDROBOTPOSITION_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 地面机器人位置数据，1Hz 频率发送
 */
struct GroundRobotPosition
{
    float hero_x; ///< 英雄 X 坐标 (m)
    float hero_y; ///< 英雄 Y 坐标 (m)
    float engineer_x; ///< 工程 X 坐标 (m)
    float engineer_y; ///< 工程 Y 坐标 (m)
    float standard_3_x; ///< 3号步兵 X 坐标 (m)
    float standard_3_y; ///< 3号步兵 Y 坐标 (m)
    float standard_4_x; ///< 4号步兵 X 坐标 (m)
    float standard_4_y; ///< 4号步兵 Y 坐标 (m)
    float reserved_0; ///< 保留位
    float reserved_1; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<GroundRobotPosition> : PacketTraitsBase<PacketTraits<GroundRobotPosition>>
{
    static constexpr uint16_t cmd = 0x020B;
    static constexpr size_t size = sizeof(GroundRobotPosition);
};
#endif // RPL_GROUNDROBOTPOSITION_HPP
