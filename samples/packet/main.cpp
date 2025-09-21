#include "RPL/Packets/SamplePacket/SamplePacket.hpp"
#include "RPL/Deserializer.hpp"
#include "RPL/Serializer.hpp"
#include "RPL/Parser.hpp"
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
    std::cout << "  a: " << static_cast<int>(packet.a) << std::endl;
    std::cout << "  b: " << packet.b << std::endl;
    std::cout << "  c: " << packet.c << std::endl;
    std::cout << "  d: " << packet.d << std::endl;
}

void print_frame_breakdown(const uint8_t* buffer, size_t frame_size)
{
    std::cout << "  Frame breakdown:" << std::endl;
    std::cout << "    Start byte (0xA5): 0x" << std::hex << static_cast<int>(buffer[0]) << std::dec << std::endl;

    uint16_t cmd = *reinterpret_cast<const uint16_t*>(buffer + 1);
    std::cout << "    Command: 0x" << std::hex << cmd << std::dec << std::endl;

    uint16_t data_length = *reinterpret_cast<const uint16_t*>(buffer + 3);
    std::cout << "    Data length: " << data_length << " bytes" << std::endl;

    uint8_t seq_num = buffer[5];
    std::cout << "    Sequence number: " << static_cast<int>(seq_num) << std::endl;

    uint8_t header_crc8 = buffer[6];
    std::cout << "    Header CRC8: 0x" << std::hex << static_cast<int>(header_crc8) << std::dec << std::endl;

    uint16_t frame_crc16 = *reinterpret_cast<const uint16_t*>(buffer + frame_size - 2);
    std::cout << "    Frame CRC16: 0x" << std::hex << frame_crc16 << std::dec << std::endl;
}

int main()
{
    std::cout << "=== RPL Packet Serialization/Deserialization Demo ===" << std::endl;

    // 创建一个示例数据包
    SamplePacket original_packet{42, -1234, 3.14f, 2.718};

    std::cout << "\n1. Original packet:" << std::endl;
    print_packet_info(original_packet);

    std::cout << "\n2. Packet info:" << std::endl;
    std::cout << "  Command: 0x" << std::hex << RPL::Meta::PacketTraits<SamplePacket>::cmd << std::dec << std::endl;
    std::cout << "  Data size: " << RPL::Meta::PacketTraits<SamplePacket>::size << " bytes" << std::endl;

    // 创建序列化器和反序列化器
    RPL::Serializer<SamplePacket> serializer;
    RPL::Deserializer<SamplePacket> deserializer;

    std::cout << "\n3. Serialization:" << std::endl;

    // 计算帧大小并创建缓冲区
    constexpr size_t frame_size = RPL::Serializer<SamplePacket>::frame_size<SamplePacket>();
    std::vector<uint8_t> buffer(frame_size);

    auto serialize_result = serializer.serialize(original_packet, buffer.data(), 1);
    if (serialize_result)
    {
        std::cout << "  Serialization successful, frame size: " << *serialize_result << " bytes" << std::endl;
        print_hex_buffer(buffer.data(), *serialize_result);
        print_frame_breakdown(buffer.data(), *serialize_result);
    }
    else
    {
        std::cout << "  Serialization failed: " << serialize_result.error().message << std::endl;
        return 1;
    }

    std::cout << "\n4. Parser + Deserializer integration:" << std::endl;

    // 创建 Parser，传入 Deserializer 引用
    RPL::Parser<SamplePacket> parser{deserializer};

    // 模拟分批接收数据（模拟 USB 接收）
    size_t half_size = buffer.size() / 2;

    std::cout << "  Simulating USB data reception in chunks..." << std::endl;

    // 第一批数据
    auto result1 = parser.push_data(buffer.data(), half_size);
    if (!result1)
    {
        std::cout << "  Error processing first batch: " << result1.error().message << std::endl;
    }
    else
    {
        std::cout << "  First batch processed successfully" << std::endl;
    }

    // 第二批数据
    auto result2 = parser.push_data(buffer.data() + half_size, buffer.size() - half_size);
    if (!result2)
    {
        std::cout << "  Error processing second batch: " << result2.error().message << std::endl;
    }
    else
    {
        std::cout << "  Second batch processed successfully" << std::endl;
    }

    std::cout << "\n5. Deserialization from memory pool:" << std::endl;

    // 从 Deserializer 获取反序列化的数据包
    auto deserialized_packet = deserializer.get<SamplePacket>();

    std::cout << "  Deserialization successful!" << std::endl;
    print_packet_info(deserialized_packet);

    // 验证数据一致性
    std::cout << "\n6. Data consistency check:" << std::endl;
    bool consistent = true;
    consistent &= (original_packet.a == deserialized_packet.a);
    consistent &= (original_packet.b == deserialized_packet.b);
    consistent &= (original_packet.c == deserialized_packet.c);
    consistent &= (original_packet.d == deserialized_packet.d);

    std::cout << "  Data consistency: " << (consistent ? "PASS" : "FAIL") << std::endl;

    std::cout << "\n7. Direct memory pool access:" << std::endl;

    // 测试直接引用访问
    auto& direct_ref = deserializer.getRawRef<SamplePacket>();
    SamplePacket backup = direct_ref; // 备份原数据

    direct_ref.a = 99;
    direct_ref.b = -9999;

    auto modified_packet = deserializer.get<SamplePacket>();
    std::cout << "  Modified packet through direct reference:" << std::endl;
    print_packet_info(modified_packet);

    // 恢复原数据
    direct_ref = backup;

    std::cout << "\n8. Parser buffer statistics:" << std::endl;
    std::cout << "  Available data: " << parser.available_data() << " bytes" << std::endl;
    std::cout << "  Available space: " << parser.available_space() << " bytes" << std::endl;
    std::cout << "  Buffer full: " << (parser.is_buffer_full() ? "Yes" : "No") << std::endl;

    std::cout << "\n9. Multiple serialization methods:" << std::endl;

    // 方法1: 使用 PacketVariant 进行类型安全的序列化
    std::vector<uint8_t> buffer2(frame_size);
    auto packet_variant = serializer.create_packet_variant(original_packet);
    auto serialize_result2 = serializer.serialize_variant(packet_variant, buffer2.data(), 2);

    if (serialize_result2)
    {
        std::cout << "  Variant-based serialization: " << *serialize_result2 << " bytes" << std::endl;
        print_hex_buffer(buffer2.data(), *serialize_result2);
    }

    // 方法2: 编译期命令码序列化（完美转发）
    std::vector<uint8_t> buffer3(frame_size);
    auto serialize_result3 = serializer.serialize_by_cmd<0x0102>(original_packet, buffer3.data(), 3);

    if (serialize_result3)
    {
        std::cout << "  Compile-time command-based serialization (perfect forwarding): " << *serialize_result3 <<
            " bytes" << std::endl;
        print_hex_buffer(buffer3.data(), *serialize_result3);
    }

    // 方法3: 运行时命令码序列化（使用 variant）
    std::vector<uint8_t> buffer4(frame_size);
    auto serialize_result4 = serializer.serialize_by_cmd_runtime(
        RPL::Meta::PacketTraits<SamplePacket>::cmd,
        packet_variant,
        buffer4.data(),
        4
    );

    if (serialize_result4)
    {
        std::cout << "  Runtime variant-based serialization: " << *serialize_result4 << " bytes" << std::endl;
        print_hex_buffer(buffer4.data(), *serialize_result4);
    }

    // 展示新的实用方法
    std::cout << "\n12. Modern C++ features demo:" << std::endl;
    std::cout << "  Command 0x0102 is valid: " << (serializer.is_valid_cmd(0x0102) ? "Yes" : "No") << std::endl;
    std::cout << "  Command 0x9999 is valid: " << (serializer.is_valid_cmd(0x9999) ? "Yes" : "No") << std::endl;
    std::cout << "  Type index for cmd 0x0102: " << serializer.get_type_index_by_cmd(0x0102) << std::endl;

    // 展示移动语义
    SamplePacket movable_packet{88, -5555, 2.71f, 3.14159};
    std::vector<uint8_t> buffer5(frame_size);
    auto serialize_result5 = serializer.serialize_by_cmd<0x0102>(std::move(movable_packet), buffer5.data(), 5);

    if (serialize_result5)
    {
        std::cout << "  Move semantics serialization: " << *serialize_result5 << " bytes" << std::endl;
    }

    std::cout << "\n10. Error handling tests:" << std::endl;

    // 测试1: 损坏的起始字节
    std::vector<uint8_t> corrupted_buffer = buffer;
    corrupted_buffer[0] = 0xFF; // 破坏起始字节
    parser.clear_buffer(); // 清空缓冲区
    auto error_result1 = parser.push_data(corrupted_buffer.data(), corrupted_buffer.size());
    if (!error_result1)
    {
        std::cout << "  Test 1 (corrupted start byte): " << error_result1.error().message << std::endl;
    }
    else
    {
        std::cout << "  Test 1: Corrupted frame ignored correctly" << std::endl;
    }

    // 测试2: 损坏的CRC
    corrupted_buffer = buffer;
    corrupted_buffer[corrupted_buffer.size() - 1] = 0xFF; // 破坏CRC16
    parser.clear_buffer();

    auto error_result2 = parser.push_data(corrupted_buffer.data(), corrupted_buffer.size());
    if (!error_result2)
    {
        std::cout << "  Test 2 (corrupted CRC): " << error_result2.error().message << std::endl;
    }
    else
    {
        std::cout << "  Test 2: Corrupted CRC frame ignored correctly" << std::endl;
    }

    // 测试3: 不完整帧处理
    parser.clear_buffer();
    auto error_result3 = parser.push_data(buffer.data(), 3); // 只发送前3个字节

    if (!error_result3)
    {
        std::cout << "  Test 3 (incomplete frame): " << error_result3.error().message << std::endl;
    }
    else
    {
        std::cout << "  Test 3: Incomplete frame handled correctly (waiting for more data)" << std::endl;
        std::cout << "    Pending data: " << parser.available_data() << " bytes" << std::endl;
    }

    std::cout << "\n11. Size information:" << std::endl;
    std::cout << "  Max frame size: " << RPL::Serializer<SamplePacket>::max_frame_size() << " bytes" << std::endl;
    std::cout << "  SamplePacket frame size: " << serializer.frame_size<SamplePacket>() << " bytes" << std::endl;
    std::cout << "  Frame size by command: " << serializer.frame_size_by_cmd(0x0102) << " bytes" << std::endl;

    parser.clear_buffer();
    std::cout << "\n=== Demo completed successfully! ===" << std::endl;
    return 0;
}
