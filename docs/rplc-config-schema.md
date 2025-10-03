# RPLC JSON配置文件结构规范

## 概述

RPLC（RPL Compiler）是RPL库的包生成工具，通过JSON配置文件生成符合RPL规范的C++头文件。本文档定义了JSON配置文件的结构和使用规范。

## JSON配置结构

### 基本模板

```json
{
  "packet_name": "PacketName",
  "command_id": "0x0104",
  "namespace": null,
  "packed": true,
  "output": {
    "header_file": "include/RPL/Packets/Custom/PacketName.hpp",
    "header_guard": "RPL_PACKETNAME_HPP"
  },
  "fields": [
    {
      "name": "field_name",
      "type": "uint8_t",
      "comment": "字段描述"
    }
  ]
}
```

### 字段详细说明

#### 顶级字段

| 字段名 | 类型 | 必需 | 描述 | 示例 |
|--------|------|------|------|------|
| `packet_name` | string | ✓ | 生成的C++结构体名称，必须符合C++标识符规范 | `"SensorData"` |
| `command_id` | string | ✓ | 16位命令ID，支持十六进制(0x开头)或十进制格式 | `"0x0104"`, `"260"` |
| `namespace` | string\|null | ✗ | C++命名空间，null表示全局命名空间 | `"Robot::Sensors"`, `null` |
| `packed` | boolean | ✗ | 是否添加`__attribute__((packed))`属性，默认true | `true`, `false` |
| `output` | object | ✓ | 输出文件配置 | 见下表 |
| `fields` | array | ✓ | 结构体字段定义数组，至少包含一个字段 | 见下表 |

#### 输出配置 (`output`)

| 字段名 | 类型 | 必需 | 描述 | 示例 |
|--------|------|------|------|------|
| `header_file` | string | ✓ | 生成的头文件路径（相对于项目根目录） | `"include/RPL/Packets/Custom/SensorData.hpp"` |
| `header_guard` | string | ✗ | 头文件保护宏，默认自动生成 | `"RPL_SENSORDATA_HPP"` |

#### 字段配置 (`fields[]`)

| 字段名 | 类型 | 必需 | 描述 | 示例 |
|--------|------|------|------|------|
| `name` | string | ✓ | 字段名称，必须符合C++标识符规范 | `"temperature"` |
| `type` | string | ✓ | C++数据类型，见支持的类型列表 | `"float"` |
| `comment` | string | ✗ | 字段注释，支持中英文 | `"温度值(摄氏度)"` |

### 支持的数据类型

#### 整数类型
- `uint8_t` - 8位无符号整数 (0-255)
- `int8_t` - 8位有符号整数 (-128到127)
- `uint16_t` - 16位无符号整数 (0-65535)
- `int16_t` - 16位有符号整数 (-32768到32767)
- `uint32_t` - 32位无符号整数
- `int32_t` - 32位有符号整数
- `uint64_t` - 64位无符号整数
- `int64_t` - 64位有符号整数
- `int` - 平台相关的整数类型

#### 浮点类型
- `float` - 32位单精度浮点数
- `double` - 64位双精度浮点数

## 配置示例

### 示例1：传感器数据包

```json
{
  "packet_name": "SensorDataPacket",
  "command_id": "0x0104",
  "namespace": null,
  "packed": true,
  "output": {
    "header_file": "include/RPL/Packets/Custom/SensorDataPacket.hpp",
    "header_guard": "RPL_SENSORDATAPACKET_HPP"
  },
  "fields": [
    {
      "name": "sensor_id",
      "type": "uint8_t",
      "comment": "传感器ID"
    },
    {
      "name": "temperature",
      "type": "float",
      "comment": "温度值(摄氏度)"
    },
    {
      "name": "humidity",
      "type": "float", 
      "comment": "湿度百分比"
    },
    {
      "name": "timestamp",
      "type": "uint64_t",
      "comment": "时间戳(毫秒)"
    }
  ]
}
```

### 示例2：机器人位置包

```json
{
  "packet_name": "RobotPosition",
  "command_id": "0x0201",
  "namespace": "Robot::Navigation",
  "packed": true,
  "output": {
    "header_file": "include/RPL/Packets/Robot/RobotPosition.hpp"
  },
  "fields": [
    {
      "name": "robot_id",
      "type": "uint16_t",
      "comment": "机器人ID"
    },
    {
      "name": "position_x",
      "type": "double",
      "comment": "X坐标(米)"
    },
    {
      "name": "position_y", 
      "type": "double",
      "comment": "Y坐标(米)"
    },
    {
      "name": "rotation",
      "type": "float",
      "comment": "旋转角度(弧度)"
    },
    {
      "name": "speed",
      "type": "float",
      "comment": "速度(m/s)"
    }
  ]
}
```

## 生成的代码结构

使用上述配置会生成如下格式的C++头文件：

```cpp
#ifndef RPL_SENSORDATAPACKET_HPP
#define RPL_SENSORDATAPACKET_HPP

#include <cstdint>
#include <RPL/Meta/PacketTraits.hpp>

struct __attribute__((packed)) SensorDataPacket
{
    uint8_t sensor_id;      // 传感器ID
    float temperature;      // 温度值(摄氏度)
    float humidity;         // 湿度百分比
    uint64_t timestamp;     // 时间戳(毫秒)
};

template <>
struct RPL::Meta::PacketTraits<SensorDataPacket> : PacketTraitsBase<PacketTraits<SensorDataPacket>>
{
    static constexpr uint16_t cmd = 0x0104;
    static constexpr size_t size = sizeof(SensorDataPacket);
};

#endif //RPL_SENSORDATAPACKET_HPP
```

如果指定了namespace，生成的代码会相应包装：

```cpp
namespace Robot::Navigation {

struct __attribute__((packed)) RobotPosition
{
    // ... 字段定义
};

} // namespace Robot::Navigation

template <>
struct RPL::Meta::PacketTraits<Robot::Navigation::RobotPosition> : PacketTraitsBase<PacketTraits<Robot::Navigation::RobotPosition>>
{
    static constexpr uint16_t cmd = 0x0201;
    static constexpr size_t size = sizeof(Robot::Navigation::RobotPosition);
};
```

## 验证规则

### 命令ID验证
- 必须是有效的16位无符号整数（0-65535）
- 十六进制格式：以"0x"或"0X"开头，后跟1-4位十六进制数字
- 十进制格式：1-5位数字
- 建议使用十六进制格式以保持一致性

### 标识符验证
- `packet_name`和`name`必须符合C++标识符规范
- 以字母或下划线开头
- 只包含字母、数字和下划线
- 不能是C++关键字

### 文件路径验证
- `header_file`必须以`.hpp`结尾
- 建议放在`include/RPL/Packets/`目录下
- 路径使用正斜杠分隔符

## 使用rplc工具

```bash
# 基本用法
./rplc generate config.json

# 指定输出目录
./rplc generate config.json --output-dir ./generated/

# 验证配置文件
./rplc validate config.json
```

## 版本信息

- 规范版本：1.0
- 兼容RPL版本：0.1+
- 文档更新日期：2025年