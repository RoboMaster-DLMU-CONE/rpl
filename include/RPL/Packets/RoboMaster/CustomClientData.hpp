#ifndef RPL_CUSTOMCLIENTDATA_HPP
#define RPL_CUSTOMCLIENTDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 自定义控制器模拟键鼠数据，30Hz 频率上限
 */
struct CustomClientData
{
    uint16_t key_value; ///< 键盘键值
    uint16_t x_position : 12; ///< 鼠标 X 轴像素位置
    uint16_t mouse_left : 4; ///< 鼠标左键状态
    uint16_t y_position : 12; ///< 鼠标 Y 轴像素位置
    uint16_t mouse_right : 4; ///< 鼠标右键状态
    uint16_t reserved; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<CustomClientData> : PacketTraitsBase<PacketTraits<CustomClientData>>
{
    static constexpr uint16_t cmd = 0x0306;
    static constexpr size_t size = sizeof(CustomClientData);
};
#endif // RPL_CUSTOMCLIENTDATA_HPP
