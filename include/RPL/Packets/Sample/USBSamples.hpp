#ifndef RPL_USB_SAMPLES_HPP
#define RPL_USB_SAMPLES_HPP

#include "RPL/Meta/PacketTraits.hpp"
#include <cstdint>

struct SensorData {
  float accel_x;
  float accel_y;
  float accel_z;
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<SensorData>
    : PacketTraitsBase<PacketTraits<SensorData>> {
  static constexpr uint16_t cmd = 0x20;
  static constexpr size_t size = sizeof(SensorData);
  using Protocol = USBBaseProto;
};

struct MotorSpeedCmd {
  uint8_t req_id;
  uint8_t motor_id;
  int16_t speed;
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<MotorSpeedCmd>
    : PacketTraitsBase<PacketTraits<MotorSpeedCmd>> {
  static constexpr uint16_t cmd = 0x10;
  static constexpr size_t size = sizeof(MotorSpeedCmd);
  static constexpr PacketCategory category = PacketCategory::Request;
  using Protocol = USBRequestProto;
};

struct LEDCmd {
  uint8_t req_id;
  uint8_t led_id;
  uint8_t brightness;
} __attribute__((packed));

template <>
struct RPL::Meta::PacketTraits<LEDCmd>
    : PacketTraitsBase<PacketTraits<LEDCmd>> {
  static constexpr uint16_t cmd = 0x11;
  static constexpr size_t size = sizeof(LEDCmd);
  static constexpr PacketCategory category = PacketCategory::Request;
  using Protocol = USBRequestProto;
};

#endif // RPL_USB_SAMPLES_HPP
