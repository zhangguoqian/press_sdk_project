# Press SDK C++ 接口使用说明

> 适用头文件：`press/include/press/press.hpp`、`press/include/press/presstype.h`
> SDK 版本：1.0.0（以 `Press::version()` 为准）
> 构建标准：C++17（`press/CMakeLists.txt` 中 `CXX_STANDARD 17`）

本文覆盖 `Press` 类的全部公开接口、`PressDataInterface` 回调接口以及序列化自由函数，并提供「不使用回调」「使用回调 + 定时器」「使用回调 + 不使用定时器」三套完整示例。

> **安全提示**：示例中的 `setPressing`、`setStart`、`setStartHeat` 等接口会驱动真实设备动作。运行前请确认设备处于空载或已确认的安全状态。

---

## 1. 集成与构建

### 1.1 头文件

```cpp
#include <press/press.hpp>   // Press 类、PressDataInterface、序列化函数
#include <press/presstype.h> // 数据结构、枚举、MAX_PStep
```

`press.hpp` 已包含 `presstype.h`，通常只需包含前者。

### 1.2 CMake 集成

源码内直接使用：

```cmake
add_subdirectory(press)
target_link_libraries(my_app PRIVATE press::press)
```

使用安装包：

```cmake
find_package(press CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE press::press)
```

`press::press` 会自动带上公共包含目录。MSVC Debug 构建带有 `d` 后缀（如 `pressd.dll` / `pressd.lib`），CMake 目标已处理该差异。

### 1.3 运行环境

- Windows：串口（`COMx`）或 TCP；需要 `press.dll`（Debug 为 `pressd.dll`）在可执行文件搜索路径中。
- Linux/macOS：串口（`/dev/tty*`）或 TCP；链接 `libpress.so` / `libpress.dylib`。
- 一个 `Press` 实例同一时间只维护一个连接。

---

## 2. 基本概念

### 2.1 连接与线程

- `connect()` 打开端口并完成版本握手，同时启动内部调度线程。
- 所有请求由内部调度线程串行发送，天然保证「一问一答」；多个调用线程的请求会排队。
- 所有公共方法线程安全，可从任意线程调用。
- 默认单次请求超时 300 ms，可用 `setRequestTimeout()` 调整（上限 60000 ms）。
- `run()` 仅用于向后兼容，`connect()` 后无需调用；`stop()` 会停止调度线程并断开连接。

### 2.2 三种「状态」不要混淆

| 接口 | 含义 |
|---|---|
| `isConnected()` | 通信端口是否已打开并完成握手 |
| `isRunning()` | SDK 内部调度线程是否在运行（连接成功后为 true） |
| `getMachineState()` / RT JSON | 设备侧的运行状态（加压、加热、当前段、倒计时等） |

### 2.3 JSON 数据类型

| 缩写 | 含义 | 读取接口 | 写入接口 |
|---|---|---|---|
| ROD | 只读设备信息（名称、型号、限制、显示设置等） | `getReadOnlyDataJson` | — |
| PD | 压力参数（完整多段曲线） | `getPressDataJson` | `setPressDataJson` |
| RT | 实时运行状态 | `getRealTimeDataJson` | — |
| TD | 温度参数（完整多段曲线） | `getTempDataJson` | `setTempDataJson` |
| MD | 混合模式参数（完整多段曲线） | `getMixDataJson` | `setMixDataJson` |

`isCompressed` 会原样传给固件，用于选择设备端压缩 JSON 格式；普通场景保持 `false` 即可。

### 2.4 索引与单位约定

- 所有段索引从 **0** 开始，必须小于对应段数（`getPressStepCount` / `getTempStepCount` / `getMixStepCount`）。
- `PressData` / 温度 / 混合数组最大长度均为 `MAX_PStep`（30），设备只使用前 N 项。
- 所有 `bool` 接口：成功返回 `true`，失败返回 `false`，失败后调用 `getLastErrorInfo()` 获取原因。
- 时间字段：`m_KPTime` / `m_KTime` / `m_PTime` / `m_TTime` / `m_MTime` 单位为毫秒；`m_AirTime`、`m_StartDelay` 单位为秒。
- `DateTimeInfo::m_Year` 是 **tm_year**（自 1900 年起的偏移），例如 2026 年应填 `126`。

---

## 3. 全部接口说明

### 3.1 连接管理

| 接口 | 说明 |
|---|---|
| `bool connect(const char* portName, PressPortType portType)` | 打开端口并握手。串口示例 `"COM3"`、`"/dev/ttyUSB0"`；TCP 示例 `"192.168.1.50:9000"`。失败返回 false。 |
| `void disconnect()` | 停止调度/定时线程并关闭端口，可重复调用。 |
| `bool isConnected() const` | 端口是否已连接。 |
| `uint64_t getMachineRegisterNo() const` | 握手分配的 64 位设备注册号；未连接时为 0。 |
| `const char* getLastErrorInfo()` | 最近一次失败的可读原因，静态存储，勿释放；成功调用后内容可能仍是旧错误。 |
| `void run()` | 启动调度线程；`connect()` 已自动启动，通常无需调用。 |
| `void stop()` | 停止调度线程并断开连接。 |
| `bool isRunning() const` | 调度线程是否在运行，不代表设备正在加压。 |
| `void setRequestTimeout(unsigned int ms)` | 单次请求超时（毫秒）。`0` 恢复默认 300 ms，内部上限 60000 ms；弱网 TCP 可调大。 |

`PressPortType` 取值：

```cpp
enum PressPortType
{
    SerialPortType,    // 串口
    TcpSocketPortType, // TCP
};
```

### 3.2 回调注册

| 接口 | 说明 |
|---|---|
| `void registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds = 1)` | 注册回调对象（非拥有指针）。`intervalSeconds > 0` 时启动定时线程，每秒拉取一次 RT；`<= 0` 不启动定时器。重复注册会替换旧对象。 |
| `void registerDataInterfaceOwned(std::shared_ptr<PressDataInterface> dataInterface, int intervalSeconds = 1)` | 共享所有权版本，SDK 持有 `shared_ptr`，注销后在途回调可安全完成。 |
| `void unregisterDataInterface()` | 注销回调，停止定时线程并等待当前定时回调结束。 |

**生命周期要求（非拥有指针版本）**：回调对象必须在 `unregisterDataInterface()` 返回前保持存活；注销期间不要在其他线程执行同步 `get*Json` 并同时析构该对象。需要 SDK 托管生命周期时使用 `registerDataInterfaceOwned()`。

### 3.3 JSON 数据接口

| 接口 | 说明 |
|---|---|
| `bool getReadOnlyDataJson(std::string& json, bool isCompressed = false)` | 读取只读设备信息（ROD JSON）。 |
| `bool getPressDataJson(std::string& json, bool isCompressed = false)` | 读取完整压力参数（PD JSON）。 |
| `bool setPressDataJson(const std::string& json, bool isCompressed = false)` | 以 PD JSON 写入完整压力参数。 |
| `bool getRealTimeDataJson(std::string& json, bool isCompressed = false)` | 读取实时状态（RT JSON）。定时采集开启时本调用不触发 `onRealTimeData`。 |
| `bool getTempDataJson(std::string& json, bool isCompressed = false)` | 读取完整温度参数（TD JSON）。 |
| `bool setTempDataJson(const std::string& json, bool isCompressed = false)` | 以 TD JSON 写入完整温度参数。 |
| `bool getMixDataJson(std::string& json, bool isCompressed = false)` | 读取完整混合模式参数（MD JSON）。 |
| `bool setMixDataJson(const std::string& json, bool isCompressed = false)` | 以 MD JSON 写入完整混合模式参数。 |

**回调语义**：

- 主动调用 `getReadOnlyDataJson` / `getPressDataJson` / `getTempDataJson` / `getMixDataJson` 会同步触发对应 `on*` 回调。
- `getRealTimeDataJson` 在未启用定时采集时触发 `onRealTimeData`；启用定时采集后由定时线程统一上报，主动调用只返回数据，不重复触发回调。
- `set*` 成功时不触发对应的数据回调；失败时可能触发 `onError`。

### 3.4 只读设备信息

| 接口 | 说明 |
|---|---|
| `bool getName(std::string& name, uint8_t language = 0)` | 设备名称。`language`：0 中文，2 英文。 |
| `bool getMachineType(std::string& type)` | 机型字符串。 |
| `bool getSerialNumber(std::string& serialNumber)` | 硬件序列号。 |
| `bool getPressParameter(PressParameter& param)` | 压力参数块（21 字节：上下限、补压值、油缸直径、最大段数、小数位、模式、输出类型）。 |
| `bool getTempParameter(TempParameter& param)` | 温度参数块（12 字节：上下限、最大段数、小数位、水冷/速度能力）。 |
| `bool getOtherInfo(OtherInfo& info)` | 其他信息（截屏、启动延迟、隐藏语言菜单、远程、版本类型）。 |

### 3.5 压力段接口

| 接口 | 说明 |
|---|---|
| `bool getPressStepCount(uint8_t& step)` | 读取压力段数。 |
| `bool setPressStepCount(uint8_t step)` | 写入压力段数。 |
| `bool getCalibrationValue(float& value)` | 读取压力校准值。 |
| `bool setCalibrationValue(float value)` | 写入压力校准值。 |
| `bool getMoldType(uint8_t& type)` | 读取模具类型。 |
| `bool setMoldType(uint8_t type)` | 写入模具类型。 |
| `bool getCircle(float& diameter)` | 读取圆形容器直径。 |
| `bool setCircle(float diameter)` | 写入圆形容器直径。 |
| `bool getRectangle(float& a, float& b)` | 读取矩形容器 A/B 尺寸。 |
| `bool setRectangle(float a, float b)` | 写入矩形容器 A/B 尺寸。 |
| `bool getAirTime(uint8_t& time)` | 读取排气时间（秒）。 |
| `bool setAirTime(uint8_t time)` | 写入排气时间（秒）。 |
| `bool getPressValueIndex(uint8_t index, float& value)` | 读取指定段设定压力。 |
| `bool setPressValueIndex(uint8_t index, float value)` | 写入指定段设定压力。 |
| `bool getAfterValueIndex(uint8_t index, float& value)` | 读取指定段保压值。 |
| `bool setAfterValueIndex(uint8_t index, float value)` | 写入指定段保压值。 |
| `bool getKeepPressTime(uint8_t index, int32_t& time)` | 读取指定段保压时间（毫秒）。 |
| `bool setKeepPressTime(uint8_t index, int32_t time)` | 写入指定段保压时间（毫秒）。 |
| `bool getPressStepParameter(uint8_t index, PressStepParameter& param)` | 一次读取指定段的设定压力、保压值、保压时间。 |
| `bool setPressStepParameter(uint8_t index, const PressStepParameter& param)` | 一次写入指定段的三个参数。 |

### 3.6 温度段接口

| 接口 | 说明 |
|---|---|
| `bool getTempStepCount(uint8_t& step)` | 读取温度段数。 |
| `bool setTempStepCount(uint8_t step)` | 写入温度段数。 |
| `bool getTempSpeedMold(uint8_t& mold)` | 读取温度速度模式。 |
| `bool setTempSpeedMold(uint8_t mold)` | 写入温度速度模式。 |
| `bool getStartTempIndex(uint8_t index, float& value)` | 读取指定段温度值。 |
| `bool setTempIndex(uint8_t index, float value)` | 写入指定段温度值。 |
| `bool getTempSpeedIndex(uint8_t index, float& value)` | 读取指定段速度值。 |
| `bool setTempSpeedIndex(uint8_t index, float value)` | 写入指定段速度值。 |
| `bool getKeepTempTime(uint8_t index, int32_t& time)` | 读取指定段保温时间（毫秒）。 |
| `bool setKeepTempTime(uint8_t index, int32_t time)` | 写入指定段保温时间（毫秒）。 |
| `bool getMaxWaterValue(float& value)` | 读取最大水冷值。 |
| `bool setMaxWaterValue(float value)` | 写入最大水冷值。 |
| `bool getMinWaterValue(float& value)` | 读取最小水冷值。 |
| `bool setMinWaterValue(float value)` | 写入最小水冷值。 |
| `bool getTempStepParameter(uint8_t index, TempStepParameter& param)` | 一次读取指定段的温度、速度、保温时间。 |
| `bool setTempStepParameter(uint8_t index, const TempStepParameter& param)` | 一次写入指定段的三个参数。 |

### 3.7 混合模式段接口

| 接口 | 说明 |
|---|---|
| `bool getMixStepCount(uint8_t& step)` | 读取混合模式段数。 |
| `bool setMixStepCount(uint8_t step)` | 写入混合模式段数。 |
| `bool getMixMoldType(uint8_t& type)` | 读取混合模式模具类型。 |
| `bool setMixMoldType(uint8_t type)` | 写入混合模式模具类型。 |
| `bool getMixRectangle(float& a, float& b)` | 读取混合模式矩形尺寸 A/B。 |
| `bool setMixRectangle(float a, float b)` | 写入混合模式矩形尺寸 A/B。 |
| `bool getMixCircle(float& diameter)` | 读取混合模式圆形容器直径。 |
| `bool setMixCircle(float diameter)` | 写入混合模式圆形容器直径。 |
| `bool getMixIsPressIndex(uint8_t index, bool& isPress)` | 读取指定段是否加压。 |
| `bool setMixIsPressIndex(uint8_t index, bool isPress)` | 写入指定段是否加压。 |
| `bool getMixPressIndex(uint8_t index, float& value)` | 读取指定段压力值。 |
| `bool setMixPressIndex(uint8_t index, float value)` | 写入指定段压力值。 |
| `bool getMixAfterPressIndex(uint8_t index, float& value)` | 读取指定段保压值。 |
| `bool setMixAfterPressIndex(uint8_t index, float value)` | 写入指定段保压值。 |
| `bool getMixIsHeatUpIndex(uint8_t index, bool& isHeatUp)` | 读取指定段是否加热。 |
| `bool setMixIsHeatUpIndex(uint8_t index, bool isHeatUp)` | 写入指定段是否加热。 |
| `bool getMixHeatUpIndex(uint8_t index, float& value)` | 读取指定段加热温度。 |
| `bool setMixHeatUpIndex(uint8_t index, float value)` | 写入指定段加热温度。 |
| `bool getMixIsHeatDownIndex(uint8_t index, bool& isHeatDown)` | 读取指定段是否冷却。 |
| `bool setMixIsHeatDownIndex(uint8_t index, bool isHeatDown)` | 写入指定段是否冷却。 |
| `bool getMixHeatDownIndex(uint8_t index, float& value)` | 读取指定段冷却温度。 |
| `bool setMixHeatDownIndex(uint8_t index, float value)` | 写入指定段冷却温度。 |
| `bool getMixKeepTime(uint8_t index, int32_t& time)` | 读取指定段保持时间（毫秒）。 |
| `bool setMixKeepTime(uint8_t index, int32_t time)` | 写入指定段保持时间（毫秒）。 |
| `bool getMixStepParameter(uint8_t index, MixStepParameter& param)` | 一次读取指定段的完整 8 个参数。 |
| `bool setMixStepParameter(uint8_t index, const MixStepParameter& param)` | 一次写入指定段的完整参数。 |

> 混合段参数帧为 24 字节（index + 23）。旧固件按 23 字节校验会拒绝写入，遇到长度错误请升级固件。

### 3.8 系统命令

| 接口 | 说明 |
|---|---|
| `bool getDateTime(DateTimeInfo& dateTime)` | 读取设备日期时间；`m_Year` 为 tm_year。 |
| `bool setDateTime(const DateTimeInfo& dateTime)` | 写入日期时间；`m_Year` 使用 tm_year（2026 → 126）。 |
| `bool getBrightness(uint8_t& brightness)` | 读取屏幕亮度。 |
| `bool setBrightness(uint8_t brightness)` | 设置屏幕亮度。 |
| `bool getButtonSound(bool& enabled)` | 读取按键提示音开关。 |
| `bool setButtonSound(bool enabled)` | 设置按键提示音开关。 |
| `bool getLanguage(uint8_t& language)` | 读取界面语言。 |
| `bool setLanguage(uint8_t language)` | 设置界面语言。 |

### 3.9 状态与控制

| 接口 | 说明 |
|---|---|
| `bool getMachineState(MachineState& state)` | 读取设备当前状态；响应长度随模式为 20/25/30 字节，`m_Count` 表示实际解析字节数。 |
| `bool setPressing(bool isPressing)` | 单独启动/停止加压，不影响加热。回执方向必须与请求一致。 |
| `bool setDemolding(bool isDemolding)` | 单独启动/停止脱模；**当前固件未实现 0x1523，调用会失败或超时**。 |
| `bool setStartHeat(bool isHeating)` | 单独启动/停止加热。 |
| `bool setStart(bool isStart)` | 启动/停止完整流程（加压 + 加热）。 |
| `bool selectModelState(uint8_t model)` | 选择工作模式，取值由固件定义（通常 1 或 2）。 |

### 3.10 静态工具

| 接口 | 说明 |
|---|---|
| `static std::vector<std::string> getPortList()` | 枚举串口：Windows 为 `COMx`，Linux/macOS 为 `/dev/tty*`；TCP 不参与枚举。 |
| `static std::string version()` | SDK 版本字符串，例如 `"1.0.0"`。 |

### 3.11 序列化自由函数

| 接口 | 说明 |
|---|---|
| `std::string toJsonString(const ReadOnlyData& data)` | 将只读结构体序列化为 JSON 文本。 |
| `std::string toJsonString(const PressData& data)` | 将压力参数序列化为 JSON，可直接传给 `setPressDataJson()`。 |
| `std::string toJsonString(const RealTimeData& data)` | 将实时结构体序列化为 JSON 文本。 |
| `std::vector<uint8_t> toFrameData(const PressData& data)` | 将压力参数序列化为 UTF-8 JSON 字节载荷（不含帧头/校验和），主要用于调试或自定义协议封装。 |

### 3.12 公共数据结构

| 结构体 | 用途 |
|---|---|
| `ReadOnlyData` | 设备身份、限制、显示设置。使用前建议零初始化：`ReadOnlyData data{};` |
| `PressData` | 完整压力曲线：段数、模具参数、每段设定压力/保压值/保压时间，以及尾部追加的 `m_AirTime`。 |
| `RealTimeData` | 实时状态：模式、加压/加热标志、当前段、压力/温度、倒计时等。`m_PdChanged` 为兼容字段，恒为 0。 |
| `VersionInfo` | 固件版本：应用类型 + 主/次/修订号。 |
| `PressParameter` / `TempParameter` / `OtherInfo` | 只读参数块。 |
| `DateTimeInfo` | 日期时间，年份为 tm_year。 |
| `MachineState` | 状态命令的变长解析结果，`m_Count` 为有效字节数。 |
| `PressStepParameter` / `TempStepParameter` / `MixStepParameter` | 单段参数聚合，便于一次读写。 |

`PressData::m_Speed`：0 = 等静压，1 = 普通压制。`RealTimeData::m_ModelState`：0 普通、1 速度、2 三速模式。

### 3.13 回调接口 `PressDataInterface`

派生类必须实现以下 6 个纯虚函数：

| 回调 | 触发时机 |
|---|---|
| `onReadOnlyData(int errorCode, uint64_t registerNo, const std::string& json)` | 主动调用 `getReadOnlyDataJson()` 后触发（成功或失败）。 |
| `onRealTimeData(int errorCode, const std::string& json)` | 定时采集（`intervalSeconds > 0`）或未启用定时器时主动调用 `getRealTimeDataJson()`。 |
| `onPressData(int errorCode, const std::string& json)` | 主动调用 `getPressDataJson()` 后触发。 |
| `onTempData(int errorCode, const std::string& json)` | 主动调用 `getTempDataJson()` 后触发。 |
| `onMixData(int errorCode, const std::string& json)` | 主动调用 `getMixDataJson()` 后触发。 |
| `onError(uint16_t cmdCode, std::vector<uint8_t> response)` | 请求失败、响应异常或写命令回执异常。`response` 仅在回调期间有效。 |

`errorCode == 0` 表示成功，非 0 表示失败，失败时 JSON 为空字符串。

**线程模型**：

- 定时采集触发的 `onRealTimeData` 在 SDK 定时线程执行。
- 主动 `get*Json` 触发的回调在调用线程同步执行。
- 因此回调中不要执行长耗时操作；UI 更新请自行切回主线程。
- 回调中可以安全调用 `disconnect()` / `stop()` / `unregisterDataInterface()`，但不要在回调中析构 `Press` 对象本身。

---

## 4. 示例一：不使用回调（同步轮询）

适合命令行工具、脚本化测试，或希望完全掌控采集节奏的应用。

```cpp
#include <press/press.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main()
{
    std::cout << "SDK version: " << Press::version() << "\n";

    // 1. 枚举串口（TCP 不会出现在列表中）
    for (const auto& port : Press::getPortList())
    {
        std::cout << "port: " << port << "\n";
    }

    Press press;

    // 2. 连接设备（串口）
    if (!press.connect("COM3", SerialPortType))
    {
        std::cerr << "connect failed: " << press.getLastErrorInfo() << "\n";
        return 1;
    }
    std::cout << "connected, registerNo=" << press.getMachineRegisterNo() << "\n";

    // 3. 读取只读信息（ROD JSON）
    std::string rodJson;
    if (press.getReadOnlyDataJson(rodJson))
    {
        std::cout << "ROD: " << rodJson << "\n";
    }

    // 4. 读取结构化只读参数
    PressParameter pressParam{};
    if (press.getPressParameter(pressParam))
    {
        std::cout << "max pressure: " << pressParam.m_MaxPLimit
                  << ", min pressure: " << pressParam.m_MinPLimit << "\n";
    }

    // 5. 轮询实时状态（不使用回调）
    for (int i = 0; i < 20; ++i)
    {
        std::string rtJson;
        if (press.getRealTimeDataJson(rtJson))
        {
            std::cout << "[RT] " << rtJson << "\n";
        }
        else
        {
            std::cerr << "[RT] failed: " << press.getLastErrorInfo() << "\n";
        }

        MachineState state{};
        if (press.getMachineState(state))
        {
            std::cout << "[state] count=" << static_cast<int>(state.m_Count)
                      << " press=" << state.m_PressValue
                      << " ptime=" << state.m_PTime << " ms\n";
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // 6. 控制：单独加压 2 秒后停止（请在安全状态下运行）
    if (!press.setPressing(true))
    {
        std::cerr << "start pressing failed: " << press.getLastErrorInfo() << "\n";
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
    press.setPressing(false);

    // 7. 断开连接
    press.disconnect();
    return 0;
}
```

**要点**：

- 不使用回调时，所有数据都通过返回值 + 输出参数获取。
- 轮询周期完全由调用方控制；建议不小于单次命令往返时间（串口 115200 下通常数十毫秒量级）。
- `machine state` 与 `RT JSON` 都可用；前者适合直接取数值，后者包含固件完整字段。

---

## 5. 示例二：使用回调 + 定时器

适合 UI/上位机：实时数据由 SDK 定时推送，业务线程只处理回调。

```cpp
#include <press/press.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

class ConsoleCallbacks : public PressDataInterface
{
public:
    void onReadOnlyData(int errorCode, uint64_t registerNo,
                        const std::string& json) override
    {
        if (errorCode == 0)
        {
            std::cout << "[ROD] registerNo=" << registerNo
                      << " json=" << json << "\n";
        }
        else
        {
            std::cerr << "[ROD] error=" << errorCode << "\n";
        }
    }

    void onRealTimeData(int errorCode, const std::string& json) override
    {
        if (errorCode == 0)
        {
            std::cout << "[RT] " << json << "\n";
        }
        else
        {
            std::cerr << "[RT] error=" << errorCode << "\n";
        }
    }

    void onPressData(int errorCode, const std::string& json) override
    {
        if (errorCode == 0)
        {
            std::cout << "[PD] " << json << "\n";
        }
    }

    void onTempData(int errorCode, const std::string& json) override
    {
        if (errorCode == 0)
        {
            std::cout << "[TD] " << json << "\n";
        }
    }

    void onMixData(int errorCode, const std::string& json) override
    {
        if (errorCode == 0)
        {
            std::cout << "[MD] " << json << "\n";
        }
    }

    void onError(uint16_t cmdCode, std::vector<uint8_t> response) override
    {
        std::cerr << "[onError] cmd=0x" << std::hex << cmdCode << std::dec
                  << " responseBytes=" << response.size() << "\n";
    }
};

int main()
{
    Press press;
    if (!press.connect("COM3", SerialPortType))
    {
        std::cerr << "connect failed: " << press.getLastErrorInfo() << "\n";
        return 1;
    }

    ConsoleCallbacks callbacks;

    // 注册回调并启用 1 秒定时采集：SDK 每秒自动拉取一次 RT 并回调 onRealTimeData
    press.registerDataInterface(&callbacks, 1);

    // 定时器只负责 RT；ROD/PD/TD/MD 仍通过主动读取获取，并同步触发对应回调
    std::string json;
    press.getReadOnlyDataJson(json); // -> onReadOnlyData
    press.getPressDataJson(json);    // -> onPressData
    press.getTempDataJson(json);     // -> onTempData
    press.getMixDataJson(json);      // -> onMixData

    // 定时器运行期间，主动读取 RT 只返回数据，不会重复触发 onRealTimeData
    press.getRealTimeDataJson(json);

    // 业务运行（示例为 10 秒）
    std::this_thread::sleep_for(std::chrono::seconds(10));

    // 注销回调（会停止定时线程并等待当前回调结束）
    press.unregisterDataInterface();
    press.disconnect();
    return 0;
}
```

**要点**：

- `registerDataInterface(&callbacks, 1)` 的 `1` 是秒，表示每秒采集一次 RT。
- 定时器**只定时读取 RT**，不会自动推送 ROD/PD/TD/MD；需要这些数据时主动调用对应 `get*Json`（会触发回调）。
- 回调对象 `callbacks` 必须比注册期存活更久；本例为栈对象，在 `unregisterDataInterface()` 之后析构。
- 若回调对象需要跨线程/动态管理，优先使用共享所有权版本：

```cpp
auto callbacks = std::make_shared<ConsoleCallbacks>();
press.registerDataInterfaceOwned(callbacks, 1);
```

---

## 6. 示例三：使用回调 + 不使用定时器

适合需要自己控制采集节奏、但仍希望统一走回调分发数据的应用。

```cpp
#include <press/press.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

class ConsoleCallbacks : public PressDataInterface
{
public:
    void onReadOnlyData(int errorCode, uint64_t registerNo,
                        const std::string&) override
    {
        if (errorCode == 0)
        {
            std::cout << "[ROD] registerNo=" << registerNo << "\n";
        }
    }

    void onRealTimeData(int errorCode, const std::string& json) override
    {
        if (errorCode == 0)
        {
            std::cout << "[RT] " << json << "\n";
        }
    }

    void onPressData(int, const std::string&) override {}
    void onTempData(int, const std::string&) override {}
    void onMixData(int, const std::string&) override {}
    void onError(uint16_t cmdCode, std::vector<uint8_t>) override
    {
        std::cerr << "[onError] cmd=0x" << std::hex << cmdCode << std::dec << "\n";
    }
};

int main()
{
    Press press;
    if (!press.connect("COM3", SerialPortType))
    {
        std::cerr << "connect failed: " << press.getLastErrorInfo() << "\n";
        return 1;
    }

    ConsoleCallbacks callbacks;

    // intervalSeconds <= 0：不启动定时线程
    press.registerDataInterface(&callbacks, 0);

    std::string json;
    press.getReadOnlyDataJson(json); // 同步触发 onReadOnlyData

    // 自己控制采集节奏：每次 getRealTimeDataJson 都会同步触发 onRealTimeData
    for (int i = 0; i < 50; ++i)
    {
        press.getRealTimeDataJson(json);
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    // 需要温度/混合数据时同样主动读取，回调在调用线程执行
    press.getTempDataJson(json);
    press.getMixDataJson(json);
    press.getPressDataJson(json);

    press.unregisterDataInterface();
    press.disconnect();
    return 0;
}
```

**要点**：

- `intervalSeconds <= 0` 时不创建定时线程，`onRealTimeData` 只会在主动调用 `getRealTimeDataJson()` 时触发。
- 回调在调用线程执行，因此可以在回调中直接访问调用线程的上下文；但不要做长耗时操作，否则会拖慢采集循环。
- 该模式与「不使用回调」的区别：数据仍通过 `get*Json` 获取，同时额外获得统一的回调分发入口。

---

## 7. 常见用法片段

### 7.1 TCP 连接与超时

```cpp
Press press;
press.setRequestTimeout(3000); // 弱网环境适当调大
if (!press.connect("192.168.1.50:9000", TcpSocketPortType))
{
    std::cerr << press.getLastErrorInfo() << "\n";
}
```

### 7.2 写入完整压力曲线

```cpp
PressData pd{};
pd.m_PStep = 2;
pd.m_Type = 1;
pd.m_Speed = 1;                 // 普通压制
pd.m_SetPValue[0] = 5.0f;
pd.m_AfterValue[0] = 4.5f;
pd.m_KPTime[0] = 3000;
pd.m_SetPValue[1] = 8.0f;
pd.m_AfterValue[1] = 7.5f;
pd.m_KPTime[1] = 5000;

const std::string pdJson = toJsonString(pd);
if (!press.setPressDataJson(pdJson))
{
    std::cerr << "write PD failed: " << press.getLastErrorInfo() << "\n";
}
```

### 7.3 只改某一段的三个参数

```cpp
press.setPressStepCount(3);

PressStepParameter step{};
step.m_SetPValue = 6.0f;
step.m_AfterValue = 5.5f;
step.m_KPTime = 2000;
press.setPressStepParameter(0, step);

// 单字段写入也可以
press.setPressValueIndex(1, 7.0f);
press.setKeepPressTime(1, 4000);
```

### 7.4 读取并写回日期时间

```cpp
DateTimeInfo dt{};
if (press.getDateTime(dt))
{
    std::cout << (1900 + dt.m_Year) << "-" << int(dt.m_Month) << "-"
              << int(dt.m_Day) << " " << int(dt.m_Hour) << ":"
              << int(dt.m_Minute) << ":" << int(dt.m_Second) << "\n";

    dt.m_Year = 126; // 2026 年，tm_year
    press.setDateTime(dt);
}
```

### 7.5 读取完整只读信息结构体

```cpp
std::string rodJson;
if (press.getReadOnlyDataJson(rodJson))
{
    std::cout << rodJson << "\n"; // 固件完整字段
}

PressParameter pp{};
TempParameter tp{};
OtherInfo oi{};
press.getPressParameter(pp);
press.getTempParameter(tp);
press.getOtherInfo(oi);
```

---

## 8. 错误处理、线程与性能

### 8.1 错误处理

- 所有 `bool` 接口失败后立即调用 `getLastErrorInfo()`；该指针为静态存储，不要释放。
- `getLastErrorInfo()` 反映最近一次可能设置错误的调用；成功调用后不保证被清空。
- 回调中的 `errorCode != 0` 表示失败；`onError` 还会给出原始命令码与响应字节。
- 连接失败会自动回滚：调度线程停止、端口关闭、注册号清零。

### 8.2 线程安全

- `Press` 的公共方法线程安全，可从任意线程调用。
- 内部请求串行执行：一个线程正在执行长命令（如 MD 写入）时，其他线程的请求会排队。
- 回调线程：定时采集在 SDK 定时线程，主动 `get*Json` 在调用线程。
- 不要在回调中析构 `Press`；不要在其他线程执行同步 `get*Json` 时析构回调对象。

### 8.3 性能参考

- 115200 串口下单次小命令往返通常约 46 ms（固件处理占主要部分）。
- TD（约 494 字节）读取约 90 ms，MD（约 1077 字节）读取约 150 ms；实际值随固件与链路变化。
- 定时采集与用户命令共用同一串口队列，密集用户请求会挤占定时采集节拍。
- 需要更高吞吐时优先使用 JSON 批量读写，而不是逐字段命令。

### 8.4 其他注意事项

- `run()` 不需要在 `connect()` 后调用；`stop()` 等价于「停止调度 + 断开连接」。
- `isRunning()` 表示 SDK 调度线程在运行，不是设备正在加压。
- 段索引从 0 开始，越界会由固件拒绝并返回失败。
- `setDemolding()` 当前固件未实现，调用会失败或超时。
- JSON 写入长度受协议限制（单帧数据区上限 65535 字节），超长会返回失败。
- 全部接口回调均可能包含中文字段（UTF-8）；Windows 控制台建议设置 UTF-8 代码页。

---

## 9. 接口选择建议

| 场景 | 推荐方式 |
|---|---|
| 只读取实时状态并自己控制节奏 | `getRealTimeDataJson` / `getMachineState` 轮询 |
| UI 每秒刷新实时数据 | `registerDataInterface(&cb, 1)` + `onRealTimeData` |
| 需要同时处理多种数据回调 | `registerDataInterface(&cb, 0)` + 主动 `get*Json` |
| 完整备份/恢复压力、温度、混合参数 | PD / TD / MD JSON 接口 |
| 只修改少量字段 | 单值/索引接口或 `*StepParameter` |
| 需要跨线程共享回调对象 | `registerDataInterfaceOwned(std::shared_ptr<...>, n)` |
| 需要完整固件字段 | JSON 接口（结构体仅暴露常用字段） |
