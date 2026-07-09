#ifndef RPL_ROBOTPOS_HPP
#define RPL_ROBOTPOS_HPP

#include <cstdint>
#include <array>
#include <RPL/Meta/PacketTraits.hpp>
#pragma pack(push, 1)

/**
 * @brief 机器人位置数据，1Hz 频率发送
 */
struct RobotPos
{
    float x; ///< 位置 X 坐标 (m)
    float y; ///< 位置 Y 坐标 (m)
    float angle; ///< 测速模块朝向 (度)，正北为0
};

template <>
struct RPL::Meta::PacketTraits<RobotPos> : PacketTraitsBase<PacketTraits<RobotPos>>
{
    static constexpr uint16_t cmd = 0x0203;
    static constexpr size_t size = sizeof(RobotPos);
};
#pragma pack(pop)
#endif // RPL_ROBOTPOS_HPP
