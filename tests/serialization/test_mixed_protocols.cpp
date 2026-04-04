#include <RPL/Packets/RoboMaster/CustomControllerData.hpp>
#include <RPL/Packets/VT03RemotePacket.hpp>
#include <RPL/Serializer.hpp>
#include <cassert>
#include <iostream>
#include <vector>
#include <cstring>

using namespace RPL;
using namespace RPL;

void test_mixed_protocol_serialization() {
    std::cout << "Test: Mixed Protocol Serialization..." << std::endl;

    Serializer<CustomControllerData, VT03RemotePacket> serializer;
    
    // Prepare packets
    CustomControllerData rm_packet;
    std::memset(rm_packet.data, 0xAB, sizeof(rm_packet.data)); // Fill with dummy data

    VT03RemotePacket vt_packet{};
    vt_packet.right_stick_x = 1024;
    vt_packet.right_stick_y = 1024;
    vt_packet.switch_state = 1;
    vt_packet.mouse_x = -100;
    vt_packet.key_w = 1;

    // Calculate expected sizes
    constexpr size_t rm_frame_size = Serializer<CustomControllerData>::frame_size<CustomControllerData>();
    // RM: 7 header + 30 data + 2 CRC = 39 bytes
    
    constexpr size_t vt_frame_size = Serializer<VT03RemotePacket>::frame_size<VT03RemotePacket>();
    // VT: 2 header + 17 data + 2 CRC = 21 bytes

    std::vector<uint8_t> buffer(rm_frame_size + vt_frame_size);

    // Serialize both
    auto result = serializer.serialize(buffer.data(), buffer.size(), rm_packet, vt_packet);
    
    assert(result.has_value());
    assert(*result == rm_frame_size + vt_frame_size);

    // --- Verify RM Packet (First) ---
    uint8_t* rm_ptr = buffer.data();
    assert(rm_ptr[0] == 0xA5); // Standard Start Byte
    // Length: 30 (0x1E, 0x00)
    assert(rm_ptr[1] == 0x1E);
    assert(rm_ptr[2] == 0x00);
    // Seq: 0
    assert(rm_ptr[3] == 0x00);
    // Header CRC at [4]
    // Cmd: 0x0302 (0x02, 0x03)
    assert(rm_ptr[5] == 0x02);
    assert(rm_ptr[6] == 0x03);
    // Data check
    assert(rm_ptr[7] == 0xAB);

    // --- Verify VT Packet (Second) ---
    uint8_t* vt_ptr = buffer.data() + rm_frame_size;
    assert(vt_ptr[0] == 0xA9); // VT Start Byte 1
    assert(vt_ptr[1] == 0x53); // VT Start Byte 2
    // No length, seq, header CRC, or cmd fields.
    // Payload starts immediately at offset 2.
    // VT payload size is 17 bytes.
    // Frame tail (CRC16) starts at offset 2 + 17 = 19.
    
    // Verify payload (spot check)
    // right_stick_x is 11 bits. 1024 = 0x400.
    // bytes 0-1 of payload:
    // bit 0-10: 1024
    // byte 0: 0x00
    // byte 1: 0x04 (partial)
    // Let's check the first few bytes of the payload
    // byte 0 = 0x00 (low 8 bits of 0x400)
    // byte 1 (low 3 bits are high 3 bits of 0x400 -> 100b = 4)
    // right_stick_y starts at bit 11. 1024 = 0x400.
    // bit 11-21.
    // byte 1 (high 5 bits) = low 5 bits of 1024 -> 0
    // so byte 1 should be 0x04 | 0x00 = 0x04.
    assert(vt_ptr[2] == 0x00);
    assert((vt_ptr[3] & 0x07) == 0x04); // Check low 3 bits of byte 1 match high 3 bits of stick_x

    std::cout << "✓ Mixed Protocol Serialization passed" << std::endl;
}

int main() {
    try {
        test_mixed_protocol_serialization();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
