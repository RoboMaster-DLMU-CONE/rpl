#ifndef RPL_ROBOTCUSTOMDATA_HPP
#define RPL_ROBOTCUSTOMDATA_HPP

#include <cstdint>
#include <array>
#include <RPL/Meta/PacketTraits.hpp>
#pragma pack(push, 1)

/**
 * @brief 机器人发送给自定义客户端的数据，频率上限 50Hz
 */
struct RobotCustomData
{
    std::array<uint8_t, 150> data; ///< 自定义数据段
};

template <>
struct RPL::Meta::PacketTraits<RobotCustomData> : PacketTraitsBase<PacketTraits<RobotCustomData>>
{
    static constexpr uint16_t cmd = 0x0310;
    static constexpr size_t size = sizeof(RobotCustomData);
};
#pragma pack(pop)
#endif // RPL_ROBOTCUSTOMDATA_HPP
