RPL - RoboMaster Packet Library

## 项目简介

RPL 是一个针对 RoboMaster 使用的硬件平台打造的的 C++ 数据包 序列 / 反序列化库。

- 开箱即用的裁判系统数据解析
- 快速生成自定义数据包

## 项目结构

- rplc 用于解析自定义数据包
- rpl 是核心库，用户需要将其链接到目标
- include/packets中包含RoboMaster或其它常用数据包，开箱即用。

## 使用