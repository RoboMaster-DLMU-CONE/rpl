#ifndef RPL_ROBOTCUSTOMDATA_HPP
#define RPL_ROBOTCUSTOMDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 机器人发送给自定义客户端的数据，频率上限 50Hz
 */
struct RobotCustomData
{
    uint8_t[150] data; ///< 自定义数据段
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RobotCustomData> : PacketTraitsBase<PacketTraits<RobotCustomData>>
{
    static constexpr uint16_t cmd = 0x0310;
    static constexpr size_t size = sizeof(RobotCustomData);
};
#endif // RPL_ROBOTCUSTOMDATA_HPP
