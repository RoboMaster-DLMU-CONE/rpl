#ifndef RPL_RFIDSTATUS_HPP
#define RPL_RFIDSTATUS_HPP

#include <cstdint>
#include <array>
#include <tuple>
#include <RPL/Meta/BitstreamTraits.hpp>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief RFID 模块状态，3Hz 频率发送
 */
struct RFIDStatus
{
    uint32_t rfid_status; ///< 增益点 RFID 状态位掩码 (Base, Highlands, Outpost, etc.)
    uint8_t rfid_status_2 : 2; ///< bit 0-1: 地形跨越增益点 (隧道)
    uint8_t reserved : 6; ///< 保留位
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<RFIDStatus> : PacketTraitsBase<PacketTraits<RFIDStatus>>
{
    static constexpr uint16_t cmd = 0x0209;
    static constexpr size_t size = 5;
    using BitLayout = std::tuple<
        Field<uint32_t, 32>,
        Field<uint8_t, 2>,
        Field<uint8_t, 6>
    >;
};
#endif // RPL_RFIDSTATUS_HPP
