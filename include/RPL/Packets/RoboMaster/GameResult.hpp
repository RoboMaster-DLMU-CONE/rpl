#ifndef RPL_GAMERESULT_HPP
#define RPL_GAMERESULT_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 比赛结果数据，比赛结束触发
 */
struct GameResult
{
    uint8_t winner; ///< 0-平局, 1-红方胜利, 2-蓝方胜利
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<GameResult> : PacketTraitsBase<PacketTraits<GameResult>>
{
    static constexpr uint16_t cmd = 0x0002;
    static constexpr size_t size = sizeof(GameResult);
};
#endif // RPL_GAMERESULT_HPP
