# ZTable Press Bot

> 压片机上位机控制库与 Qt 示例应用

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.27+-brightgreen.svg)](https://cmake.org/)
[![Qt](https://img.shields.io/badge/Qt-5%20|%206-green.svg)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[English](README.md) | **中文**

---

## 简介

**ZTable Press Bot** 是一个用于控制压片机（粉末成型液压机）的跨平台上位机项目。项目包含两部分：

- **press 库**：核心 C++ 动态库，封装了与压片机的通信协议，提供线程安全的同步/异步 API
- **Qt 示例应用**：基于 Qt5/Qt6 的图形界面上位机软件，演示如何使用 press 库进行压力参数配置、实时状态监控和压制循环控制

### 核心特性

| 特性 | 说明 |
|------|------|
| 🔌 双通道通信 | 支持 **串口**（COM / /dev/tty*）和 **TCP 套接字** 两种连接方式 |
| 🧵 线程安全 | 所有公共方法均为线程安全，内部调度线程独立运行 |
| 📡 异步回调 | 实现 `MachineDataInterface` 即可接收实时数据推送 |
| 📊 多步压制曲线 | 支持配置多步压力、保压值和保压时间 |
| 🔄 JSON 压缩传输 | 可选 JSON 压缩格式，减少通信数据量 |
| 💻 跨平台 | Windows / Linux / macOS / Android |
| 🛠 CMake 构建 | 现代 CMake 项目，支持自动导出 install targets |

---

## 项目结构

```
ztable_press_bot_project/
├── CMakeLists.txt              # 根 CMake 构建文件
├── press/                      # 核心控制库
│   ├── CMakeLists.txt
│   ├── include/press/          # 公共头文件
│   │   ├── machine.h           # Machine 主控制器
│   │   ├── machinetype.h       # 数据结构与枚举定义
│   │   └── typeprivate.h       # 内部常量
│   └── src/                    # 库源码
│       ├── machine.cpp          # Machine 实现
│       ├── machinedata.cpp/h   # 数据编解码
│       ├── machineprivate.cpp/h # Pimpl 私有实现
│       ├── port/               # 通信端口抽象
│       │   ├── portbase.h
│       │   ├── serialport.cpp/h
│       │   └── tcpsocket.cpp/h
│       ├── json/               # JSON 解析器
│       ├── common/             # 公共工具
│       └── tool/               # 工具函数
├── example/Qt/                 # Qt GUI 示例应用
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── ui/                     # 界面模块
│   │   ├── login.*             # 登录界面
│   │   ├── uihome.*            # 主界面
│   │   ├── admin/              # 管理员控制面板
│   │   ├── chartview/          # 实时压力图表
│   │   ├── steplist/           # 压制步骤表格
│   │   └── widget/             # 自定义控件
│   └── ztable_press_bot.*      # 应用图标与资源
├── setup/                      # 安装包相关
└── LICENSE                     # MIT 协议
```

---

## 快速开始

### 环境要求

- **CMake 3.27+**
- **C++17** 兼容编译器（MSVC 2019+、GCC 8+、Clang 8+）
- **Qt 5.15+ 或 Qt 6.x**（仅构建 GUI 示例时需要）

### 构建步骤

```bash
# 1. 克隆项目
git clone <repository-url>
cd ztable_press_bot_project

# 2. 创建构建目录
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. 构建
cmake --build build --config Release

# 4. 运行 Qt 示例（Windows）
./build/example/Qt/press_bot_qtgui.exe
```

仅构建 press 库（不包含 GUI）：

```bash
cmake -B build -DBUILD_QT_EXAMPLE=OFF
cmake --build build
```

### 在其他项目中使用 press 库

#### 方式一：通过 find_package（推荐）

```cmake
find_package(press REQUIRED)
target_link_libraries(your_target PRIVATE press::press)
```

#### 方式二：直接子目录

```cmake
add_subdirectory(press)
target_link_libraries(your_target PRIVATE press::press)
```

#### 方式三：手动安装后使用

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=./install
cmake --build build
cmake --install build
```

---

## API 概览

### 连接管理

```cpp
#include "press/machine.h"

Machine machine;

// 串口连接
machine.connect("COM3", SerialPortType);

// TCP 连接
machine.connect("192.168.0.100:8010", TcpSocketPortType);

machine.disconnect();
```

### 运行控制

```cpp
machine.run();       // 启动压制循环
machine.stop();      // 停止运行
machine.isRunning(); // 查询运行状态
```

### 读取/写入压力参数

```cpp
PressData pd;
if (machine.getPressData(pd)) {
    pd.m_PStep = 3;                             // 设置步数
    pd.m_SetPValue = {10.0f, 20.0f, 30.0f};     // 每步压力
    pd.m_AfterValue = {5.0f, 10.0f, 15.0f};     // 每步保压
    pd.m_KPTime = {5000, 5000, 5000};            // 每步保压时间(ms)
    machine.setPressData(pd);
}
```

### 读取实时状态

```cpp
RealTimeData rt;
if (machine.getRealTimeData(rt)) {
    // rt.m_PressState : 0空闲, 1加压中, 2脱模中
    // rt.m_PressValue  : 实时压力值
    // rt.m_CPStep      : 当前执行步骤
    // rt.m_PTime       : 剩余倒计时(ms)
}
```

### 异步回调

```cpp
class MyHandler : public MachineDataInterface {
public:
    void onRealTimeData(int errorCode, const RealTimeData& rt) override {
        // 实时数据更新回调
    }
    void onPressData(int errorCode, const PressData& pd) override {
        // 压力参数更新回调
    }
    void onReadOnlyData(int errorCode, uint64_t registerNo,
                        const ReadOnlyData& rod) override {
        // 只读数据更新回调
    }
    void onError(uint16_t cmdCode, std::vector<uint8_t> response) override {
        // 错误回调
    }
};

MyHandler handler;
machine.registerDataInterface(&handler);
```

### 枚举可用端口

```cpp
auto ports = Machine::getPortList();
for (auto& p : ports) {
    // Windows: "COM1", "COM3", ...
    // Linux:   "/dev/ttyUSB0", ...
}
```

---

## 数据结构

| 结构体 | 说明 |
|--------|------|
| `ReadOnlyData` | 设备静态信息：型号、序列号、压力/温度限制、油缸参数等 |
| `PressData` | 压力控制参数：压制类型、模具参数、多步压力/保压/时间序列 |
| `RealTimeData` | 实时状态：工作模式、加压状态、当前步骤、实时压力、倒计时 |

### PressData 多步压制曲线示意

```
压力
  ^
  |     ┌─ Step 3 ─────┐
  |    /  KP=5s          \
  |   /                   \
  |  / Step 2 ────┐       \
  | /  KP=5s        \       \
  |/                  \──┐   \
  | Step 1 ─┐             \   \
  | KP=5s    \ After=5     \---
  +----------------------------------> 时间
```

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