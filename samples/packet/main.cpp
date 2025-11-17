#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Serializer.hpp>
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

void print_packet_info(const SampleA& packet)
{
    std::cout << "Packet data:" << std::endl;
    std::cout << "  a: " << static_cast<int>(packet.a) << std::endl;
    std::cout << "  b: " << packet.b << std::endl;
    std::cout << "  c: " << packet.c << std::endl;
    std::cout << "  d: " << packet.d << std::endl;
}

void print_packet_info(const SampleB& packet)
{
    std::cout << "Packet data:" << std::endl;
    std::cout << "  x: " << packet.x << std::endl;
    std::cout << "  y: " << packet.y << std::endl;
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
    SampleA original_packet_a{42, -1234, 3.14f, 2.718};
    SampleB original_packet_b{1337, 9.876};

    std::cout << "\n1. Original packets:" << std::endl;
    print_packet_info(original_packet_a);
    print_packet_info(original_packet_b);

    std::cout << "\n2. Packet info:" << std::endl;
    std::cout << "  SampleA Command: 0x" << std::hex << RPL::Meta::PacketTraits<SampleA>::cmd << std::dec << std::endl;
    std::cout << "  SampleA Data size: " << RPL::Meta::PacketTraits<SampleA>::size << " bytes" << std::endl;
    std::cout << "  SampleB Command: 0x" << std::hex << RPL::Meta::PacketTraits<SampleB>::cmd << std::dec << std::endl;
    std::cout << "  SampleB Data size: " << RPL::Meta::PacketTraits<SampleB>::size << " bytes" << std::endl;

    // 创建序列化器和反序列化器
    RPL::Serializer<SampleA, SampleB> serializer;
    RPL::Deserializer<SampleA, SampleB> deserializer;

    std::cout << "\n3. Multi-packet Serialization:" << std::endl;

    // 计算帧大小并创建缓冲区
    constexpr size_t total_frame_size = RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>() + RPL::Serializer<
        SampleA, SampleB>::frame_size<SampleB>();
    std::vector<uint8_t> buffer(total_frame_size);

    auto serialize_result = serializer.serialize(buffer.data(), buffer.size(), original_packet_a, original_packet_b);
    if (serialize_result)
    {
        std::cout << "  Serialization successful, total frame size: " << *serialize_result << " bytes" << std::endl;
        print_hex_buffer(buffer.data(), *serialize_result);
        std::cout << "  Breaking down first frame (SampleA):" << std::endl;
        print_frame_breakdown(buffer.data(), RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>());
        std::cout << "  Breaking down second frame (SampleB):" << std::endl;
        print_frame_breakdown(buffer.data() + RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>(),
                              RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>());
    }
    else
    {
        std::cout << "  Serialization failed: " << serialize_result.error().message << std::endl;
        return 1;
    }

    std::cout << "\n4. Parser + Deserializer integration:" << std::endl;

    // 创建 Parser，传入 Deserializer 引用
    RPL::Parser<SampleA, SampleB> parser{deserializer};

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
    auto deserialized_packet_a = deserializer.get<SampleA>();
    auto deserialized_packet_b = deserializer.get<SampleB>();

    std::cout << "  Deserialization successful!" << std::endl;
    std::cout << "  --- Deserialized SampleA ---" << std::endl;
    print_packet_info(deserialized_packet_a);
    std::cout << "  --- Deserialized SampleB ---" << std::endl;
    print_packet_info(deserialized_packet_b);

    // 验证数据一致性
    std::cout << "\n6. Data consistency check:" << std::endl;
    bool consistent_a = true;
    consistent_a &= (original_packet_a.a == deserialized_packet_a.a);
    consistent_a &= (original_packet_a.b == deserialized_packet_a.b);
    consistent_a &= (original_packet_a.c == deserialized_packet_a.c);
    consistent_a &= (original_packet_a.d == deserialized_packet_a.d);
    std::cout << "  Data consistency for SampleA: " << (consistent_a ? "PASS" : "FAIL") << std::endl;

    bool consistent_b = true;
    consistent_b &= (original_packet_b.x == deserialized_packet_b.x);
    consistent_b &= (original_packet_b.y == deserialized_packet_b.y);
    std::cout << "  Data consistency for SampleB: " << (consistent_b ? "PASS" : "FAIL") << std::endl;


    std::cout << "\n7. Direct memory pool access:" << std::endl;

    // 测试直接引用访问
    auto& direct_ref_a = deserializer.getRawRef<SampleA>();
    SampleA backup_a = direct_ref_a; // 备份原数据

    direct_ref_a.a = 99;
    direct_ref_a.b = -9999;

    auto modified_packet_a = deserializer.get<SampleA>();
    std::cout << "  Modified packet A through direct reference:" << std::endl;
    print_packet_info(modified_packet_a);

    // 恢复原数据
    direct_ref_a = backup_a;

    std::cout << "\n8. Parser buffer statistics:" << std::endl;
    std::cout << "  Available data: " << parser.available_data() << " bytes" << std::endl;
    std::cout << "  Available space: " << parser.available_space() << " bytes" << std::endl;
    std::cout << "  Buffer full: " << (parser.is_buffer_full() ? "Yes" : "No") << std::endl;

    std::cout << "\n9. Error handling tests:" << std::endl;

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

    std::cout << "\n10. Size information:" << std::endl;
    std::cout << "  Max frame size: " << RPL::Serializer<SampleA, SampleB>::max_frame_size() << " bytes" << std::endl;
    std::cout << "  SampleA frame size: " << serializer.frame_size<SampleA>() << " bytes" << std::endl;
    std::cout << "  SampleB frame size: " << serializer.frame_size<SampleB>() << " bytes" << std::endl;
    std::cout << "  Frame size by command (SampleA): " << serializer.frame_size_by_cmd(
        RPL::Meta::PacketTraits<SampleA>::cmd) << " bytes" << std::endl;
    std::cout << "  Frame size by command (SampleB): " << serializer.frame_size_by_cmd(
        RPL::Meta::PacketTraits<SampleB>::cmd) << " bytes" << std::endl;

    parser.clear_buffer();
    std::cout << "\n=== Demo completed successfully! ===" << std::endl;
    return 0;
}
