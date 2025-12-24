#ifndef RPL_REMOTECONTROL_HPP
#define RPL_REMOTECONTROL_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 键鼠遥控数据，30Hz 频率发送
 */
struct RemoteControl
{
    int16_t mouse_x; ///< 鼠标 X 轴移动速度
    int16_t mouse_y; ///< 鼠标 Y 轴移动速度
    int16_t mouse_z; ///< 鼠标滚轮移动速度
    uint8_t left_button_down; ///< 鼠标左键按下 (1=按下)
    uint8_t right_button_down; ///< 鼠标右键按下 (1=按下)
    uint16_t keyboard_value; ///< 键盘按键掩码
    uint16_t reserved; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RemoteControl> : PacketTraitsBase<PacketTraits<RemoteControl>>
{
    static constexpr uint16_t cmd = 0x0304;
    static constexpr size_t size = sizeof(RemoteControl);
};
#endif // RPL_REMOTECONTROL_HPP
