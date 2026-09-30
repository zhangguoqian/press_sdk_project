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

---

## 简介

Press SDK 是一个面向压片机设备的跨平台 C++11 控制库，并附带一个 Qt 示例应用。它主要用于封装设备通信、参数读取/写入、实时状态监控以及控制命令执行，并为上位机软件提供稳定的 API 接口。

项目整体分为四部分：

- Press 库：负责协议处理、数据模型、JSON/帧转换、通信端口抽象以及线程安全 C++ API。
- C 互操作层：通过 `cpress_*` 前缀函数暴露 C ABI，支持 C 应用、DLL 消费方和跨语言绑定。
- 语言绑定层：在 `interface/` 目录下提供 Java、C#、Python、JavaScript、Dart 等多种语言的原生绑定示例。
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
│   │       ├── cpress.h
│   │       ├── press.hpp
│   │       ├── pressinterface.hpp
│   │       ├── presstype.h
│   │       └── typeprivate.h
│   └── src/
│       ├── c/
│       │   └── cpress.cpp
│       ├── jni/
│       │   └── press_jni.cpp
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
│       │   ├── jsondata.cpp
│       │   ├── jsondata.h
│       │   ├── jsontovalue.cpp
│       │   └── jsontovalue.h
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
├── interface/
│   ├── java/
│   ├── csharp/
│   ├── python/
│   ├── javascript/
│   └── dart/
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

### C 语言接口概览

项目还提供了一个 C 语言兼容接口，用于不直接使用 C++ 类的集成场景，例如 C 应用程序、DLL 调用方或跨语言互操作层。

- 公共头文件：`press/include/press/cpress.h`
- 导出的 C 符号统一使用 `cpress_` 前缀。
- C 接口通过不透明句柄 `PressCContext` 包装底层的 C++ `Press` 实现。
- 常见能力包括创建/销毁对象、连接/断开设备、读取/写入参数，以及通过 `PressCDataCallbacks` 注册回调。

示例：

```c
#include "press/cpress.h"

PressCContext* ctx = cpress_create();
if (!ctx) {
    return 1;
}

if (!cpress_connect(ctx, "COM3", 0)) {
    cpress_destroy(ctx);
    return 1;
}

PressData data = {0};
if (cpress_get_press_data(ctx, &data, 0)) {
    // 使用 data
}

cpress_disconnect(ctx);
cpress_destroy(ctx);
```

这种接口适合 C 工具链、脚本语言集成和跨语言绑定场景，同时保持核心实现位于 C++ 层。

### 语言绑定示例

项目还提供了常见运行时环境下的绑定示例，这些封装都基于同一套 `cpress_*` API，代码位于 `interface/` 目录中。

- Java：`interface/java/PressSdk.java`
- C#：`interface/csharp/PressSdk.cs`
- Python：`interface/python/press_sdk.py`
- JavaScript：`interface/javascript/pressSdk.js`
- Dart：`interface/dart/press_sdk.dart`

#### 本地库文件及使用方式

SDK 的原生动态库名称和位置会根据平台不同而变化：

- Windows：`press.dll` 或 `pressd.dll`
- Linux：`libpress.so`
- macOS：`libpress.dylib`

常见使用方式包括：

- 直接复制到程序输出目录或当前工作目录
- 将目录加入 `PATH`、`LD_LIBRARY_PATH` 或 `DYLD_LIBRARY_PATH`
- 设置 `PRESS_SDK_LIB` / `PRESS_SDK_LIB_PATH` 等自定义环境变量
- 在运行时显式指定动态库路径

#### Java

```java
PressSdk sdk = new PressSdk();
System.setProperty("java.library.path", "C:/path/to/native/lib");
if (!sdk.connect("COM3", PressSdk.PortType.SerialPortType)) {
    return;
}
System.out.println(sdk.getLastErrorInfo());
sdk.close();
```

注意：

- 需要把原生库放到 JVM 可发现的位置。
- 可在启动时加 `-Djava.library.path=...`。
- 库与 JVM 的位数必须一致。

#### C#

```csharp
using PressSdk.Interop;

using var sdk = new PressSdk();
if (!sdk.Connect("COM3", PortType.SerialPortType)) {
    return;
}

PressData data;
if (sdk.GetPressData(out data)) {
    Console.WriteLine(data.m_PStep);
}
```

注意：

- 把 `press.dll`/`libpress.so` 放到应用输出目录或设置 `PRESS_SDK_LIB_PATH`。
- Windows 下确保 DLL 所在目录对当前进程可见。
- 托管结构体布局必须与原生 ABI 保持一致。

#### Python

```python
from press_sdk import PressSdk, PortType

sdk = PressSdk()
if sdk.connect("COM3", PortType.SerialPortType):
    data = sdk.get_press_data()
    print(data.m_PStep)
sdk.close()
```

注意：

- 若库未自动查找到，可设置 `PRESS_SDK_LIB` 指向完整路径。
- Linux/macOS 可通过 `LD_LIBRARY_PATH` 或 `DYLD_LIBRARY_PATH` 暴露库路径。
- Python 与库的架构必须匹配。

#### JavaScript

```javascript
const { PressSdk, PortType } = require('./interface/javascript/pressSdk');

const sdk = new PressSdk();
if (sdk.connect('COM3', PortType.SerialPortType)) {
  const data = sdk.getPressData();
  console.log(data && data.m_PStep);
}
sdk.close();
```

注意：

- 需要安装 `ffi-napi`、`ref-*` 等原生依赖。
- 首次调用前确认库路径已经可访问。
- 加载器通常会搜索 `build/press/Debug` 等常见输出目录。

#### Dart

```dart
final sdk = PressSdk();
if (sdk.connect('COM3', portType: PortType.serialPortType)) {
  final data = sdk.getPressData();
  if (data != null) {
    print(data.mPStep);
  }
  sdk.close();
}
```

注意：

- `pubspec.yaml` 中需要声明 `ffi` 依赖。
- 将本地库放到可被加载器识别的位置，或使用显式路径打开。
- 调用完成后应及时关闭 SDK，释放原生句柄。

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
- MachineDataInterface：在主公开头文件中定义的异步回调接口

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
```

---

## 说明

- 本项目是设备控制 SDK，重点在于协议封装和数据交互，不包含底层硬件驱动层之外的完整工厂级控制逻辑。
- 示例 GUI 主要用于参考和验证 SDK 能力，适合作为开发样例，不等同于完整生产界面。
- 在实际部署前，建议根据目标设备的固件协议、通信参数和现场要求进行端到端验证。
- 若需要扩展到新的设备型号，通常应先确认协议帧格式、命令字和状态字段，再补充相应的数据结构与解析逻辑。

---

## 跨平台编译说明

本项目默认不编译示例程序，仅构建 SDK 共享库和安装产物；如需启用 Qt 示例，可额外设置 `-DBUILD_QT_EXAMPLE=ON`。

### Windows (MSVC / MinGW)

```bash
# MSVC x64
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build

# MinGW
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

### Linux

```bash
sudo apt install build-essential cmake
cmake -B build
cmake --build build
```

### macOS

```bash
brew install cmake
cmake -B build
cmake --build build
```

### Android

```bash
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
      -DANDROID_ABI=arm64-v8a
cmake --build build
```

### 可选：启用示例程序

```bash
cmake -B build -DBUILD_QT_EXAMPLE=ON
cmake --build build
```

---

## 许可证

本项目基于 [MIT 协议](LICENSE) 开源。

---

## 作者

Created by 11518 on 2024/7/29.