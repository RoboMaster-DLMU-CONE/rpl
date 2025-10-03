#ifndef RPL_TESTPACKET_HPP
#define RPL_TESTPACKET_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
struct __attribute__((packed)) TestPacket
{
    uint8_t  sensor_id;                     // 传感器ID
    float    temperature;                   // 温度值(摄氏度)
    float    humidity;                      // 湿度百分比
    uint64_t timestamp;                     // 时间戳(毫秒)
};
template <>
struct RPL::Meta::PacketTraits<TestPacket> : PacketTraitsBase<PacketTraits<TestPacket>>
{
    static constexpr uint16_t cmd = 0x0105;
    static constexpr size_t size = sizeof(TestPacket);
};
#endif //RPL_TESTPACKET_HPP
