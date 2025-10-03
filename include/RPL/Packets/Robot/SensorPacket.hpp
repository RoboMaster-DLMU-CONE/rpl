#ifndef RPL_SENSORPACKET_HPP
#define RPL_SENSORPACKET_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
namespace Robot::Sensors {

struct __attribute__((packed)) SensorPacket
{
    uint16_t device_id;                     // 设备ID
    double   position_x;                    // X坐标
    double   position_y;                    // Y坐标
};
} // namespace Robot::Sensors
template <>
struct RPL::Meta::PacketTraits<Robot::Sensors::SensorPacket> : PacketTraitsBase<PacketTraits<Robot::Sensors::SensorPacket>>
{
    static constexpr uint16_t cmd = 0x0201;
    static constexpr size_t size = sizeof(Robot::Sensors::SensorPacket);
};
#endif //RPL_SENSORPACKET_HPP
