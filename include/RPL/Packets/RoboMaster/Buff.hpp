#ifndef RPL_BUFF_HPP
#define RPL_BUFF_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 机器人增益和底盘能量数据，3Hz 频率发送
 */
struct Buff
{
    uint8_t recovery_buff; ///< 回血增益 (百分比)
    uint16_t cooling_buff; ///< 冷却增益 (值)
    uint8_t defence_buff; ///< 防御增益 (百分比)
    uint8_t vulnerability_buff; ///< 负防御增益 (百分比)
    uint16_t attack_buff; ///< 攻击增益 (百分比)
    uint8_t remaining_energy; ///< 剩余能量值反馈，bit位表示不同阈值
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<Buff> : PacketTraitsBase<PacketTraits<Buff>>
{
    static constexpr uint16_t cmd = 0x0204;
    static constexpr size_t size = sizeof(Buff);
};
#endif // RPL_BUFF_HPP
