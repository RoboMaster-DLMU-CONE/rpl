#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include <cppcrc.h> // Include cppcrc directly for CRC tests

// Helper to access private members for testing if needed, 
// but here we test public behavior which is better.

void test_ringbuffer_wrap_around()
{
    std::cout << "Test: RingBuffer Wrap-Around Edge Case..." << std::endl;

    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    RPL::Serializer<SampleA> serializer;

    SampleA packet{42, -1234, 3.14f, 2.718};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> frame(frame_size);
    auto ser_res = serializer.serialize(frame.data(), frame.size(), packet);
    (void)ser_res; // Suppress unused warning

    // We need to manipulate the internal RingBuffer state to force a wrap-around.
    // Since we can't access private members, we simulate it by filling and draining.
    
    // BipBuffer Strategy:
    // Maintain at least 1 byte in buffer to prevent 'reset to 0'.
    // Use a pipeline: Write F1, Write F2, Read F1, Write F3, Read F2...
    // This advances the window [Start, End] forward until we hit the end of buffer.
    
    parser.clear_buffer();
    size_t f_size = frame.size();
    
    // Prime with one partial frame
    parser.push_data(frame.data(), f_size - 1);
    
    int iterations = 0;
    while (iterations < 1000) {
        auto span = parser.get_write_buffer();
        if (span.size() < frame_size) {
            // Found the edge
            break;
        }
        
        // Append F_next (partial)
        parser.push_data(frame.data(), f_size - 1);
        
        // Finish F_curr (write missing byte)
        // The missing byte is frame[f_size-1].
        uint8_t last_byte = frame[f_size - 1];
        parser.push_data(&last_byte, 1);
        
        // Clear the result from deserializer to avoid overflow there (optional)
        deserializer.get<SampleA>();
        
        iterations++;
    }
    
    // Loop finished. space at tail is small.
    // Buffer contains a partial frame (F_last).
    // Write a Full Frame. It should wrap.
    auto result = parser.push_data(frame.data(), frame.size());
    assert(result.has_value());
    
    // Verify parse of the wrapped frame
    // Note: The partial frame is still in buffer (incomplete), followed by the wrapped frame.
    // Parser stops at Partial (Incomplete).
    
    // So we must finish the Partial first!
    uint8_t last_byte = frame[f_size - 1];
    parser.push_data(&last_byte, 1);
    
    // Now Partial is parsed, then Wrapped is parsed.
    // So deserializer will hold the LATEST packet (Wrapped).
    
    auto p_final = deserializer.get<SampleA>();
    assert(p_final.a == packet.a);

     std::cout << "✓ RingBuffer Wrap-Around passed" << std::endl;
}

void test_zero_copy_write()
{
    std::cout << "Test: Zero-Copy Write Interface..." << std::endl;

    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    RPL::Serializer<SampleA> serializer;

    SampleA packet{99, 8888, 1.23f, 4.56};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> frame(frame_size);
    auto ser_res = serializer.serialize(frame.data(), frame.size(), packet);
    (void)ser_res;

    // 1. Get write buffer
    auto span = parser.get_write_buffer();
    assert(span.size() >= frame_size);

    // 2. Simulate DMA write
    std::memcpy(span.data(), frame.data(), frame.size());

    // 3. Advance write index
    auto result = parser.advance_write_index(frame.size());
    assert(result.has_value());

    // 4. Verify parse
    auto parsed = deserializer.get<SampleA>();
    assert(parsed.a == packet.a);
    assert(parsed.b == packet.b);

    std::cout << "✓ Zero-Copy Write passed" << std::endl;
}

void test_segmented_crc_logic()
{
    std::cout << "Test: Segmented CRC Logic..." << std::endl;

    // This tests the underlying assumption of our optimization:
    // CRC16(part1 + part2) == CRC16(part2, CRC16(part1))
    
    std::vector<uint8_t> data(256);
    for(size_t i=0; i<data.size(); i++) data[i] = (uint8_t)i;

    // 1. Full CRC
    uint16_t expected = CRC16::CCITT_FALSE::calc(data.data(), data.size());

    // 2. Segmented CRC (split at various points)
    for (size_t split = 1; split < data.size(); split++)
    {
        uint16_t part1 = CRC16::CCITT_FALSE::calc(data.data(), split);
        
        // Note: For CCITT_FALSE (refl_out=false, xor_out=0), the intermediate value 
        // can be passed directly.
        uint16_t combined = CRC16::CCITT_FALSE::calc(data.data() + split, data.size() - split, part1);
        
        assert(combined == expected);
    }

    std::cout << "✓ Segmented CRC Logic passed" << std::endl;
}

int main()
{
    std::cout << "=== RPL Advanced Parser Tests ===" << std::endl;
    try {
        test_ringbuffer_wrap_around();
        test_zero_copy_write();
        test_segmented_crc_logic();
        std::cout << "✓ All advanced tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;
        return 1;
    }
}
