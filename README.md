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
得益于模板元编程和激进的内联优化，RPL 的运行时开销几乎可以忽略不计。在 11th Gen Intel Core i7-1185G7 平台上进行 Benchmark，结果如下：

| 操作 | 延迟 | 说明 |
| :--- | :--- | :--- |
| **反序列化 (Get)** | **0.526 ns** | 从内存池提取已解析包，仅需约 11 条指令 |
| **单包序列化** | **21.2 ns** | 包含帧头组装、CRC8 + CRC16 计算 |
| **位域序列化** | **13.7 ns** | 编译期位域布局，高效跨字节操作 |
| **小包解析 (16B)** | **75.0 ns** | 含包头搜索、CRC 校验、分段拷贝入池 |
| **大包解析 (8KB)** | **14.49 μs** | 吞吐量达 539.5 MiB/s |

> 详细跨平台性能数据请参阅 [性能基准测试](docs/pages/benchmark.md)。

### 零拷贝
RPL 实现了从硬件外设 (DMA) 到应用层的全链路零拷贝：
- **DMA 直接写入**: 提供 `get_write_buffer()` 接口，允许 DMA 直接将数据搬运至内部 BipBuffer，无需中间缓冲。
- **分段 CRC 计算**: 即使数据包在 BipBuffer 中跨越了物理边界（Wrap-Around），RPL 也能通过分段 CRC 算法直接校验，**无需将数据拼接到临时缓冲区**。

### 安全可靠
- **BipBuffer + 内存池两段式架构**: 解析后的 Payload 拷贝至独立内存池，CRC 校验失败的数据包直接丢弃，绝不污染业务内存。
- **SeqLock (顺序锁)**: 每次写入内存池前后递增版本号，业务线程通过 `get<T>()` 读取时检查版本号一致性，无需互斥锁，避免读线程被写线程阻塞。
- **非对齐访问保护**: 自动检测硬件平台，在 Cortex-M0 等不支持非对齐访问的架构上自动回退到安全模式，防止 HardFault。
- **编译期检查**: 利用 C++20 Concepts 确保类型安全，`start_byte` 冲突等问题在编译期暴露。
- **无异常设计**: 使用 `tl::expected<T, Error>` 处理错误，8 种错误码覆盖所有异常情况，适合禁用异常的嵌入式环境。

### 编译期元编程
RPL 将一切可能的计算转移到编译期：
- **O(1) 协议路由**: 编译期生成 256 字节直接查找表 (`header_lut`)，解析时无需运行时查表或分支判断。
- **连续内存池映射**: 借助 `frozen` 库编译期构造哈希表，将 `cmd_id` 快速映射到内存池偏移量。
- **位域解析**: 通过 `std::tuple<Field<T, Bits>...>` 声明位段布局，编译期生成偏移量和掩码，运行时无需查表或分支判断。

### 嵌入式友好
- **无动态内存分配**: 核心路径完全不使用 `new/malloc`，杜绝内存碎片。
- **Header-Only**: 核心库仅需包含头文件即可使用，无编译依赖。
- **多平台支持**: 轻松集成到 STM32、Linux 或 Zephyr 项目中。

### 兼容 RoboMaster 裁判系统
RPL 的协议层设计完全兼容 RoboMaster 官方裁判系统串口协议。
- **开箱即用**: 默认支持裁判系统数据帧格式（帧头、CRC8、CRC16）。
- **无缝集成**: 可以直接使用 RPL 解析裁判系统下发的比赛数据（如比赛状态、血量、伤害信息等），无需额外编写解析逻辑。

### 连接健康检测
RPL 提供编译期策略模式的连接健康检测机制：
- **NullConnectionMonitor**: 默认实现，编译器完全优化掉，零开销。
- **TickConnectionMonitor**: 基于时间戳的超时检测，适配任意平台（STM32 `HAL_GetTick()`、Linux `clock_gettime()` 等）。
- **CallbackConnectionMonitor**: 用户自定义回调，灵活处理连接异常。

## 快速上手

### 1. 生成协议代码
访问 [rplc.cone.team](https://rplc.cone.team)，设计你的数据包结构，导出生成的 `.hpp` 文件。

### 2. 集成到项目

#### 定义数据包结构
```cpp
#include <RPL/Parser.hpp>
#include <RPL/Deserializer.hpp>

// 自定义数据包（使用位域压缩）
struct ChassisControl {
    uint8_t  mode : 3;           // 3 bits: 底盘模式
    bool     power_limited : 1;   // 1 bit:  功率限制
    uint16_t vx;                  // 2 bytes: X 轴速度
    uint16_t vy;                  // 2 bytes: Y 轴速度
    float    omega;               // 4 bytes: 角速度
} __attribute__((packed));

// 编译期位域布局描述（通常由生成器自动生成）
template <>
struct RPL::Meta::PacketTraits<ChassisControl> : RPL::Meta::PacketTraitsBase<RPL::Meta::PacketTraits<ChassisControl>> {
    static constexpr uint16_t cmd = 0x0200;
    static constexpr size_t   size = 10;
    using BitLayout = std::tuple<
        RPL::Meta::Field<uint8_t, 3>,   // mode:       bits [0:2]
        RPL::Meta::Field<bool,    1>,   // power_limited: bit [3]
        RPL::Meta::Field<uint16_t,16>,  // vx:         bits [16:31]
        RPL::Meta::Field<uint16_t,16>,  // vy:         bits [32:47]
        RPL::Meta::Field<float,   32>   // omega:      bits [48:79]
    >;
};
```

#### 初始化解析器与反序列化器
```cpp
// 注册需要处理的所有数据包类型
RPL::Deserializer<GameStatus, RobotStatus, ChassisControl> deserializer;
RPL::Parser<GameStatus, RobotStatus, ChassisControl> parser{deserializer};
```

#### 接收端：DMA / 串口中断回调
```cpp
// 方式一：推模式 —— 每次收到数据调用 push_data()
void UART_IRQHandler(const uint8_t* rx_buf, size_t len) {
    auto result = parser.push_data(rx_buf, len);
    if (!result) {
        // 处理错误：缓冲区满、CRC 不匹配等
        // result.error().code → RPL::ErrorCode::BufferOverflow
        // result.error().message → 人类可读的描述
    }
}

// 方式二：零拷贝 DMA 直写 —— DMA 直接搬运到 BipBuffer 内部
void DMA_StartReceive() {
    auto span = parser.get_write_buffer();  // 获取连续可写区域
    HAL_UART_Receive_DMA(&huart1, span.data(), span.size());
}

// DMA 传输完成回调
void DMA_TransComplete(size_t received_len) {
    parser.advance_write_index(received_len);  // 通知 Parser 解析
}
```

#### 业务线程：获取最新数据
```cpp
void control_loop() {
    // O(1) 获取内存对齐后的最新状态，无需关心数据何时到达
    auto chassis = deserializer.get<ChassisControl>();
    auto game    = deserializer.get<GameStatus>();

    // 直接使用，结构体成员已按 CPU 最优对齐
    if (chassis.mode == 2) {  // 云台跟随模式
        // 处理逻辑...
    }
}
```

## 核心架构

RPL 的架构设计可以分为"数据流接收缓冲区"与"编译期内存池"两个独立且协同的模块：

1. **BipBuffer 解析**: Parser 从 BipBuffer 获取连续读视图，执行帧头校验、CRC8/CRC16 校验。
2. **内存池拷贝**: 校验通过的数据被分段拷贝至 Deserializer 内部的连续对齐内存池。
3. **业务获取**: 业务线程通过 `get<T>()` O(1) 获取最新数据，SeqLock 保证读取一致性。

详细的架构说明请参阅 [设计文档](docs/pages/design_document.md)。

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

### CMake 选项
| 选项 | 说明 | 默认 |
|------|------|------|
| `BUILD_RPL_LIBRARY` | 构建 RPL 库 | ON |
| `BUILD_RPL_TESTS` | 构建单元测试 | OFF |
| `BUILD_RPL_BENCHMARK` | 构建性能测试 | OFF |
| `BUILD_RPL_SAMPLES` | 构建示例代码 | OFF |

## 文档

详细文档请访问 [RPL 文档中心](https://rpl.doc.cone.team/)。

- [快速入门](docs/pages/quick_start.md)
- [集成指南](docs/pages/integration_guide.md)
- [打包指南](docs/pages/packaging.md)
- [性能基准测试](docs/pages/benchmark.md)
- [设计文档](docs/pages/design_document.md)

## 目录结构
```
/
├── include/RPL/              # 核心库（Header-Only）
│   ├── Containers/           # BipBuffer（支持零拷贝）、MemoryPool
│   ├── Meta/                 # 位域解析、编译期哈希、PacketTraits
│   ├── Packets/              # 预定义数据包（裁判系统等）
│   ├── Utils/                # 工具类（编译器屏障、连接监控器）
│   ├── Parser.hpp            # 流式解析器（支持分段CRC）
│   ├── Serializer.hpp        # 序列化器
│   └── Deserializer.hpp      # 反序列化器（内存池 + SeqLock）
├── samples/                  # 使用示例
├── tests/                    # 单元测试（含边界条件测试）
├── benchmark/                # 性能基准测试
└── zephyr/                   # Zephyr RTOS 集成
```

## 许可证
ISC License - 详见 [LICENSE](LICENSE) 文件。
