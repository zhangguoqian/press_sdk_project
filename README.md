# ZTable Press Bot

> Press machine host control library and Qt example application

[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard)
[![CMake](https://img.shields.io/badge/CMake-3.27+-brightgreen.svg)](https://cmake.org/)
[![Qt](https://img.shields.io/badge/Qt-5%20|%206-green.svg)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

**English** | [中文](README.zh-CN.md)

---

## Overview

**ZTable Press Bot** is a cross-platform host-control project for press machines (powder-forming hydraulic presses). The project consists of two parts:

- **press library** — A core C++ shared library that encapsulates the machine communication protocol and provides a thread-safe synchronous / asynchronous API
- **Qt example application** — A Qt5/Qt6-based graphical host-control software demonstrating pressure parameter configuration, real-time state monitoring, and press cycle control using the press library

### Features

| Feature | Description |
|---------|-------------|
| 🔌 Dual-channel communication | Supports both **serial port** (COM / /dev/tty*) and **TCP socket** connections |
| 🧵 Thread-safe | All public methods are thread-safe; an internal dispatch thread runs independently |
| 📡 Async callbacks | Implement `MachineDataInterface` to receive push-style real-time data updates |
| 📊 Multi-step press curve | Configurable multi-step pressure, hold-pressure, and hold-pressure time |
| 🔄 Optional JSON compression | JSON-compressed payload supported to reduce communication overhead |
| 💻 Cross-platform | Windows / Linux / macOS / Android |
| 🛠 CMake build | Modern CMake project with automatic install target export |

---

## Project Structure

```
ztable_press_bot_project/
├── CMakeLists.txt              # Root CMake build file
├── press/                      # Core control library
│   ├── CMakeLists.txt
│   ├── include/press/          # Public headers
│   │   ├── machine.h           # Machine main controller
│   │   ├── machinetype.h       # Data structures & enums
│   │   └── typeprivate.h       # Internal constants
│   └── src/                    # Library sources
│       ├── machine.cpp          # Machine implementation
│       ├── machinedata.cpp/h   # Data encoding / decoding
│       ├── machineprivate.cpp/h # Pimpl private implementation
│       ├── port/               # Communication port abstraction
│       │   ├── portbase.h
│       │   ├── serialport.cpp/h
│       │   └── tcpsocket.cpp/h
│       ├── json/               # JSON parser
│       ├── common/             # Common utilities
│       └── tool/               # Helper tools
├── example/Qt/                 # Qt GUI example application
│   ├── CMakeLists.txt
│   ├── main.cpp
│   ├── ui/                     # UI modules
│   │   ├── login.*             # Login dialog
│   │   ├── uihome.*            # Main window
│   │   ├── admin/              # Admin control panel
│   │   ├── chartview/          # Real-time pressure chart
│   │   ├── steplist/           # Press step table
│   │   └── widget/             # Custom widgets
│   └── ztable_press_bot.*      # Application icon & resources
├── setup/                      # Installer related
└── LICENSE                     # MIT License
```

---

## Getting Started

### Prerequisites

- **CMake 3.27+**
- **C++17** compatible compiler (MSVC 2019+, GCC 8+, Clang 8+)
- **Qt 5.15+ or Qt 6.x** (only required when building the GUI example)

### Build Steps

```bash
# 1. Clone the repository
git clone <repository-url>
cd ztable_press_bot_project

# 2. Create build directory
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 3. Build
cmake --build build --config Release

# 4. Run the Qt example (Windows)
./build/example/Qt/press_bot_qtgui.exe
```

Build only the press library (without the GUI):

```bash
cmake -B build -DBUILD_QT_EXAMPLE=OFF
cmake --build build
```

### Using the press Library in Your Own Project

#### Option 1: via find_package (recommended)

```cmake
find_package(press REQUIRED)
target_link_libraries(your_target PRIVATE press::press)
```

#### Option 2: as a subdirectory

```cmake
add_subdirectory(press)
target_link_libraries(your_target PRIVATE press::press)
```

#### Option 3: install first, then use

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=./install
cmake --build build
cmake --install build
```

---

## API Overview

### Connection Management

```cpp
#include "press/machine.h"

Machine machine;

// Serial port connection
machine.connect("COM3", SerialPortType);

// TCP connection
machine.connect("192.168.0.100:8010", TcpSocketPortType);

machine.disconnect();
```

### Run Control

```cpp
machine.run();       // Start the press cycle
machine.stop();      // Stop running
machine.isRunning(); // Query running state
```

### Read / Write Pressure Parameters

```cpp
PressData pd;
if (machine.getPressData(pd)) {
    pd.m_PStep = 3;                             // Number of steps
    pd.m_SetPValue = {10.0f, 20.0f, 30.0f};     // Pressure per step
    pd.m_AfterValue = {5.0f, 10.0f, 15.0f};     // Hold-pressure per step
    pd.m_KPTime = {5000, 5000, 5000};            // Hold-pressure time per step (ms)
    machine.setPressData(pd);
}
```

### Read Real-Time State

```cpp
RealTimeData rt;
if (machine.getRealTimeData(rt)) {
    // rt.m_PressState : 0 idle, 1 pressing, 2 demolding
    // rt.m_PressValue  : live pressure reading
    // rt.m_CPStep      : current active step index
    // rt.m_PTime       : countdown remaining (ms)
}
```

### Asynchronous Callbacks

```cpp
class MyHandler : public MachineDataInterface {
public:
    void onRealTimeData(int errorCode, const RealTimeData& rt) override {
        // Real-time data update callback
    }
    void onPressData(int errorCode, const PressData& pd) override {
        // Pressure parameter update callback
    }
    void onReadOnlyData(int errorCode, uint64_t registerNo,
                        const ReadOnlyData& rod) override {
        // Read-only data update callback
    }
    void onError(uint16_t cmdCode, std::vector<uint8_t> response) override {
        // Error callback
    }
};

MyHandler handler;
machine.registerDataInterface(&handler);
```

### Enumerate Available Ports

```cpp
auto ports = Machine::getPortList();
for (auto& p : ports) {
    // Windows: "COM1", "COM3", ...
    // Linux:   "/dev/ttyUSB0", ...
}
```

---

## Data Structures

| Struct | Description |
|--------|-------------|
| `ReadOnlyData` | Static device information: model, serial number, pressure/temperature limits, cylinder parameters, etc. |
| `PressData` | Pressure control parameters: press type, mold parameters, multi-step pressure / hold / time sequences |
| `RealTimeData` | Live state: work mode, press state, current step, live pressure, countdown |

### PressData Multi-Step Press Curve

```
Pressure
  ^
  |     ┌─ Step 3 ─────┐
  |    /  KP=5s          \
  |   /                   \
  |  / Step 2 ────┐       \
  | /  KP=5s        \       \
  |/                  \──┐   \
  | Step 1 ─┐             \   \
  | KP=5s    \ After=5     \---
  +----------------------------------> Time
```

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