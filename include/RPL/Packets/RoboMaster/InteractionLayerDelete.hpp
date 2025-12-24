#ifndef RPL_INTERACTIONLAYERDELETE_HPP
#define RPL_INTERACTIONLAYERDELETE_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

/**
 * @brief 客户端删除图形 (子协议)
 */
struct InteractionLayerDelete
{
    uint8_t delete_type; ///< 0-空, 1-删除图层, 2-删除所有
    uint8_t layer; ///< 图层数 (0-9)
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<InteractionLayerDelete> : PacketTraitsBase<PacketTraits<InteractionLayerDelete>>
{
    static constexpr uint16_t cmd = 0x0100;
    static constexpr size_t size = sizeof(InteractionLayerDelete);
};
#endif // RPL_INTERACTIONLAYERDELETE_HPP
