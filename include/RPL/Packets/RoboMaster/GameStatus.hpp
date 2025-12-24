#ifndef RPL_GAMESTATUS_HPP
#define RPL_GAMESTATUS_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 比赛状态数据，1Hz 频率发送
 */
struct GameStatus
{
    uint8_t game_type : 4; ///< 比赛类型：1-超级对抗赛, 2-单项赛, 3-ICRA, 4-3V3, 5-步兵对抗
    uint8_t game_progress : 4; ///< 当前比赛阶段：0-未开始, 1-准备, 2-自检, 3-5s倒计时, 4-比赛中, 5-结算中
    uint16_t stage_remain_time; ///< 当前阶段剩余时间，单位：秒
    uint64_t sync_timestamp; ///< UNIX 时间，当机器人正确连接到裁判系统的 NTP 服务器后生效
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<GameStatus> : PacketTraitsBase<PacketTraits<GameStatus>>
{
    static constexpr uint16_t cmd = 0x0001;
    static constexpr size_t size = sizeof(GameStatus);
};
#endif // RPL_GAMESTATUS_HPP
