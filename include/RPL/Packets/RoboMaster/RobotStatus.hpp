#ifndef RPL_ROBOTSTATUS_HPP
#define RPL_ROBOTSTATUS_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 机器人性能体系数据，10Hz 频率发送
 */
struct RobotStatus
{
    uint8_t robot_id; ///< 本机器人 ID
    uint8_t robot_level; ///< 机器人等级
    uint16_t current_hp; ///< 机器人当前血量
    uint16_t maximum_hp; ///< 机器人血量上限
    uint16_t shooter_barrel_cooling_value; ///< 机器人射击热量每秒冷却值
    uint16_t shooter_barrel_heat_limit; ///< 机器人射击热量上限
    uint16_t chassis_power_limit; ///< 机器人底盘功率上限
    uint8_t power_management_gimbal_output : 1; ///< Gimbal口输出：0-无输出，1-24V输出
    uint8_t power_management_chassis_output : 1; ///< Chassis口输出：0-无输出，1-24V输出
    uint8_t power_management_shooter_output : 1; ///< Shooter口输出：0-无输出，1-24V输出
    uint8_t reserved : 5; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RobotStatus> : PacketTraitsBase<PacketTraits<RobotStatus>>
{
    static constexpr uint16_t cmd = 0x0201;
    static constexpr size_t size = sizeof(RobotStatus);
};
#endif // RPL_ROBOTSTATUS_HPP
