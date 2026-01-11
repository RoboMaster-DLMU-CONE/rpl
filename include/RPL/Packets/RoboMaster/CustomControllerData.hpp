#ifndef RPL_CUSTOMCONTROLLERDATA_HPP
#define RPL_CUSTOMCONTROLLERDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 自定义控制器与机器人交互数据，频率上限 30Hz
 */
struct CustomControllerData
{
    uint8_t data[30]; ///< 自定义数据内容
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<CustomControllerData> : PacketTraitsBase<PacketTraits<CustomControllerData>>
{
    static constexpr uint16_t cmd = 0x0302;
    static constexpr size_t size = sizeof(CustomControllerData);
};
#endif // RPL_CUSTOMCONTROLLERDATA_HPP
