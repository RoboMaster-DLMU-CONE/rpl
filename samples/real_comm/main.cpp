#include <HySerial/HySerial.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Packets/RoboMaster/HurtData.hpp>
#include <RPL/Packets/RoboMaster/PowerHeatData.hpp>
#include <RPL/Packets/RoboMaster/RobotStatus.hpp>
#include <RPL/Parser.hpp>
#include <algorithm>
#include <format>
#include <iostream>
#include <span>

std::string print_packet(const HurtData &packet) {
  return std::format("HurtData: armor_id={}, hp_deduction_reason={}",
                     packet.armor_id, packet.hp_deduction_reason);
}

std::string print_packet(const PowerHeatData &packet) {
  return std::format(
      "PowerHeatData: buffer_energy={}, shooter_17mm_1_barrel_heat={}, "
      "shooter_42mm_barrel_heat={}",
      packet.buffer_energy, packet.shooter_17mm_1_barrel_heat,
      packet.shooter_42mm_barrel_heat);
}

std::string print_packet(const RobotStatus &packet) {
  return std::format(
      "RobotStatus: robot_id={}, robot_level={}, current_hp={}, "
      "maximum_hp={}, cooling_value={}, heat_limit={}, "
      "chassis_power_limit={}, gimbal_output={}, chassis_output={}, "
      "shooter_output={}",
      packet.robot_id, packet.robot_level, packet.current_hp,
      packet.maximum_hp, packet.shooter_barrel_cooling_value,
      packet.shooter_barrel_heat_limit, packet.chassis_power_limit,
      packet.power_management_gimbal_output,
      packet.power_management_chassis_output,
      packet.power_management_shooter_output);
}

static constexpr auto DEVICE_PATH = "/dev/ttyUSB0";

int main() {
  RPL::Deserializer<HurtData, PowerHeatData, RobotStatus> deserializer;
  RPL::Parser<HurtData, PowerHeatData, RobotStatus> parser{deserializer};

  HySerial::Builder builder;
  builder.device(DEVICE_PATH)
      .baud_rate(115200)
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

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    
    auto hurt_data = deserializer.get<HurtData>();
    auto power_heat = deserializer.get<PowerHeatData>();
    auto robot_status = deserializer.get<RobotStatus>();
    
    std::cout << print_packet(hurt_data) << std::endl;
    std::cout << print_packet(power_heat) << std::endl;
    std::cout << print_packet(robot_status) << std::endl;
  }
}
