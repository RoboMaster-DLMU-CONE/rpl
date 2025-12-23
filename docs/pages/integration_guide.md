\page integration_guide 集成指南

本指南介绍如何在不同平台（STM32, Linux, Zephyr）中集成 RPL 库。

## 1. 获取 RPL

### 方法 A: CMake FetchContent (推荐)

在你的 `CMakeLists.txt` 中添加：

```cmake
include(FetchContent)

FetchContent_Declare(
    rpl
    GIT_REPOSITORY https://github.com/RoboMaster-DLMU-CONE/rpl.git
    GIT_TAG main
)
FetchContent_MakeAvailable(rpl)

target_link_libraries(your_target PRIVATE rpl::rpl)
```

### 方法 B: 子模块 (Submodule)

```bash
git submodule add https://github.com/RoboMaster-DLMU-CONE/rpl.git libs/rpl
```

然后在 CMake 中：
```cmake
add_subdirectory(libs/rpl)
target_link_libraries(your_target PRIVATE rpl::rpl)
```

## 2. 嵌入式平台集成 (STM32/GD32)

在嵌入式平台上，推荐使用 **零拷贝 (Zero-Copy)** 模式配合 DMA 使用。

### 核心代码模式

```cpp
#include <RPL/Parser.hpp>
#include "MyPackets.hpp"

// 全局实例
RPL::Deserializer<PacketA, PacketB> deserializer;
RPL::Parser<PacketA, PacketB> parser(deserializer);

// 开启 DMA 接收
void start_dma_receive() {
    // 1. 获取 RPL 内部缓冲区的写入地址
    auto span = parser.get_write_buffer();
    
    // 2. 启动 DMA，直接写入 RPL 内部
    // 注意：这里假设是循环模式或单次传输，具体视 HAL 库而定
    HAL_UART_Receive_DMA(&huart1, span.data(), span.size());
}

// DMA 传输完成/空闲中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        // 1. 计算实际接收长度 (根据 DMA 寄存器或 HAL 状态)
        size_t received_len = ...; 

        // 2. 告知 RPL 写入了多少数据
        // RPL 会自动更新写指针并尝试解析
        parser.advance_write_index(received_len);
        
        // 3. 重新启动接收 (如果是单次模式)
        start_dma_receive();
    }
}

// 主循环
void main() {
    start_dma_receive();
    
    while(1) {
        // 直接读取最新数据
        auto pkt = deserializer.get<PacketA>();
        // ...
    }
}
```

## 3. Linux 集成

在 Linux 上，通常处理串口 (`/dev/ttyUSB0`) 或 SocketCAN。

```cpp
void read_thread() {
    std::vector<uint8_t> temp_buf(1024);
    
    while(running) {
        // 从文件描述符读取
        int n = read(fd, temp_buf.data(), temp_buf.size());
        if (n > 0) {
            // 推入解析器
            parser.push_data(temp_buf.data(), n);
        }
    }
}

你也可以使用我们开发的[HySerial]("https://github.com/RoboMaster-DLMU-CONE/HySerial")或者[HyCAN]("https://github.com/RoboMaster-DLMU-CONE/HyCAN")。请参考`samples`文件夹下的`real_comm`示例。
```

## 4. Zephyr RTOS 集成

RPL 提供了 `west.yml`，可以作为 Zephyr 模块导入。

1. 在 `west.yml` 中添加项目：
```yaml
projects:
  - name: rpl
    repo-path: https://github.com/RoboMaster-DLMU-CONE/rpl
    path: modules/lib/rpl
    revision: main
```

2. 更新 west：
```bash
west update
```

3. 在 `prj.conf` 中启用：
```ini
CONFIG_CPP=y
CONFIG_STD_CPP20=y
```

4. 在代码中使用：
```cpp
#include <RPL/Parser.hpp>
// ... 与标准用法一致
```

你也可以直接在[one-framework]("https://github.com/RoboMaster-DLMU-CONE/one-framework")里直接使用RPL。
