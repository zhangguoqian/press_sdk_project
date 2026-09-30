/******************************************************************************
 * press.hpp — Machine 主控制器与序列化工具
 *
 *  本文件是 SDK 对用户暴露的主要入口，包含：
 *    - Machine 类: 高级控制器，管理通信端口 (串口 / TCP)、内部调度线程，
 *      提供同步 API 读写设备状态，同时支持通过 MachineDataInterface 异步回调。
 *    - 自由函数 (toJsonString / toFrameData): 结构体 ↔ JSON / 帧数据 转换。
 *
 *  构建选项:
 *    - PRESS_BUILDING_LIBRARY: 构建动态库时定义 (自动导出符号)
 *    - PRESS_EXPORT: 外部自动定义，Windows 上等价 __declspec(dllimport)
 *
 *  依赖: presstype.h (类型定义), pressinterface.hpp (回调接口),
 *        <memory>, <string>, <vector> (C++11 标准库)
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_PRESS_HPP
#define PRESS_SDK_PROJECT_PRESS_HPP

#ifndef PRESS_EXPORT
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef PRESS_BUILDING_LIBRARY
            #define PRESS_EXPORT __declspec(dllexport)
        #else
            #define PRESS_EXPORT __declspec(dllimport)
        #endif
    #else
        #if __GNUC__ >= 4
            #define PRESS_EXPORT __attribute__((visibility("default")))
        #else
            #define PRESS_EXPORT
        #endif
    #endif
#endif


#include "presstype.h"
#include "pressinterface.hpp"

#include <memory>
#include <string>
#include <vector>

class MachinePrivate;


/******************************************************************************
 * Machine — High-level controller for a press machine / 压片机高级控制器
 *
 *  本库的主要入口类。Machine 管理通信端口（串口或 TCP）、运行内部调度线程，
 *  提供同步 API 读写机器状态，并通过 MachineDataInterface 支持异步回调。
 *
 *  Thread safety / 线程安全:
 *    - 所有公共方法都是线程安全的，可从任意线程调用。
 *    - 通过 MachineDataInterface 注册的回调在内部调度线程中执行，
 *      耗时操作应移交给其他线程处理。
 *****************************************************************************/

class PRESS_EXPORT Machine
{
public:
    //! Constructor / 构造函数
    Machine();

    Machine(const Machine& other) = delete;
    Machine& operator=(const Machine& other) = delete;
    Machine(Machine&& other) = delete;
    Machine& operator=(Machine&& other) = delete;

    //! Destructor — disconnects and joins all threads / 析构函数 — 断开连接并回收所有线程
    ~Machine();


    /**************************************************************************
     * Connection management / 连接管理
     *************************************************************************/

    //! Open a communication port and connect to the machine.
    //! @param portName  Serial port name (e.g. "COM3") or TCP host:port.
    //! @param portType  SerialPortType or TcpSocketPortType.
    //! @return true on success, false on failure (call getLastErrorInfo()).
    //! 打开通信端口并连接到机器。
    bool connect(const char* portName, PortType portType);

    //! Disconnect from the machine and close the port.
    //! 断开与机器的连接并关闭端口。
    void disconnect();

    //! Query whether the machine is currently connected.
    //! @return true if connected / 当前是否已连接。
    bool isConnected() const;


    /**************************************************************************
     * Machine identity & error reporting / 机器身份与错误信息
     *************************************************************************/

    //! Get the machine's unique register number (assigned on connect).
    //! @return 64-bit register ID / 获取机器唯一注册号 (连接时分配)。
    uint64_t getMachineRegisterNo() const;

    //! Get the last human-readable error message.
    //! @return Read-only C string (valid until next call that may set an error).
    //! 获取最后一条可读错误信息。
    const char* getLastErrorInfo() const;


    /**************************************************************************
     * Run control / 运行控制
     *************************************************************************/

    //! Start the machine's pressing cycle. Sends the SET_START_PRESS command.
    //! 启动机器运行 (加压 + 加热)。发送 SET_START_PRESS 命令。
    void run();

    //! Stop the machine's pressing cycle (pressing + heating).
    //! 停止机器运行。停止加压和加热动作。
    void stop();

    //! Query whether the machine is currently in run state.
    //! @return true if running / 是否正在运行。
    bool isRunning() const;


    /**************************************************************************
     * Synchronous data exchange / 同步数据交换
     *
     *  以下 get/set 方法均为同步调用，内部通过通信协议与设备交互。
     *  isCompressed = true 时使用 JSON 批量读写命令 (CMDID_*_JSON)。
     *************************************************************************/

    //! Read static (read-only) device information.
    //! @param data         [out] Decoded ReadOnlyData on success.
    //! @param isCompressed [in]  Request JSON-compressed response from device.
    //! @return true on success / 读取静态 (只读) 设备信息。
    bool getReadOnlyData(ReadOnlyData& data, bool isCompressed = false);

    //! Read the current pressure control parameters.
    //! @param data         [out] Decoded PressData on success.
    //! @param isCompressed [in]  Request JSON-compressed response from device.
    //! @return true on success / 读取当前压力控制参数。
    bool getPressData(PressData& data, bool isCompressed = false);

    //! Write pressure control parameters to the device.
    //! @param data         [in]  New pressure parameters to send.
    //! @param isCompressed [in]  Encode the payload as JSON before sending.
    //! @return true on success / 写入压力控制参数到设备。
    bool setPressData(const PressData& data, bool isCompressed = false);

    //! Read live real-time machine state.
    //! @param data         [out] Decoded RealTimeData on success.
    //! @param isCompressed [in]  Request JSON-compressed response from device.
    //! @return true on success / 读取实时机器状态。
    bool getRealTimeData(RealTimeData& data, bool isCompressed = false);


    /**************************************************************************
     * Direct state commands / 直接状态命令
     *
     *  与 run()/stop() 不同，以下命令只影响单一模块 (加压或脱模)。
     *************************************************************************/

    //! Start or stop pressing only (does not affect heating).
    //! @param isPressing true to start pressing, false to stop.
    //! @return true on success / 单独启动或停止加压 (不影响加热)。
    bool setPressing(bool isPressing);

    //! Start or stop demolding only.
    //! @param isDemolding true to start demolding, false to stop.
    //! @return true on success / 单独启动或停止脱模。
    bool setDemolding(bool isDemolding);


    /**************************************************************************
     * Asynchronous callback registration / 异步回调注册
     *************************************************************************/

    //! Register an asynchronous data callback interface.
    //! Only one interface may be registered at a time; a second call replaces
    //! the previous one (unregistering is not required first).
    //! @param dataInterface Non-owning pointer to the implementation.
    //! 注册异步数据回调接口。同一时间只能注册一个，重复注册将覆盖旧的。
    void registerDataInterface(MachineDataInterface* dataInterface);

    //! Unregister the previously registered callback interface.
    //! 注销之前注册的回调接口。
    void unregisterDataInterface();


    /**************************************************************************
     * Static utilities / 静态工具
     *************************************************************************/

    //! Enumerate available communication ports on the current system.
    //! Works with both serial ports and TCP; returns COM-style list on Windows,
    //! /dev/tty* style on Linux/macOS.
    //! @return Vector of port name strings.
    //! 枚举当前系统可用的通信端口。
    static std::vector<std::string> getPortList();

    //! Get current SDK version string.
    //! 获取当前 SDK 的版本号。
    //! @return Version string.
    static std::string version();

private:
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable: 4251)
#endif
    std::unique_ptr<MachinePrivate> mpPrivate;
#ifdef _MSC_VER
#   pragma warning(pop)
#endif
};


/******************************************************************************
 * Free-function serialization utilities / 自由函数序列化工具体
 *****************************************************************************/

//! Serialize ReadOnlyData to JSON string / 将 ReadOnlyData 序列化为 JSON 字符串
PRESS_EXPORT std::string toJsonString(const ReadOnlyData& data);

//! Serialize PressData to JSON string / 将 PressData 序列化为 JSON 字符串
PRESS_EXPORT std::string toJsonString(const PressData& data);

//! Serialize RealTimeData to JSON string / 将 RealTimeData 序列化为 JSON 字符串
PRESS_EXPORT std::string toJsonString(const RealTimeData& data);

//! Serialize PressData to frame data (wire format) / 将 PressData 序列化为帧数据 (传输格式)
PRESS_EXPORT std::vector<uint8_t> toFrameData(const PressData& data);


#endif /* PRESS_SDK_PROJECT_PRESS_HPP */