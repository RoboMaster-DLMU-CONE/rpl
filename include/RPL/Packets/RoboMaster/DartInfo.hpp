#ifndef RPL_DARTINFO_HPP
#define RPL_DARTINFO_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 飞镖发射相关数据，1Hz 频率发送
 */
struct DartInfo
{
    uint8_t dart_remaining_time; ///< 己方飞镖发射剩余时间 (秒)
    uint16_t dart_target : 3; ///< bit 0-2: 最近一次击中目标
    uint16_t target_hit_count : 3; ///< bit 3-5: 对方最近被击中目标累计次数
    uint16_t selected_target : 2; ///< bit 6-7: 飞镖此时选定的击打目标
    uint16_t reserved : 8; ///< bit 8-15: 保留
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<DartInfo> : PacketTraitsBase<PacketTraits<DartInfo>>
{
    static constexpr uint16_t cmd = 0x0105;
    static constexpr size_t size = sizeof(DartInfo);
};
#endif // RPL_DARTINFO_HPP
