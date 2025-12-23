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
    
    // 1. Fill the buffer to near capacity to advance write_index
    // We need to know the buffer size. It's a compile time constant in Parser.
    // We can infer it or just push enough data.
    // Let's push dummy data and read it out until write_index is near the end.
    
    // Note: This relies on the implementation detail that buffer size is power of 2
    // and likely 256 or 512 for small packets.
    // Let's just push a large amount of data in chunks and read it immediately.
    
    const size_t large_offset = 1024 * 16; // Should be enough to wrap around multiple times
    std::vector<uint8_t> dummy(100, 0);
    
    // Advance write_index to align such that the next frame will wrap around.
    // We want write_index % buffer_size to be buffer_size - split_point.
    // But we don't know buffer_size easily from outside.
    
    // Alternative strategy:
    // Use the Zero-Copy interface to inspect the buffer state!
    // get_write_buffer() returns a span. The size of the span tells us 
    // how much contiguous space is left at the tail.
    
    // 1. Consume space until tail space is smaller than frame_size
    while (true)
    {
        auto span = parser.get_write_buffer();
        if (span.size() < frame_size && span.size() > 0)
        {
            // Found the edge!
            // span.size() is the space left at the tail.
            // If we write a frame now, it will wrap around.
            break;
        }
        
        // If span is huge (start of buffer), fill it up to move pointer to end
        size_t fill_len = std::min(span.size(), (size_t)128);
        
        // If we are at start and span is full buffer, we need to be careful not to fill it completely
        // if we want to test wrap around.
        // Actually, just writing 1 byte at a time until condition met is safest.
        if (span.size() >= frame_size)
        {
             // Write dummy bytes to advance pointer
             uint8_t dummy_byte = 0;
             auto res = parser.push_data(&dummy_byte, 1);
             (void)res;
             // Read it out to keep buffer empty
             auto parse_res = parser.try_parse_packets(); // Should fail/discard
             (void)parse_res;
        }
    }
    
    // Now write_index is close to the end.
    // The contiguous space (span.size()) is < frame_size.
    // Writing 'frame' now will force a wrap-around write in RingBuffer,
    // and subsequently a wrap-around read/parse in Parser.
    
    auto result = parser.push_data(frame.data(), frame.size());
    assert(result.has_value());
    
    // Verify parse
    auto parsed = deserializer.get<SampleA>();
    assert(parsed.a == packet.a);
    
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
