#ifndef RPL_GAMEROBOTHP_HPP
#define RPL_GAMEROBOTHP_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 机器人血量数据，3Hz 频率发送
 */
struct GameRobotHP
{
    uint16_t ally_1_robot_hp; ///< 己方 1 号英雄机器人血量
    uint16_t ally_2_robot_hp; ///< 己方 2 号工程机器人血量
    uint16_t ally_3_robot_hp; ///< 己方 3 号步兵机器人血量
    uint16_t ally_4_robot_hp; ///< 己方 4 号步兵机器人血量
    uint16_t ally_5_robot_hp; ///< 保留位 (原5号位)
    uint16_t ally_7_robot_hp; ///< 己方 7 号哨兵机器人血量
    uint16_t ally_outpost_hp; ///< 己方前哨站血量
    uint16_t ally_base_hp; ///< 己方基地血量
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<GameRobotHP> : PacketTraitsBase<PacketTraits<GameRobotHP>>
{
    static constexpr uint16_t cmd = 0x0003;
    static constexpr size_t size = sizeof(GameRobotHP);
};
#endif // RPL_GAMEROBOTHP_HPP
