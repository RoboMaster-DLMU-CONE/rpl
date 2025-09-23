#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Serializer.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

// Helper function to compare floating point values
bool float_equal(float a, float b, float epsilon = 1e-6f) {
    return std::abs(a - b) < epsilon;
}

bool double_equal(double a, double b, double epsilon = 1e-9) {
    return std::abs(a - b) < epsilon;
}

// Test 1: Basic packet deserialization
void test_basic_packet_deserialization() {
    std::cout << "Test 1: Basic packet deserialization..." << std::endl;
    
    // First serialize a packet
    RPL::Serializer<SampleA> serializer;
    SampleA original{42, -1234, 3.14f, 2.718};
    
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);
    
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), 1, original);
    assert(serialize_result.has_value());
    
    // Now deserialize
    RPL::Deserializer<SampleA> deserializer;
    
    // Manually set the raw data (simulating parser output)
    auto& raw_ref = deserializer.getRawRef<SampleA>();
    raw_ref = original;
    
    // Get the deserialized packet
    auto deserialized = deserializer.get<SampleA>();
    
    // Verify data consistency
    assert(deserialized.a == original.a);
    assert(deserialized.b == original.b);
    assert(float_equal(deserialized.c, original.c));
    assert(double_equal(deserialized.d, original.d));
    
    std::cout << "✓ Basic packet deserialization passed" << std::endl;
}

// Test 2: Memory pool access (get<T>() and getRawRef<T>())
void test_memory_pool_access() {
    std::cout << "Test 2: Memory pool access..." << std::endl;
    
    RPL::Deserializer<SampleA, SampleB> deserializer;
    
    // Test getRawRef for SampleA
    auto& raw_ref_a = deserializer.getRawRef<SampleA>();
    raw_ref_a.a = 99;
    raw_ref_a.b = -9999;
    raw_ref_a.c = 1.23f;
    raw_ref_a.d = 4.56;
    
    // Test get for SampleA
    auto packet_a = deserializer.get<SampleA>();
    assert(packet_a.a == 99);
    assert(packet_a.b == -9999);
    assert(float_equal(packet_a.c, 1.23f));
    assert(double_equal(packet_a.d, 4.56));
    
    // Test getRawRef for SampleB
    auto& raw_ref_b = deserializer.getRawRef<SampleB>();
    raw_ref_b.x = 777;
    raw_ref_b.y = 8.88;
    
    // Test get for SampleB
    auto packet_b = deserializer.get<SampleB>();
    assert(packet_b.x == 777);
    assert(double_equal(packet_b.y, 8.88));
    
    std::cout << "✓ Memory pool access passed" << std::endl;
}

// Test 3: Data consistency validation
void test_data_consistency_validation() {
    std::cout << "Test 3: Data consistency validation..." << std::endl;
    
    RPL::Deserializer<SampleA, SampleB> deserializer;
    
    // Set up test data for both packet types
    SampleA original_a{42, -1234, 3.14f, 2.718};
    SampleB original_b{1337, 9.876};
    
    // Set raw references
    auto& raw_ref_a = deserializer.getRawRef<SampleA>();
    auto& raw_ref_b = deserializer.getRawRef<SampleB>();
    
    raw_ref_a = original_a;
    raw_ref_b = original_b;
    
    // Get deserialized packets
    auto deserialized_a = deserializer.get<SampleA>();
    auto deserialized_b = deserializer.get<SampleB>();
    
    // Validate SampleA consistency
    bool consistent_a = true;
    consistent_a &= (original_a.a == deserialized_a.a);
    consistent_a &= (original_a.b == deserialized_a.b);
    consistent_a &= float_equal(original_a.c, deserialized_a.c);
    consistent_a &= double_equal(original_a.d, deserialized_a.d);
    assert(consistent_a);
    
    // Validate SampleB consistency
    bool consistent_b = true;
    consistent_b &= (original_b.x == deserialized_b.x);
    consistent_b &= double_equal(original_b.y, deserialized_b.y);
    assert(consistent_b);
    
    std::cout << "✓ Data consistency validation passed" << std::endl;
}

// Test 4: Direct memory pool modification
void test_direct_memory_pool_modification() {
    std::cout << "Test 4: Direct memory pool modification..." << std::endl;
    
    RPL::Deserializer<SampleA> deserializer;
    
    // Set initial data
    auto& raw_ref = deserializer.getRawRef<SampleA>();
    raw_ref.a = 42;
    raw_ref.b = -1234;
    raw_ref.c = 3.14f;
    raw_ref.d = 2.718;
    
    // Get initial packet
    auto initial_packet = deserializer.get<SampleA>();
    assert(initial_packet.a == 42);
    
    // Modify through direct reference
    raw_ref.a = 99;
    raw_ref.b = -9999;
    
    // Get modified packet
    auto modified_packet = deserializer.get<SampleA>();
    assert(modified_packet.a == 99);
    assert(modified_packet.b == -9999);
    assert(float_equal(modified_packet.c, 3.14f)); // Should remain unchanged
    assert(double_equal(modified_packet.d, 2.718)); // Should remain unchanged
    
    std::cout << "✓ Direct memory pool modification passed" << std::endl;
}

// Test 5: Multiple packet type handling
void test_multiple_packet_type_handling() {
    std::cout << "Test 5: Multiple packet type handling..." << std::endl;
    
    RPL::Deserializer<SampleA, SampleB> deserializer;
    
    // Set different data for each packet type
    auto& ref_a = deserializer.getRawRef<SampleA>();
    auto& ref_b = deserializer.getRawRef<SampleB>();
    
    ref_a.a = 11;
    ref_a.b = 22;
    ref_a.c = 3.3f;
    ref_a.d = 4.4;
    
    ref_b.x = 55;
    ref_b.y = 6.6;
    
    // Get packets independently
    auto packet_a = deserializer.get<SampleA>();
    auto packet_b = deserializer.get<SampleB>();
    
    // Verify separation
    assert(packet_a.a == 11);
    assert(packet_a.b == 22);
    assert(float_equal(packet_a.c, 3.3f));
    assert(double_equal(packet_a.d, 4.4));
    
    assert(packet_b.x == 55);
    assert(double_equal(packet_b.y, 6.6));
    
    // Modify one and verify the other is unchanged
    ref_a.a = 99;
    
    auto packet_a_modified = deserializer.get<SampleA>();
    auto packet_b_unchanged = deserializer.get<SampleB>();
    
    assert(packet_a_modified.a == 99);
    assert(packet_b_unchanged.x == 55); // Should be unchanged
    
    std::cout << "✓ Multiple packet type handling passed" << std::endl;
}

// Test 6: Packet copy semantics
void test_packet_copy_semantics() {
    std::cout << "Test 6: Packet copy semantics..." << std::endl;
    
    RPL::Deserializer<SampleA> deserializer;
    
    // Set initial data
    auto& raw_ref = deserializer.getRawRef<SampleA>();
    raw_ref.a = 42;
    raw_ref.b = -1234;
    raw_ref.c = 3.14f;
    raw_ref.d = 2.718;
    
    // Get a copy of the packet
    auto packet_copy = deserializer.get<SampleA>();
    
    // Modify the original
    raw_ref.a = 99;
    
    // Verify the copy is unchanged
    assert(packet_copy.a == 42); // Should remain the original value
    assert(packet_copy.b == -1234);
    assert(float_equal(packet_copy.c, 3.14f));
    assert(double_equal(packet_copy.d, 2.718));
    
    // Verify the pool has the new value
    auto new_packet = deserializer.get<SampleA>();
    assert(new_packet.a == 99);
    
    std::cout << "✓ Packet copy semantics passed" << std::endl;
}

int main() {
    std::cout << "=== RPL Deserialization Tests ===" << std::endl;
    
    try {
        test_basic_packet_deserialization();
        test_memory_pool_access();
        test_data_consistency_validation();
        test_direct_memory_pool_modification();
        test_multiple_packet_type_handling();
        test_packet_copy_semantics();
        
        std::cout << "✓ All deserialization tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}