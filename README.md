RPL - RoboMaster Packet Library

RPL 是一个面向 RoboMaster 竞赛硬件平台的高性能 C++20 数据包序列化/反序列化库。

- 开箱即用的裁判系统数据解析与示例数据包
- 快速生成自定义数据包头文件，支持图形化界面（[PRTS-App](https://github.com/RoboMaster-DLMU-CONE/PRTS-App)）
- 高性能、零异常、constexpr 友好
- 支持流式解析、分片接收、并发多包处理

## 功能亮点

- 序列化/反序列化：快速、安全；支持多包并发
- 解析器：面向流式/分片接收场景，带噪声容错
- 内存池：减少动态分配，提升实时性
- 跨平台：Windows、Linux、arm-none-eabi、Zephyr等均可使用
- 性能参考（依环境不同略有差异）：
    - 序列化：≈ 15ns/包
    - 反序列化：≈ 400ns/包

## 组件概览

- rpl：核心库（header-only）
- samples：示例程序
- benchmark：性能测试
- tests：功能单元测试

## 编译环境与依赖

- 语言标准：C++20
- 构建系统：CMake 3.20+
- 编译器：GCC 13+ 或 Clang 15+
- 主要依赖：tl::expected、frozen、cppcrc、ringbuffer、fmt、CLI11、nlohmann_json、Google Benchmark（可选）

## 构建与安装

1) 标准构建

```bash
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

2) 启用基准与测试

```bash
mkdir -p build
cd build
cmake -DBUILD_RPL_BENCHMARK=ON -DBUILD_RPL_TESTS=ON ..
make -j$(nproc)
```


运行示例/基准/测试

```bash
# 示例
cd build
./samples/packet/rpl_sample_packet

# 基准
cd build
./benchmark/rpl_benchmark

# 测试（如果已开启）
cd build
ctest --output-on-failure
```

## 图形化界面生成自定义包

使用[PRTS-App](https://github.com/RoboMaster-DLMU-CONE/PRTS-App)

## 代码结构

```
/
├── include/RPL/              # 核心API（header-only）
│   ├── Containers/
│   ├── Meta/
│   ├── Packets/Sample/
│   ├── Utils/
│   ├── Deserializer.hpp
│   ├── Parser.hpp
│   └── Serializer.hpp
├── src/
│   └── rpl/                  # 核心库（仅 CMakeLists）
├── samples/packet/           # 示例
├── benchmark/                # 性能基准
├── tests/                    # 单元与集成测试
├── cmake/                    # CMake 模块与依赖脚本
└── build/                    # 构建输出（被 .gitignore 忽略）
```

## 许可证

- ISC License，详见 LICENSE

## 常见问题

- 构建时间较长或内存占用高：启用基准或测试时请确保 ≥2GB 可用内存
- 依赖下载失败：检查网络或代理；可预装 fmt/CLI11/json 等
- Windows 路径问题：建议在 JSON 与命令行中使用正斜杠 /

如需更多帮助，请查阅 docs/ 目录或示例代码。

