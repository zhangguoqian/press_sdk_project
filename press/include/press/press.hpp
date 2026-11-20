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

#include <cstdint>
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

    //! Read-only data callback (ROD JSON) / 只读数据回调（ROD JSON）
    //! @param errorCode  0 on success / 0 表示成功
    //! @param registerNo Device register number / 设备注册号
    //! @param json       Raw ROD JSON payload, empty on failure / 原始 ROD JSON 数据，失败时为空字符串
    virtual void onReadOnlyData(int errorCode, uint64_t registerNo, const std::string& json) = 0;

    //! Real-time data callback (RT JSON) / 实时数据回调（RT JSON）
    //! @param errorCode 0 on success / 0 表示成功
    //! @param json      Raw RT JSON payload, empty on failure / 原始 RT JSON 数据，失败时为空字符串
    virtual void onRealTimeData(int errorCode, const std::string& json) = 0;

    //! Pressure parameter callback (PD JSON) / 压力参数回调（PD JSON）
    //! @param errorCode 0 on success / 0 表示成功
    //! @param json      Raw PD JSON payload, empty on failure / 原始 PD JSON 数据，失败时为空字符串
    virtual void onPressData(int errorCode, const std::string& json) = 0;

    //! Temperature data callback (TD JSON) / 温度数据回调（TD JSON）
    //! @param errorCode 0 on success / 0 表示成功
    //! @param json      Raw TD JSON payload / 原始 TD JSON 数据
    virtual void onTempData(int errorCode, const std::string& json) = 0;

    //! Mix data callback (MD JSON) / 混合数据回调（MD JSON）
    //! @param errorCode 0 on success / 0 表示成功
    //! @param json      Raw MD JSON payload / 原始 MD JSON 数据
    virtual void onMixData(int errorCode, const std::string& json) = 0;

    //! Error or unhandled response callback / 错误或未处理响应回调
    virtual void onError(uint16_t cmdCode, std::vector<uint8_t> response) = 0;
};


/******************************************************************************
 * Press — High-level controller for a press machine / 压片机高级控制器
 *
 *  本库的主要入口类。Press 管理通信端口（串口或 TCP）、运行内部调度线程，
 *  提供同步 API 读写机器状态，并通过 PressDataInterface 支持异步回调。
 *
 *  Return & index conventions / 返回与索引约定:
 *    - Every bool method returns true on success and false on failure; call
 *      getLastErrorInfo() to obtain a readable reason after a failure.
 *    - Output parameters (json / structs) are only meaningful when the method
 *      returns true.
 *    - Step indices are 0-based and must be smaller than the current step count.
 *    - 所有 bool 接口成功返回 true、失败返回 false；失败后可通过
 *      getLastErrorInfo() 获取可读原因。
 *    - 输出参数（json / 结构体）仅在接口返回 true 时有效。
 *    - 段索引从 0 开始，且必须小于当前段数。
 *
 *  Thread safety / 线程安全:
 *    - All public methods are thread-safe and may be called from any thread.
 *    - Timer callbacks (intervalSeconds > 0) run on the SDK timer thread;
 *      callbacks caused by a synchronous get*Json call run on the calling
 *      thread. Move time-consuming work to another thread if needed.
 *    - Lifecycle methods such as disconnect()/stop()/unregisterDataInterface()
 *      may be called from a callback, but the Press object itself must not be
 *      destroyed inside a callback.
 *    - 所有公共方法都是线程安全的，可从任意线程调用。
 *    - 定时采集（intervalSeconds > 0）触发的回调在 SDK 定时线程执行；主动调用
 *      get*Json 触发的回调在调用方线程同步执行，耗时操作应移交给其他线程。
 *    - 回调中可安全调用 disconnect()/stop()/unregisterDataInterface() 等生命周期
 *      接口；但不要在回调中析构 Press 对象本身。
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

    //! Get the device firmware version cached during the connect handshake.
    //! This returns the cached m_JsonData.m_VersionInfo (CMDID_VERSION 0x0000
    //! negotiated when connect() succeeds) without issuing a new request.
    //! @param versionInfo [out] Cached firmware version on success.
    //! @return true if connected and version info is available, false otherwise.
    //! 获取连接握手时缓存的设备固件版本 (m_JsonData.m_VersionInfo)，
    //! 不发起新的请求；未连接时返回 false。
    bool getDeviceVersionInfo(VersionInfo& versionInfo) const;

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


    //! Register an asynchronous data callback interface.
    //! Only one interface may be registered at a time; a second call replaces
    //! the previous one (calling unregister first is optional).
    //! @param dataInterface   Non-owning pointer. The caller must keep the
    //!                        object alive until unregisterDataInterface()
    //!                        returns, and must not destroy it while another
    //!                        thread is inside a synchronous get*Json call.
    //! @param intervalSeconds Periodic RealTimeData polling interval in
    //!                        seconds. When > 0, a timer thread fetches
    //!                        RealTimeData every intervalSeconds seconds and
    //!                        reports it via onRealTimeData(). When <= 0, no
    //!                        timer is started.
    //! 注册异步数据回调接口。同一时间只能注册一个，重复注册将覆盖旧的。
    //! @param dataInterface   非拥有指针。调用方必须保证对象在
    //!                        unregisterDataInterface() 返回前存活，且不得在其
    //!                        他线程执行同步 get*Json 期间析构该对象。
    //! @param intervalSeconds 定时获取 RealTimeData 的周期（秒）。
    //!                        大于 0 时启动定时线程并通过 onRealTimeData()
    //!                        上报；小于等于 0 时不启动定时采集。
    void registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds = 1);

    //! Register a callback interface with shared ownership.
    //! The SDK keeps a shared_ptr, so callbacks already in flight can safely
    //! finish after unregistration.
    //! @param dataInterface   Shared callback object; ownership is shared with
    //!                        the SDK while registered.
    //! @param intervalSeconds Same semantics as registerDataInterface().
    //! 以共享所有权方式注册回调接口：SDK 持有 shared_ptr，注销后仍在途的
    //! 回调可安全执行。
    //! @param dataInterface   共享回调对象；注册期间与 SDK 共享所有权。
    //! @param intervalSeconds 与 registerDataInterface() 相同。
    void registerDataInterfaceOwned(std::shared_ptr<PressDataInterface> dataInterface, int intervalSeconds = 1);

    //! Unregister the previously registered callback interface.
    //! Stops the timer thread and waits for the current timer callback to finish.
    //! 注销之前注册的回调接口；会停止定时线程并等待当前定时回调结束。
    void unregisterDataInterface();


    /**************************************************************************
     * JSON data exchange / JSON 数据接口
     *
     *  These methods exchange raw JSON strings with the device. Payload codes:
     *    ROD = read-only device info, PD = pressure profile,
     *    RT  = live machine state,      TD = temperature profile,
     *    MD  = mixed-mode profile.
     *  isCompressed selects the device-side compressed JSON format for bulk
     *  data; both get and set forward the flag to the firmware.
     *  以下 get/set 方法直接以原始 JSON 字符串与设备交换数据：
     *    ROD = 只读设备信息，PD = 压力参数，RT = 实时状态，
     *    TD = 温度参数，MD = 混合模式参数。
     *  isCompressed 选择设备端压缩 JSON 格式，读写时都会把该标志传给固件。
     *
     *  Callback semantics / 回调语义:
     *    - getReadOnlyDataJson / getPressDataJson / getTempDataJson /
     *      getMixDataJson trigger the matching on* callback synchronously.
     *    - getRealTimeDataJson triggers onRealTimeData only when periodic
     *      polling is disabled; with polling enabled, the timer reports it to
     *      avoid duplicate delivery.
     *    - 主动调用 ROD/PD/TD/MD 的 get 接口会同步触发对应回调；
     *      getRealTimeDataJson 在未启用定时采集时触发回调，启用后由定时线程
     *      统一上报，避免同一份实时数据重复上报。
     *************************************************************************/

    //! Read static (read-only) device information as ROD JSON.
    //! @param json         [out] Raw ROD JSON on success; empty on failure.
    //! @param isCompressed [in]  Request compressed JSON payload.
    //! @return true on success / 读取只读设备信息（ROD JSON）。
    bool getReadOnlyDataJson(std::string& json, bool isCompressed = false);

    //! Read pressure control parameters as PD JSON.
    //! @param json         [out] Raw PD JSON on success; empty on failure.
    //! @param isCompressed [in]  Request compressed JSON payload.
    //! @return true on success / 读取压力控制参数（PD JSON）。
    bool getPressDataJson(std::string& json, bool isCompressed = false);

    //! Write pressure control parameters from PD JSON.
    //! @param json         [in]  Raw PD JSON payload, NUL not required.
    //! @param isCompressed [in]  Use the compressed JSON format.
    //! @return true on success / 写入压力控制参数（PD JSON）。
    bool setPressDataJson(const std::string& json, bool isCompressed = false);

    //! Read live real-time machine state as RT JSON.
    //! @param json         [out] Raw RT JSON on success; empty on failure.
    //! @param isCompressed [in]  Request compressed JSON payload.
    //! @return true on success / 读取实时机器状态（RT JSON）。
    //! @note With periodic polling enabled (intervalSeconds > 0), this call
    //!       does not trigger onRealTimeData; the timer reports it instead.
    //!       启用定时采集后，本接口不再触发 onRealTimeData，由定时器统一上报。
    bool getRealTimeDataJson(std::string& json, bool isCompressed = false);

    //! Read all temperature parameters as TD JSON.
    //! @param json         [out] Raw TD JSON on success; empty on failure.
    //! @param isCompressed [in]  Request compressed JSON payload.
    //! @return true on success / 读取全部温度参数（TD JSON）。
    bool getTempDataJson(std::string& json, bool isCompressed = false);

    //! Write all temperature parameters from TD JSON.
    //! @param json         [in]  Raw TD JSON payload.
    //! @param isCompressed [in]  Use the compressed JSON format.
    //! @return true on success / 写入全部温度参数（TD JSON）。
    bool setTempDataJson(const std::string& json, bool isCompressed = false);

    //! Read all mixed-mode parameters as MD JSON.
    //! @param json         [out] Raw MD JSON on success; empty on failure.
    //! @param isCompressed [in]  Request compressed JSON payload.
    //! @return true on success / 读取全部混合模式参数（MD JSON）。
    bool getMixDataJson(std::string& json, bool isCompressed = false);

    //! Write all mixed-mode parameters from MD JSON.
    //! @param json         [in]  Raw MD JSON payload.
    //! @param isCompressed [in]  Use the compressed JSON format.
    //! @return true on success / 写入全部混合模式参数（MD JSON）。
    bool setMixDataJson(const std::string& json, bool isCompressed = false);


    /**************************************************************************
     * Read-only data exchange / 只读数据接口
     *
     *  These commands read device identity and limits. Successful results are
     *  also cached inside the SDK for internal use.
     *  以下命令读取设备身份与限制信息，成功结果同时缓存在 SDK 内部。
     *************************************************************************/

    //! Read the device name (CMDID_NAME).
    //! @param name     [out] UTF-8 device name on success.
    //! @param language Language selector: 0 = Chinese, 2 = English.
    //! @return true on success / 读取设备名称；language: 0 中文，2 英文。
    bool getName(std::string& name, uint8_t language = 0);

    //! Read the machine type string (CMDID_MACHINE_TYPE).
    //! @param type [out] Machine model string on success.
    //! @return true on success / 读取机型字符串。
    bool getMachineType(std::string& type);

    //! Read the device serial number (CMDID_SERIAL_NUMBER 0x0003).
    //! @param serialNumber [out] Decoded serial number on success.
    //! @return true on success / 读取设备序列号。
    bool getSerialNumber(std::string& serialNumber);

    //! Read the pressure parameter block (CMDID_PRESS_PARAMETER, 21 bytes).
    //! @param param [out] Parsed pressure limits and display options.
    //! @return true on success / 读取压力参数块（21 字节）。
    bool getPressParameter(PressParameter& param);

    //! Read the temperature parameter block (CMDID_TEMP_PARAMETER, 12 bytes).
    //! @param param [out] Parsed temperature limits and options.
    //! @return true on success / 读取温度参数块（12 字节）。
    bool getTempParameter(TempParameter& param);

    //! Read the miscellaneous info block (CMDID_OTHER_INFO, 5 bytes).
    //! @param info [out] Parsed screenshot/delay/language/remote/version flags.
    //! @return true on success / 读取其他信息块（5 字节）。
    bool getOtherInfo(OtherInfo& info);

    /**************************************************************************
     * Press data exchange / 压力数据段参数接口
     *
     *  All indexed getters/setters return true on success and false on failure.
     *  `index` is 0-based and must be smaller than getPressStepCount().
     *  Arrays in PressData use MAX_PStep entries; only the first m_PStep entries
     *  are meaningful to the device.
     *  所有索引读写接口成功返回 true、失败返回 false；index 从 0 开始且必须
     *  小于 getPressStepCount()。PressData 中数组长度固定为 MAX_PStep，设备
     *  仅使用前 m_PStep 项。
     *************************************************************************/
    
    //! Read pressure step count (CMDID_PRESS_STEP, 1 byte).
    //! 读取压力段数量。
    bool getPressStepCount(uint8_t& step);

    //! Write pressure step count (CMDID_PRESS_STEP with SET bit).
    //! 写入压力段数量。
    bool setPressStepCount(uint8_t step);

    //! Read pressure calibration value (CMDID_PRESS_CALIBRATE, 4-byte float).
    //! 读取压力校准值。
    bool getCalibrationValue(float& value);

    //! Write pressure calibration value / 写入压力校准值。
    bool setCalibrationValue(float value);

    //! Read mold type (CMDID_MOLD_TYPE, 1 byte).
    //! 读取模具类型（压力段）。
    bool getMoldType(uint8_t& type);

    //! Write mold type / 写入模具类型（压力段）。
    bool setMoldType(uint8_t type);

    //! Read circular mold diameter (CMDID_CIRCLE_D, 4-byte float).
    //! 读取圆形容器直径。
    bool getCircle(float& diameter);

    //! Write circular mold diameter / 写入圆形容器直径。
    bool setCircle(float diameter);

    //! Read rectangular mold A/B dimensions (CMDID_RECT_AB, two floats).
    //! 读取矩形容器 A/B 尺寸。
    bool getRectangle(float& a, float& b);

    //! Write rectangular mold A/B dimensions / 写入矩形容器 A/B 尺寸。
    bool setRectangle(float a, float b);

    //! Read air (exhaust) time (CMDID_AIR_TIME, 1 byte, seconds).
    //! 读取排气时间（秒）。
    bool getAirTime(uint8_t& time);

    //! Write air (exhaust) time / 写入排气时间（秒）。
    bool setAirTime(uint8_t time);

    //! Read the target pressure of one pressure step (CMDID_INDEX_PRESS).
    //! 读取指定压力段的设定压力。
    bool getPressValueIndex(uint8_t index, float& value);

    //! Write the target pressure of one pressure step / 写入指定压力段的设定压力。
    bool setPressValueIndex(uint8_t index, float value);

    //! Read the hold pressure of one pressure step (CMDID_INDEX_AFTER).
    //! 读取指定压力段的保压值。
    bool getAfterValueIndex(uint8_t index, float& value);

    //! Write the hold pressure of one pressure step / 写入指定压力段的保压值。
    bool setAfterValueIndex(uint8_t index, float value);

    //! Read the keep-pressure time of one pressure step (CMDID_INDEX_KPTIME, int32 ms).
    //! 读取指定压力段的保压时间（毫秒）。
    bool getKeepPressTime(uint8_t index, int32_t& time);

    //! Write the keep-pressure time of one pressure step / 写入指定压力段的保压时间（毫秒）。
    bool setKeepPressTime(uint8_t index, int32_t time);

    //! Read one pressure step parameter (CMDID_INDEX_PSTEP_PARAMETER, index + 12 bytes).
    //! 读取压力段参数（索引 + 12 字节）。
    bool getPressStepParameter(uint8_t index, PressStepParameter& param);

    //! Write one pressure step parameter / 写入压力段参数。
    bool setPressStepParameter(uint8_t index, const PressStepParameter& param);

    /**************************************************************************
     * Temperature data exchange / 温度数据段参数接口
     *
     *  Same conventions as the pressure section: 0-based indices, bool result,
     *  and step counts bounded by getTempStepCount().
     *  约定与压力段接口一致：索引从 0 开始、返回 bool，段数上限由
     *  getTempStepCount() 给出。
    *************************************************************************/

    //! Read temperature step count (CMDID_TEMP_STEP, 1 byte).
    //! 读取温度段数量。    
    bool getTempStepCount(uint8_t& step);

    //! Write temperature step count (CMDID_TEMP_STEP with SET bit).
    //! 写入温度段数量。
    bool setTempStepCount(uint8_t step);

    //! Read temperature speed mold (CMDID_SPEED_MOLD, 1 byte).
    //! 读取温度速度模式。
    bool getTempSpeedMold(uint8_t& mold);

    //! Write temperature speed mold (CMDID_SPEED_MOLD with SET bit).
    //! 写入温度速度模式。
    bool setTempSpeedMold(uint8_t mold);

    //! Read temperature value of one step (CMDID_INDEX_TEMP, index + 4-byte float).
    //! 读取指定温度段的温度值。
    bool getStartTempIndex(uint8_t index, float& value);
    
    //! Write temperature value of one step (CMDID_INDEX_TEMP with SET bit).
    //! 写入指定温度段的温度值。
    bool setTempIndex(uint8_t index, float value);

    //! Read speed value of one step (CMDID_INDEX_SPEED, index + 4-byte float).
    //! 读取指定温度段的速度值。
    bool getTempSpeedIndex(uint8_t index, float& value);
    
    //! Write speed value of one step (CMDID_INDEX_SPEED with SET bit).
    //! 写入指定温度段的速度值。
    bool setTempSpeedIndex(uint8_t index, float value);

    //! Read keep-temperature time of one temperature step (CMDID_INDEX_KTTIME, int32 ms).
    //! 读取指定温度段的保温时间（毫秒）。
    bool getKeepTempTime(uint8_t index, int32_t& time);
    
    //! Write keep-temperature time of one temperature step (CMDID_INDEX_KTTIME with SET bit).
    //! 写入指定温度段的保温时间（毫秒）。
    bool setKeepTempTime(uint8_t index, int32_t time);

    //! Read max water value (CMDID_WATER_UP, 4-byte float).
    //! 读取最大水冷。
    bool getMaxWaterValue(float& value);
    
    //! Write max water value (CMDID_WATER_UP with SET bit).
    //! 写入最大水冷。
    bool setMaxWaterValue(float value);

    //! Read min water value (CMDID_WATER_DOWN, 4-byte float).
    //! 读取最小水冷。
    bool getMinWaterValue(float& value);
    
    //! Write min water value (CMDID_WATER_DOWN with SET bit).
    //! 写入最小水冷。
    bool setMinWaterValue(float value);
    
    //! Read one temperature step parameter (CMDID_INDEX_TSTEP_PARAMETER, index + 12 bytes).
    //! 读取温度段参数（索引 + 12 字节）。
    bool getTempStepParameter(uint8_t index, TempStepParameter& param);

    //! Write one temperature step parameter / 写入温度段参数。
    bool setTempStepParameter(uint8_t index, const TempStepParameter& param);

    
    /**************************************************************************
     * Mixed-mode data exchange / 混合模式数据段参数接口
     *
     *  Same conventions as the pressure section: 0-based indices, bool result,
     *  and step counts bounded by getMixStepCount().
     *  约定与压力段接口一致：索引从 0 开始、返回 bool，段数上限由
     *  getMixStepCount() 给出。
    *************************************************************************/
    
    //! Read mixed-mode step count (CMDID_MIX_STEP, 1 byte).
    //! 读取混合模式段数量。
    bool getMixStepCount(uint8_t& step);

    //! Write mixed-mode step count (CMDID_MIX_STEP with SET bit).
    //! 写入混合模式段数量。
    bool setMixStepCount(uint8_t step);

    //! Read mixed-mode mold type (CMDID_MIX_MOLD_TYPE, 1 byte).
    //! 读取混合模式模具类型。
    bool getMixMoldType(uint8_t& type);
    
    //! Write mixed-mode mold type (CMDID_MIX_MOLD_TYPE with SET bit).
    //! 写入混合模式模具类型。
    bool setMixMoldType(uint8_t type);

    //! Read mixed-mode rectangle A/B (CMDID_MIX_RECT_AB, 2 x float32).
    //! 读取混合模式矩形尺寸 A/B。
    bool getMixRectangle(float& a,float& b);

    //! Write mixed-mode rectangle A/B / 写入混合模式矩形尺寸 A/B。
    bool setMixRectangle(float a,float b);

    //! Read mixed-mode circle diameter (CMDID_MIX_CIRCLE_D, float32).
    //! @param diameter [out] Circle diameter on success.
    //! 读取混合模式圆形容器直径。
    bool getMixCircle(float& diameter);

    //! Write mixed-mode circle diameter.
    //! @param diameter Circle diameter to write.
    //! 写入混合模式圆形容器直径。
    bool setMixCircle(float diameter);

    //! Read per-step is-press flag (CMDID_MIX_INDEX_ISPRESS, index + 1 byte).
    //! 读取指定段是否加压。
    bool getMixIsPressIndex(uint8_t index, bool& isPress);

    //! Write per-step is-press flag / 写入指定段是否加压。
    bool setMixIsPressIndex(uint8_t index, bool isPress);

    //! Read per-step pressure value (CMDID_MIX_INDEX_PRESS, index + float32).
    //! 读取指定段压力值。
    bool getMixPressIndex(uint8_t index, float& value);

    //! Write per-step pressure value / 写入指定段压力值。
    bool setMixPressIndex(uint8_t index, float value);

    //! Read per-step hold-pressure value (CMDID_MIX_INDEX_AFTER, index + float32).
    //! 读取指定段保压值。
    bool getMixAfterPressIndex(uint8_t index, float& value);
    
    //! Write per-step hold-pressure value / 写入指定段保压值。
    bool setMixAfterPressIndex(uint8_t index, float value);

    //! Read per-step heat-up flag (CMDID_MIX_INDEX_ISHEAT_UP, index + 1 byte).
    //! 读取指定段是否加热。
    bool getMixIsHeatUpIndex(uint8_t index, bool& isHeatUp);

    //! Write per-step heat-up flag / 写入指定段是否加热。
    bool setMixIsHeatUpIndex(uint8_t index, bool isHeatUp);

    //! Read per-step heat-up temperature (CMDID_MIX_INDEX_TEMP_UP, index + float32).
    //! 读取指定段加热温度。
    bool getMixHeatUpIndex(uint8_t index, float& value);
    
    //! Write per-step heat-up temperature / 写入指定段加热温度。
    bool setMixHeatUpIndex(uint8_t index, float value);

    //! Read per-step heat-down flag (CMDID_MIX_INDEX_ISHEAT_DOWN, index + 1 byte).
    //! 读取指定段是否冷却。
    bool getMixIsHeatDownIndex(uint8_t index, bool& isHeatDown);
    
    //! Write per-step heat-down flag / 写入指定段是否冷却。
    bool setMixIsHeatDownIndex(uint8_t index, bool isHeatDown);

    //! Read per-step heat-down temperature (CMDID_MIX_INDEX_TEMP_DOWN, index + float32).
    //! 读取指定段冷却温度。
    bool getMixHeatDownIndex(uint8_t index, float& value);
    
    //! Write per-step heat-down temperature / 写入指定段冷却温度。
    bool setMixHeatDownIndex(uint8_t index, float value);

    //! Read per-step keep time (CMDID_MIX_INDEX_KTIME, index + int32 ms).
    //! 读取指定段保持时间（毫秒）。
    bool getMixKeepTime(uint8_t index, int32_t& time);
    
    //! Write per-step keep time / 写入指定段保持时间（毫秒）。
    bool setMixKeepTime(uint8_t index, int32_t time);
    
    //! Read one mixed-mode step parameter (CMDID_MIX_INDEX_STEP_PARAMETER).
    //! The payload is 24 bytes (index + 23). Older firmware validated 23 bytes
    //! and rejected writes; upgrade the firmware if a write fails with a length
    //! error.
    //! 读取混合模式段参数。帧布局为 24 字节（index + 23）；旧固件按 23 字节
    //! 校验会拒绝写入，如遇长度错误请升级固件。
    bool getMixStepParameter(uint8_t index, MixStepParameter& param);

    //! Write one mixed-mode step parameter / 写入混合模式段参数。
    bool setMixStepParameter(uint8_t index, const MixStepParameter& param);


    /**************************************************************************
     * System commands / 系统命令
     *************************************************************************/

    //! Read device date and time (CMDID_DATE_TIME).
    //! @param dateTime [out] Parsed fields; m_Year is tm_year (year - 1900).
    //! @return true on success / 读取设备日期时间；m_Year 为 tm_year。
    bool getDateTime(DateTimeInfo& dateTime);

    //! Write device date and time.
    //! Layout: [sec][min][hour][day][month][year], all uint8.
    //! @param dateTime Fields to write; m_Year must be tm_year (year - 1900).
    //! @return true on success / 写入设备日期时间，m_Year 使用 tm_year。
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

    //! Read current machine state (CMDID_GET_CURRENT_STATE, 20/25/30 bytes by mode).
    //! 读取机器当前状态（响应长度依模式为 20/25/30 字节，见 MachineState）。
    bool getMachineState(MachineState& state);

    //! Start or stop pressing only (does not affect heating).
    //! @param isPressing true to start pressing, false to stop.
    //! @return true on success / 单独启动或停止加压 (不影响加热)。
    bool setPressing(bool isPressing);

    //! Start or stop demolding only.
    //! @param isDemolding true to start demolding, false to stop.
    //! @return true on success / 单独启动或停止脱模。
    //! 适用机型：自动压片机可用；自动热压机不可用（固件未实现 CMDID_SET_START_DEMOLD 0x1523，调用返回失败）。
    //! Device support: available on automatic tablet press, NOT available on automatic hot press.
    bool setDemolding(bool isDemolding);

    //! Start air venting (CMDID_SET_START_AIR, 0x1524).
    //! @return true on success / 启动排气，释放管路内压力。
    bool startAir();

    //! Start or stop heating (CMDID_SET_START_HEAT).
    //! @param isHeating true to start heating, false to stop.
    //! @return true on success / 启动或停止加热。
    bool setStartHeat(bool isHeating);

    //! Start or stop the whole process (CMDID_SET_START, pressing + heating).
    //! @param isStart true to start, false to stop.
    //! @return true on success / 启动或停止全部流程（加压 + 加热）。
    bool setStart(bool isStart);

    //! Select the work mode (CMDID_SELECT_MODEL_STATE).
    //! @param model Mode value understood by the firmware (typically 1 or 2).
    //! @return true on success / 选择工作模式；model 取值由固件定义（通常为 1/2）。
    bool selectModelState(uint8_t model);


    /**************************************************************************
     * Static utilities / 静态工具
     *************************************************************************/

    //! Enumerate the serial ports available on the current system.
    //! Returns COM-style names on Windows and /dev/tty* names on Linux/macOS.
    //! TCP endpoints are not enumerated; pass "host:port" to connect() directly.
    //! @return Vector of port name strings; empty if none are found.
    //! 枚举当前系统可用的串口：Windows 返回 COM 名称，Linux/macOS 返回
    //! /dev/tty* 名称；TCP 端点不参与枚举，直接使用 connect("host:port")。
    static std::vector<std::string> getPortList();

    //! Get the SDK version string.
    //! @return Version string such as "1.0.0".
    //! 获取当前 SDK 版本号，例如 "1.0.0"。
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
 * Free-function serialization utilities / 自由函数序列化工具
 *
 *  These helpers are stateless and do not access the device. They are useful
 *  for logging, persisting parameters, or building payloads before calling
 *  the corresponding setter.
 *  以下函数无状态、不访问设备，可用于日志、参数持久化，或在调用 setter 前
 *  自行构造载荷。
 *****************************************************************************/

//! Serialize ReadOnlyData to a JSON string.
//! @return JSON text using the SDK's field names.
//! 将 ReadOnlyData 序列化为 JSON 字符串。
PRESS_EXPORT std::string toJsonString(const ReadOnlyData& data);

//! Serialize PressData to a JSON string.
//! @return JSON text suitable for setPressDataJson().
//! 将 PressData 序列化为 JSON 字符串，可直接用于 setPressDataJson()。
PRESS_EXPORT std::string toJsonString(const PressData& data);

//! Serialize RealTimeData to a JSON string.
//! @return JSON text using the SDK's field names.
//! 将 RealTimeData 序列化为 JSON 字符串。
PRESS_EXPORT std::string toJsonString(const RealTimeData& data);

//! Serialize PressData to a byte payload (UTF-8 JSON bytes).
//! @return JSON bytes without the frame header or checksum.
//! 将 PressData 序列化为字节载荷（UTF-8 JSON），不含帧头与校验和。
PRESS_EXPORT std::vector<uint8_t> toFrameData(const PressData& data);


#endif /* PRESS_SDK_PROJECT_PRESS_HPP */
