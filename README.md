# Press SDK

> Press machine host control library and Qt reference application

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.27+-brightgreen.svg)](https://cmake.org/)
[![Qt](https://img.shields.io/badge/Qt-5%20|%206-green.svg)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**English** | [中文](README.zh-CN.md)

---

## Project Progress

- [X] Add SDK support for press machines
- [ ] Add SDK support for automatic hot press machines
- [X] Complete API documentation for the press library
- [ ] Improve the Qt example application UI
- [X] Support additional communication protocols (e.g. TCP sockets)
- [ ] Optimize performance and stability

---

## Overview

Press SDK is a cross-platform C++17 host-control library for press machines and a Qt example application. It provides the communication layer, parameter access, real-time status monitoring, and control commands needed by upper-layer software.

The project is organized into four main parts:

- Press library: protocol handling, data models, JSON and frame conversion, communication abstraction, and a thread-safe C++ API.
- C interop layer: a public C ABI exposed through `cpress_*` functions for C, scripting, and cross-language integrations.
- Language bindings: native binding examples for Java, C#, Python, JavaScript, and Dart under `interface/`.
- Qt example application: a reference UI showing how to connect to a device, read and write pressure settings, and monitor current state.

The current SDK focuses on serial and TCP communication for press-machine protocols and keeps the abstraction extensible for other device families.

### Features

- Serial port and TCP socket communication
- C++17 public API with cross-platform CMake support
- Synchronous access plus asynchronous callback notifications
- Multi-step pressure curve support with hold-pressure and timing values
- JSON and raw protocol frame conversion helpers
- Qt reference UI example for rapid integration

---

## Project Structure

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
│   │       ├── presstype.h
│   │       └── typeprivate.h
│   └── src/
│       ├── c/
│       │   └── cpress.cpp
│       ├── jni/
│       │   └── press_jni.cpp
│       ├── common/
│       │   ├── unity.cpp
│       │   └── unity.h
│       ├── json/
│       │   ├── json_reader.cpp
│       │   ├── json_value.cpp
│       │   ├── json_writer.cpp
│       │   └── ...
│       ├── press.cpp
│       ├── pressprivate.cpp
│       ├── pressprivate.h
│       ├── port/
│       │   ├── portbase.h
│       │   ├── serialport.cpp
│       │   ├── serialport.h
│       │   ├── tcpsocket.cpp
│       │   └── tcpsocket.h
│       └── tool/
│           ├── jsondata.cpp
│           ├── jsondata.h
│           ├── jsontovalue.cpp
│           ├── jsontovalue.h
│           └── ...
├── example/
│   └── Qt/
│       ├── CMakeLists.txt
│       ├── main.cpp
│       └── ui/
├── interface/
│   ├── java/
│   │   └── PressSdk.java
│   ├── csharp/
│   │   └── PressSdk.cs
│   ├── python/
│   │   └── press_sdk.py
│   ├── javascript/
│   │   └── pressSdk.js
│   └── dart/
│       └── press_sdk.dart
├── build/
└── setup/
    └── table_press_bot.iss
```

Notes:

- The headers under `press/include/press` are the public SDK API.
- The implementation details and protocol logic live under `press/src`.
- The `example/Qt` directory demonstrates how to integrate the library into an application.

---

## Getting Started

### Prerequisites

- CMake 3.27+
- A C++17-compatible compiler such as MSVC, GCC, or Clang
- Qt 5 or Qt 6 when building the GUI example

### C API overview

The project also provides a C-compatible interface for integration scenarios where the C++ class cannot be used directly, such as C applications, DLL consumers, or language interop layers.

- Public header: `press/include/press/cpress.h`
- The exported C symbols all use the `cpress_` prefix.
- The C API wraps the underlying C++ `Press` implementation behind an opaque `PressCContext` handle.
- Common operations include create/destroy, connect/disconnect, read/write data, and optional callback registration through `PressCDataCallbacks`.
- The Java JNI bridge uses native names such as `press_create` and `press_connect` to match the Java-side naming style, but it still calls the same underlying `cpress_*` implementation.

Example:

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
    // use data here
}

cpress_disconnect(ctx);
cpress_destroy(ctx);
```

This interface is useful for C toolchains, scripting integrations, and cross-language bindings while keeping the main implementation in C++.

### Language bindings

The project also includes native binding examples for common runtime environments. These wrappers target the same platform ABI and are organized under `interface/`.

- Java: `interface/java/PressSdk.java` (JNI bridge over the shared library)
- C#: `interface/csharp/PressSdk.cs`
- Python: `interface/python/press_sdk.py`
- JavaScript: `interface/javascript/pressSdk.js`
- Dart: `interface/dart/press_sdk.dart`

The Java binding uses JNI symbol names such as `Java_PressSdk_press_create` and `Java_PressSdk_press_connect`, while the underlying native implementation is still backed by `cpress_*` functions.

#### Native library files and usage

The native SDK is built as a shared library. Its name and location vary by OS:

- Windows: `press.dll` or `pressd.dll`
- Linux: `libpress.so`
- macOS: `libpress.dylib`

Typical ways to use the library are:

- copy it beside the executable or application output folder
- set the OS library search path such as `PATH`, `LD_LIBRARY_PATH`, or `DYLD_LIBRARY_PATH`
- set a custom environment variable such as `PRESS_SDK_LIB` or `PRESS_SDK_LIB_PATH`
- pass the explicit library path from the runtime loader

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

Important:

- Place the native library in a folder visible to the JVM.
- Use `-Djava.library.path=...` when launching the Java process.
- The architecture must match the JVM bitness.

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

Important:

- Copy `press.dll`/`libpress.so` into the app output folder, or set `PRESS_SDK_LIB_PATH`.
- On Windows, ensure the DLL directory is discoverable by the process.
- Keep the C# struct layout aligned with the native ABI.

#### Python

```python
from press_sdk import PressSdk, PortType

sdk = PressSdk()
if sdk.connect("COM3", PortType.SerialPortType):
    data = sdk.get_press_data()
    print(data.m_PStep)
sdk.close()
```

Important:

- Set `PRESS_SDK_LIB` to the full path of the native library if it is not auto-detected.
- On Linux/macOS, make the library visible to `ctypes` by path or `LD_LIBRARY_PATH`/`DYLD_LIBRARY_PATH`.
- The Python runtime and library must match the same architecture.

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

Important:

- Install native dependencies such as `ffi-napi` and `ref-*`.
- Ensure the library path is set before first use.
- The loader may search common build folders such as `build/press/Debug`.

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

Important:

- Add `ffi` to `pubspec.yaml`.
- Place the native library in a loader-visible directory or use `DynamicLibrary.open` with an explicit path.
- Always close the SDK after use to release the native handle.

---

## Quick API Example

```cpp
#include "press/press.hpp"

Press press;

if (!press.connect("COM3", SerialPortType)) {
    return 1;
}

ReadOnlyData rod;
if (press.getReadOnlyData(rod)) {
    // use rod fields here
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

### Register a callback handler

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

## Public API Summary

### Main types

- Press: high-level controller
- ReadOnlyData: static device identity and parameter limits
- PressData: multi-step pressure control configuration
- RealTimeData: live machine operating state
- MachineDataInterface: asynchronous callback interface defined in the main public header

### Main functions

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

Typical includes:

```cpp
#include "press/press.hpp"
#include "press/presstype.h"
```

---

## Notes

- This project is a device-control SDK focused on protocol encapsulation and data exchange rather than complete end-to-end factory automation.
- The example GUI is intended as a reference and validation sample, not a complete production UI.
- Before production deployment, validate the protocol implementation against the actual firmware and communication requirements of the target machine.
- For new device variants, confirm the command frame format, command IDs, and state fields before extending the SDK.

---

## Cross-platform build notes

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

## License

This project is open source under the [MIT License](LICENSE).

---

## Author

Created by 11518 on 2024/7/29.