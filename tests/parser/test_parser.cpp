#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <random>

// Test 1: Chunked data processing (simulating USB reception)
void test_chunked_data_processing() {
    std::cout << "Test 1: Chunked data processing..." << std::endl;
    
    // Create test data
    RPL::Serializer<SampleA, SampleB> serializer;
    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<SampleA, SampleB> parser{deserializer};
    
    SampleA packet_a{42, -1234, 3.14f, 2.718};
    SampleB packet_b{1337, 9.876};
    
    // Serialize packets
    constexpr size_t total_size = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>() + 
                                  RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();
    std::vector<uint8_t> buffer(total_size);
    
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), 1, packet_a, packet_b);
    assert(serialize_result.has_value());
    
    // Test chunked reception
    size_t chunk_size = buffer.size() / 3; // Split into 3 chunks
    
    // First chunk
    auto result1 = parser.push_data(buffer.data(), chunk_size);
    assert(result1.has_value());
    
    // Second chunk
    auto result2 = parser.push_data(buffer.data() + chunk_size, chunk_size);
    assert(result2.has_value());
    
    // Final chunk
    auto result3 = parser.push_data(buffer.data() + 2 * chunk_size, buffer.size() - 2 * chunk_size);
    assert(result3.has_value());
    
    // Verify parsed data
    auto parsed_a = deserializer.get<SampleA>();
    auto parsed_b = deserializer.get<SampleB>();
    
    assert(parsed_a.a == packet_a.a);
    assert(parsed_a.b == packet_a.b);
    assert(parsed_b.x == packet_b.x);
    
    std::cout << "✓ Chunked data processing passed" << std::endl;
}

// Test 2: Buffer management and statistics
void test_buffer_management_and_statistics() {
    std::cout << "Test 2: Buffer management and statistics..." << std::endl;
    
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    
    // Check initial state
    assert(parser.available_data() == 0);
    assert(parser.available_space() > 0);
    assert(!parser.is_buffer_full());
    
    // Add some test data (incomplete frame)
    std::vector<uint8_t> partial_data = {0xA5, 0x02, 0x01}; // Start of frame
    auto result = parser.push_data(partial_data.data(), partial_data.size());
    assert(result.has_value());
    
    // Check statistics after partial data
    assert(parser.available_data() == partial_data.size());
    assert(parser.available_space() > 0);
    assert(!parser.is_buffer_full());
    
    // Clear buffer and verify
    parser.clear_buffer();
    assert(parser.available_data() == 0);
    
    std::cout << "✓ Buffer management and statistics passed" << std::endl;
}

// Test 3: Error handling - corrupted frames
void test_error_handling_corrupted_frames() {
    std::cout << "Test 3: Error handling - corrupted frames..." << std::endl;
    
    RPL::Serializer<SampleA> serializer;
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    
    SampleA packet{42, -1234, 3.14f, 2.718};
    
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);
    
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), 1, packet);
    assert(serialize_result.has_value());
    
    // Test 1: Corrupted start byte
    std::vector<uint8_t> corrupted_buffer = buffer;
    corrupted_buffer[0] = 0xFF; // Wrong start byte
    
    parser.clear_buffer();
    auto result1 = parser.push_data(corrupted_buffer.data(), corrupted_buffer.size());
    // Should still return successfully but ignore the corrupted frame
    assert(result1.has_value());
    
    // Test 2: Corrupted CRC
    corrupted_buffer = buffer;
    corrupted_buffer[corrupted_buffer.size() - 1] = 0xFF; // Corrupt CRC
    
    parser.clear_buffer();
    auto result2 = parser.push_data(corrupted_buffer.data(), corrupted_buffer.size());
    // Should handle corrupted CRC gracefully
    assert(result2.has_value());
    
    std::cout << "✓ Error handling - corrupted frames passed" << std::endl;
}

// Test 4: Incomplete data handling
void test_incomplete_data_handling() {
    std::cout << "Test 4: Incomplete data handling..." << std::endl;
    
    RPL::Serializer<SampleA> serializer;
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    
    SampleA packet{42, -1234, 3.14f, 2.718};
    
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);
    
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), 1, packet);
    assert(serialize_result.has_value());
    
    // Send incomplete frame (only first 5 bytes)
    parser.clear_buffer();
    auto result1 = parser.push_data(buffer.data(), 5);
    assert(result1.has_value());
    
    // Check that data is pending
    assert(parser.available_data() == 5);
    
    // Send the rest of the frame
    auto result2 = parser.push_data(buffer.data() + 5, buffer.size() - 5);
    assert(result2.has_value());
    
    // Verify the complete frame was processed
    auto parsed_packet = deserializer.get<SampleA>();
    assert(parsed_packet.a == packet.a);
    assert(parsed_packet.b == packet.b);
    
    std::cout << "✓ Incomplete data handling passed" << std::endl;
}

// Test 5: Buffer clearing and state management
void test_buffer_clearing_and_state_management() {
    std::cout << "Test 5: Buffer clearing and state management..." << std::endl;
    
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    
    // Add some data to the buffer
    std::vector<uint8_t> test_data = {0xA5, 0x02, 0x01, 0x0F, 0x00, 0x01, 0xC8};
    auto result1 = parser.push_data(test_data.data(), test_data.size());
    assert(result1.has_value());
    
    // Verify data is in buffer
    assert(parser.available_data() > 0);
    
    // Clear buffer
    parser.clear_buffer();
    
    // Verify buffer is empty
    assert(parser.available_data() == 0);
    
    // Verify we can add data again after clearing
    auto result2 = parser.push_data(test_data.data(), test_data.size());
    assert(result2.has_value());
    assert(parser.available_data() == test_data.size());
    
    std::cout << "✓ Buffer clearing and state management passed" << std::endl;
}

// Test 6: Multi-packet parsing in single push
void test_multi_packet_parsing_single_push() {
    std::cout << "Test 6: Multi-packet parsing in single push..." << std::endl;
    
    RPL::Serializer<SampleA, SampleB> serializer;
    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<SampleA, SampleB> parser{deserializer};
    
    SampleA packet_a{42, -1234, 3.14f, 2.718};
    SampleB packet_b{1337, 9.876};
    
    // Serialize multiple packets
    constexpr size_t total_size = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>() + 
                                  RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();
    std::vector<uint8_t> buffer(total_size);
    
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), 1, packet_a, packet_b);
    assert(serialize_result.has_value());
    
    // Parse all packets in one push
    parser.clear_buffer();
    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());
    
    // Verify both packets were parsed correctly
    auto parsed_a = deserializer.get<SampleA>();
    auto parsed_b = deserializer.get<SampleB>();
    
    assert(parsed_a.a == packet_a.a);
    assert(parsed_a.b == packet_a.b);
    assert(parsed_b.x == packet_b.x);
    
    std::cout << "✓ Multi-packet parsing in single push passed" << std::endl;
}

// Test 7: Noise resilience
void test_noise_resilience() {
    std::cout << "Test 7: Noise resilience..." << std::endl;
    
    RPL::Serializer<SampleA> serializer;
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    
    SampleA packet{42, -1234, 3.14f, 2.718};
    
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> valid_frame(frame_size);
    
    auto serialize_result = serializer.serialize(valid_frame.data(), valid_frame.size(), 1, packet);
    assert(serialize_result.has_value());
    
    // Create buffer with noise before and after valid frame
    std::vector<uint8_t> noise_before = {0xFF, 0xFE, 0xFD, 0xFC};
    std::vector<uint8_t> noise_after = {0x00, 0x11, 0x22, 0x33};
    
    std::vector<uint8_t> noisy_buffer;
    noisy_buffer.insert(noisy_buffer.end(), noise_before.begin(), noise_before.end());
    noisy_buffer.insert(noisy_buffer.end(), valid_frame.begin(), valid_frame.end());
    noisy_buffer.insert(noisy_buffer.end(), noise_after.begin(), noise_after.end());
    
    // Parse noisy buffer
    parser.clear_buffer();
    auto result = parser.push_data(noisy_buffer.data(), noisy_buffer.size());
    assert(result.has_value());
    
    // Verify the valid frame was extracted correctly
    auto parsed_packet = deserializer.get<SampleA>();
    assert(parsed_packet.a == packet.a);
    assert(parsed_packet.b == packet.b);
    
    std::cout << "✓ Noise resilience passed" << std::endl;
}

int main() {
    std::cout << "=== RPL Parser Tests ===" << std::endl;
    
    try {
        test_chunked_data_processing();
        test_buffer_management_and_statistics();
        test_error_handling_corrupted_frames();
        test_incomplete_data_handling();
        test_buffer_clearing_and_state_management();
        test_multi_packet_parsing_single_push();
        test_noise_resilience();
        
        std::cout << "✓ All parser tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}