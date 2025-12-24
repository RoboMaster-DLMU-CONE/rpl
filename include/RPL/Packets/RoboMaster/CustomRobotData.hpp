#ifndef RPL_CUSTOMROBOTDATA_HPP
#define RPL_CUSTOMROBOTDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 自定义控制器接收机器人数据，频率上限 10Hz
 */
struct CustomRobotData
{
    uint8_t[30] data; ///< 自定义数据段
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<CustomRobotData> : PacketTraitsBase<PacketTraits<CustomRobotData>>
{
    static constexpr uint16_t cmd = 0x0309;
    static constexpr size_t size = sizeof(CustomRobotData);
};
#endif // RPL_CUSTOMROBOTDATA_HPP
