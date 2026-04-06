#include "RPL/Meta/BitstreamSerializer.hpp"
#include "RPL/Meta/PacketTraits.hpp"
#include <vector>
#include <iostream>

using namespace RPL;
using namespace RPL::Meta;

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
        using BitLayout = std::tuple<
            Field<uint8_t, 1>,
            Field<uint8_t, 3>,
            Field<uint8_t, 4>,
            Field<uint16_t, 16>
        >;
    };
}

void test_simple_serialize() {
    RobotStatus status;
    status.is_online = 1;
    status.work_mode = 5;
    status.error_code = 9;
    status.voltage = 0x1234;

    std::vector<uint8_t> buffer(3, 0);
    RPL::serialize_bitstream<RobotStatus>(std::span<uint8_t>(buffer), status);
    
    if (buffer[0] != 0x9B || buffer[1] != 0x34 || buffer[2] != 0x12) {
        std::cerr << "test_simple_serialize failed!" << std::endl;
        std::cerr << "b0: " << std::hex << (int)buffer[0] << std::endl;
        std::cerr << "b1: " << std::hex << (int)buffer[1] << std::endl;
        std::cerr << "b2: " << std::hex << (int)buffer[2] << std::endl;
        exit(1);
    }
    std::cout << "test_simple_serialize passed!" << std::endl;
}

struct CrossByteTest {
    uint32_t val1 : 12;
    uint32_t val2 : 12;
    uint8_t  val3 : 8; 
};

namespace RPL::Meta {
    template <>
    struct PacketTraits<CrossByteTest> : PacketTraitsBase<PacketTraits<CrossByteTest>> {
        static constexpr uint16_t cmd = 0x1002;
        using BitLayout = std::tuple<
            Field<uint32_t, 12>,
            Field<uint32_t, 12>,
            Field<uint8_t, 8>
        >;
    };
}

void test_cross_byte_serialize() {
    CrossByteTest ct;
    ct.val1 = 0xABC;
    ct.val2 = 0xDEF;
    ct.val3 = 0x55;

    std::vector<uint8_t> buffer(4, 0);
    RPL::serialize_bitstream<CrossByteTest>(std::span<uint8_t>(buffer), ct);
    
    if (buffer[0] != 0xBC || buffer[1] != 0xFA || buffer[2] != 0xDE || buffer[3] != 0x55) {
        std::cerr << "test_cross_byte_serialize failed!" << std::endl;
        exit(1);
    }
    std::cout << "test_cross_byte_serialize passed!" << std::endl;
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
        using BitLayout = std::tuple<
            Field<uint8_t, 4>,
            Field<uint8_t, 4>,
            Field<std::array<uint8_t, 2>, 16>
        >;
    };
}

void test_mixed_array_serialize() {
    MixedPacket p;
    p.a = 2;
    p.b = 1;
    p.data = {0xAA, 0xBB};
    
    std::vector<uint8_t> buffer(3, 0);
    RPL::serialize_bitstream<MixedPacket>(std::span<uint8_t>(buffer), p);
    
    if (buffer[0] != 0x12 || buffer[1] != 0xAA || buffer[2] != 0xBB) {
        std::cerr << "test_mixed_array_serialize failed!" << std::endl;
        exit(1);
    }
    std::cout << "test_mixed_array_serialize passed!" << std::endl;
}

int main() {
    test_simple_serialize();
    test_cross_byte_serialize();
    test_mixed_array_serialize();
    return 0;
}
