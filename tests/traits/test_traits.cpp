#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Meta/PacketTraits.hpp>
#include <RPL/Serializer.hpp>
#include <iostream>
#include <cassert>
#include <type_traits>

// Test 1: PacketTraits validation for SampleA
void test_sample_a_packet_traits() {
    std::cout << "Test 1: SampleA PacketTraits validation..." << std::endl;
    
    // Check that PacketTraits is properly specialized
    static_assert(std::is_same_v<decltype(RPL::Meta::PacketTraits<SampleA>::cmd), const uint16_t>);
    static_assert(std::is_same_v<decltype(RPL::Meta::PacketTraits<SampleA>::size), const size_t>);
    
    // Check command ID value
    assert(RPL::Meta::PacketTraits<SampleA>::cmd == 0x0102);
    std::cout << "  SampleA Command ID: 0x" << std::hex << RPL::Meta::PacketTraits<SampleA>::cmd << std::dec << std::endl;
    
    // Check size matches actual struct size
    assert(RPL::Meta::PacketTraits<SampleA>::size == sizeof(SampleA));
    std::cout << "  SampleA Size: " << RPL::Meta::PacketTraits<SampleA>::size << " bytes" << std::endl;
    std::cout << "  sizeof(SampleA): " << sizeof(SampleA) << " bytes" << std::endl;
    
    // Verify the struct is packed correctly
    SampleA sample{};
    assert(sizeof(sample) == sizeof(uint8_t) + sizeof(int16_t) + sizeof(float) + sizeof(double));
    
    std::cout << "✓ SampleA PacketTraits validation passed" << std::endl;
}

// Test 2: PacketTraits validation for SampleB
void test_sample_b_packet_traits() {
    std::cout << "Test 2: SampleB PacketTraits validation..." << std::endl;
    
    // Check that PacketTraits is properly specialized
    static_assert(std::is_same_v<decltype(RPL::Meta::PacketTraits<SampleB>::cmd), const uint16_t>);
    static_assert(std::is_same_v<decltype(RPL::Meta::PacketTraits<SampleB>::size), const size_t>);
    
    // Check command ID value
    assert(RPL::Meta::PacketTraits<SampleB>::cmd == 0x0103);
    std::cout << "  SampleB Command ID: 0x" << std::hex << RPL::Meta::PacketTraits<SampleB>::cmd << std::dec << std::endl;
    
    // Check size matches actual struct size
    assert(RPL::Meta::PacketTraits<SampleB>::size == sizeof(SampleB));
    std::cout << "  SampleB Size: " << RPL::Meta::PacketTraits<SampleB>::size << " bytes" << std::endl;
    std::cout << "  sizeof(SampleB): " << sizeof(SampleB) << " bytes" << std::endl;
    
    // Verify the struct size (note: SampleB is not packed, so may have padding)
    SampleB sample{};
    assert(sizeof(sample) == sizeof(SampleB)); // Just verify consistent sizing
    
    std::cout << "✓ SampleB PacketTraits validation passed" << std::endl;
}

// Test 3: Command ID uniqueness
void test_command_id_uniqueness() {
    std::cout << "Test 3: Command ID uniqueness..." << std::endl;
    
    // Ensure different packet types have different command IDs
    assert(RPL::Meta::PacketTraits<SampleA>::cmd != RPL::Meta::PacketTraits<SampleB>::cmd);
    
    std::cout << "  SampleA CMD: 0x" << std::hex << RPL::Meta::PacketTraits<SampleA>::cmd << std::dec << std::endl;
    std::cout << "  SampleB CMD: 0x" << std::hex << RPL::Meta::PacketTraits<SampleB>::cmd << std::dec << std::endl;
    
    std::cout << "✓ Command ID uniqueness passed" << std::endl;
}

// Test 4: Frame size calculations based on traits
void test_frame_size_calculations_from_traits() {
    std::cout << "Test 4: Frame size calculations from traits..." << std::endl;
    
    RPL::Serializer<SampleA, SampleB> serializer;
    
    // Frame = header(7) + data + CRC16(2)
    constexpr size_t expected_frame_size_a = 7 + RPL::Meta::PacketTraits<SampleA>::size + 2;
    constexpr size_t expected_frame_size_b = 7 + RPL::Meta::PacketTraits<SampleB>::size + 2;
    
    auto actual_frame_size_a = serializer.frame_size<SampleA>();
    auto actual_frame_size_b = serializer.frame_size<SampleB>();
    
    assert(actual_frame_size_a == expected_frame_size_a);
    assert(actual_frame_size_b == expected_frame_size_b);
    
    std::cout << "  SampleA Expected frame size: " << expected_frame_size_a << " bytes" << std::endl;
    std::cout << "  SampleA Actual frame size: " << actual_frame_size_a << " bytes" << std::endl;
    std::cout << "  SampleB Expected frame size: " << expected_frame_size_b << " bytes" << std::endl;
    std::cout << "  SampleB Actual frame size: " << actual_frame_size_b << " bytes" << std::endl;
    
    std::cout << "✓ Frame size calculations from traits passed" << std::endl;
}

// Test 5: Command ID to frame size mapping
void test_command_id_to_frame_size_mapping() {
    std::cout << "Test 5: Command ID to frame size mapping..." << std::endl;
    
    RPL::Serializer<SampleA, SampleB> serializer;
    
    // Test frame size lookup by command ID
    auto size_by_cmd_a = serializer.frame_size_by_cmd(RPL::Meta::PacketTraits<SampleA>::cmd);
    auto size_by_cmd_b = serializer.frame_size_by_cmd(RPL::Meta::PacketTraits<SampleB>::cmd);
    
    auto direct_size_a = serializer.frame_size<SampleA>();
    auto direct_size_b = serializer.frame_size<SampleB>();
    
    assert(size_by_cmd_a == direct_size_a);
    assert(size_by_cmd_b == direct_size_b);
    
    std::cout << "  SampleA: CMD 0x" << std::hex << RPL::Meta::PacketTraits<SampleA>::cmd 
              << std::dec << " -> " << size_by_cmd_a << " bytes" << std::endl;
    std::cout << "  SampleB: CMD 0x" << std::hex << RPL::Meta::PacketTraits<SampleB>::cmd 
              << std::dec << " -> " << size_by_cmd_b << " bytes" << std::endl;
    
    std::cout << "✓ Command ID to frame size mapping passed" << std::endl;
}

// Test 6: Max frame size calculation
void test_max_frame_size_calculation() {
    std::cout << "Test 6: Max frame size calculation..." << std::endl;
    
    auto max_frame_size = RPL::Serializer<SampleA, SampleB>::max_frame_size();
    
    RPL::Serializer<SampleA, SampleB> serializer;
    auto frame_size_a = serializer.frame_size<SampleA>();
    auto frame_size_b = serializer.frame_size<SampleB>();
    
    auto expected_max = std::max(frame_size_a, frame_size_b);
    
    assert(max_frame_size == expected_max);
    assert(max_frame_size >= frame_size_a);
    assert(max_frame_size >= frame_size_b);
    
    std::cout << "  SampleA frame size: " << frame_size_a << " bytes" << std::endl;
    std::cout << "  SampleB frame size: " << frame_size_b << " bytes" << std::endl;
    std::cout << "  Max frame size: " << max_frame_size << " bytes" << std::endl;
    std::cout << "  Expected max: " << expected_max << " bytes" << std::endl;
    
    std::cout << "✓ Max frame size calculation passed" << std::endl;
}

// Test 7: Struct memory layout verification
void test_struct_memory_layout() {
    std::cout << "Test 7: Struct memory layout verification..." << std::endl;
    
    // Test SampleA layout
    SampleA sample_a{};
    constexpr std::size_t off_a = offsetof(SampleA, a);
    constexpr std::size_t off_b = offsetof(SampleA, b);
    constexpr std::size_t off_c = offsetof(SampleA, c);

    constexpr std::size_t off_d = offsetof(SampleA, d);

    // 断言预期偏移（根据 SampleA 的定义）
    assert(off_a == 0);
    assert(off_b == 1);
    assert(off_c == 3);
    assert(off_d == 7);

    std::cout << "  SampleA field offsets:" << std::endl;
    std::cout << "    a: " << off_a << std::endl;
    std::cout << "    b: " << off_b << std::endl;
    std::cout << "    c: " << off_c << std::endl;
    std::cout << "    d: " << off_d << std::endl;

    
    // Test SampleB layout (not packed, so may have padding)
    SampleB sample_b{};
    constexpr std::size_t off_x = offsetof(SampleB, x);
    constexpr std::size_t off_y = offsetof(SampleB, y);

    assert(off_x == 0);
    // y 应在 x 之后（可能有填充）
    assert(off_y >= off_x + sizeof(int));

    std::cout << "  SampleB field offsets:" << std::endl;
    std::cout << "    x: " << off_x << std::endl;
    std::cout << "    y: " << off_y << std::endl;

    std::cout << "✓ Struct memory layout verification passed" << std::endl;
}

// Test 8: Compile-time trait calculations
void test_compile_time_trait_calculations() {
    std::cout << "Test 8: Compile-time trait calculations..." << std::endl;
    
    // Test that traits can be used in constexpr contexts
    constexpr uint16_t cmd_a = RPL::Meta::PacketTraits<SampleA>::cmd;
    constexpr uint16_t cmd_b = RPL::Meta::PacketTraits<SampleB>::cmd;
    constexpr size_t size_a = RPL::Meta::PacketTraits<SampleA>::size;
    constexpr size_t size_b = RPL::Meta::PacketTraits<SampleB>::size;
    
    // Test constexpr frame size calculations
    constexpr size_t frame_size_a = RPL::Serializer<SampleA>::frame_size<SampleA>();
    constexpr size_t frame_size_b = RPL::Serializer<SampleB>::frame_size<SampleB>();
    constexpr size_t max_frame_size = RPL::Serializer<SampleA, SampleB>::max_frame_size();
    
    assert(cmd_a == 0x0102);
    assert(cmd_b == 0x0103);
    assert(size_a == sizeof(SampleA));
    assert(size_b == sizeof(SampleB));
    
    std::cout << "  Constexpr calculations successful:" << std::endl;
    std::cout << "    SampleA CMD: 0x" << std::hex << cmd_a << std::dec << std::endl;
    std::cout << "    SampleB CMD: 0x" << std::hex << cmd_b << std::dec << std::endl;
    std::cout << "    SampleA size: " << size_a << " bytes" << std::endl;
    std::cout << "    SampleB size: " << size_b << " bytes" << std::endl;
    std::cout << "    SampleA frame size: " << frame_size_a << " bytes" << std::endl;
    std::cout << "    SampleB frame size: " << frame_size_b << " bytes" << std::endl;
    std::cout << "    Max frame size: " << max_frame_size << " bytes" << std::endl;
    
    std::cout << "✓ Compile-time trait calculations passed" << std::endl;
}

int main() {
    std::cout << "=== RPL Packet Traits Tests ===" << std::endl;
    
    try {
        test_sample_a_packet_traits();
        test_sample_b_packet_traits();
        test_command_id_uniqueness();
        test_frame_size_calculations_from_traits();
        test_command_id_to_frame_size_mapping();
        test_max_frame_size_calculation();
        test_struct_memory_layout();
        test_compile_time_trait_calculations();
        
        std::cout << "✓ All packet traits tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}