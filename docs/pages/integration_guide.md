\page integration_guide 集成指南

这里是关于如何在 Linux 、 Windows和 Zephyr 项目中引入 OneMotor 库的说明。

## Linux

### 前置依赖

- 构建工具
    - CMake ( >= 3.20 )
- 编译器
    - GCC ( >= 13 ) 或 Clang ( >= 15 )
- 系统环境
    - Linux can 和 vcan 内核模块（通常通过 linux-modules-extra 包提供）
    - 使用`modprobe`加载内核模块：

    ```shell
    sudo modprobe vcan
    sudo modprobe can
    ```
    - 需要[HyCAN](https://github.com/RoboMaster-DLMU-CONE/HyCAN)
      库。你可以从源代码构建，或者直接在[HyCAN Release](https://github.com/RoboMaster-DLMU-CONE/HyCAN/releases)里下载最新的安装包

### 使用CMake远程获取

可以直接在CMake工程中通过Github仓库远程获取OneMotor仓库。请参考下面的`CMakeLists.txt`示例代码：

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyConsumerApp)

include(FetchContent)

# 尝试查找已安装的 OneMotor
find_package(OneMotor QUIET)

if (NOT OneMotor_FOUND)
    message(STATUS "本地未找到 OneMotor 包，尝试从 GitHub 获取...")
    FetchContent_Declare(
            OneMotor_fetched
            GIT_REPOSITORY "https://github.com/RoboMaster-DLMU-CONE/OneMotor"
            GIT_TAG "main"
    )
    FetchContent_MakeAvailable(OneMotor_fetched)
else ()
    message(STATUS "已找到 OneMotor 版本 ${OneMotor_VERSION}")
endif ()

add_executable(MyConsumerApp main.cpp)

target_link_libraries(MyConsumerApp PRIVATE OneMotor::OneMotor)
```

### 从源代码构建

#### CMake构建

```shell
cmake -S . -B build
cmake --build build
#可选：安装到系统
sudo cmake --install build
```

## Windows (WSL2)

TBD

## Zephyr

> 推荐使用[one-framework](https://github.com/RoboMaster-DLMU-CONE/one-framework)来获取开箱即用的OneMotor模组
> 
