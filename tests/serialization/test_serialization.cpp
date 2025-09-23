#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Serializer.hpp>
#include <iostream>
#include <vector>
#include <cassert>

// Test helper function to check if two arrays are equal
bool arrays_equal(const uint8_t* a, const uint8_t* b, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

// Test 1: Single packet serialization
void test_single_packet_serialization() {
    std::cout << "Test 1: Single packet serialization..." << std::endl;
    
    RPL::Serializer<SampleA> serializer;
    SampleA packet{42, -1234, 3.14f, 2.718};
    
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);
    
    auto result = serializer.serialize(buffer.data(), buffer.size(), 1, packet);
    
    assert(result.has_value());
    assert(*result == frame_size);
    
    // Check frame structure
    assert(buffer[0] == 0xA5); // Start byte
    
    // Check command (little endian)
    uint16_t cmd = *reinterpret_cast<const uint16_t*>(buffer.data() + 1);
    assert(cmd == RPL::Meta::PacketTraits<SampleA>::cmd);
    
    // Check data length
    uint16_t data_length = *reinterpret_cast<const uint16_t*>(buffer.data() + 3);
    assert(data_length == RPL::Meta::PacketTraits<SampleA>::size);
    
    // Check sequence number
    assert(buffer[5] == 1);
    
    std::cout << "✓ Single packet serialization passed" << std::endl;
}

// Test 2: Multi-packet serialization
void test_multi_packet_serialization() {
    std::cout << "Test 2: Multi-packet serialization..." << std::endl;
    
    RPL::Serializer<SampleA, SampleB> serializer;
    SampleA packet_a{42, -1234, 3.14f, 2.718};
    SampleB packet_b{1337, 9.876};
    
    constexpr size_t total_frame_size = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>() + 
                                       RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();
    std::vector<uint8_t> buffer(total_frame_size);
    
    auto result = serializer.serialize(buffer.data(), buffer.size(), 1, packet_a, packet_b);
    
    assert(result.has_value());
    assert(*result == total_frame_size);
    
    // Check first frame (SampleA)
    assert(buffer[0] == 0xA5);
    uint16_t cmd_a = *reinterpret_cast<const uint16_t*>(buffer.data() + 1);
    assert(cmd_a == RPL::Meta::PacketTraits<SampleA>::cmd);
    
    // Check second frame (SampleB)
    size_t offset = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>();
    assert(buffer[offset] == 0xA5);
    uint16_t cmd_b = *reinterpret_cast<const uint16_t*>(buffer.data() + offset + 1);
    assert(cmd_b == RPL::Meta::PacketTraits<SampleB>::cmd);
    
    std::cout << "✓ Multi-packet serialization passed" << std::endl;
}

// Test 3: Frame size calculations
void test_frame_size_calculations() {
    std::cout << "Test 3: Frame size calculations..." << std::endl;
    
    RPL::Serializer<SampleA, SampleB> serializer;
    
    // Test individual frame sizes
    auto frame_size_a = serializer.frame_size<SampleA>();
    auto frame_size_b = serializer.frame_size<SampleB>();
    
    // Frame = header(7) + data + CRC16(2)
    assert(frame_size_a == 7 + RPL::Meta::PacketTraits<SampleA>::size + 2);
    assert(frame_size_b == 7 + RPL::Meta::PacketTraits<SampleB>::size + 2);
    
    // Test frame size by command
    auto size_by_cmd_a = serializer.frame_size_by_cmd(RPL::Meta::PacketTraits<SampleA>::cmd);
    auto size_by_cmd_b = serializer.frame_size_by_cmd(RPL::Meta::PacketTraits<SampleB>::cmd);
    
    assert(size_by_cmd_a == frame_size_a);
    assert(size_by_cmd_b == frame_size_b);
    
    // Test max frame size
    auto max_size = RPL::Serializer<SampleA, SampleB>::max_frame_size();
    assert(max_size >= frame_size_a);
    assert(max_size >= frame_size_b);
    
    std::cout << "✓ Frame size calculations passed" << std::endl;
}

// Test 4: Buffer size error handling
void test_buffer_size_error_handling() {
    std::cout << "Test 4: Buffer size error handling..." << std::endl;
    
    RPL::Serializer<SampleA> serializer;
    SampleA packet{42, -1234, 3.14f, 2.718};
    
    // Create buffer that's too small
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> small_buffer(frame_size - 1);
    
    auto result = serializer.serialize(small_buffer.data(), small_buffer.size(), 1, packet);
    
    // Should fail due to insufficient buffer size
    assert(!result.has_value());
    
    std::cout << "✓ Buffer size error handling passed" << std::endl;
}

// Test 5: Sequence number handling
void test_sequence_number_handling() {
    std::cout << "Test 5: Sequence number handling..." << std::endl;
    
    RPL::Serializer<SampleA> serializer;
    SampleA packet{42, -1234, 3.14f, 2.718};
    
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);
    
    // Test different sequence numbers
    for (uint8_t seq = 0; seq < 5; ++seq) {
        auto result = serializer.serialize(buffer.data(), buffer.size(), seq, packet);
        assert(result.has_value());
        assert(buffer[5] == seq); // Sequence number at position 5
    }
    
    std::cout << "✓ Sequence number handling passed" << std::endl;
}

int main() {
    std::cout << "=== RPL Serialization Tests ===" << std::endl;
    
    try {
        test_single_packet_serialization();
        test_multi_packet_serialization();
        test_frame_size_calculations();
        test_buffer_size_error_handling();
        test_sequence_number_handling();
        
        std::cout << "✓ All serialization tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}