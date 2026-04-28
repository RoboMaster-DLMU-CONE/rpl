#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Meta/BitstreamTraits.hpp>
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>

// ========== Test Data Structures ==========

// 1. Plain POD packet with after_parse + skip_memory_pool
struct HookPacketA
{
    uint8_t flags;
    int16_t value;
};

static bool g_hook_a_called = false;
static HookPacketA g_hook_a_received{};

template <>
struct RPL::Meta::PacketTraits<HookPacketA> : PacketTraitsBase<PacketTraits<HookPacketA>>
{
    static constexpr uint16_t cmd = 0x0201;
    static constexpr size_t size = sizeof(HookPacketA);
    static constexpr bool skip_memory_pool = true;

    static void after_parse(const HookPacketA& packet)
    {
        g_hook_a_called = true;
        g_hook_a_received = packet;
    }
};

// 2. BitLayout packet with after_parse + skip_memory_pool
struct HookPacketBit
{
    uint8_t field_a; // 3 bits
    uint8_t field_b; // 5 bits
};

static bool g_hook_bit_called = false;
static HookPacketBit g_hook_bit_received{};

template <>
struct RPL::Meta::PacketTraits<HookPacketBit> : PacketTraitsBase<PacketTraits<HookPacketBit>>
{
    static constexpr uint16_t cmd = 0x0202;
    static constexpr size_t size = 1; // 3 + 5 = 8 bits = 1 byte
    static constexpr bool skip_memory_pool = true;

    using BitLayout = std::tuple<
        Field<uint8_t, 3>,
        Field<uint8_t, 5>
    >;

    static void after_parse(const HookPacketBit& packet)
    {
        g_hook_bit_called = true;
        g_hook_bit_received = packet;
    }
};

// 3. Packet with after_parse but NO skip_memory_pool (default false)
struct HookPacketNoSkip
{
    uint32_t data;
};

static bool g_hook_noskip_called = false;
static HookPacketNoSkip g_hook_noskip_received{};

template <>
struct RPL::Meta::PacketTraits<HookPacketNoSkip> : PacketTraitsBase<PacketTraits<HookPacketNoSkip>>
{
    static constexpr uint16_t cmd = 0x0203;
    static constexpr size_t size = sizeof(HookPacketNoSkip);
    // skip_memory_pool defaults to false

    static void after_parse(const HookPacketNoSkip& packet)
    {
        g_hook_noskip_called = true;
        g_hook_noskip_received = packet;
    }
};

// ========== Test Cases ==========

// Test 1: Plain POD with after_parse and skip_memory_pool
void test_after_parse_plain_pod()
{
    std::cout << "Test 1: after_parse plain POD with skip_memory_pool..." << std::endl;

    g_hook_a_called = false;
    std::memset(&g_hook_a_received, 0, sizeof(g_hook_a_received));

    RPL::Serializer<HookPacketA> serializer;
    RPL::Deserializer<HookPacketA> deserializer;
    RPL::Parser<HookPacketA> parser{deserializer};

    HookPacketA packet{0xAB, 0x1234};

    constexpr size_t frame_size = RPL::Serializer<HookPacketA>::frame_size<HookPacketA>();
    std::vector<uint8_t> buffer(frame_size);
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), packet);
    assert(serialize_result.has_value());

    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // Verify after_parse was called with correct data
    assert(g_hook_a_called);
    assert(g_hook_a_received.flags == packet.flags);
    assert(g_hook_a_received.value == packet.value);

    // Verify data was NOT written to MemoryPool (skip_memory_pool = true)
    // Since no write() occurred, the version counter is still 0 (even).
    // get() should return default-initialized data (pool was zero-initialized)
    auto parsed = deserializer.get<HookPacketA>();
    assert(parsed.flags == 0); // pool is zero-initialized, no write happened
    assert(parsed.value == 0);

    std::cout << "✓ after_parse plain POD with skip_memory_pool passed" << std::endl;
}

// Test 2: BitLayout packet with after_parse and skip_memory_pool
void test_after_parse_bitlayout()
{
    std::cout << "Test 2: after_parse BitLayout with skip_memory_pool..." << std::endl;

    g_hook_bit_called = false;
    std::memset(&g_hook_bit_received, 0, sizeof(g_hook_bit_received));

    RPL::Serializer<HookPacketBit> serializer;
    RPL::Deserializer<HookPacketBit> deserializer;
    RPL::Parser<HookPacketBit> parser{deserializer};

    HookPacketBit packet{0x05, 0x0A}; // field_a = 5 (3 bits), field_b = 10 (5 bits)

    constexpr size_t frame_size = RPL::Serializer<HookPacketBit>::frame_size<HookPacketBit>();
    std::vector<uint8_t> buffer(frame_size);
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), packet);
    assert(serialize_result.has_value());

    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // Verify after_parse was called with correct data
    assert(g_hook_bit_called);
    assert(g_hook_bit_received.field_a == packet.field_a);
    assert(g_hook_bit_received.field_b == packet.field_b);

    // Verify data was NOT written to MemoryPool
    auto parsed = deserializer.get<HookPacketBit>();
    assert(parsed.field_a == 0);
    assert(parsed.field_b == 0);

    std::cout << "✓ after_parse BitLayout with skip_memory_pool passed" << std::endl;
}

// Test 3: Mixed scenario - HookPacketA skips pool, SampleA goes to pool
void test_mixed_skip_and_normal()
{
    std::cout << "Test 3: mixed skip_pool and normal packets..." << std::endl;

    g_hook_a_called = false;
    std::memset(&g_hook_a_received, 0, sizeof(g_hook_a_received));

    RPL::Serializer<HookPacketA, SampleA> serializer;
    RPL::Deserializer<HookPacketA, SampleA> deserializer;
    RPL::Parser<HookPacketA, SampleA> parser{deserializer};

    HookPacketA hook_packet{0xCD, -5678};
    SampleA normal_packet{42, -1234, 3.14f, 2.718};

    constexpr size_t total_size =
        RPL::Serializer<HookPacketA, SampleA>::frame_size<HookPacketA>() +
        RPL::Serializer<HookPacketA, SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(total_size);

    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), hook_packet, normal_packet);
    assert(serialize_result.has_value());

    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // HookPacketA should have triggered after_parse but skipped pool
    assert(g_hook_a_called);
    assert(g_hook_a_received.flags == hook_packet.flags);
    assert(g_hook_a_received.value == hook_packet.value);

    auto parsed_hook = deserializer.get<HookPacketA>();
    assert(parsed_hook.flags == 0); // not written
    assert(parsed_hook.value == 0);

    // SampleA should be normally available in pool
    auto parsed_normal = deserializer.get<SampleA>();
    assert(parsed_normal.a == normal_packet.a);
    assert(parsed_normal.b == normal_packet.b);

    std::cout << "✓ Mixed skip_pool and normal packets passed" << std::endl;
}

// Test 4: after_parse without skip_memory_pool - callback fires AND data goes to pool
void test_after_parse_without_skip()
{
    std::cout << "Test 4: after_parse without skip_memory_pool..." << std::endl;

    g_hook_noskip_called = false;
    std::memset(&g_hook_noskip_received, 0, sizeof(g_hook_noskip_received));

    RPL::Serializer<HookPacketNoSkip> serializer;
    RPL::Deserializer<HookPacketNoSkip> deserializer;
    RPL::Parser<HookPacketNoSkip> parser{deserializer};

    HookPacketNoSkip packet{0xDEADBEEF};

    constexpr size_t frame_size = RPL::Serializer<HookPacketNoSkip>::frame_size<HookPacketNoSkip>();
    std::vector<uint8_t> buffer(frame_size);
    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), packet);
    assert(serialize_result.has_value());

    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // after_parse should have been called
    assert(g_hook_noskip_called);
    assert(g_hook_noskip_received.data == packet.data);

    // Data should ALSO be available in MemoryPool (skip_memory_pool defaults to false)
    auto parsed = deserializer.get<HookPacketNoSkip>();
    assert(parsed.data == packet.data);

    std::cout << "✓ after_parse without skip_memory_pool passed" << std::endl;
}

int main()
{
    std::cout << "=== RPL Parser Hooks Tests ===" << std::endl;

    try
    {
        test_after_parse_plain_pod();
        test_after_parse_bitlayout();
        test_mixed_skip_and_normal();
        test_after_parse_without_skip();

        std::cout << "✓ All parser hooks tests passed!" << std::endl;
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
