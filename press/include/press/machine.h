//
// Created by 11518 on 2026/9/13.
//

#ifndef PRESS_BOT_PROJECT_MACHINE_H
#define PRESS_BOT_PROJECT_MACHINE_H

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

#include <vector>
#include <memory>
#include "machinetype.h"

class MachinePrivate;


/******************************************************************************
 * Machine — High-level controller for a press machine / 压片机高级控制器
 *
 * This is the primary entry point of the library. Machine manages the
 * communication port (serial or TCP), runs an internal dispatch thread,
 * and provides a synchronous API for reading/writing machine state, as
 * well as an asynchronous callback interface via MachineDataInterface.
 * 本库的主要入口。Machine管理通信端口(串口或TCP)、运行内部调度线程、
 * 提供同步读写机器状态的API，并通过MachineDataInterface支持异步回调。
 *
 * Thread safety / 线程安全:
 *   - All public methods are thread-safe and may be called from any thread.
 *   - Callbacks registered via MachineDataInterface are invoked from the
 *     internal dispatch thread; heavy work should be offloaded.
 *   - 所有公共方法都是线程安全的，可从任意线程调用。
 *   - 通过MachineDataInterface注册的回调在内部调度线程中执行，
 *     耗时操作应移交给其他线程处理。
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

    //! Destructor — disconnects and joins all threads / 析构函数 - 断开连接并回收所有线程
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
    //! @return 64-bit register ID / 获取机器唯一注册号(连接时分配)。
    uint64_t getMachineRegisterNo() const;

    //! Get the last human-readable error message.
    //! @return Read-only C string (valid until next call that may set an error).
    //! 获取最后一条可读错误信息。
    const char* getLastErrorInfo() const;


    /**************************************************************************
     * Run control / 运行控制
     *************************************************************************/

    //! Start the machine's pressing cycle. Sends the SET_START_PRESS command.
    //! 启动机器运行。发送SET_START_PRESS命令。
    void run();

    //! Stop the machine's pressing cycle (pressing + heating).
    //! 停止机器运行。停止加压和加热动作。
    void stop();

    //! Query whether the machine is currently in run state.
    //! @return true if running / 是否正在运行。
    bool isRunning() const;


    /**************************************************************************
     * Synchronous data exchange / 同步数据交换
     *************************************************************************/

    //! Read static (read-only) device information.
    //! @param data         [out] Decoded ReadOnlyData on success.
    //! @param isCompressed [in]  Request JSON-compressed response from device.
    //! @return true on success / 读取静态(只读)设备信息。
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
     *************************************************************************/

    //! Start or stop pressing only (does not affect heating).
    //! @param isPressing true to start pressing, false to stop.
    //! @return true on success / 单独启动或停止加压(不影响加热)。
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


private:
    std::unique_ptr<MachinePrivate> mpPrivate;
};

#endif