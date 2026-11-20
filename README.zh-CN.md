# Press SDK

> 压片机控制库与 Qt 示例应用

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
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

Press SDK 是一个面向压片机设备的跨平台 C++17 控制库，并附带一个 Qt 示例应用。它主要用于封装设备通信、参数读取/写入、实时状态监控以及控制命令执行，并为上位机软件提供稳定的 API 接口。

项目整体分为四部分：

- Press 库：负责协议处理、数据模型、JSON/帧转换、通信端口抽象以及线程安全 C++ API。
- C 互操作层：通过 `cpress_*` 前缀函数暴露 C ABI，支持 C 应用、DLL 消费方和跨语言绑定。
- 语言绑定层：在 `interface/` 目录下提供 Java、C#、Python、JavaScript、Dart 等多种语言的原生绑定示例。
- Qt 示例程序：演示如何连接压片机、读取压力参数、查看实时状态、发送运行控制命令，并验证 SDK 的使用方式。

当前 SDK 以串口通信和 TCP 连接两种方式为主，适用于压片机类设备的控制场景，并保留了继续扩展到其他设备类型的接口设计。

### 主要特性

- 支持串口和 TCP 套接字通信
- 使用 C++17 标准接口，兼容主流编译器和跨平台构建
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
│   │       ├── press.h
│   │       ├── press.hpp
│   │       └── presstype.h
│   └── src/
│       ├── c/
│       │   └── cpress.cpp
│       ├── jni/
│       │   └── press_jni.cpp
│       ├── press.cpp
│       ├── pressprivate.cpp
│       ├── pressprivate.h
│       ├── typeprivate.h
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
│   └── press_desktop_qt/
│       ├── CMakeLists.txt
│       ├── src/
│       └── tests/
├── test/
│   └── cpp/
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
- `example/press_desktop_qt` 为 Qt 桌面应用示例（含 JSON 解析单元测试）；`test/cpp` 为 SDK C++ 接口测试。

---

## 环境要求

- CMake 3.27+
- 支持 C++17 的编译器（MSVC、GCC、Clang 等）
- 若构建 Qt 示例，需要 Qt 5.12+（当前示例验证于 Qt 5.15.2 / mingw81_32）
- Windows 下可使用 MSVC 或 MinGW；Linux/macOS 可使用系统自带工具链

---

### C 语言接口概览

项目还提供了一个 C 语言兼容接口，用于不直接使用 C++ 类的集成场景，例如 C 应用程序、DLL 调用方或跨语言互操作层。

- 公共头文件：`press/include/press/press.h`
- 导出的 C 符号统一使用 `cpress_` 前缀。
- C 接口通过不透明句柄 `PressCContext` 包装底层的 C++ `Press` 实现。
- 常见能力包括创建/销毁对象、连接/断开设备、读取/写入参数、读取缓存固件版本（`cpress_get_device_version_info` / `CPressVersionInfo`），以及通过 `PressCDataCallbacks` 注册回调。

示例：

```c
#include "press/press.h"

PressCContext* ctx = cpress_create();
if (!ctx) {
    return 1;
}

if (!cpress_connect(ctx, "COM3", 0)) {
    cpress_destroy(ctx);
    return 1;
}

PressData data = {0};
if (cpress_get_press_data_json(ctx, &data, 0)) {
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
PressSdk.DeviceVersionInfo v = sdk.getDeviceVersionInfo(); // 未连接返回 null
if (v != null) {
    System.out.println(v.versionString()); // "V<major>.<minor>.<patch>"
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

DeviceVersionInfo ver;
if (sdk.GetDeviceVersionInfo(out ver)) {
    Console.WriteLine($"V{ver.major}.{ver.minor}.{ver.patch}");
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
    v = sdk.get_device_version_info()  # 未连接时抛 RuntimeError
    print(f"V{v.major}.{v.minor}.{v.patch}")
    print(sdk.get_press_data_json())   # 原始 PD JSON 字符串
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
  const v = sdk.getDeviceVersionInfo(); // 未连接返回 null
  if (v) console.log(`V${v.major}.${v.minor}.${v.patch}`);
  const data = JSON.parse(sdk.getPressDataJson());
  console.log(data.PStep);
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
  final v = sdk.getDeviceVersionInfo(); // 未连接返回 null
  if (v != null) print('V${v.major}.${v.minor}.${v.patch}');
  print(sdk.isConnected());
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

// 读取连接握手时缓存的固件版本，不发起新请求；未连接时返回 false。
VersionInfo ver{};
if (press.getDeviceVersionInfo(ver)) {
    // ver.m_AppNo / ver.m_Major / ver.m_Minor / ver.m_Patch
}

std::string rodJson;
if (press.getReadOnlyDataJson(rodJson)) {
    // rodJson 为原始 ROD JSON
}

std::string pdJson;
if (press.getPressDataJson(pdJson)) {
    // 修改 pdJson 后整包写回
    press.setPressDataJson(pdJson);
}

std::string rtJson;
if (press.getRealTimeDataJson(rtJson)) {
    // rtJson 为原始 RT JSON
}

press.disconnect();
```

### 注册回调处理器

```cpp
class MyHandler : public PressDataInterface {
public:
    void onReadOnlyData(int errorCode, uint64_t registerNo,
                        const std::string& json) override {
    }

    void onRealTimeData(int errorCode, const std::string& json) override {
    }

    void onPressData(int errorCode, const std::string& json) override {
    }

    void onTempData(int errorCode, const std::string& json) override {
    }

    void onMixData(int errorCode, const std::string& json) override {
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
- VersionInfo：连接握手时缓存的设备固件版本（应用类型 + 主.次.修订），connect 成功后填充
- PressDataInterface：在主公开头文件中定义的异步回调接口

### 主要函数

- Press::connect
- Press::disconnect
- Press::getReadOnlyDataJson
- Press::getPressDataJson
- Press::setPressDataJson
- Press::getRealTimeDataJson
- Press::getDeviceVersionInfo
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

Qt 示例与 C++ 测试分别由 `BUILD_QT_EXAMPLE`、`BUILD_CPP_TEST` 控制（默认 ON）。Qt 路径不再写死在 CMake 中，按 `-DCMAKE_PREFIX_PATH` / 环境变量 / PATH 中的 qmake 自动确定。

### Windows (MinGW + Qt)

```powershell
$env:Path = "C:\Qt\5.15.2\mingw81_32\bin;C:\Qt\Tools\mingw810_32\bin;" + $env:Path
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/mingw81_32
cmake --build build -- -j 8
ctest --test-dir build -R press_json_test --output-on-failure
```

### 运行参数配置

Qt 示例首次启动会在 `%APPDATA%\AUTO HOT PRESS\自动热压机 · 上位机\config.ini` 写入默认值，修改后重启生效（越界值会被自动收敛到合法范围）：

| 配置项 | 默认值 | 说明 |
| --- | --- | --- |
| `runtime/tickIntervalMs` | 500 | 界面节拍 / 本地模拟步长 |
| `runtime/watchdogIntervalMs` | 1000 | 通讯看门狗检查周期 |
| `runtime/staleTimeoutMs` | 3000 | 超过该时长未收到 RT 判为中断 |
| `runtime/reconnectTriggerMs` | 10000 | 中断持续该时长后触发自动重连 |
| `runtime/commandTimeoutMs` | 3000 | 控制指令等待确认的超时 |
| `runtime/writeDebounceMs` | 400 | 参数写入去抖 |
| `logs/maxEntries` | 500 | 界面日志最大条数 |
| `logs/mergeWindowMs` | 5000 | 重复日志合并窗口 |
| `logs/maxFileBytes` | 5242880 | 单个日志文件上限（字节） |
| `logs/retentionDays` | 7 | 日志保留天数 |

### Windows (MSVC)

```bash
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
cmake --build build --config Release
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