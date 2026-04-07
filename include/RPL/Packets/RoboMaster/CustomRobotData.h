#ifndef RPL_CUSTOMROBOTDATA_H
#define RPL_CUSTOMROBOTDATA_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 自定义控制器接收机器人数据，频率上限 10Hz
 */
struct CustomRobotData
{
    uint8_t data[30]; ///< 自定义数据段
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<CustomRobotData> : PacketTraitsBase<PacketTraits<CustomRobotData>>
{
    static constexpr uint16_t cmd = 0x0309;
    static constexpr size_t size = sizeof(CustomRobotData);
};
#endif // __cplusplus
#endif // RPL_CUSTOMROBOTDATA_H
