# RPL (RoboMaster Packet Library)

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/license-ISC-green.svg)](LICENSE)
[![Online Generator](https://img.shields.io/badge/Generator-rplc.cone.team-orange)](https://rplc.cone.team)

RPL 是一个专为嵌入式高性能通信设计的 C++20 数据包序列化/反序列化库，特别针对 RoboMaster 竞赛硬件平台进行了深度优化。

> 🚀 **立即体验在线图形化配置工具**: [rplc.cone.team](https://rplc.cone.team)
> 
> 无需手写繁琐的协议定义代码，通过图形界面即可一键生成高性能的 C++ 头文件。

## 核心特性

### 极致性能
得益于模板元编程和激进的内联优化，RPL 的运行时开销几乎可以忽略不计。在x86平台上进行Benchmark（基于 AMD Ryzen 7 7840HS CPU），结果如下：
- **反序列化**: **< 1ns** (等同于直接内存访问)
- **解析 (Parser)**: **~40ns** (含 CRC8 + CRC16 校验)
- **序列化**: **~20ns**

### 零拷贝
RPL 实现了从硬件外设(DMA)到应用层的全链路零拷贝：
- **DMA 直接写入**: 提供 `get_write_buffer()` 接口，允许 DMA 直接将数据搬运至内部 RingBuffer，无需中间缓冲。
- **分段 CRC 计算**: 即使数据包在 RingBuffer 中跨越了物理边界（Wrap-Around），RPL 也能通过分段 CRC 算法直接校验，**无需将数据拼接到临时缓冲区**。

### 安全可靠
- **非对齐访问保护**: 自动检测硬件平台，在 Cortex-M0 等不支持非对齐访问的架构上自动回退到安全模式，防止 HardFault。
- **编译期检查**: 利用 C++20 Concepts 确保类型安全，将错误暴露在编译阶段。
- **无异常设计**: 全程使用 `tl::expected` 处理错误，适合禁用异常的嵌入式环境。

### 嵌入式友好
- **无动态内存分配**: 核心路径完全不使用 `new/malloc`，杜绝内存碎片。
- **Header-Only**: 核心库仅需包含头文件即可使用，轻松集成到 STM32、Linux 或 Zephyr 项目中。

### 兼容 RoboMaster 裁判系统
RPL 的协议层设计完全兼容 RoboMaster 官方裁判系统串口协议。
- **开箱即用**: 默认支持裁判系统数据帧格式（帧头、CRC8、CRC16）。
- **无缝集成**: 可以直接使用 RPL 解析裁判系统下发的比赛数据（如比赛状态、血量、伤害信息等），无需额外编写解析逻辑。

## 快速上手

### 1. 生成协议代码
访问 [rplc.cone.team](https://rplc.cone.team)，设计你的数据包结构，导出生成的 `.hpp` 文件。

### 2. 集成到项目
```cpp
#include <RPL/Parser.hpp>
#include "MyPackets.hpp" // 生成的协议文件

// 定义解析器
RPL::Deserializer<MyPacketA, MyPacketB> deserializer;
RPL::Parser<MyPacketA, MyPacketB> parser(deserializer);

// 模拟串口接收中断/DMA回调
void UART_IRQHandler() {
    // 方式1: 传统拷贝写入
    // parser.push_data(rx_buffer, len);

    // 方式2: 零拷贝写入 (推荐)
    // auto span = parser.get_write_buffer();
    // HAL_UART_Receive_DMA(&huart, span.data(), span.size());
    // ... 在传输完成回调中:
    // parser.advance_write_index(received_len);
}

// 主循环
void main_loop() {
    // 获取解析出的数据
    auto packet_a = deserializer.get<MyPacketA>();
    // 使用数据...
}
```

## 构建与测试

### 环境要求
- C++20 兼容编译器 (GCC 12+, Clang 15+, MSVC 19.30+)
- CMake 3.20+

### 编译步骤
```bash
mkdir build && cd build
cmake -DBUILD_RPL_TESTS=ON ..
make -j$(nproc)
ctest --output-on-failure
```

## 文档

详细文档请访问 [RPL 文档中心](https://rpl.doc.cone.team/)。

- [快速入门](https://rpl.doc.cone.team/quick_start)
- [集成指南](https://rpl.doc.cone.team/integration_guide)
- [打包指南](https://rpl.doc.cone.team/packaging)

## 目录结构
```
/
├── include/RPL/              # 核心库
│   ├── Containers/           # RingBuffer (支持零拷贝)
│   ├── Parser.hpp            # 流式解析器 (支持分段CRC)
│   └── ...
├── samples/                  # 使用示例
├── tests/                    # 单元测试 (含边界条件测试)
└── benchmark/                # 性能基准测试
```

## 许可证
ISC License - 详见 [LICENSE](LICENSE) 文件。
