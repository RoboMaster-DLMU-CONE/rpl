#ifndef RPL_CUSTOMINFO_HPP
#define RPL_CUSTOMINFO_HPP

#include <cstdint>
#include <array>
#include <RPL/Meta/PacketTraits.hpp>
#pragma pack(push, 1)

/**
 * @brief 选手端小地图接收机器人数据，3Hz 频率上限
 */
struct CustomInfo
{
    uint16_t sender_id; ///< 发送者 ID
    uint16_t receiver_id; ///< 接收者 ID
    std::array<uint8_t, 30> user_data; ///< 自定义数据 (UTF-16 编码)
};

template <>
struct RPL::Meta::PacketTraits<CustomInfo> : PacketTraitsBase<PacketTraits<CustomInfo>>
{
    static constexpr uint16_t cmd = 0x0308;
    static constexpr size_t size = sizeof(CustomInfo);
};
#pragma pack(pop)
#endif // RPL_CUSTOMINFO_HPP
