#ifndef RPL_USB_ACK_HPP
#define RPL_USB_ACK_HPP

#include "RPL/Meta/PacketTraits.hpp"
#include <cstdint>
#pragma pack(push, 1)

struct USBAck {
  uint8_t req_id;
  uint8_t status;
};

namespace RPL::Meta {
template <>
struct PacketTraits<USBAck>
    : PacketTraitsBase<PacketTraits<USBAck>> {
  static constexpr uint16_t cmd = 0x0000;
  static constexpr size_t size = sizeof(USBAck);
  static constexpr PacketCategory category = PacketCategory::Ack;
  static constexpr bool skip_memory_pool = true;
  using Protocol = USBAckProto;
};
} // namespace RPL::Meta

#pragma pack(pop)
#endif // RPL_USB_ACK_HPP
