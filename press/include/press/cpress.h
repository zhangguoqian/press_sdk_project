/******************************************************************************
 * cpress.h — C language wrapper for the Press SDK
 * cpress.h — Press SDK 的 C 语言封装接口
 *
 *  This header exposes the same functionality of the C++ Press class through a
 *  pure C API. It is intended for C projects or interop code that cannot use
 *  C++ classes directly.
 *  该头文件通过纯 C 接口暴露 Press SDK 的同等功能，适合不能直接使用
 *  C++ 类的 C 项目或跨语言互操作代码。
 *
 *  The API is intentionally simple: create a handle, connect to a port, call
 *  get/set operations, and optionally register callback handlers. All exported
 *  functions are wrapped in extern "C" for safe C++ linkage.
 *  该 API 设计保持简单：创建句柄、连接端口、调用 get/set 操作，并可选择
 *  注册回调处理函数。所有导出函数都使用 extern "C" 以保证 C++ 链接兼容。
 *
 *  Threading model / 线程模型:
 *    - The underlying SDK remains internally thread-safe.
 *    - Callbacks are invoked by the SDK worker thread.
 *    - UI code should dispatch to the UI thread if needed.
 *    - 底层 SDK 内部仍然保持线程安全。
 *    - 回调由 SDK 工作线程触发。
 *    - 如果 UI 代码需要更新界面，应自行进行线程分发。
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_CPRESS_H
#define PRESS_SDK_PROJECT_CPRESS_H

#include "presstype.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
    #ifdef PRESS_BUILDING_LIBRARY
        #define PRESS_C_EXPORT __declspec(dllexport)
    #else
        #define PRESS_C_EXPORT __declspec(dllimport)
    #endif
#else
    #if __GNUC__ >= 4
        #define PRESS_C_EXPORT __attribute__((visibility("default")))
    #else
        #define PRESS_C_EXPORT
    #endif
#endif

//! Opaque handle to the underlying Press object / 底层 Press 对象的不透明句柄
//! Use press_create() and press_destroy() to manage lifetime / 使用 press_create() 和 press_destroy() 管理生命周期
typedef struct PressCContext PressCContext;

//! Read-only data callback prototype / 只读数据回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param errorCode     Result code; 0 means success / 结果码；0 表示成功
//! @param registerNo    Device register number / 设备注册号
//! @param readOnlyData  Decoded read-only data / 解码后的只读数据
//! @note This callback may be invoked on the SDK worker thread. If the caller
//!       needs UI updates, it should marshal the callback onto the UI thread.
//!       该回调可能在 SDK 工作线程中触发；如果调用方需要更新 UI，应该自行
//!       将回调切到 UI 线程处理。
typedef void (*PressCReadOnlyDataCallback)(void *userData, int errorCode, uint64_t registerNo, const ReadOnlyData *readOnlyData);

//! Real-time data callback prototype / 实时数据回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param errorCode     Result code; 0 means success / 结果码；0 表示成功
//! @param realTimeData  Decoded real-time machine state / 解码后的实时机器状态
//! @note This callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCRealTimeDataCallback)(void* userData, int errorCode, const RealTimeData* realTimeData);

//! Pressure data callback prototype / 压力参数回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param errorCode     Result code; 0 means success / 结果码；0 表示成功
//! @param pressData     Decoded pressure configuration / 解码后的压力配置
//! @note This callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCPressDataCallback)(void* userData, int errorCode, const PressData* pressData);

//! Error callback prototype / 错误回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param cmdCode       Command code that produced the error / 触发错误的命令码
//! @param response      Raw response bytes / 原始响应字节数组
//! @param responseLen   Length of response bytes / 响应字节长度
//! @note The callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCErrorCallback)(void* userData, uint16_t cmdCode, const uint8_t* response, size_t responseLen);

//! Callback registration structure / 回调注册结构体
//! Set any callback pointer to NULL if it is not needed / 如不需要，任意回调指针可设为 NULL
//! @note The callbacks are invoked on the SDK worker thread. The registered
//!       structure is copied by value during registration, so the caller can
//!       safely release its local copy after the function returns.
//!       回调会在 SDK 工作线程中触发。注册时会按值复制该结构体，因此调用方
//!       在函数返回后可安全释放其本地副本。
typedef struct PressCDataCallbacks
{
    void* userData;
    PressCReadOnlyDataCallback onReadOnlyData;
    PressCRealTimeDataCallback onRealTimeData;
    PressCPressDataCallback onPressData;
    PressCErrorCallback onError;
} PressCDataCallbacks;

//! Create a new Press SDK handle / 创建新的 Press SDK 句柄
//! @return Pointer to a new PressCContext, or NULL on failure / 新的 PressCContext 指针；失败返回 NULL
PRESS_C_EXPORT PressCContext* cpress_create(void);

//! Destroy a Press SDK handle / 销毁 Press SDK 句柄
//! @param handle Context to destroy / 要销毁的上下文
PRESS_C_EXPORT void cpress_destroy(PressCContext* handle);

//! Connect to a serial or TCP port / 连接串口或 TCP 端口
//! @param handle     Active context / 有效的上下文
//! @param portName   Port name, such as "COM3" or "127.0.0.1:9000" / 端口名，例如 "COM3" 或 "127.0.0.1:9000"
//! @param portType   One of SerialPortType or TcpSocketPortType / SerialPortType 或 TcpSocketPortType
//!                  Use 0 for SerialPortType and 1 for TcpSocketPortType.
//!                  0 表示 SerialPortType，1 表示 TcpSocketPortType。
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_connect(PressCContext* handle, const char* portName, int portType);

//! Disconnect from the machine and close the connection / 断开与设备的连接并关闭端口
PRESS_C_EXPORT void cpress_disconnect(PressCContext* handle);

//! Check connection state / 查询连接状态
//! @return 1 if connected, 0 otherwise / 已连接返回 1，否则返回 0
PRESS_C_EXPORT int cpress_is_connected(const PressCContext* handle);

//! Query the unique hardware register number / 获取唯一设备注册号
//! @return Device register number / 设备注册号
PRESS_C_EXPORT uint64_t cpress_get_machine_register_no(const PressCContext* handle);

//! Query the last error message / 获取最后一条错误信息
//! @return Null-terminated string. The pointer remains valid until the next API
//!         call that updates the same handle's internal error state, or until the
//!         handle is destroyed. / 以 '\0' 结尾的字符串。该指针在同一句柄的
//!         下一次错误状态更新前，或在句柄销毁前保持有效。
PRESS_C_EXPORT const char* cpress_get_last_error_info(const PressCContext* handle);

//! Start the machine's pressing cycle / 启动设备运行
PRESS_C_EXPORT void cpress_run(PressCContext* handle);

//! Stop the machine's pressing cycle / 停止设备运行
PRESS_C_EXPORT void cpress_stop(PressCContext* handle);

//! Query whether the machine is currently running / 查询设备是否正在运行
//! @return 1 if running, 0 otherwise / 运行中返回 1，否则返回 0
PRESS_C_EXPORT int cpress_is_running(const PressCContext* handle);

//! Read the read-only device information / 读取只读设备信息
//! @param handle       Active context / 有效的上下文
//! @param data         Output buffer / 输出缓冲区
//! @param isCompressed Whether the request should use compressed JSON mode.
//!                    0 = false, non-zero = true. / 是否使用压缩 JSON 模式。
//!                    0 表示 false，非 0 表示 true。
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_get_read_only_data(PressCContext* handle, ReadOnlyData* data, int isCompressed);

//! Read pressure configuration / 读取压力参数
//! @param isCompressed 0 = false, non-zero = true / 0 表示 false，非 0 表示 true
PRESS_C_EXPORT int cpress_get_press_data(PressCContext* handle, PressData* data, int isCompressed);

//! Write pressure configuration / 写入压力参数
//! @param isCompressed 0 = false, non-zero = true / 0 表示 false，非 0 表示 true
PRESS_C_EXPORT int cpress_set_press_data(PressCContext* handle, const PressData* data, int isCompressed);

//! Read live machine state / 读取实时机器状态
//! @param isCompressed 0 = false, non-zero = true / 0 表示 false，非 0 表示 true
PRESS_C_EXPORT int cpress_get_real_time_data(PressCContext* handle, RealTimeData* data, int isCompressed);

//! Start or stop the pressing action only / 启动或停止单独的加压动作
//! @param isPressing   0 = stop, non-zero = start / 0 表示停止，非 0 表示启动
PRESS_C_EXPORT int cpress_set_pressing(PressCContext* handle, int isPressing);

//! Start or stop the demolding action only / 启动或停止单独的脱模动作
//! @param isDemolding  0 = stop, non-zero = start / 0 表示停止，非 0 表示启动
PRESS_C_EXPORT int cpress_set_demolding(PressCContext* handle, int isDemolding);

//! Register callback handlers / 注册回调处理器
//! @param handle     Active context / 有效的上下文
//! @param callbacks  Callback table. Each callback pointer may be NULL when not
//!                   needed. Callbacks are invoked on the SDK worker thread.
//!                   / 回调表。每个回调指针在不需要时都可为 NULL。回调会在 SDK
//!                   工作线程中触发。
PRESS_C_EXPORT void cpress_register_data_interface(PressCContext* handle, const PressCDataCallbacks* callbacks);

//! Unregister callback handlers / 注销回调处理器
PRESS_C_EXPORT void cpress_unregister_data_interface(PressCContext* handle);

//! Get the SDK version string / 获取 SDK 版本字符串
//! @return Version string. The returned pointer points to internal static storage
//!         and remains valid at least until the next call to cpress_version().
//!         / 版本字符串。返回指针指向内部静态存储区，至少在下一次调用
//!         cpress_version() 前保持有效。
PRESS_C_EXPORT const char* cpress_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PRESS_SDK_PROJECT_CPRESS_H */
