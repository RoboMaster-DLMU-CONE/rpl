#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>
#include <random>

// Helper function to compare floating point values
bool float_equal(float a, float b, float epsilon = 1e-6f)
{
    return std::abs(a - b) < epsilon;
}

bool double_equal(double a, double b, double epsilon = 1e-9)
{
    return std::abs(a - b) < epsilon;
}

// Test 1: End-to-end serialization → parsing → deserialization
void test_end_to_end_single_packet()
{
    std::cout << "Test 1: End-to-end single packet processing..." << std::endl;

    // Step 1: Create original packet
    SampleA original_packet{42, -1234, 3.14f, 2.718};

    // Step 2: Serialize
    RPL::Serializer<SampleA> serializer;
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);

    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), original_packet);
    assert(serialize_result.has_value());

    // Step 3: Parse
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};

    auto parse_result = parser.push_data(buffer.data(), buffer.size());
    assert(parse_result.has_value());

    // Step 4: Deserialize
    auto deserialized_packet = deserializer.get<SampleA>();

    // Step 5: Verify data integrity
    assert(deserialized_packet.a == original_packet.a);
    assert(deserialized_packet.b == original_packet.b);
    assert(float_equal(deserialized_packet.c, original_packet.c));
    assert(double_equal(deserialized_packet.d, original_packet.d));

    std::cout << "✓ End-to-end single packet processing passed" << std::endl;
}

// Test 2: End-to-end multi-packet processing
void test_end_to_end_multi_packet()
{
    std::cout << "Test 2: End-to-end multi-packet processing..." << std::endl;

    // Step 1: Create original packets
    SampleA original_a{42, -1234, 3.14f, 2.718};
    SampleB original_b{1337, 9.876};

    // Step 2: Serialize
    RPL::Serializer<SampleA, SampleB> serializer;
    constexpr size_t total_size = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>() +
        RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();
    std::vector<uint8_t> buffer(total_size);

    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), original_a, original_b);
    assert(serialize_result.has_value());

    // Step 3: Parse
    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<SampleA, SampleB> parser{deserializer};

    auto parse_result = parser.push_data(buffer.data(), buffer.size());
    assert(parse_result.has_value());

    // Step 4: Deserialize
    auto deserialized_a = deserializer.get<SampleA>();
    auto deserialized_b = deserializer.get<SampleB>();

    // Step 5: Verify data integrity
    assert(deserialized_a.a == original_a.a);
    assert(deserialized_a.b == original_a.b);
    assert(float_equal(deserialized_a.c, original_a.c));
    assert(double_equal(deserialized_a.d, original_a.d));

    assert(deserialized_b.x == original_b.x);
    assert(double_equal(deserialized_b.y, original_b.y));

    std::cout << "✓ End-to-end multi-packet processing passed" << std::endl;
}

// Test 3: Real-world data flow scenario with chunked reception
void test_real_world_chunked_scenario()
{
    std::cout << "Test 3: Real-world chunked data flow scenario..." << std::endl;

    // Simulate a real scenario: multiple packets sent over time in chunks
    std::vector<SampleA> packets_a = {
        {1, 100, 1.1f, 1.1},
        {2, 200, 2.2f, 2.2},
        {3, 300, 3.3f, 3.3}
    };

    std::vector<SampleB> packets_b = {
        {1000, 10.1},
        {2000, 20.2}
    };

    // Serialize all packets
    RPL::Serializer<SampleA, SampleB> serializer;
    constexpr size_t frame_size_a = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>();
    constexpr size_t frame_size_b = RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();

    std::vector<uint8_t> all_data;

    // Serialize packets in mixed order (realistic scenario)
    for (size_t i = 0; i < std::max(packets_a.size(), packets_b.size()); ++i)
    {
        if (i < packets_a.size())
        {
            std::vector<uint8_t> buffer_a(frame_size_a);
            auto result = serializer.serialize(buffer_a.data(), buffer_a.size(), packets_a[i]);
            assert(result.has_value());
            all_data.insert(all_data.end(), buffer_a.begin(), buffer_a.end());
        }

        if (i < packets_b.size())
        {
            std::vector<uint8_t> buffer_b(frame_size_b);
            auto result = serializer.serialize(buffer_b.data(), buffer_b.size(), packets_b[i]);
            assert(result.has_value());
            all_data.insert(all_data.end(), buffer_b.begin(), buffer_b.end());
        }
    }

    // Setup parser and deserializer
    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<SampleA, SampleB> parser{deserializer};

    // Simulate chunked reception (random chunk sizes)
    std::mt19937 rng(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<size_t> chunk_dist(1, 20);

    size_t offset = 0;
    while (offset < all_data.size())
    {
        size_t chunk_size = std::min(chunk_dist(rng), all_data.size() - offset);
        auto result = parser.push_data(all_data.data() + offset, chunk_size);
        assert(result.has_value());
        offset += chunk_size;
    }

    // Verify that all packets were processed correctly
    // Note: Due to the nature of the memory pool, we can only verify the last packet of each type
    auto final_a = deserializer.get<SampleA>();
    auto final_b = deserializer.get<SampleB>();

    // Should have the last packet values
    assert(final_a.a == packets_a.back().a);
    assert(final_b.x == packets_b.back().x);

    std::cout << "✓ Real-world chunked data flow scenario passed" << std::endl;
}

// Test 4: Error recovery and continued processing
void test_error_recovery_continued_processing()
{
    std::cout << "Test 4: Error recovery and continued processing..." << std::endl;

    RPL::Serializer<SampleA> serializer;
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};

    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();

    // Create valid packets
    SampleA packet1{1, 100, 1.1f, 1.1};
    SampleA packet2{2, 200, 2.2f, 2.2};
    SampleA packet3{3, 300, 3.3f, 3.3};

    // Serialize valid packets
    std::vector<uint8_t> valid_frame1(frame_size);
    std::vector<uint8_t> valid_frame2(frame_size);
    std::vector<uint8_t> valid_frame3(frame_size);

    auto result1 = serializer.serialize(valid_frame1.data(), valid_frame1.size(), packet1);
    auto result2 = serializer.serialize(valid_frame2.data(), valid_frame2.size(), packet2);
    auto result3 = serializer.serialize(valid_frame3.data(), valid_frame3.size(), packet3);

    assert(result1.has_value() && result2.has_value() && result3.has_value());

    // Create corrupted frame
    std::vector<uint8_t> corrupted_frame = valid_frame2;
    corrupted_frame[0] = 0xFF; // Corrupt start byte

    // Create combined data: valid1 + corrupted + valid3
    std::vector<uint8_t> combined_data;
    combined_data.insert(combined_data.end(), valid_frame1.begin(), valid_frame1.end());
    combined_data.insert(combined_data.end(), corrupted_frame.begin(), corrupted_frame.end());
    combined_data.insert(combined_data.end(), valid_frame3.begin(), valid_frame3.end());

    // Process all data
    auto parse_result = parser.push_data(combined_data.data(), combined_data.size());
    assert(parse_result.has_value());

    // Verify that valid packets were processed despite corruption
    // Note: Due to memory pool behavior, we'll see the last valid packet
    auto final_packet = deserializer.get<SampleA>();

    // Should be packet3 (the last valid one)
    assert(final_packet.a == packet3.a);
    assert(final_packet.b == packet3.b);

    std::cout << "✓ Error recovery and continued processing passed" << std::endl;
}

// Test 5: Multiple processing cycles
void test_multiple_processing_cycles()
{
    std::cout << "Test 5: Multiple processing cycles..." << std::endl;

    RPL::Serializer<SampleA> serializer;
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};

    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();

    // Process multiple cycles of data
    for (int cycle = 0; cycle < 5; ++cycle)
    {
        SampleA packet{
            static_cast<uint8_t>(cycle + 1),
            static_cast<int16_t>((cycle + 1) * 100),
            static_cast<float>((cycle + 1) * 1.1f),
            static_cast<double>((cycle + 1) * 2.2)
        };

        // Serialize
        std::vector<uint8_t> buffer(frame_size);
        auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), packet);
        assert(serialize_result.has_value());

        // Clear parser buffer for each cycle
        parser.clear_buffer();

        // Parse
        auto parse_result = parser.push_data(buffer.data(), buffer.size());
        assert(parse_result.has_value());

        // Verify
        auto deserialized = deserializer.get<SampleA>();
        assert(deserialized.a == packet.a);
        assert(deserialized.b == packet.b);
        assert(float_equal(deserialized.c, packet.c));
        assert(double_equal(deserialized.d, packet.d));
    }

    std::cout << "✓ Multiple processing cycles passed" << std::endl;
}

// Test 6: High-frequency data processing simulation
void test_high_frequency_data_processing()
{
    std::cout << "Test 6: High-frequency data processing simulation..." << std::endl;

    RPL::Serializer<SampleA, SampleB> serializer;
    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<SampleA, SampleB> parser{deserializer};

    constexpr size_t num_iterations = 100;
    constexpr size_t frame_size_a = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>();
    constexpr size_t frame_size_b = RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();

    // Simulate high-frequency alternating packets
    for (size_t i = 0; i < num_iterations; ++i)
    {
        if (i % 2 == 0)
        {
            // Send SampleA
            SampleA packet{
                static_cast<uint8_t>(i),
                static_cast<int16_t>(i * 10),
                static_cast<float>(i * 0.1f),
                static_cast<double>(i * 0.01)
            };

            std::vector<uint8_t> buffer(frame_size_a);
            auto result = serializer.serialize(buffer.data(), buffer.size(), packet);
            assert(result.has_value());

            auto parse_result = parser.push_data(buffer.data(), buffer.size());
            assert(parse_result.has_value());

            // Verify latest SampleA
            auto deserialized = deserializer.get<SampleA>();
            assert(deserialized.a == packet.a);
        }
        else
        {
            // Send SampleB
            SampleB packet{static_cast<int>(i * 100), static_cast<double>(i * 0.5)};

            std::vector<uint8_t> buffer(frame_size_b);
            auto result = serializer.serialize(buffer.data(), buffer.size(), packet);
            assert(result.has_value());

            auto parse_result = parser.push_data(buffer.data(), buffer.size());
            assert(parse_result.has_value());

            // Verify latest SampleB
            auto deserialized = deserializer.get<SampleB>();
            assert(deserialized.x == packet.x);
        }
    }

    std::cout << "✓ High-frequency data processing simulation passed" << std::endl;
}

int main()
{
    std::cout << "=== RPL Integration Tests ===" << std::endl;

    try
    {
        test_end_to_end_single_packet();
        test_end_to_end_multi_packet();
        test_real_world_chunked_scenario();
        test_error_recovery_continued_processing();
        test_multiple_processing_cycles();
        test_high_frequency_data_processing();

        std::cout << "✓ All integration tests passed!" << std::endl;
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
