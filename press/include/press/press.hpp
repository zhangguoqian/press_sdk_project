/******************************************************************************
 * press.hpp — public SDK API for the press controller and serialization helpers
 * press.hpp — 压片机控制器与序列化辅助函数的公开 SDK 接口
 *
 *  This header is the main public entry point for application code that uses the
 *  SDK. It exposes the high-level Press controller and the JSON/frame conversion
 *  helpers required for reading and writing machine parameters.
 *  该头文件是应用代码使用 SDK 的主入口，公开了高层 Press 控制器，以及
 *  读取和写入机器参数所需的 JSON / 帧数据转换辅助函数。
 *
 *  Core contents / 主要内容:
 *    - Press class / Press 类: manages the communication port, background worker,
 *      and synchronous device I/O; it also supports asynchronous callbacks via
 *      PressDataInterface.
 *      管理通信端口、后台工作线程和同步设备 I/O；同时支持通过
 *      PressDataInterface 进行异步回调。
 *    - Free functions / 自由函数: convert between structured data and wire-format
 *      payloads used by the device protocol.
 *      实现结构体与设备协议帧/JSON 之间的相互转换。
 *
 *  Build configuration / 构建配置:
 *    - PRESS_BUILDING_LIBRARY: defined when compiling the shared library itself
 *      构建动态库时定义
 *    - PRESS_EXPORT: exported/imported symbol macro; on Windows it resolves to
 *      __declspec(dllexport/dllimport)
 *      导出/导入符号宏；在 Windows 上解析为 __declspec(dllexport/dllimport)
 *
 *  Dependencies / 依赖:
 *    - presstype.h / presstype.h: public data structures and command constants
 *      公共数据结构和命令常量
 *    - <memory>, <string>, <vector> / C++11 标准库头文件
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

#include <memory>
#include <string>
#include <vector>

class PressPrivate;


/******************************************************************************
 * PressDataInterface — Asynchronous data callback interface / 异步数据回调接口
 *
 *  This interface is part of the public Press SDK header so C++ applications can
 *  derive from it without depending on a separate auxiliary header file.
 *  该接口位于公共 Press SDK 头文件中，因此 C++ 应用无需依赖单独的辅助头文件即可派生实现。
 *
 *  Application code can derive from PressDataInterface and register the object
 *  with Press::registerDataInterface().
 *  应用代码可以从 PressDataInterface 派生，并通过 Press::registerDataInterface()
 *  注册回调对象。
 *****************************************************************************/
class PressDataInterface
{
public:
    virtual ~PressDataInterface() = default;

    //! Read-only data callback / 只读数据回调
    virtual void onReadOnlyData(int errorCode, uint64_t registerNo,
                                const ReadOnlyData& readOnlyData) = 0;

    //! Real-time data callback / 实时数据回调
    virtual void onRealTimeData(int errorCode, const RealTimeData& realTimeData) = 0;

    //! Pressure parameter callback / 压力参数回调
    virtual void onPressData(int errorCode, const PressData& pressData) = 0;

    //! Error or unhandled response callback / 错误或未处理响应回调
    virtual void onError(uint16_t cmdCode, std::vector<uint8_t> response) = 0;
};


/******************************************************************************
 * Press — High-level controller for a press machine / 压片机高级控制器
 *
 *  本库的主要入口类。Press 管理通信端口（串口或 TCP）、运行内部调度线程，
 *  提供同步 API 读写机器状态，并通过 PressDataInterface 支持异步回调。
 *
 *  Thread safety / 线程安全:
 *    - 所有公共方法都是线程安全的，可从任意线程调用。
 *    - 通过 PressDataInterface 注册的回调在内部调度线程中执行，
 *      耗时操作应移交给其他线程处理。
 *****************************************************************************/

class PRESS_EXPORT Press
{
public:
    //! Constructor / 构造函数
    Press();

    Press(const Press& other) = delete;
    Press& operator=(const Press& other) = delete;
    Press(Press&& other) = delete;
    Press& operator=(Press&& other) = delete;

    //! Destructor — disconnects and joins all threads / 析构函数 — 断开连接并回收所有线程
    ~Press();


    /**************************************************************************
     * Connection management / 连接管理
     *************************************************************************/

    //! Open a communication port and connect to the machine.
    //! @param portName  Serial port name (e.g. "COM3") or TCP host:port.
    //! @param portType  SerialPortType or TcpSocketPortType.
    //! @return true on success, false on failure (call getLastErrorInfo()).
    //! 打开通信端口并连接到机器。
    bool connect(const char* portName, PressPortType portType);

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
    const char* getLastErrorInfo();


    /**************************************************************************
     * Run control / 运行控制
     *************************************************************************/

    //! Start the SDK scheduler thread that drives all protocol I/O.
    //! After connect() succeeds the scheduler is started automatically,
    //! so calling run() is optional. Kept for backward compatibility.
    //! To start the device pressing action, use setPressing(true) instead.
    //! 启动 SDK 调度线程，所有协议 I/O 都由该线程驱动。
    //! connect() 成功后会自动启动调度线程，因此 run() 可省略，保留是为了向后兼容。
    //! 若要启动设备加压动作，请使用 setPressing(true)。
    void run();

    //! Stop the SDK scheduler thread and disconnect from the machine.
    //! 停止 SDK 调度线程并断开与机器的连接。
    void stop();

    //! Query whether the machine is currently in run state.
    //! @return true if running / 是否正在运行。
    bool isRunning() const;

    //! Set the per-request timeout in milliseconds (default 300ms).
    //! @param ms Timeout in ms; 0 restores the default; clamped to 60000ms max.
    //!           Weak TCP links may need a larger value than the serial default.
    //! 设置单次请求超时（毫秒），默认 300ms；0 恢复默认，上限 60000ms。
    //! TCP 弱网环境下可适当调大。
    void setRequestTimeout(unsigned int ms);


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
    bool getReadOnlyDataJson(ReadOnlyData& data, bool isCompressed = false);

    //! Read the device serial number (CMDID_SERIAL_NUMBER 0x0003).
    //! On success the value is also cached into ReadOnlyData.m_SerialNumber.
    //! @param serialNumber [out] Decoded serial number on success.
    //! @return true on success / 读取设备序列号。
    bool getSerialNumber(std::string& serialNumber);

    //! Read the current pressure control parameters.
    //! @param data         [out] Decoded PressData on success.
    //! @param isCompressed [in]  Request JSON-compressed response from device.
    //! @return true on success / 读取当前压力控制参数。
    bool getPressDataJson(PressData& data, bool isCompressed = false);

    //! Write pressure control parameters to the device.
    //! @param data         [in]  New pressure parameters to send.
    //! @param isCompressed [in]  Encode the payload as JSON before sending.
    //! @return true on success / 写入压力控制参数到设备。
    bool setPressDataJson(const PressData& data, bool isCompressed = false);

    //! Read live real-time machine state.
    //! @param data         [out] Decoded RealTimeData on success.
    //! @param isCompressed [in]  Request JSON-compressed response from device.
    //! @return true on success / 读取实时机器状态。
    bool getRealTimeDataJson(RealTimeData& data, bool isCompressed = false);


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
     * Device info & parameters / 设备信息与参数块
     *************************************************************************/

    //! Read device name (CMDID_NAME). language: 0=Chinese, 2=English.
    //! 读取设备名称。成功后缓存到 ReadOnlyData.m_NameZH / m_NameEN。
    bool getName(std::string& name, uint8_t language = 0);

    //! Read machine type string (CMDID_MACHINE_TYPE). Cached to ReadOnlyData.m_Type.
    //! 读取机型字符串，成功后缓存到 ReadOnlyData.m_Type。
    bool getMachineType(std::string& type);

    //! Read the pressure parameter block (CMDID_PRESS_PARAMETER, 20 bytes).
    //! 读取压力参数块，成功后同步缓存到 ReadOnlyData。
    bool getPressParameter(PressParameter& param);

    //! Read the temperature parameter block (CMDID_TEMP_PARAMETER, 12 bytes).
    //! 读取温度参数块，成功后同步缓存到 ReadOnlyData。
    bool getTempParameter(TempParameter& param);

    //! Read misc info block (CMDID_OTHER_INFO, 5 bytes). Cached to ReadOnlyData.
    //! 读取其他信息块，成功后同步缓存到 ReadOnlyData。
    bool getOtherInfo(OtherInfo& info);

    //! Read one pressure step parameter (CMDID_INDEX_PSTEP_PARAMETER, index + 12 bytes).
    //! 读取压力段参数（索引 + 12 字节）。
    bool getPressStepParameter(uint8_t index, PressStepParameter& param);

    //! Write one pressure step parameter / 写入压力段参数。
    bool setPressStepParameter(uint8_t index, const PressStepParameter& param);

    //! Read one temperature step parameter (CMDID_INDEX_TSTEP_PARAMETER, index + 12 bytes).
    //! 读取温度段参数（索引 + 12 字节）。
    bool getTempStepParameter(uint8_t index, TempStepParameter& param);

    //! Write one temperature step parameter / 写入温度段参数。
    bool setTempStepParameter(uint8_t index, const TempStepParameter& param);

    //! Read one mixed-mode step parameter (CMDID_MIX_INDEX_STEP_PARAMETER).
    //! Note: firmware length check macro is 23 while the actual layout is 24
    //! bytes (index + 23); if firmware rejects the write, fix the macro there.
    //! 读取混合模式段参数。注意：下位机校验宏为 23 而实际布局为 24 字节，
    //! 疑似固件 off-by-one；如写入被拒需修正固件宏。
    bool getMixStepParameter(uint8_t index, MixStepParameter& param);

    //! Write one mixed-mode step parameter / 写入混合模式段参数。
    bool setMixStepParameter(uint8_t index, const MixStepParameter& param);


    /**************************************************************************
     * System commands / 系统命令
     *************************************************************************/

    //! Read device date & time (CMDID_DATE_TIME). 读取设备日期时间。
    bool getDateTime(DateTimeInfo& dateTime);

    //! Write device date & time. Layout [sec][min][hour][day][month][year].
    //! 写入设备日期时间，布局 [秒][分][时][日][月][年]。
    bool setDateTime(const DateTimeInfo& dateTime);

    //! Read screen brightness (CMDID_BRIGHTNESS). 读取屏幕亮度。
    bool getBrightness(uint8_t& brightness);

    //! Write screen brightness / 设置屏幕亮度。
    bool setBrightness(uint8_t brightness);

    //! Read key beep enable (CMDID_BUTTON_SOUND). 读取按键提示音开关。
    bool getButtonSound(bool& enabled);

    //! Write key beep enable / 设置按键提示音开关。
    bool setButtonSound(bool enabled);

    //! Read UI language (CMDID_LANGUAGE). 读取界面语言。
    bool getLanguage(uint8_t& language);

    //! Write UI language / 设置界面语言。
    bool setLanguage(uint8_t language);


    /**************************************************************************
     * State & control commands / 状态与控制命令
     *
     *  说明：SET_START_P (0x1531) / SET_START_TU (0x1532) / SET_START_TD (0x1533)
     *  与 GET_GPIO4_STATE (0x05A4) 当前固件未实现（发超时）；MIX_RING_OUT_IN
     *  (0x0304) 固件已注释，以上命令暂不提供封装。
     *************************************************************************/

    //! Read current machine state (CMDID_GET_CURRENT_STATE, 3-5 bytes by mode).
    //! 读取机器当前状态（响应长度依模式为 3/4/5 字节，见 MachineState）。
    bool getMachineState(MachineState& state);

    //! Start or stop heating (CMDID_SET_START_HEAT). 启动或停止加热。
    bool setStartHeat(bool isHeating);

    //! Start or stop everything (CMDID_SET_START). 启动或停止全部（加压+加热）。
    bool setStart(bool isStart);

    //! Select work mode (CMDID_SELECT_MODEL_STATE, value 1/2). 选择工作模式。
    bool selectModelState(uint8_t model);


    /**************************************************************************
     * JSON bulk commands (raw string form) / JSON 批量命令（字符串形式）
     *
     *  以下接口直接传输 JSON 字节流，仅支持未压缩模式（下位机响应总是未压缩）。
     *  温度/混合模式参数的强类型读写可通过上述索引命令完成。
     *************************************************************************/

    //! Read all temperature parameters as raw JSON (GET_TD_JSON).
    //! 以原始 JSON 字符串读取全部温度参数。
    bool getTempDataJson(std::string& json);

    //! Write all temperature parameters from raw JSON (SET_TD_JSON).
    //! 以原始 JSON 字符串写入全部温度参数。
    bool setTempDataJson(const std::string& json);

    //! Read all mixed-mode parameters as raw JSON (GET_MD_JSON).
    //! 以原始 JSON 字符串读取全部混合模式参数。
    bool getMixDataJson(std::string& json);

    //! Write all mixed-mode parameters from raw JSON (SET_MD_JSON).
    //! 以原始 JSON 字符串写入全部混合模式参数。
    bool setMixDataJson(const std::string& json);


    /**************************************************************************
     * Asynchronous callback registration / 异步回调注册
     *************************************************************************/

    //! Register an asynchronous data callback interface.
    //! Only one interface may be registered at a time; a second call replaces
    //! the previous one (unregistering is not required first).
    //! @param dataInterface   Non-owning pointer to the implementation.
    //! @param intervalSeconds Periodic RealTimeData polling interval in
    //!                        seconds. When > 0, a timer thread is started
    //!                        that fetches RealTimeData every intervalSeconds
    //!                        seconds and reports it via onRealTimeData().
    //!                        When <= 0, no timer is started.
    //! 注册异步数据回调接口。同一时间只能注册一个，重复注册将覆盖旧的。
    //! @param intervalSeconds 定时获取 RealTimeData 的周期，单位秒。
    //!                        大于 0 时启动定时线程，循环获取 RealTimeData
    //!                        并通过 onRealTimeData() 上报；小于等于 0 时不启动。
    void registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds = 1);

    //! Unregister the previously registered callback interface.
    //! 注销之前注册的回调接口。
    void unregisterDataInterface();


    /**************************************************************************
     * Static utilities / 静态工具
     *************************************************************************/

    //! Enumerate available serial ports on the current system.
    //! Returns COM-style list on Windows, /dev/tty* style on Linux/macOS.
    //! TCP endpoints cannot be enumerated; use connect() with "host:port" directly.
    //! @return Vector of port name strings.
    //! 枚举当前系统可用的串口。
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
    std::unique_ptr<PressPrivate> mpPrivate;
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
