#include <RPL/Packets/Sample/SampleA.hpp>
#include <RPL/Packets/Sample/SampleB.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include <RPL/Serializer.hpp>
#include <RPL/Utils/ConnectionMonitor.hpp>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

// 模拟 Tick 提供器 (用于测试)
struct MockTickProvider {
    using tick_type = uint32_t;
    static inline tick_type current_tick = 0;

    static tick_type now() { return current_tick; }
    static void advance(tick_type delta) { current_tick += delta; }
    static void reset() { current_tick = 0; }
};

// Test 1: NullConnectionMonitor 零开销验证
void test_null_monitor_zero_overhead() {
    std::cout << "Test 1: NullConnectionMonitor zero overhead..." << std::endl;

    // 验证 NullConnectionMonitor 是空类型
    static_assert(sizeof(RPL::NullConnectionMonitor) == 1,
                  "NullConnectionMonitor should be empty class");

    // 创建 Parser 不带 Monitor (使用默认 NullConnectionMonitor)
    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<SampleA, SampleB> parser{deserializer};

    // on_packet_received 应该是空操作
    RPL::NullConnectionMonitor monitor;
    monitor.on_packet_received(); // 编译通过即可

    std::cout << "✓ NullConnectionMonitor zero overhead passed" << std::endl;
}

// Test 2: TickConnectionMonitor 基本功能
void test_tick_monitor_basic() {
    std::cout << "Test 2: TickConnectionMonitor basic functionality..."
              << std::endl;

    MockTickProvider::reset();

    RPL::TickConnectionMonitor<MockTickProvider> monitor;

    // 初始状态: 未收到任何包
    assert(monitor.get_last_tick() == 0);

    // 模拟收到包
    MockTickProvider::advance(100);
    monitor.on_packet_received();
    assert(monitor.get_last_tick() == 100);

    // 检查连接状态
    MockTickProvider::advance(50);
    assert(monitor.is_connected(100)); // 50ms < 100ms timeout

    MockTickProvider::advance(60);
    assert(!monitor.is_connected(100)); // 110ms > 100ms timeout

    // 测试 get_elapsed
    MockTickProvider::current_tick = 200;
    monitor.on_packet_received();
    MockTickProvider::advance(30);
    assert(monitor.get_elapsed() == 30);

    // 测试 reset
    monitor.reset();
    assert(monitor.get_last_tick() == MockTickProvider::now());

    std::cout << "✓ TickConnectionMonitor basic functionality passed"
              << std::endl;
}

// Test 3: Parser 与 Monitor 集成
void test_parser_with_monitor() {
    std::cout << "Test 3: Parser with TickConnectionMonitor integration..."
              << std::endl;

    MockTickProvider::reset();

    using Monitor = RPL::TickConnectionMonitor<MockTickProvider>;

    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<Monitor, SampleA, SampleB> parser{deserializer};
    RPL::Serializer<SampleA, SampleB> serializer;

    // 初始状态
    assert(parser.get_connection_monitor().get_last_tick() == 0);

    // 准备测试数据
    SampleA packet_a{42, -1234, 3.14f, 2.718};
    constexpr size_t frame_size =
        RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);

    auto serialize_result =
        serializer.serialize(buffer.data(), buffer.size(), packet_a);
    assert(serialize_result.has_value());

    // 推送数据前设置时间
    MockTickProvider::current_tick = 1000;

    // 推送数据并解析
    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // 验证监控器已更新
    assert(parser.get_connection_monitor().get_last_tick() == 1000);

    // 模拟时间流逝
    MockTickProvider::advance(50);
    assert(parser.get_connection_monitor().is_connected(100));

    MockTickProvider::advance(60);
    assert(!parser.get_connection_monitor().is_connected(100));

    std::cout << "✓ Parser with TickConnectionMonitor integration passed"
              << std::endl;
}

// Test 4: 多包解析时监控器更新
void test_monitor_updates_on_multiple_packets() {
    std::cout << "Test 4: Monitor updates on multiple packets..." << std::endl;

    MockTickProvider::reset();

    using Monitor = RPL::TickConnectionMonitor<MockTickProvider>;

    RPL::Deserializer<SampleA, SampleB> deserializer;
    RPL::Parser<Monitor, SampleA, SampleB> parser{deserializer};
    RPL::Serializer<SampleA, SampleB> serializer;

    SampleA packet_a{42, -1234, 3.14f, 2.718};
    SampleB packet_b{1337, 9.876};

    constexpr size_t total_size =
        RPL::Serializer<SampleA, SampleB>::frame_size<SampleA>() +
        RPL::Serializer<SampleA, SampleB>::frame_size<SampleB>();
    std::vector<uint8_t> buffer(total_size);

    auto serialize_result =
        serializer.serialize(buffer.data(), buffer.size(), packet_a, packet_b);
    assert(serialize_result.has_value());

    // 设置初始时间
    MockTickProvider::current_tick = 500;

    // 推送两个包
    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    // 由于两个包都在同一时刻解析，last_tick 应该是 500
    // (实际上每个包解析时都会调用 on_packet_received，但时间没变)
    assert(parser.get_connection_monitor().get_last_tick() == 500);

    std::cout << "✓ Monitor updates on multiple packets passed" << std::endl;
}

// Test 5: 向后兼容性测试
void test_backward_compatibility() {
    std::cout << "Test 5: Backward compatibility..." << std::endl;

    // 不带 Monitor 的 Parser 应该正常工作
    RPL::Deserializer<SampleA> deserializer;
    RPL::Parser<SampleA> parser{deserializer};
    RPL::Serializer<SampleA> serializer;

    SampleA packet{42, -1234, 3.14f, 2.718};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);

    auto serialize_result =
        serializer.serialize(buffer.data(), buffer.size(), packet);
    assert(serialize_result.has_value());

    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    auto parsed = deserializer.get<SampleA>();
    assert(parsed.a == packet.a);
    assert(parsed.b == packet.b);

    // 验证默认 Monitor 是 NullConnectionMonitor
    static_assert(std::is_same_v<decltype(parser.get_connection_monitor()),
                                 RPL::NullConnectionMonitor &>,
                  "Default monitor should be NullConnectionMonitor");

    std::cout << "✓ Backward compatibility passed" << std::endl;
}

// Test 6: CallbackConnectionMonitor 测试
void test_callback_monitor() {
    std::cout << "Test 6: CallbackConnectionMonitor..." << std::endl;

    static int callback_count = 0;

    struct TestCallback {
        static void on_packet() { callback_count++; }
    };

    using Monitor = RPL::CallbackConnectionMonitor<TestCallback>;

    // Deserializer 只包含 Packet 类型，不包含 Monitor
    RPL::Deserializer<SampleA> deserializer;
    // Parser 的模板参数是 Monitor + Packets
    RPL::Parser<Monitor, SampleA> parser{deserializer};
    RPL::Serializer<SampleA> serializer;

    SampleA packet{42, -1234, 3.14f, 2.718};
    constexpr size_t frame_size = RPL::Serializer<SampleA>::frame_size<SampleA>();
    std::vector<uint8_t> buffer(frame_size);

    callback_count = 0;

    auto serialize_result =
        serializer.serialize(buffer.data(), buffer.size(), packet);
    assert(serialize_result.has_value());

    auto result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    assert(callback_count == 1);

    // 推送第二个包
    parser.clear_buffer();
    result = parser.push_data(buffer.data(), buffer.size());
    assert(result.has_value());

    assert(callback_count == 2);

    std::cout << "✓ CallbackConnectionMonitor passed" << std::endl;
}

int main() {
    std::cout << "=== RPL Connection Monitor Tests ===" << std::endl;

    try {
        test_null_monitor_zero_overhead();
        test_tick_monitor_basic();
        test_parser_with_monitor();
        test_monitor_updates_on_multiple_packets();
        test_backward_compatibility();
        test_callback_monitor();

        std::cout << "✓ All connection monitor tests passed!" << std::endl;
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "❌ Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ Test failed with unknown exception" << std::endl;
        return 1;
    }
}
