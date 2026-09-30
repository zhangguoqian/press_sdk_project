# Press Sdk

> Press machine host control library and Qt example application

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

Press SDK is a cross-platform C++11 host-control library for press machines and a Qt example application. It provides the communication layer, parameter access, real-time status monitoring, and control commands needed by upper-layer software.

The project is organized into two main parts:

- Press library: protocol handling, data models, JSON and frame conversion, communication abstraction, and a thread-safe API.
- Qt example application: a reference UI showing how to connect to a device, read and write pressure settings, and monitor the running state.

The current SDK focuses on serial and TCP communication for press-machine protocols and keeps the abstraction extensible for other device families.

### Features

- Serial port and TCP socket communication
- C++11 public API with cross-platform CMake support
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

Notes:

- The headers under `press/include/press` are the public SDK API.
- The implementation details and protocol logic live under `press/src`.
- The `example/Qt` directory demonstrates how to integrate the library into an application.

---

## Getting Started

### Prerequisites

- CMake 3.27+
- A C++11-compatible compiler such as MSVC, GCC, or Clang
- Qt 5 or Qt 6 when building the GUI example

### Build Steps

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --config Debug
```

Build only the library without the Qt example:

```bash
cmake -B build -DBUILD_QT_EXAMPLE=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run the Qt example on Windows:

```powershell
./build/example/Qt/Debug/press_bot_qtgui.exe
```

### Project generator example

```bash
# MSVC x64
cmake -B build -G "Visual Studio 17 2022" -A x64
```

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
- MachineDataInterface: asynchronous callback interface

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
#include "press/pressinterface.hpp"
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

---

## Cross-Platform Build Notes

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

This project is licensed under the [MIT License](LICENSE).

---

## Author

Created by 11518 on 2024/7/29.