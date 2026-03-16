#include <RPL/Packets/RoboMaster/CustomControllerData.hpp>
#include <RPL/Packets/VT03RemotePacket.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Deserializer.hpp>
#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>

using namespace RPL;
using namespace RPL::Packets;

void test_mixed_protocol_parsing() {
    std::cout << "Test: Mixed Protocol Parsing..." << std::endl;

    // Use Serializer to create valid packets
    Serializer<CustomControllerData, VT03RemotePacket> serializer;
    
    // Create test packets
    CustomControllerData rm_packet;
    std::memset(rm_packet.data, 0x11, sizeof(rm_packet.data));

    VT03RemotePacket vt_packet{};
    vt_packet.right_stick_x = 1024;
    vt_packet.right_stick_y = 1024;
    vt_packet.switch_state = 1;

    // Serialize to buffer
    std::vector<uint8_t> buffer(200);
    size_t size = serializer.serialize(buffer.data(), buffer.size(), 
        rm_packet, vt_packet, rm_packet, vt_packet).value();
    buffer.resize(size);

    // Setup Parser
    Deserializer<CustomControllerData, VT03RemotePacket> deserializer;
    Parser<CustomControllerData, VT03RemotePacket> parser{deserializer};

    // Parse the buffer
    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // Verify deserialized data
    auto rm_out = deserializer.get<CustomControllerData>();
    auto vt_out = deserializer.get<VT03RemotePacket>();

    // Verify last RM packet
    assert(rm_out.data[0] == 0x11);
    assert(rm_out.data[29] == 0x11);

    // Verify last VT packet
    assert(vt_out.right_stick_x == 1024);
    assert(vt_out.right_stick_y == 1024);
    assert(vt_out.switch_state == 1);

    std::cout << "✓ Mixed Protocol Parsing passed" << std::endl;
}

void test_corrupted_mixed_stream() {
    std::cout << "Test: Corrupted Mixed Stream..." << std::endl;

    Serializer<CustomControllerData, VT03RemotePacket> serializer;
    std::vector<uint8_t> buffer(200);

    CustomControllerData rm_packet;
    std::memset(rm_packet.data, 0x22, sizeof(rm_packet.data));

    VT03RemotePacket vt_packet{};
    vt_packet.right_stick_x = 512;
    vt_packet.switch_state = 2;

    size_t size = serializer.serialize(buffer.data(), buffer.size(), 
        rm_packet, vt_packet).value();
    buffer.resize(size);

    // Introduce corruption between packets
    std::vector<uint8_t> noisy_buffer;
    noisy_buffer.insert(noisy_buffer.end(), buffer.begin(), buffer.begin() + 20); // Part of RM packet
    // Add noise
    noisy_buffer.push_back(0xFF);
    noisy_buffer.push_back(0xA5); // Fake RM header start
    noisy_buffer.push_back(0x00); // Invalid length
    noisy_buffer.push_back(0xAA); // Junk
    // Add rest of RM packet (CRC fail effectively)
    noisy_buffer.insert(noisy_buffer.end(), buffer.begin() + 20, buffer.begin() + 39); // Rest of RM packet
    // Add VT packet
    noisy_buffer.insert(noisy_buffer.end(), buffer.begin() + 39, buffer.end());

    Deserializer<CustomControllerData, VT03RemotePacket> deserializer;
    Parser<CustomControllerData, VT03RemotePacket> parser{deserializer};

    // Push noisy data
    // The parser should skip the corrupted RM packet and potentially find the VT packet if sync recovers
    parser.push_data(noisy_buffer.data(), noisy_buffer.size());

    // Verify if VT packet was recovered (it follows the RM packet)
    auto vt_out = deserializer.get<VT03RemotePacket>();
    assert(vt_out.right_stick_x == 512);
    assert(vt_out.switch_state == 2);

    std::cout << "✓ Corrupted Mixed Stream passed" << std::endl;
}

int main() {
    try {
        test_mixed_protocol_parsing();
        test_corrupted_mixed_stream();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
