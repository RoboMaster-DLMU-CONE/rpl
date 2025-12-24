#ifndef RPL_MAPCOMMAND_HPP
#define RPL_MAPCOMMAND_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 选手端小地图交互数据，触发发送
 */
struct MapCommand
{
    float target_position_x; ///< 目标位置 X (m)
    float target_position_y; ///< 目标位置 Y (m)
    uint8_t cmd_keyboard; ///< 按下的键盘按键
    uint8_t target_robot_id; ///< 对方机器人 ID
    uint16_t cmd_source; ///< 信息来源 ID
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<MapCommand> : PacketTraitsBase<PacketTraits<MapCommand>>
{
    static constexpr uint16_t cmd = 0x0303;
    static constexpr size_t size = sizeof(MapCommand);
};
#endif // RPL_MAPCOMMAND_HPP
