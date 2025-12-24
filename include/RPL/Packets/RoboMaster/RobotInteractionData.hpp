#ifndef RPL_ROBOTINTERACTIONDATA_HPP
#define RPL_ROBOTINTERACTIONDATA_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 机器人间交互数据，发送方触发，频率上限 30Hz
 */
struct RobotInteractionData
{
    uint16_t data_cmd_id; ///< 子内容 ID (0x0200-0x02FF 等)
    uint16_t sender_id; ///< 发送者 ID
    uint16_t receiver_id; ///< 接收者 ID
    uint8_t[112] user_data; ///< 内容数据段 (最大 112 字节)
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RobotInteractionData> : PacketTraitsBase<PacketTraits<RobotInteractionData>>
{
    static constexpr uint16_t cmd = 0x0301;
    static constexpr size_t size = sizeof(RobotInteractionData);
};
#endif // RPL_ROBOTINTERACTIONDATA_HPP
