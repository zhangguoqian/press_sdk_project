# Press SDK

> 压片机控制库与 Qt 示例应用

[![C++11](https://img.shields.io/badge/C%2B%2B-11-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.27+-brightgreen.svg)](https://cmake.org/)
[![Qt](https://img.shields.io/badge/Qt-5%20|%206-green.svg)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[English](README.md) | **中文**

---
## 项目进度

- [X] 压片机设备添加SDK支持
- [ ] 自动热压机设备添加SDK支持
- [X] 完善 press 库的 API 文档
- [ ] 完善 Qt 示例应用的用户界面
- [X] 支持更多通信协议（如 TCP 套接字）
- [ ] 优化性能和稳定性



## 简介

Press SDK 是一个面向压片机设备的跨平台 C++11 控制库，并附带一个 Qt 示例应用。它主要用于封装设备通信、参数读取/写入、实时状态监控以及控制命令执行，并为上位机软件提供稳定的 API 接口。

项目整体分为两部分：

- Press 库：负责协议处理、数据模型、JSON/帧转换、通信端口抽象以及线程安全 API。
- Qt 示例程序：演示如何连接压片机、读取压力参数、查看实时状态、发送运行控制命令，并验证 SDK 的使用方式。

当前 SDK 以串口通信和 TCP 连接两种方式为主，适用于压片机类设备的控制场景，并保留了继续扩展到其他设备类型的接口设计。

### 主要特性

- 支持串口和 TCP 套接字通信
- 使用 C++11 标准接口，兼容主流编译器和跨平台构建
- 提供同步访问与异步数据回调机制
- 支持多步压力曲线配置，包括保压值和保压时间
- 提供 JSON 与原始协议帧的相互转换辅助函数
- 提供 Qt 参考界面示例，便于快速集成和调试

---

## 项目结构

```text
press_sdk_project/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── README.zh-CN.md
├── press/
│   ├── CMakeLists.txt
│   ├── include/
│   │   └── press/
│   │       ├── press.hpp
│   │       ├── pressinterface.hpp
│   │       ├── presstype.h
│   │       └── typeprivate.h
│   └── src/
│       ├── press.cpp
│       ├── pressprivate.cpp
│       ├── pressprivate.h
│       ├── common/
│       │   ├── unity.cpp
│       │   └── unity.h
│       ├── port/
│       │   ├── portbase.h
│       │   ├── serialport.cpp
│       │   ├── serialport.h
│       │   ├── tcpsocket.cpp
│       │   └── tcpsocket.h
│       ├── tool/
│       │   ├── jsontovalue.cpp
│       │   ├── jsontovalue.h
│       │   ├── machinedata.cpp
│       │   └── machinedata.h
│       └── json/
│           ├── json_reader.cpp
│           ├── json_value.cpp
│           ├── json_writer.cpp
│           └── ...
├── example/
│   └── Qt/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── ui/
└── build/
```

说明：

- `press/include/press` 目录中的头文件是 SDK 对外公开 API。
- `press/src` 目录中包含协议实现、端口抽象和私有逻辑。
- `example/Qt` 中的代码用于展示如何在真实应用中接入该 SDK。

---

## 环境要求

- CMake 3.27+
- 支持 C++11 的编译器（MSVC、GCC、Clang 等）
- 若构建 Qt 示例，需要 Qt 5 或 Qt 6
- Windows 下可使用 MSVC 或 MinGW；Linux/macOS 可使用系统自带工具链

---

## 构建

### 构建完整项目

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

### 仅构建 SDK 库

```bash
cmake -B build -DBUILD_QT_EXAMPLE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 在 Windows 上运行示例程序

```powershell
./build/example/Qt/Debug/press_bot_qtgui.exe
```

> 若使用 Visual Studio 生成器，可按如下方式配置：
>
> ```bash
> cmake -B build -G "Visual Studio 17 2022" -A x64
> ```

---

## 快速使用示例

```cpp
#include "press/press.hpp"

Press press;

if (!press.connect("COM3", SerialPortType)) {
    return 1;
}

ReadOnlyData rod;
if (press.getReadOnlyData(rod)) {
    // 使用 rod 中的字段
}

PressData pd;
if (press.getPressData(pd)) {
    pd.m_PStep = 3;
    pd.m_SetPValue[0] = 10.0f;
    pd.m_SetPValue[1] = 20.0f;
    pd.m_SetPValue[2] = 30.0f;
    pd.m_AfterValue[0] = 5.0f;
    pd.m_AfterValue[1] = 10.0f;
    pd.m_AfterValue[2] = 15.0f;
    pd.m_KPTime[0] = 5000;
    pd.m_KPTime[1] = 5000;
    pd.m_KPTime[2] = 5000;
    press.setPressData(pd);
}

RealTimeData rt;
if (press.getRealTimeData(rt)) {
    // rt.m_PressValue, rt.m_PressState, rt.m_CPStep, rt.m_PTime
}

press.disconnect();
```

### 注册回调处理器

```cpp
class MyHandler : public MachineDataInterface {
public:
    void onReadOnlyData(int errorCode, uint64_t registerNo,
                        const ReadOnlyData& readOnlyData) override {
    }

    void onRealTimeData(int errorCode, const RealTimeData& realTimeData) override {
    }

    void onPressData(int errorCode, const PressData& pressData) override {
    }

    void onError(uint16_t cmdCode, std::vector<uint8_t> response) override {
    }
};

MyHandler handler;
press.registerDataInterface(&handler);
```

---

## 公开 API 概览

### 主要类型

- Press：高层控制器
- ReadOnlyData：设备静态身份和参数限制
- PressData：多步压力控制配置
- RealTimeData：设备实时状态
- MachineDataInterface：异步回调接口

### 主要函数

- Press::connect
- Press::disconnect
- Press::getReadOnlyData
- Press::getPressData
- Press::setPressData
- Press::getRealTimeData
- Press::setPressing
- Press::setDemolding
- Press::run
- Press::stop
- toJsonString
- toFrameData

上述函数和类型是在公开头文件中定义的，通常直接包含以下头文件即可：

```cpp
#include "press/press.hpp"
#include "press/presstype.h"
#include "press/pressinterface.hpp"
```

---

## 说明

- 本项目是设备控制 SDK，重点在于协议封装和数据交互，不包含底层硬件驱动层之外的完整工厂级控制逻辑。
- 示例 GUI 主要用于参考和验证 SDK 能力，适合作为开发样例，不等同于完整生产界面。
- 在实际部署前，建议根据目标设备的固件协议、通信参数和现场要求进行端到端验证。
- 若需要扩展到新的设备型号，通常应先确认协议帧格式、命令字和状态字段，再补充相应的数据结构与解析逻辑。

---

## 跨平台编译说明

### Windows (MSVC / MinGW)

```bash
# MSVC x64
cmake -B build -G "Visual Studio 17 2022" -A x64

# MinGW
cmake -B build -G "MinGW Makefiles"
```

### Linux

```bash
sudo apt install build-essential cmake qt5-default
cmake -B build -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.0/gcc_64
cmake --build build
```

### macOS

```bash
brew install cmake qt
cmake -B build -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build build
```

### Android

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a \
      -DCMAKE_PREFIX_PATH=/opt/Qt/6.8.0/android_arm64_v8a
cmake --build build
```

---

## 许可证

本项目基于 [MIT 协议](LICENSE) 开源。

---

## 作者

Created by 11518 on 2024/7/29.