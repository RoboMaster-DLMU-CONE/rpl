#ifndef RPL_CUSTOMCONTROLLERDATA_H
#define RPL_CUSTOMCONTROLLERDATA_H

#ifdef __cplusplus
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>
#else
#include <stdint.h>
#endif // __cplusplus

/**
 * @brief 自定义控制器与机器人交互数据，频率上限 30Hz
 */
struct CustomControllerData
{
    uint8_t data[30]; ///< 自定义数据内容
} __attribute__((packed));

#ifdef __cplusplus
template <>
struct RPL::Meta::PacketTraits<CustomControllerData> : PacketTraitsBase<PacketTraits<CustomControllerData>>
{
    static constexpr uint16_t cmd = 0x0302;
    static constexpr size_t size = sizeof(CustomControllerData);
};
#endif // __cplusplus
#endif // RPL_CUSTOMCONTROLLERDATA_H
