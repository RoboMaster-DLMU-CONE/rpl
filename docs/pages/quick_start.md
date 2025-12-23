\page quick_start 快速入门

本指南将带你快速上手 RPL，完成第一个数据包的定义、序列化和解析。

## 1. 定义数据包

推荐使用 [RPL 在线生成器](https://rplc.cone.team) 图形化生成代码。

如果你想手动编写，只需定义一个结构体并特化 `PacketTraits`：

```cpp
// MyPackets.hpp
#pragma once
#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

// 定义数据结构 (必须是标准布局)
struct ControlPacket {
    float x;
    float y;
    float w;
    uint8_t mode;
};

// 注册数据包元信息
namespace RPL::Meta {
    template <>
    struct PacketTraits<ControlPacket> {
        static constexpr uint16_t id = 0x101; // 命令码
        static constexpr size_t size = sizeof(ControlPacket);
    };
}
```

## 2. 编写主程序

创建一个 `main.cpp`：

```cpp
#include <iostream>
#include <vector>
#include <RPL/Serializer.hpp>
#include <RPL/Deserializer.hpp>
#include <RPL/Parser.hpp>
#include "MyPackets.hpp"

int main() {
    // 1. 初始化组件
    // 序列化器
    RPL::Serializer<ControlPacket> serializer;
    // 反序列化器
    RPL::Deserializer<ControlPacket> deserializer;
    // 解析器 (绑定反序列化器)
    RPL::Parser<ControlPacket> parser(deserializer);

    // 2. 创建并序列化数据
    ControlPacket tx_packet{1.0f, 2.0f, 3.14f, 1};
    std::vector<uint8_t> buffer(128);
    
    // 序列化到 buffer
    auto result = serializer.serialize(buffer.data(), buffer.size(), tx_packet);
    size_t frame_len = result.value();
    
    std::cout << "Serialized " << frame_len << " bytes." << std::endl;

    // 3. 模拟接收和解析
    // 将数据推入解析器
    parser.push_data(buffer.data(), frame_len);

    // 4. 获取解析结果
    auto rx_packet = deserializer.get<ControlPacket>();
    
    std::cout << "Received: x=" << rx_packet.x 
              << ", y=" << rx_packet.y 
              << ", mode=" << (int)rx_packet.mode << std::endl;

    return 0;
}
```

## 3. 编译运行

编写 `CMakeLists.txt`：

```cmake
cmake_minimum_required(VERSION 3.20)
project(rpl_demo)

find_package(rpl REQUIRED)

add_executable(demo main.cpp)
target_link_libraries(demo PRIVATE rpl::rpl)
```

构建并运行：

```bash
mkdir build && cd build
cmake ..
make
./demo
```

恭喜！你已经成功运行了 RPL。接下来请阅读 \ref integration_guide "集成指南" 了解如何在嵌入式项目中使用。
