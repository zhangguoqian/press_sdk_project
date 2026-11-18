/******************************************************************************
 * press.h — C language wrapper for the Press SDK
 * press.h — Press SDK 的 C 语言封装接口
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
 *  Scope / 能力范围:
 *    - The C API is a stable subset of the C++ API. JSON data access covers
 *      ROD / PD / RT / TD / MD, plus the commonly used single-value commands.
 *    - Device-info, machine-state, date-time and display settings are currently
 *      available only through the C++ API (press.hpp).
 *    - C 接口是 C++ 接口的稳定子集：JSON 覆盖 ROD / PD / RT / TD / MD 及常用
 *      单值命令；设备信息、机器状态、时间与显示设置等目前仅由 C++ 接口提供。
 *
 *  Threading model / 线程模型:
 *    - The underlying SDK remains internally thread-safe.
 *    - Timer-driven callbacks run on the SDK timer thread; callbacks caused by
 *      synchronous get*Json calls run on the calling thread.
 *    - UI code should dispatch to the UI thread if needed.
 *    - 底层 SDK 内部仍然保持线程安全。
 *    - 定时采集触发的回调在 SDK 定时线程执行；主动调用 get*Json 触发的回调
 *      在调用线程执行。
 *    - 如果 UI 代码需要更新界面，应自行进行线程分发。
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_PRESS_H
#define PRESS_SDK_PROJECT_PRESS_H

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
typedef struct CPressContext CPressContext;

//! Read-only data callback prototype / 只读数据回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param errorCode     Result code; 0 means success / 结果码；0 表示成功
//! @param registerNo    Device register number / 设备注册号
//! @param json          Raw ROD JSON payload, empty on failure / 原始 ROD JSON 数据，失败时为空字符串
//! @note This callback may be invoked on the SDK timer thread or on the thread
//!       that called a synchronous get*Json function. If the caller needs UI
//!       updates, it should marshal the callback onto the UI thread.
//!       该回调可能在 SDK 定时线程或调用 get*Json 的线程中触发；如果调用方
//!       需要更新 UI，应该自行将回调切到 UI 线程处理。
typedef void (*PressCReadOnlyDataCallback)(void *userData, int errorCode, uint64_t registerNo, const char *json);

//! Real-time data callback prototype / 实时数据回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param errorCode     Result code; 0 means success / 结果码；0 表示成功
//! @param json          Raw RT JSON payload, empty on failure / 原始 RT JSON 数据，失败时为空字符串
//! @note This callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCRealTimeDataCallback)(void* userData, int errorCode, const char *json);

//! Pressure data callback prototype / 压力参数回调原型
//! @param userData      Caller-provided context / 调用方提供的上下文
//! @param errorCode     Result code; 0 means success / 结果码；0 表示成功
//! @param json          Raw PD JSON payload, empty on failure / 原始 PD JSON 数据，失败时为空字符串
//! @note This callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCPressDataCallback)(void* userData, int errorCode, const char *json);

//! Temperature data callback prototype (TD JSON) / 温度数据回调原型（TD JSON）
//! @param userData   Caller-provided context / 调用方提供的上下文
//! @param errorCode  Result code; 0 means success / 结果码；0 表示成功
//! @param json       Raw TD JSON payload, empty string on failure / 原始 TD JSON 数据，失败时为空字符串
//! @note This callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCTempDataCallback)(void* userData, int errorCode, const char* json);

//! Mix data callback prototype (MD JSON) / 混合数据回调原型（MD JSON）
//! @param userData   Caller-provided context / 调用方提供的上下文
//! @param errorCode  Result code; 0 means success / 结果码；0 表示成功
//! @param json       Raw MD JSON payload, empty string on failure / 原始 MD JSON 数据，失败时为空字符串
//! @note This callback may be invoked on the SDK worker thread. / 该回调可能在 SDK 工作线程中触发。
typedef void (*PressCMixDataCallback)(void* userData, int errorCode, const char* json);

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
typedef struct CPressDataCallbacks
{
    void* userData;
    PressCReadOnlyDataCallback onReadOnlyData;
    PressCRealTimeDataCallback onRealTimeData;
    PressCPressDataCallback onPressData;
    PressCErrorCallback onError;
    PressCTempDataCallback onTempData;
    PressCMixDataCallback onMixData;
} CPressDataCallbacks;

//! Create a new Press SDK handle / 创建新的 Press SDK 句柄
//! @return Pointer to a new CPressContext, or NULL on failure / 新的 CPressContext 指针；失败返回 NULL
PRESS_C_EXPORT CPressContext* cpress_create(void);

//! Destroy a Press SDK handle / 销毁 Press SDK 句柄
//! @param handle Context to destroy / 要销毁的上下文
PRESS_C_EXPORT void cpress_destroy(CPressContext* handle);

//! Connect to a serial or TCP port / 连接串口或 TCP 端口
//! @param handle     Active context / 有效的上下文
//! @param portName   Port name, such as "COM3" or "127.0.0.1:9000" / 端口名，例如 "COM3" 或 "127.0.0.1:9000"
//! @param portType   One of SerialPortType or TcpSocketPortType / SerialPortType 或 TcpSocketPortType
//!                  Use 0 for SerialPortType and 1 for TcpSocketPortType.
//!                  0 表示 SerialPortType，1 表示 TcpSocketPortType。
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_connect(CPressContext* handle, const char* portName, int portType);

//! Disconnect from the machine and close the connection / 断开与设备的连接并关闭端口
PRESS_C_EXPORT void cpress_disconnect(CPressContext* handle);

//! Check connection state / 查询连接状态
//! @return 1 if connected, 0 otherwise / 已连接返回 1，否则返回 0
PRESS_C_EXPORT int cpress_is_connected(const CPressContext* handle);

//! Query the unique hardware register number / 获取唯一设备注册号
//! @return Device register number / 设备注册号
PRESS_C_EXPORT uint64_t cpress_get_machine_register_no(const CPressContext* handle);

//! Query the last error message / 获取最后一条错误信息
//! @return Null-terminated string. The pointer remains valid until the next API
//!         call that updates the same handle's internal error state, or until the
//!         handle is destroyed. / 以 '\0' 结尾的字符串。该指针在同一句柄的
//!         下一次错误状态更新前，或在句柄销毁前保持有效。
PRESS_C_EXPORT const char* cpress_get_last_error_info(const CPressContext* handle);

//! Start the SDK scheduler thread that drives all protocol I/O.
//! The scheduler is started automatically after cpress_connect() succeeds,
//! so calling this is optional; kept for backward compatibility.
//! To start the device pressing action, use cpress_set_pressing(handle, 1) instead.
//! 启动 SDK 调度线程，所有协议 I/O 都由该线程驱动。
//! cpress_connect() 成功后会自动启动调度线程，因此本函数可省略，保留是为了向后兼容。
//! 若要启动设备加压动作，请使用 cpress_set_pressing(handle, 1)。
PRESS_C_EXPORT void cpress_run(CPressContext* handle);

//! Stop the SDK scheduler thread and disconnect from the machine.
//! 停止 SDK 调度线程并断开与设备的连接。
PRESS_C_EXPORT void cpress_stop(CPressContext* handle);

//! Query whether the machine is currently running / 查询设备是否正在运行
//! @return 1 if running, 0 otherwise / 运行中返回 1，否则返回 0
PRESS_C_EXPORT int cpress_is_running(const CPressContext* handle);

//! Read the read-only device information as raw JSON / 以原始 JSON 读取只读设备信息
//! @param handle       Active context / 有效的上下文
//! @param isCompressed Whether the request should use compressed JSON mode.
//!                     0 = false, non-zero = true. / 是否使用压缩 JSON 模式。
//! @return Pointer to an internal buffer valid until the next call on the same
//!         handle, or NULL on failure. The caller must copy the data if it
//!         needs to keep it. / 指向内部缓冲区的指针，在下一次对同一句柄调用前
//!         有效；失败返回 NULL。如需保留数据请自行拷贝。
PRESS_C_EXPORT const char* cpress_get_read_only_data_json(CPressContext* handle, int isCompressed);

//! Read the device serial number (CMDID_SERIAL_NUMBER 0x0003) / 读取设备序列号
//! On success the value is also cached into ReadOnlyData.m_SerialNumber.
//! 成功后该值同时缓存到 ReadOnlyData.m_SerialNumber。
//! @param handle   Active context / 有效的上下文
//! @param buf      Output buffer / 输出缓冲区
//! @param bufSize  Output buffer capacity in bytes; the string is truncated to
//!                 fit and always null-terminated. / 缓冲区容量（字节），超出截断并保证以 '\0' 结尾
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_get_serial_number(CPressContext* handle, char* buf, size_t bufSize);

//! Set the per-request timeout in milliseconds (default 300ms) / 设置单次请求超时（毫秒），默认 300ms
//! @param ms Timeout in ms; 0 restores the default; clamped to 60000ms max.
//!           0 恢复默认，上限 60000ms。TCP 弱网环境可适当调大。
PRESS_C_EXPORT void cpress_set_request_timeout(CPressContext* handle, unsigned int ms);

//! Read pressure configuration as raw JSON / 以原始 JSON 读取压力参数
//! @param isCompressed 0 = false, non-zero = true / 0 表示 false，非 0 表示 true
//! @return Internal JSON buffer or NULL on failure / 内部 JSON 缓冲区，失败返回 NULL
PRESS_C_EXPORT const char* cpress_get_press_data_json(CPressContext* handle, int isCompressed);

//! Write pressure configuration from raw JSON / 以原始 JSON 写入压力参数
//! @param json         NUL-terminated PD JSON payload / 以 NUL 结尾的 PD JSON 数据
//! @param isCompressed 0 = false, non-zero = true / 0 表示 false，非 0 表示 true
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_set_press_data_json(CPressContext* handle, const char* json, int isCompressed);

//! Read live machine state as raw JSON / 以原始 JSON 读取实时机器状态
//! @param isCompressed 0 = false, non-zero = true / 0 表示 false，非 0 表示 true
//! @return Internal JSON buffer or NULL on failure / 内部 JSON 缓冲区，失败返回 NULL
PRESS_C_EXPORT const char* cpress_get_real_time_data_json(CPressContext* handle, int isCompressed);

//! Read the complete temperature configuration as raw JSON (TD) / 以原始 JSON 读取全部温度参数
//! @return Internal JSON buffer or NULL on failure / 内部 JSON 缓冲区，失败返回 NULL
PRESS_C_EXPORT const char* cpress_get_temp_data_json(CPressContext* handle, int isCompressed);

//! Write the complete temperature configuration from raw JSON (TD) / 以原始 JSON 写入全部温度参数
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_set_temp_data_json(CPressContext* handle, const char* json, int isCompressed);

//! Read the complete mix-mode configuration as raw JSON (MD) / 以原始 JSON 读取全部混合模式参数
//! @return Internal JSON buffer or NULL on failure / 内部 JSON 缓冲区，失败返回 NULL
PRESS_C_EXPORT const char* cpress_get_mix_data_json(CPressContext* handle, int isCompressed);

//! Write the complete mix-mode configuration from raw JSON (MD) / 以原始 JSON 写入全部混合模式参数
//! @return 1 on success, 0 on failure / 成功返回 1，失败返回 0
PRESS_C_EXPORT int cpress_set_mix_data_json(CPressContext* handle, const char* json, int isCompressed);

//! Start or stop the pressing action only / 启动或停止单独的加压动作
//! @param isPressing   0 = stop, non-zero = start / 0 表示停止，非 0 表示启动
PRESS_C_EXPORT int cpress_set_pressing(CPressContext* handle, int isPressing);

/* ---------------------------------------------------------------------------
 * Pressure single-value / indexed commands
 * 压力单值 / 索引命令
 * All getters return 1 on success and 0 on failure.
 * 所有读取函数成功返回 1，失败返回 0。
 * ------------------------------------------------------------------------- */
PRESS_C_EXPORT int cpress_get_press_step_count(CPressContext* handle, uint8_t* step);
PRESS_C_EXPORT int cpress_set_press_step_count(CPressContext* handle, uint8_t step);
PRESS_C_EXPORT int cpress_get_calibration_value(CPressContext* handle, float* value);
PRESS_C_EXPORT int cpress_set_calibration_value(CPressContext* handle, float value);
PRESS_C_EXPORT int cpress_get_mode_type(CPressContext* handle, uint8_t* type);
PRESS_C_EXPORT int cpress_set_mode_type(CPressContext* handle, uint8_t type);
PRESS_C_EXPORT int cpress_get_circle(CPressContext* handle, float* diameter);
PRESS_C_EXPORT int cpress_set_circle(CPressContext* handle, float diameter);
PRESS_C_EXPORT int cpress_get_rectangle(CPressContext* handle, float* a, float* b);
PRESS_C_EXPORT int cpress_set_rectangle(CPressContext* handle, float a, float b);
PRESS_C_EXPORT int cpress_get_air_time(CPressContext* handle, uint8_t* seconds);
PRESS_C_EXPORT int cpress_set_air_time(CPressContext* handle, uint8_t seconds);
PRESS_C_EXPORT int cpress_get_press_value_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_press_value_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_after_value_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_after_value_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_keep_press_time(CPressContext* handle, uint8_t index, int32_t* ms);
PRESS_C_EXPORT int cpress_set_keep_press_time(CPressContext* handle, uint8_t index, int32_t ms);

/* ---------------------------------------------------------------------------
 * Temperature single-value / indexed commands
 * 温度单值 / 索引命令
 * ------------------------------------------------------------------------- */
PRESS_C_EXPORT int cpress_get_temp_step_count(CPressContext* handle, uint8_t* step);
PRESS_C_EXPORT int cpress_set_temp_step_count(CPressContext* handle, uint8_t step);
PRESS_C_EXPORT int cpress_get_temp_speed_mold(CPressContext* handle, uint8_t* mold);
PRESS_C_EXPORT int cpress_set_temp_speed_mold(CPressContext* handle, uint8_t mold);
PRESS_C_EXPORT int cpress_get_start_temp_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_temp_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_temp_speed_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_temp_speed_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_keep_temp_time(CPressContext* handle, uint8_t index, int32_t* ms);
PRESS_C_EXPORT int cpress_set_keep_temp_time(CPressContext* handle, uint8_t index, int32_t ms);
PRESS_C_EXPORT int cpress_get_max_water_value(CPressContext* handle, float* value);
PRESS_C_EXPORT int cpress_set_max_water_value(CPressContext* handle, float value);
PRESS_C_EXPORT int cpress_get_min_water_value(CPressContext* handle, float* value);
PRESS_C_EXPORT int cpress_set_min_water_value(CPressContext* handle, float value);

/* ---------------------------------------------------------------------------
 * Mixed-mode single-value / indexed commands
 * 混合模式单值 / 索引命令
 * ------------------------------------------------------------------------- */
PRESS_C_EXPORT int cpress_get_mix_step_count(CPressContext* handle, uint8_t* step);
PRESS_C_EXPORT int cpress_set_mix_step_count(CPressContext* handle, uint8_t step);
PRESS_C_EXPORT int cpress_get_mix_mold_type(CPressContext* handle, uint8_t* type);
PRESS_C_EXPORT int cpress_set_mix_mold_type(CPressContext* handle, uint8_t type);
PRESS_C_EXPORT int cpress_get_mix_rectangle(CPressContext* handle, float* a, float* b);
PRESS_C_EXPORT int cpress_set_mix_rectangle(CPressContext* handle, float a, float b);
PRESS_C_EXPORT int cpress_get_mix_circle(CPressContext* handle, float* diameter);
PRESS_C_EXPORT int cpress_set_mix_circle(CPressContext* handle, float diameter);
PRESS_C_EXPORT int cpress_get_mix_is_press_index(CPressContext* handle, uint8_t index, int* isPress);
PRESS_C_EXPORT int cpress_set_mix_is_press_index(CPressContext* handle, uint8_t index, int isPress);
PRESS_C_EXPORT int cpress_get_mix_press_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_mix_press_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_mix_after_press_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_mix_after_press_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_mix_is_heat_up_index(CPressContext* handle, uint8_t index, int* isHeatUp);
PRESS_C_EXPORT int cpress_set_mix_is_heat_up_index(CPressContext* handle, uint8_t index, int isHeatUp);
PRESS_C_EXPORT int cpress_get_mix_heat_up_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_mix_heat_up_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_mix_is_heat_down_index(CPressContext* handle, uint8_t index, int* isHeatDown);
PRESS_C_EXPORT int cpress_set_mix_is_heat_down_index(CPressContext* handle, uint8_t index, int isHeatDown);
PRESS_C_EXPORT int cpress_get_mix_heat_down_index(CPressContext* handle, uint8_t index, float* value);
PRESS_C_EXPORT int cpress_set_mix_heat_down_index(CPressContext* handle, uint8_t index, float value);
PRESS_C_EXPORT int cpress_get_mix_keep_time(CPressContext* handle, uint8_t index, int32_t* ms);
PRESS_C_EXPORT int cpress_set_mix_keep_time(CPressContext* handle, uint8_t index, int32_t ms);

//! Start or stop the demolding action only / 启动或停止单独的脱模动作
//! @param isDemolding  0 = stop, non-zero = start / 0 表示停止，非 0 表示启动
PRESS_C_EXPORT int cpress_set_demolding(CPressContext* handle, int isDemolding);

//! Register callback handlers / 注册回调处理器
//! @param handle     Active context / 有效的上下文
//! @param callbacks  Callback table. Each callback pointer may be NULL when not
//!                   needed. Callbacks are invoked on the SDK worker thread.
//!                   / 回调表。每个回调指针在不需要时都可为 NULL。回调会在 SDK
//!                   工作线程中触发。
//! @param intervalSeconds  Periodic RealTimeData polling interval in seconds;
//!                         <= 0 disables the timer. / 实时数据定时采集周期（秒），
//!                         小于等于 0 时不启动定时采集。
PRESS_C_EXPORT void cpress_register_data_interface(CPressContext* handle, const CPressDataCallbacks* callbacks, int intervalSeconds);

//! Unregister callback handlers / 注销回调处理器
PRESS_C_EXPORT void cpress_unregister_data_interface(CPressContext* handle);

//! Get the SDK version string / 获取 SDK 版本字符串
//! @return Version string. The returned pointer points to internal static storage
//!         and remains valid at least until the next call to cpress_version().
//!         / 版本字符串。返回指针指向内部静态存储区，至少在下一次调用
//!         cpress_version() 前保持有效。
PRESS_C_EXPORT const char* cpress_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PRESS_SDK_PROJECT_PRESS_H */
