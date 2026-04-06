#include "RPL/Meta/BitstreamParser.hpp"
#include "RPL/Meta/BitstreamSerializer.hpp"
#include "RPL/Meta/PacketTraits.hpp"
#include <vector>
#include <iostream>
#include <iomanip>

using namespace RPL;
using namespace RPL::Meta;

// --- Test 1: Simple cross-byte bitfields ---
struct RobotStatus {
    uint8_t is_online  : 1;
    uint8_t work_mode  : 3;
    uint8_t error_code : 4;
    uint16_t voltage;       
};

namespace RPL::Meta {
    template <>
    struct PacketTraits<RobotStatus> : PacketTraitsBase<PacketTraits<RobotStatus>> {
        static constexpr uint16_t cmd = 0x1001;
        static constexpr size_t size = 3;
        using BitLayout = std::tuple<
            Field<uint8_t, 1>,
            Field<uint8_t, 3>,
            Field<uint8_t, 4>,
            Field<uint16_t, 16>
        >;
    };
}

void test_simple_parse() {
    std::vector<uint8_t> buffer = {0x9B, 0x34, 0x12};
    auto status = RPL::deserialize_bitstream<RobotStatus>(std::span<const uint8_t>(buffer));
    
    if (status.is_online != 1 || status.work_mode != 5 || 
        status.error_code != 9 || status.voltage != 0x1234) {
        std::cerr << "test_simple_parse failed!" << std::endl;
        std::cerr << "online: " << (int)status.is_online << std::endl;
        std::cerr << "mode: " << (int)status.work_mode << std::endl;
        std::cerr << "error: " << (int)status.error_code << std::endl;
        std::cerr << "voltage: " << status.voltage << std::endl;
        exit(1);
    }
    std::cout << "test_simple_parse passed!" << std::endl;
}

// --- Test 2: Cross-byte extraction ---
struct CrossByteTest {
    uint32_t val1 : 12; // takes 1.5 bytes
    uint32_t val2 : 12; // takes 1.5 bytes
    uint8_t  val3 : 8;  // takes 1 byte
};

namespace RPL::Meta {
    template <>
    struct PacketTraits<CrossByteTest> : PacketTraitsBase<PacketTraits<CrossByteTest>> {
        static constexpr uint16_t cmd = 0x1002;
        static constexpr size_t size = 4;
        using BitLayout = std::tuple<
            Field<uint32_t, 12>,
            Field<uint32_t, 12>,
            Field<uint8_t, 8>
        >;
    };
}

void test_cross_byte_parse() {
    std::vector<uint8_t> buffer = {0xBC, 0xFA, 0xDE, 0x55};
    auto result = RPL::deserialize_bitstream<CrossByteTest>(std::span<const uint8_t>(buffer));
    
    if (result.val1 != 0xABC || result.val2 != 0xDEF || result.val3 != 0x55) {
        std::cerr << "test_cross_byte_parse failed!" << std::endl;
        std::cerr << "val1: " << std::hex << result.val1 << std::endl;
        std::cerr << "val2: " << std::hex << result.val2 << std::endl;
        std::cerr << "val3: " << std::hex << (int)result.val3 << std::endl;
        exit(1);
    }
    std::cout << "test_cross_byte_parse passed!" << std::endl;
}

// --- Test 3: Mixed packet with std::array ---
struct MixedPacket {
    uint8_t a : 4;
    uint8_t b : 4;
    std::array<uint8_t, 2> data;
};

namespace RPL::Meta {
    template <>
    struct PacketTraits<MixedPacket> : PacketTraitsBase<PacketTraits<MixedPacket>> {
        static constexpr uint16_t cmd = 0x2001;
        static constexpr size_t size = 3;
        using BitLayout = std::tuple<
            Field<uint8_t, 4>,
            Field<uint8_t, 4>,
            Field<std::array<uint8_t, 2>, 16>
        >;
    };
}

void test_mixed_array_parse() {
    std::vector<uint8_t> buffer = {0x12, 0xAA, 0xBB};
    auto result = RPL::deserialize_bitstream<MixedPacket>(std::span<const uint8_t>(buffer));

    if (result.a != 2 || result.b != 1 || result.data[0] != 0xAA || result.data[1] != 0xBB) {
        std::cerr << "test_mixed_array_parse failed!" << std::endl;
        exit(1);
    }
    std::cout << "test_mixed_array_parse passed!" << std::endl;
}

int main() {
    test_simple_parse();
    test_cross_byte_parse();
    test_mixed_array_parse();
    return 0;
}