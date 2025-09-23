# RPL - RoboMaster Packet Library Copilot 指令

## 仓库功能概述

RPL 是一个针对 RoboMaster 使用的硬件平台打造的 C++ 数据包序列化/反序列化库。这是一个高性能的 C++20 库，专门设计用于处理
RoboMaster 竞赛中的数据包通信。

**主要功能：**

- 开箱即用的裁判系统数据解析
- 快速生成自定义数据包
- 高性能的数据包序列化与反序列化
- 支持多数据包并发处理
- 内存池优化的数据管理
- 模拟USB数据分批接收处理

## 高层次技术详情

**项目类型：** C++ 静态库与工具套件  
**语言标准：** C++20  
**构建系统：** CMake (最低版本 3.20)  
**许可证：** ISC License  
**项目规模：** 中型库（约 11 个头文件，核心功能集中）

**核心架构组件：**

- `rpl` - 核心接口库（header-only）
- `rplc` - 命令行工具，用于解析JSON并转换为RPL包格式
- `samples` - 示例程序，演示完整功能
- `benchmark` - 性能基准测试套件

**主要依赖库：**

- `tl::expected` - 错误处理
- `frozen` - 编译时数据结构
- `cppcrc` - CRC校验算法
- `ringbuffer` - 环形缓冲区
- `fmt` - 字符串格式化
- `CLI11` - 命令行解析（仅rplc工具）
- `nlohmann_json` - JSON处理（仅rplc工具）
- `Google Benchmark` - 性能测试（可选）

## 构建说明

### 基本构建步骤

**始终按照以下顺序执行，不可省略任何步骤：**

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

**包含基准测试、功能单元测试的构建：**

```bash
mkdir -p build
cd build
cmake -DBUILD_RPL_BENCHMARK=ON -DBUILD_RPL_TESTS=ON ..
make -j$(nproc)
```

**清理并重新构建：**

```bash
cd build
make clean
cmake ..
make -j$(nproc)
```

### 编译要求和关键注意事项

- **编译器要求：** 支持 C++20 的编译器（GCC 13+ 或 Clang 15+）
- **构建目标超时：** 构建过程通常需要 60-120 秒，在CI环境中可能需要更长时间
- **内存需求：** 编译时需要至少 2GB 可用内存（特别是 Google Benchmark）

### 已知的编译问题和解决方案

1**Google Benchmark 编译警告**  
问题：在基准测试中有未使用返回值的警告  
说明：这些是已知的非致命警告，不影响功能

### 运行示例和测试

**运行核心功能示例：**

```bash
cd build
./samples/packet/rpl_sample_packet
```

**运行性能基准测试：**

```bash
cd build
./benchmark/rpl_benchmark
```

**命令行工具（目前为占位符实现）：**

```bash
cd build
./src/rplc/rplc --help
```

## 代码库布局

### 主要目录结构

```
/
├── include/RPL/              # 头文件（核心API）
│   ├── Containers/           # 容器类（内存池、环形缓冲区）
│   ├── Meta/                 # 元编程工具（包特征、信息收集器）
│   ├── Packets/Sample/       # 示例数据包定义
│   ├── Utils/                # 工具类（错误定义、常量）
│   ├── Deserializer.hpp     # 反序列化器
│   ├── Parser.hpp           # 数据包解析器
│   └── Serializer.hpp       # 序列化器
├── src/                      # 源代码
│   ├── rpl/                  # 核心库（header-only，仅CMakeLists.txt）
│   └── rplc/                 # 命令行工具源码
├── samples/packet/           # 完整功能演示
├── benchmark/                # 性能基准测试
├── tests/                    # 功能单元测试
├── cmake/                    # CMake 模块（依赖获取脚本）
└── build/                    # 构建输出目录（gitignore）
```

### 关键配置文件

- `CMakeLists.txt` - 主构建配置，定义 C++20 标准和编译选项
- `.gitignore` - 排除构建文件和IDE配置
- `LICENSE` - ISC 许可证

### 核心头文件说明

**必须理解的核心文件：**

- `include/RPL/Meta/PacketTraits.hpp` - 数据包特征定义，包含大小和命令ID
- `include/RPL/Serializer.hpp` - 序列化核心逻辑
- `include/RPL/Deserializer.hpp` - 反序列化和内存池管理
- `include/RPL/Parser.hpp` - 流式数据解析，支持分片接收
- `include/RPL/Utils/Error.hpp` - 错误代码和错误处理机制

### 数据包定义模式

所有数据包必须：

1. 定义结构体（可使用 `__attribute__((packed))`）
2. 特化 `RPL::Meta::PacketTraits` 模板，指定 `cmd` 和 `size`
3. 命令ID使用16位无符号整数，大小为 `sizeof(结构体)`

**示例参考：** `include/RPL/Packets/Sample/SampleA.hpp`

### 性能特征

- 序列化单包：~179ns
- 序列化多包：~322ns
- 反序列化：~33ns
- 完整解析（含噪声处理）：~1058ns

## 验证步骤

### 提交前必须执行的检查

1. **编译验证：** 确保清理构建成功
2. **功能验证：** 使用ctest运行功能测试，确认输出正常
3. **性能验证：** 运行基准测试，确认性能指标合理
4. **头文件检查：** 确保所有必要的 `#include` 语句存在

### 代理建议的额外验证

- 检查内存泄漏（valgrind，如果可用）
- 确认所有编译警告已解决

## 关键实现注意事项

### 必须遵守的约定

- **始终使用 tl::expected 进行错误处理**，不抛出异常
- **新数据包必须特化 PacketTraits 模板**
- **序列化/反序列化必须是无状态的**
- **使用 constexpr 进行编译时计算**

### 常见陷阱

- 数据包结构体必须考虑内存对齐（packed属性）
- CRC计算是自动的，不需要手动处理
- 解析器缓冲区有大小限制（默认计算为最大包大小）

### 调试建议

- 使用示例程序的十六进制输出来验证数据包格式
- 通过解析器统计信息检查缓冲区状态
- 利用错误代码诊断解析问题

## 信任这些指令

**这些指令经过实际验证，包括：**

- 完整的编译和运行测试
- 性能基准验证
- 错误处理验证
- 各种构建配置测试

**仅在以下情况下进行额外搜索：**

- 指令中未涵盖的新功能实现
- 发现指令信息有误或过时
- 需要理解特定的实现细节

**始终先信任并应用这些指令，可以显著提高开发效率并减少探索时间。**