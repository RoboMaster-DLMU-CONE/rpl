#ifndef RPL_HURTDATA_HPP
#define RPL_HURTDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 伤害状态数据，伤害发生后发送
 */
struct HurtData
{
    uint8_t armor_id : 4; ///< 受击装甲板ID (0-4)
    uint8_t hp_deduction_reason : 4; ///< 扣血原因：0-弹丸, 1-撞击/离线, 5-撞击
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<HurtData> : PacketTraitsBase<PacketTraits<HurtData>>
{
    static constexpr uint16_t cmd = 0x0206;
    static constexpr size_t size = sizeof(HurtData);
};
#endif // RPL_HURTDATA_HPP
