# 首页

## 简介

RPL (RoboMaster Packet Library) 是大连民族大学 C·One 战队研发的高性能 C++20 数据包序列化/反序列化库，专为嵌入式通信场景设计。

## 核心特性

- **极致性能**: 反序列化 < 1ns，解析 ~350ns，序列化 ~15ns。
- **真·零拷贝**: 支持 DMA 直接写入 RingBuffer，配合分段 CRC 计算，实现全链路零拷贝。
- **安全可靠**: 自动处理非对齐访问，编译期类型检查，无异常设计。
- **开箱即用**: 完美兼容 RoboMaster 裁判系统协议。
- **图形化配置**: 提供 [在线生成器](https://rplc.cone.team) 快速生成代码。

## 文档导航

- \ref quick_start "快速入门" : 5分钟跑通第一个 RPL 程序。
- \ref integration_guide "集成指南" : 在 STM32、Linux、Zephyr 中集成 RPL。
- \ref packaging "打包指南" : 如何打包和分发 RPL 库。

