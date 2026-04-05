# 性能基准测试 (Benchmarks)

RPL 专为极致性能而设计。为了验证其在不同硬件架构下的表现，我们进行了跨平台基准测试，涵盖了高性能桌面 CPU 和资源受限的嵌入式 ARM Cortex-M 处理器。

## 测试环境

### 1. 桌面平台 (x86_64)
- **CPU**: 11th Gen Intel(R) Core(TM) i7-1185G7 @ 4.80 GHz
- **OS**: Linux (CachyOS)
- **工具**: [Google Benchmark](https://github.com/google/benchmark)
- **优化级别**: `-O3 -march=native`

### 2. 嵌入式模拟平台 (ARM Cortex-M3)
- **内核**: Zephyr RTOS v4.3.0
- **架构**: ARMv7-M (QEMU Cortex-M3 模拟器)
- **工具**: Zephyr `k_cycle_get_32`
- **优化级别**: `-O2 / -O3` (`CONFIG_SPEED_OPTIMIZATIONS=y`)

---

## 核心指标对比

### 1. 序列化与反序列化 (低延迟路径)
反映了数据从结构体转换为字节流，或从内存池提取已解析包的速度。

| 操作类型 | x86 (延迟) | Cortex-M3 (周期) | 评价 |
| :--- | :--- | :--- | :--- |
| **反序列化 (GetPacket)** | **0.526 ns** | **11 Cycles** | 仅需 11 条指令左右 |
| **单包序列化 (Serialize)** | **21.2 ns** | **158 Cycles** | 包含强化的 CRC8 + CRC16 计算 |
| **位域序列化 (Bitfield)** | **13.7 ns** | - | 高效的跨字节位操作 |

### 2. 解析器吞吐量 (x86 平台)
展示了 `Parser` 在不同写入负载下的数据处理能力。

| 测试场景 | 耗时/帧 | 吞吐量 (MiB/s) | 帧处理速率 |
| :--- | :--- | :--- | :--- |
| **大包解析 (XLarge)** | 14.49 us | **539.5 MiB/s** | 68.9k fps |
| **混合小包解析 (Small)** | 75.0 ns | **317.8 MiB/s** | 13.3M fps |
| **带噪声重同步 (Noise)** | 59.9 us | **536.2 MiB/s** | 550.2k fps |
| **CRC 错误剔除 (Reject)** | 7.29 us | **537.8 MiB/s** | 274.1k fps |

### 3. 嵌入式解析性能 (Cortex-M3)
针对 STM32 等微控制器的实时性参考数据（基于速度优化模式）。

| 测试场景 | 周期/操作 | 72MHz 耗时 (估算) | 480MHz 耗时 (估算) |
| :--- | :--- | :--- | :--- |
| **反序列化 (Get)** | **11 Cycles** | **152 ns** | **23 ns** |
| **单包序列化 (Serialize)** | **158 Cycles** | **2.19 us** | **329 ns** |
| **完整帧写入 (FullWrite)** | **434 Cycles** | **6.03 us** | **904 ns** |
| **逐字节碎片写入 (1B Frag)** | **3948 Cycles** | **54.8 us** | **8.2 us** |

---

## 性能特性分析

### 1. 极致的零拷贝设计
在反序列化测试中，RPL 在 ARM 平台上仅需 **11 个指令周期**。这意味着在 72MHz 的处理器上提取一个数据包仅需约 **150 纳秒**。RPL 几乎不会对任何高频实时控制回路产生可感知的阻塞。

### 2. 优化敏感度与编译器亲和性
通过对比发现，开启 `CONFIG_SPEED_OPTIMIZATIONS` 后，RPL 的性能提升了约 **50% - 60%**。这得益于其深度模板化和内联友好的设计，使得编译器能够激进地进行循环展开和寄存器优化。

### 3. ARM 架构亲和性
测试证明，RPL 的内存访问模型在 ARM Cortex-M 架构上表现稳健：
- **无对齐异常**: 内部字节拷贝逻辑完美避开了 ARM 的非对齐访问限制。
- **DMA 友好**: `FullWrite` 路径比单字节碎片写入快 **9.1 倍**。在嵌入式开发中强烈建议配合 DMA 使用 `advance_write_index()` 接口。

### 4. 位域优化
位域序列化仅需 **13.7 ns**，证明了 RPL 的元编程布局引擎生成的指令非常精简，能够高效处理 RoboMaster 协议中常见的紧凑数据格式。

---

## 如何复现

### 运行 x86 Benchmark
```bash
mkdir build && cd build
cmake -DBUILD_RPL_BENCHMARK=ON ..
make -j
./benchmark/rpl_benchmark
```

### 运行 Zephyr/QEMU Benchmark
```bash
cd tests/zephyr_benchmark
# 确保 prj.conf 中开启了 CONFIG_SPEED_OPTIMIZATIONS=y
west build -b qemu_cortex_m3 . -t run
```
