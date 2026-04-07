#ifndef RPL_ROBOTCUSTOMDATA_H
#define RPL_ROBOTCUSTOMDATA_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 机器人发送给自定义客户端的数据，频率上限 50Hz
 */
struct RobotCustomData
{
    uint8_t data[150]; ///< 自定义数据段
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<RobotCustomData> : PacketTraitsBase<PacketTraits<RobotCustomData>>
{
    static constexpr uint16_t cmd = 0x0310;
    static constexpr size_t size = sizeof(RobotCustomData);
};
#endif // __cplusplus
#endif // RPL_ROBOTCUSTOMDATA_H
