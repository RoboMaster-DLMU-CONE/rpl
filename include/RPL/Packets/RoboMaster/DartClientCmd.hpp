#ifndef RPL_DARTCLIENTCMD_HPP
#define RPL_DARTCLIENTCMD_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 飞镖选手端指令数据，3Hz 频率发送
 */
struct DartClientCmd
{
    uint8_t dart_launch_opening_status; ///< 飞镖发射站状态：0-开启, 1-关闭, 2-切换中
    uint8_t reserved; ///< 保留位
    uint16_t target_change_time; ///< 切换击打目标时的比赛剩余时间 (秒)
    uint16_t latest_launch_cmd_time; ///< 最后一次确定发射指令时的比赛剩余时间 (秒)
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<DartClientCmd> : PacketTraitsBase<PacketTraits<DartClientCmd>>
{
    static constexpr uint16_t cmd = 0x020A;
    static constexpr size_t size = sizeof(DartClientCmd);
};
#endif // RPL_DARTCLIENTCMD_HPP
