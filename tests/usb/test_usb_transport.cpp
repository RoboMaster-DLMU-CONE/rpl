#include "RPL/Packets/Sample/USBSamples.hpp"
#include "RPL/Packets/USBAck.hpp"
#include "RPL/USBTransport.hpp"
#include <iostream>
#include <cassert>
#include <cstring>

struct SysTickProvider {
  using tick_type = uint64_t;
  static tick_type now() { return ++now_; }
  static void reset() { now_ = 0; }

  static inline uint64_t now_ = 0;
};

using AckMgr = RPL::AckManager<SysTickProvider>;

static bool g_sent = false;
static void simple_send(const uint8_t *, size_t) { g_sent = true; }

using PacketList =
    RPL::USBTransport<AckMgr, decltype(&simple_send), USBAck, SensorData,
                      MotorSpeedCmd, LEDCmd>;

static PacketList *g_tp = nullptr;
static void loopback_send(const uint8_t *buf, size_t len) {
  if (!g_tp || len < 4)
    return;

  uint16_t cmd;
  std::memcpy(&cmd, buf + 3, 2);

  if (cmd == 0x10 || cmd == 0x11) {
    // Echo request frame back first so request data is written to pool
    auto r3 = g_tp->receive(buf, len);
    (void)r3;

    uint8_t req_id = buf[5];
    USBAck ack{req_id, 0};
    constexpr size_t ack_sz =
        RPL::Serializer<USBAck, SensorData, MotorSpeedCmd,
                        LEDCmd>::template frame_size<USBAck>();
    uint8_t ack_buf[ack_sz];
    RPL::Serializer<USBAck, SensorData, MotorSpeedCmd, LEDCmd> ser;
    auto r = ser.serialize(ack_buf, ack_sz, ack);
    assert(r.has_value());
    auto r2 = g_tp->receive(ack_buf, *r);
    (void)r2;
    return;
  }

  auto r = g_tp->receive(buf, len);
  (void)r;
}

void test_notification() {
  std::cout << "Test 1: Notification (fire-and-forget)..." << std::endl;

  g_sent = false;
  PacketList transport;
  transport.on_send(simple_send);

  SensorData sensor{1.0f, 2.0f, 9.8f};
  auto result = transport.notify(sensor);
  assert(result.has_value());
  assert(g_sent);

  std::cout << "  PASS" << std::endl;
}

void test_request_response_loopback() {
  std::cout << "Test 2: Request-Response loopback..." << std::endl;

  SysTickProvider::reset();
  PacketList transport;
  transport.on_send(loopback_send);
  g_tp = &transport;

  SensorData sensor{1.0f, 2.0f, 9.8f};
  (void)transport.notify(sensor);

  MotorSpeedCmd motor{};
  motor.motor_id = 1;
  motor.speed = 500;

  auto result = transport.request(motor, 100);
  assert(result.has_value());
  assert(result.value() == 0);

  auto rx_sensor = transport.deserializer().get<SensorData>();
  assert(rx_sensor.accel_x == 1.0f);
  assert(rx_sensor.accel_y == 2.0f);
  assert(rx_sensor.accel_z == 9.8f);

  auto rx_motor = transport.deserializer().get<MotorSpeedCmd>();
  assert(rx_motor.motor_id == 1);
  assert(rx_motor.speed == 500);

  g_tp = nullptr;
  std::cout << "  PASS" << std::endl;
}

void test_mixed_notify_and_request() {
  std::cout << "Test 3: Mixed notification and request..." << std::endl;

  SysTickProvider::reset();
  PacketList transport;
  transport.on_send(loopback_send);
  g_tp = &transport;

  for (int i = 0; i < 5; i++) {
    SensorData sensor{float(i), float(i + 1), float(i + 2)};
    (void)transport.notify(sensor);
  }

  MotorSpeedCmd motor{};
  motor.motor_id = 1;
  motor.speed = 500;
  auto r1 = transport.request(motor, 100);
  assert(r1.has_value());

  LEDCmd led{};
  led.led_id = 2;
  led.brightness = 128;
  auto r2 = transport.request(led, 100);
  assert(r2.has_value());

  for (int i = 0; i < 5; i++) {
    SensorData sensor{float(i + 10), float(i + 11), float(i + 12)};
    (void)transport.notify(sensor);
  }

  auto latest = transport.deserializer().get<SensorData>();
  assert(latest.accel_x == 14.0f);

  g_tp = nullptr;
  std::cout << "  PASS" << std::endl;
}

void test_ack_timeout() {
  std::cout << "Test 4: Ack timeout..." << std::endl;

  SysTickProvider::reset();
  AckMgr mgr;

  uint8_t req_id = mgr.allocate();
  assert(mgr.is_pending(req_id));

  auto result = mgr.wait_ack(req_id, 5);
  assert(!result.has_value());
  assert(result.error().code == RPL::ErrorCode::Timeout);

  assert(!mgr.is_pending(req_id));

  std::cout << "  PASS" << std::endl;
}

void test_raw_parser_usb_frame() {
  std::cout << "Test 5: Raw Parser with USB frames..." << std::endl;

  RPL::Deserializer<SensorData, MotorSpeedCmd> deserializer;
  RPL::Parser<SensorData, MotorSpeedCmd> parser{deserializer};

  RPL::Serializer<SensorData, MotorSpeedCmd> serializer;

  SensorData sensor{3.14f, 2.71f, 1.41f};
  constexpr size_t frame_sz =
      RPL::Serializer<SensorData, MotorSpeedCmd>::template frame_size<
          SensorData>();
  uint8_t buf[frame_sz];
  auto result = serializer.serialize(buf, frame_sz, sensor);
  assert(result.has_value());
  assert(*result == frame_sz);

  auto push_result = parser.push_data(buf, *result);
  assert(push_result.has_value());

  auto rx = deserializer.get<SensorData>();
  assert(rx.accel_x == 3.14f);
  assert(rx.accel_y == 2.71f);
  assert(rx.accel_z == 1.41f);

  std::cout << "  PASS" << std::endl;
}

void test_no_crc_on_wire() {
  std::cout << "Test 6: USB frames have no CRC on wire..." << std::endl;

  RPL::Deserializer<SensorData> deserializer;
  RPL::Parser<SensorData> parser{deserializer};
  RPL::Serializer<SensorData> serializer;

  SensorData sensor{9.8f, 0.0f, 0.0f};
  constexpr size_t frame_sz =
      RPL::Serializer<SensorData>::template frame_size<SensorData>();
  uint8_t buf[frame_sz];
  auto result = serializer.serialize(buf, frame_sz, sensor);
  assert(result.has_value());

  constexpr size_t expected_size = 5 + sizeof(SensorData);
  assert(*result == expected_size);

  assert(buf[0] == 0xA5);

  uint16_t wire_len;
  std::memcpy(&wire_len, buf + 1, 2);
  assert(wire_len == sizeof(SensorData));

  uint16_t wire_cmd;
  std::memcpy(&wire_cmd, buf + 3, 2);
  assert(wire_cmd == RPL::Meta::PacketTraits<SensorData>::cmd);

  auto push_result = parser.push_data(buf, *result);
  assert(push_result.has_value());

  auto rx = deserializer.get<SensorData>();
  assert(rx.accel_x == 9.8f);

  uint8_t corrupted[frame_sz];
  std::memcpy(corrupted, buf, frame_sz);
  corrupted[sizeof(SensorData) + 3] = 0xFF;
  (void)parser.push_data(corrupted, frame_sz);

  std::cout << "  PASS" << std::endl;
}

int main() {
  std::cout << "=== RPL USB Transport Tests ===" << std::endl;

  try {
    test_notification();
    test_request_response_loopback();
    test_mixed_notify_and_request();
    test_ack_timeout();
    test_raw_parser_usb_frame();
    test_no_crc_on_wire();

    std::cout << "\nAll USB transport tests passed!" << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Test failed: " << e.what() << std::endl;
    return 1;
  }
}
