#ifndef RPL_POWERHEATDATA_HPP
#define RPL_POWERHEATDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 实时底盘缓冲能量和射击热量数据，10Hz 频率发送
 */
struct PowerHeatData
{
    uint16_t reserved_0; ///< 保留位
    uint16_t reserved_1; ///< 保留位
    float reserved_2; ///< 保留位
    uint16_t buffer_energy; ///< 缓冲能量 (J)
    uint16_t shooter_17mm_1_barrel_heat; ///< 第1个17mm发射机构的射击热量
    uint16_t shooter_42mm_barrel_heat; ///< 42mm发射机构的射击热量
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<PowerHeatData> : PacketTraitsBase<PacketTraits<PowerHeatData>>
{
    static constexpr uint16_t cmd = 0x0202;
    static constexpr size_t size = sizeof(PowerHeatData);
};
#endif // RPL_POWERHEATDATA_HPP
