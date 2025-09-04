#include "sample_packet/SamplePacket.hpp"
#include <RPL/Parser.hpp>
#include <iostream>
#include <iomanip>
#include <vector>

void print_hex_buffer(const uint8_t* buffer, size_t length)
{
    std::cout << "Buffer (hex): ";
    for (size_t i = 0; i < length; ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

void print_packet_info(const SamplePacket& packet)
{
    std::cout << "Packet data:" << std::endl;
    std::cout << "  Sequence Number: " << static_cast<int>(packet.get_sequence_number()) << std::endl;
    std::cout << "  lift: " << packet.get_lift() << std::endl;
    std::cout << "  stretch: " << packet.get_stretch() << std::endl;
    std::cout << "  shift: " << packet.get_shift() << std::endl;
    std::cout << "  suck_rotate: " << packet.get_suck_rotate() << std::endl;
    std::cout << "  r1: " << packet.get_r1() << std::endl;
    std::cout << "  r2: " << packet.get_r2() << std::endl;
    std::cout << "  r3: " << packet.get_r3() << std::endl;
}


int main()
{
    std::cout << "=== RPL Packet Serialization/Deserialization Demo ===" << std::endl;

    // 创建一个示例数据包
    SamplePacket original_packet{1.5f, 2.3f, 0.8f, 45.0f, 10.1f, 20.2f, 30.3f};
    original_packet.set_sequence_number(42);

    std::cout << "\n1. Original packet:" << std::endl;
    print_packet_info(original_packet);

    std::cout << "\n2. Frame format info:" << std::endl;
    std::cout << "  Data size: " << original_packet.data_size() << " bytes" << std::endl;
    std::cout << "  Frame size: " << original_packet.frame_size() << " bytes" << std::endl;
    std::cout << "  Frame structure: 5 bytes header + " << original_packet.data_size()
        << " bytes data + 2 bytes tail" << std::endl;

    // 序列化
    std::cout << "\n3. Serialization:" << std::endl;
    rpl::Parser<SamplePacket> parser;
    std::vector<uint8_t> buffer(original_packet.frame_size());

    auto serialize_result = parser.serialize(original_packet, buffer.data());
    if (serialize_result)
    {
        std::cout << "  Serialization successful, frame size: " << *serialize_result << " bytes" << std::endl;
        print_hex_buffer(buffer.data(), *serialize_result);

        // 解析帧格式
        std::cout << "  Frame breakdown:" << std::endl;
        std::cout << "    Start byte (0xA5): 0x" << std::hex << static_cast<int>(buffer[0]) << std::dec << std::endl;

        uint16_t data_length = *reinterpret_cast<uint16_t*>(buffer.data() + 1);
        std::cout << "    Data length: " << data_length << " bytes" << std::endl;

        uint8_t seq_num = buffer[3];
        std::cout << "    Sequence number: " << static_cast<int>(seq_num) << std::endl;

        uint8_t header_crc8 = buffer[4];
        std::cout << "    Header CRC8: 0x" << std::hex << static_cast<int>(header_crc8) << std::dec << std::endl;

        uint16_t frame_crc16 = *reinterpret_cast<uint16_t*>(buffer.data() + *serialize_result - 2);
        std::cout << "    Frame CRC16: 0x" << std::hex << frame_crc16 << std::dec << std::endl;
    }
    else
    {
        std::cout << "  Serialization failed: " << serialize_result.error().message << std::endl;
        return 1;
    }

    // 反序列化
    std::cout << "\n4. Deserialization:" << std::endl;
    auto deserialize_result = parser.deserialize(buffer.data(), *serialize_result);
    if (deserialize_result)
    {
        std::cout << "  Deserialization successful!" << std::endl;
        const auto& deserialized_packet = *deserialize_result;
        print_packet_info(deserialized_packet);

        // 验证数据一致性
        std::cout << "\n5. Data consistency check:" << std::endl;
        bool consistent = true;
        consistent &= (original_packet.get_sequence_number() == deserialized_packet.get_sequence_number());
        consistent &= (original_packet.get_lift() == deserialized_packet.get_lift());
        consistent &= (original_packet.get_stretch() == deserialized_packet.get_stretch());
        consistent &= (original_packet.get_shift() == deserialized_packet.get_shift());
        consistent &= (original_packet.get_suck_rotate() == deserialized_packet.get_suck_rotate());
        consistent &= (original_packet.get_r1() == deserialized_packet.get_r1());
        consistent &= (original_packet.get_r2() == deserialized_packet.get_r2());
        consistent &= (original_packet.get_r3() == deserialized_packet.get_r3());

        std::cout << "  Data consistency: " << (consistent ? "PASS" : "FAIL") << std::endl;
    }
    else
    {
        std::cout << "  Deserialization failed: " << deserialize_result.error().message << std::endl;
        return 1;
    }
    // 测试错误处理
    std::cout << "\n6. Error handling test:" << std::endl;

    // 测试1: 损坏的起始字节
    std::vector<uint8_t> corrupted_buffer = buffer;
    corrupted_buffer[0] = 0xFF; // 破坏起始字节
    auto error_result1 = parser.deserialize(corrupted_buffer.data(), corrupted_buffer.size());
    if (!error_result1)
    {
        std::cout << "  Test 1 (corrupted start byte): " << error_result1.error().message << std::endl;
    }

    // 测试2: 损坏的帧头CRC8
    corrupted_buffer = buffer;
    corrupted_buffer[4] = 0xFF; // 破坏帧头CRC8
    auto error_result2 = parser.deserialize(corrupted_buffer.data(), corrupted_buffer.size());
    if (!error_result2)
    {
        std::cout << "  Test 2 (corrupted header CRC8): " << error_result2.error().message << std::endl;
    }

    // 测试3: 损坏的帧尾CRC16
    corrupted_buffer = buffer;
    corrupted_buffer[corrupted_buffer.size() - 1] = 0xFF; // 破坏帧尾CRC16
    auto error_result3 = parser.deserialize(corrupted_buffer.data(), corrupted_buffer.size());
    if (!error_result3)
    {
        std::cout << "  Test 3 (corrupted frame CRC16): " << error_result3.error().message << std::endl;
    }

    std::cout << "\n=== Demo completed successfully! ===" << std::endl;
    return 0;
}
