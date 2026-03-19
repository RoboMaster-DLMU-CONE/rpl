#include <HySerial/HySerial.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Packets/RoboMaster/CustomControllerData.hpp>
#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Packets/VT03RemotePacket.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Serializer.hpp>
#include <algorithm>
#include <format>
#include <iostream>
#include <ranges>
#include <span>

std::string print_remote(const VT03RemotePacket &packet) {
  return std::format("x1: {}, y1: {}, x2: {}, y2: {}", packet.left_stick_x,
                     packet.left_stick_y, packet.right_stick_x,
                     packet.right_stick_y);
}

std::string print_sample_a(const SampleA &sample) {
  auto &[a, b, c, d] = sample;
  return std::format("a: {} b: {} c: {} d: {}", a, b, c, d);
}

std::string print_cc(const CustomControllerData &data) {
  return std::format("CC: {}, {}, {}, {}...", data.data[0], data.data[1],
                     data.data[2], data.data[3]);
}

std::string print_sample_b(const SampleB &sample) {
  auto &[x, y] = sample;
  return std::format("x: {}, y: {}", x, y);
}

static constexpr auto DEVICE_PATH = "/dev/ttyUSB0";

int main() {
  RPL::Deserializer<CustomControllerData, VT03RemotePacket> deserializer;
  RPL::Parser parser{deserializer};
  RPL::Serializer<SampleB> serializer;

  HySerial::Builder builder;
  builder.device(DEVICE_PATH)
      .baud_rate(921600)
      .data_bits(HySerial::DataBits::BITS_8)
      .parity(HySerial::Parity::NONE)
      .stop_bits(HySerial::StopBits::ONE);

  builder.on_read([&parser](const std::span<const std::byte> data) {
    auto res = parser.push_data(reinterpret_cast<const uint8_t *>(data.data()),
                                data.size());
    if (!res)
      std::cerr << res.error().message << std::endl;
  });

  auto serial_or_err = builder.build();
  if (!serial_or_err) {
    std::cerr << "Failed to create Serial: " << serial_or_err.error().message
              << "\n";
    return 1;
  }

  auto serial = std::move(serial_or_err.value());

  serial->start_read();
  SampleB packet_send{};

  uint8_t s = 0;
  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto &[x, y] = packet_send;
    x += 1;
    y += 2;
    std::array<uint8_t, 1024> buffer{};
    auto res = serializer.serialize(buffer.data(), buffer.size(), packet_send);
    if (!res) {
      std::cerr << res.error().message << std::endl;
      continue;
    }
    serial->send({std::span(reinterpret_cast<const std::byte *>(buffer.data()),
                            res.value())});
    auto cc_packet = deserializer.get<CustomControllerData>();
    auto vt_packet = deserializer.get<VT03RemotePacket>();
    std::cout << print_cc(cc_packet) << std::endl;
    std::cout << print_remote(vt_packet) << std::endl;
    // std::cout << print_sample_b(packet_send) << std::endl;
    s++;
  }
}
