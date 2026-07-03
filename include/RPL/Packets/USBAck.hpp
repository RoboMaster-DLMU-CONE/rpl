#ifndef RPL_USB_ACK_HPP
#define RPL_USB_ACK_HPP

#include "RPL/Meta/PacketTraits.hpp"
#include <cstdint>

struct USBAck {
  uint8_t req_id;
  uint8_t status;
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<USBAck>
    : PacketTraitsBase<PacketTraits<USBAck>> {
  static constexpr uint16_t cmd = 0x0000;
  static constexpr size_t size = sizeof(USBAck);
  static constexpr PacketCategory category = PacketCategory::Ack;
  static constexpr bool skip_memory_pool = true;
  using Protocol = USBAckProto;
};

#endif // RPL_USB_ACK_HPP
