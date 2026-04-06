#include <RPL/RPL.hpp>
#include <iostream>
#include <cassert>

// Sample packet for testing
struct SamplePacket {
    uint8_t val;
};

// Define PacketTraits for SamplePacket
namespace RPL::Meta {
template <> struct PacketTraits<SamplePacket> : PacketTraitsBase<SamplePacket> {
    static constexpr uint16_t cmd = 0x01;
    static constexpr size_t size = sizeof(SamplePacket);
    static constexpr bool is_fixed_size = true;
};
} // namespace RPL::Meta

int main() {
    // Test Deserializer
    RPL::Deserializer<SamplePacket> deserializer;
    
    // Test Serializer
    RPL::Serializer<SamplePacket> serializer;
    
    // Test Parser
    RPL::Parser<SamplePacket> parser(deserializer);
    
    std::cout << "Amalgamation test passed!" << std::endl;
    return 0;
}
