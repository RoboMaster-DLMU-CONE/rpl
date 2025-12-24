#ifndef RPL_CUSTOMINFO_HPP
#define RPL_CUSTOMINFO_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 选手端小地图接收机器人数据，3Hz 频率上限
 */
struct CustomInfo
{
    uint16_t sender_id; ///< 发送者 ID
    uint16_t receiver_id; ///< 接收者 ID
    uint8_t[30] user_data; ///< 自定义数据 (UTF-16 编码)
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<CustomInfo> : PacketTraitsBase<PacketTraits<CustomInfo>>
{
    static constexpr uint16_t cmd = 0x0308;
    static constexpr size_t size = sizeof(CustomInfo);
};
#endif // RPL_CUSTOMINFO_HPP
