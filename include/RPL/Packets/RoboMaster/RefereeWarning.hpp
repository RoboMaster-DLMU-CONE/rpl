#ifndef RPL_REFEREEWARNING_HPP
#define RPL_REFEREEWARNING_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 裁判警告数据
 */
struct RefereeWarning
{
    uint8_t level; ///< 判罚等级：1-双方黄牌, 2-黄牌, 3-红牌, 4-判负
    uint8_t offending_robot_id; ///< 违规机器人 ID (如 1, 101)
    uint8_t count; ///< 违规机器人对应等级的违规次数
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RefereeWarning> : PacketTraitsBase<PacketTraits<RefereeWarning>>
{
    static constexpr uint16_t cmd = 0x0104;
    static constexpr size_t size = sizeof(RefereeWarning);
};
#endif // RPL_REFEREEWARNING_HPP
