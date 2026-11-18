//
// Created by 11518 on 2026/9/13.
//
#ifndef PRESS_BOT_PROJECT_MACHINEPRIVATE_H
#define PRESS_BOT_PROJECT_MACHINEPRIVATE_H

#include "port/portbase.h"
#include "tool/jsondata.h"
#include "typeprivate.h"
#include "press.hpp"
#include <memory>
#include <vector>
#include <cstdint>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

// ==================== 命令 ID 常量（static constexpr） ====================
// 读取命令 / read commands
static constexpr uint16_t GET_VERSION_V                  = CMDID_VERSION;
static constexpr uint16_t GET_NAME_V                     = CMDID_NAME;
static constexpr uint16_t GET_MACHINE_TYPE_V             = CMDID_MACHINE_TYPE;
static constexpr uint16_t GET_SERIAL_NUMBER_V            = CMDID_SERIAL_NUMBER;
static constexpr uint16_t GET_PRESS_PARAMETER_V          = CMDID_PRESS_PARAMETER;
static constexpr uint16_t GET_TEMP_PARAMETER_V           = CMDID_TEMP_PARAMETER;
static constexpr uint16_t GET_OTHER_INFO_V               = CMDID_OTHER_INFO;
static constexpr uint16_t GET_TEMP_STEP_V                = CMDID_TEMP_STEP;
static constexpr uint16_t GET_SPEED_MOLD_V               = CMDID_SPEED_MOLD;
static constexpr uint16_t GET_INDEX_TEMP_V               = CMDID_INDEX_TEMP;
static constexpr uint16_t GET_INDEX_SPEED_V              = CMDID_INDEX_SPEED;
static constexpr uint16_t GET_INDEX_KTTIME_V             = CMDID_INDEX_KTTIME;
static constexpr uint16_t GET_WATER_UP_V                 = CMDID_WATER_UP;
static constexpr uint16_t GET_WATER_DOWN_V               = CMDID_WATER_DOWN;
static constexpr uint16_t GET_PRESS_STEP_V               = CMDID_PRESS_STEP;
static constexpr uint16_t GET_PRESS_CALIBRATE_V          = CMDID_PRESS_CALIBRATE;
static constexpr uint16_t GET_MOLD_TYPE_V                = CMDID_MOLD_TYPE;
static constexpr uint16_t GET_CIRCLE_D_V                 = CMDID_CIRCLE_D;
static constexpr uint16_t GET_RECT_AB_V                  = CMDID_RECT_AB;
static constexpr uint16_t GET_AIR_TIME_V                 = CMDID_AIR_TIME;
static constexpr uint16_t GET_INDEX_PRESS_V              = CMDID_INDEX_PRESS;
static constexpr uint16_t GET_INDEX_AFTER_V              = CMDID_INDEX_AFTER;
static constexpr uint16_t GET_INDEX_KPTIME_V             = CMDID_INDEX_KPTIME;
static constexpr uint16_t GET_INDEX_PSTEP_PARAMETER_V    = CMDID_INDEX_PSTEP_PARAMETER;
static constexpr uint16_t GET_INDEX_TSTEP_PARAMETER_V    = CMDID_INDEX_TSTEP_PARAMETER;
static constexpr uint16_t GET_MIX_INDEX_STEP_PARAMETER_V = CMDID_MIX_INDEX_STEP_PARAMETER;
static constexpr uint16_t GET_MIX_STEP_V                 = CMDID_MIX_STEP;
static constexpr uint16_t GET_MIX_MOLD_TYPE_V            = CMDID_MIX_MOLD_TYPE;
static constexpr uint16_t GET_MIX_CIRCLE_D_V             = CMDID_MIX_CIRCLE_D;
static constexpr uint16_t GET_MIX_RECT_AB_V              = CMDID_MIX_RECT_AB;
static constexpr uint16_t GET_MIX_INDEX_ISPRESS_V        = CMDID_MIX_INDEX_ISPRESS;
static constexpr uint16_t GET_MIX_INDEX_PRESS_V          = CMDID_MIX_INDEX_PRESS;
static constexpr uint16_t GET_MIX_INDEX_AFTER_V          = CMDID_MIX_INDEX_AFTER;
static constexpr uint16_t GET_MIX_INDEX_ISHEAT_UP_V      = CMDID_MIX_INDEX_ISHEAT_UP;
static constexpr uint16_t GET_MIX_INDEX_TEMP_UP_V        = CMDID_MIX_INDEX_TEMP_UP;
static constexpr uint16_t GET_MIX_INDEX_ISHEAT_DOWN_V    = CMDID_MIX_INDEX_ISHEAT_DOWN;
static constexpr uint16_t GET_MIX_INDEX_TEMP_DOWN_V      = CMDID_MIX_INDEX_TEMP_DOWN;
static constexpr uint16_t GET_MIX_INDEX_KTIME_V          = CMDID_MIX_INDEX_KTIME;
static constexpr uint16_t GET_DATE_TIME_V                = CMDID_DATE_TIME;
static constexpr uint16_t GET_BRIGHTNESS_V               = CMDID_BRIGHTNESS;
static constexpr uint16_t GET_BUTTON_SOUND_V             = CMDID_BUTTON_SOUND;
static constexpr uint16_t GET_LANGUAGE_V                 = CMDID_LANGUAGE;
static constexpr uint16_t GET_CURRENT_STATE_V            = CMDID_GET_CURRENT_STATE;
static constexpr uint16_t GET_ROD_JSON_V                 = CMDID_ROD_JSON;
static constexpr uint16_t GET_PD_JSON_V                  = CMDID_PD_JSON;
static constexpr uint16_t GET_TD_JSON_V                  = CMDID_TD_JSON;
static constexpr uint16_t GET_MD_JSON_V                  = CMDID_MD_JSON;
static constexpr uint16_t GET_RT_JSON_V                  = CMDID_GET_CURRENT_STATE_JSON;

// 写入/控制命令 / write & control commands
static constexpr uint16_t SET_PRESS_V                    = CMDID_SET_START_PRESS;
static constexpr uint16_t SET_DEMOLD_V                   = CMDID_SET_START_DEMOLD;
static constexpr uint16_t SET_START_HEAT_V               = CMDID_SET_START_HEAT;
static constexpr uint16_t SET_START_V                    = CMDID_SET_START;
static constexpr uint16_t SELECT_MODEL_STATE_V           = CMDID_SELECT_MODEL_STATE;
static constexpr uint16_t SET_PRESS_STEP_V               = SET_CMD(CMDID_PRESS_STEP);
static constexpr uint16_t SET_PRESS_CALIBRATE_V          = SET_CMD(CMDID_PRESS_CALIBRATE);
static constexpr uint16_t SET_MOLD_TYPE_V                = SET_CMD(CMDID_MOLD_TYPE);
static constexpr uint16_t SET_CIRCLE_D_V                 = SET_CMD(CMDID_CIRCLE_D);
static constexpr uint16_t SET_RECT_AB_V                  = SET_CMD(CMDID_RECT_AB);
static constexpr uint16_t SET_AIR_TIME_V                 = SET_CMD(CMDID_AIR_TIME);
static constexpr uint16_t SET_INDEX_PRESS_V              = SET_CMD(CMDID_INDEX_PRESS);
static constexpr uint16_t SET_INDEX_AFTER_V              = SET_CMD(CMDID_INDEX_AFTER);
static constexpr uint16_t SET_INDEX_KPTIME_V             = SET_CMD(CMDID_INDEX_KPTIME);
static constexpr uint16_t SET_TEMP_STEP_V                = SET_CMD(CMDID_TEMP_STEP);
static constexpr uint16_t SET_SPEED_MOLD_V               = SET_CMD(CMDID_SPEED_MOLD);
static constexpr uint16_t SET_INDEX_TEMP_V               = SET_CMD(CMDID_INDEX_TEMP);
static constexpr uint16_t SET_INDEX_SPEED_V              = SET_CMD(CMDID_INDEX_SPEED);
static constexpr uint16_t SET_INDEX_KTTIME_V             = SET_CMD(CMDID_INDEX_KTTIME);
static constexpr uint16_t SET_WATER_UP_V                 = SET_CMD(CMDID_WATER_UP);
static constexpr uint16_t SET_WATER_DOWN_V               = SET_CMD(CMDID_WATER_DOWN);
static constexpr uint16_t SET_INDEX_PSTEP_PARAMETER_V    = SET_CMD(CMDID_INDEX_PSTEP_PARAMETER);
static constexpr uint16_t SET_INDEX_TSTEP_PARAMETER_V    = SET_CMD(CMDID_INDEX_TSTEP_PARAMETER);
static constexpr uint16_t SET_MIX_INDEX_STEP_PARAMETER_V = SET_CMD(CMDID_MIX_INDEX_STEP_PARAMETER);
static constexpr uint16_t SET_MIX_STEP_V                 = SET_CMD(CMDID_MIX_STEP);
static constexpr uint16_t SET_MIX_MOLD_TYPE_V            = SET_CMD(CMDID_MIX_MOLD_TYPE);
static constexpr uint16_t SET_MIX_CIRCLE_D_V             = SET_CMD(CMDID_MIX_CIRCLE_D);
static constexpr uint16_t SET_MIX_RECT_AB_V              = SET_CMD(CMDID_MIX_RECT_AB);
static constexpr uint16_t SET_MIX_INDEX_ISPRESS_V        = SET_CMD(CMDID_MIX_INDEX_ISPRESS);
static constexpr uint16_t SET_MIX_INDEX_PRESS_V          = SET_CMD(CMDID_MIX_INDEX_PRESS);
static constexpr uint16_t SET_MIX_INDEX_AFTER_V          = SET_CMD(CMDID_MIX_INDEX_AFTER);
static constexpr uint16_t SET_MIX_INDEX_ISHEAT_UP_V      = SET_CMD(CMDID_MIX_INDEX_ISHEAT_UP);
static constexpr uint16_t SET_MIX_INDEX_TEMP_UP_V        = SET_CMD(CMDID_MIX_INDEX_TEMP_UP);
static constexpr uint16_t SET_MIX_INDEX_ISHEAT_DOWN_V    = SET_CMD(CMDID_MIX_INDEX_ISHEAT_DOWN);
static constexpr uint16_t SET_MIX_INDEX_TEMP_DOWN_V      = SET_CMD(CMDID_MIX_INDEX_TEMP_DOWN);
static constexpr uint16_t SET_MIX_INDEX_KTIME_V          = SET_CMD(CMDID_MIX_INDEX_KTIME);
static constexpr uint16_t SET_DATE_TIME_V                = SET_CMD(CMDID_DATE_TIME);
static constexpr uint16_t SET_BRIGHTNESS_V               = SET_CMD(CMDID_BRIGHTNESS);
static constexpr uint16_t SET_BUTTON_SOUND_V             = SET_CMD(CMDID_BUTTON_SOUND);
static constexpr uint16_t SET_LANGUAGE_V                 = SET_CMD(CMDID_LANGUAGE);
static constexpr uint16_t SET_PD_JSON_V                  = SET_CMD(CMDID_PD_JSON);
static constexpr uint16_t SET_TD_JSON_V                  = SET_CMD(CMDID_TD_JSON);
static constexpr uint16_t SET_MD_JSON_V                  = SET_CMD(CMDID_MD_JSON);

//! 实时数据定时采集连续失败上报阈值
static constexpr int TIMER_FAIL_REPORT_THRESHOLD = 3;

enum class MachineError
{
    None = 0,
    PortNotOpen,
    WriteFailed,
    ResponseTooShort,
    ChecksumMismatch,
    RegisterNoMismatch,
    CommandMismatch,
    Timeout,
    Stopped,
    ConnectFailed,
    JsonParseFailed,

    OutSignalBlocked,
    ImmediateSignalBlocked,
    StateUnchanged,

    UnknownError,
};

struct FrameData
{
    uint16_t m_Command = 0;
    uint64_t m_Seq = 0;                     //!< 请求序号，用于匹配响应
    MachineError m_Error = MachineError::None;  //!< 本请求的处理结果（随序号传递，避免全局错误被其他请求覆盖）
    std::vector<uint8_t> m_FrameDataList = {};
};

class PressPrivate
{
public:
    PressPrivate();
    ~PressPrivate();

    bool connect(const char* portName, PressPortType portType);
    void disconnect();
    bool isConnected() const;
    uint64_t getMachineRegisterNo() const;

    bool getReadOnlyDataJson(std::string& json, bool isCompressed = false);
    bool getPressDataJson(std::string& json, bool isCompressed = false);
    bool setPressDataJson(const std::string& json, bool isCompressed = false);
    bool getRealTimeDataJson(std::string& json, bool isCompressed = false);
    bool getVersionInfo(VersionInfo& versionInfo);
    bool getSerialNumber(std::string& serialNumber);
    bool setPressing(bool isPressing);
    bool setDemolding(bool isDemolding);

    //! 字符串命令读取（请求 {arg}，响应 [arg, string...]）；arg 为哑元 0 / 语言代码
    bool getStringValue(uint16_t command, uint8_t arg, std::string& text);

    // ==================== 只读设备命令（ROD） ====================
    bool getName(std::string& name, uint8_t language = 0);          //!< CMDID_NAME: 0=中文 2=英文
    bool getMachineType(std::string& type);                        //!< CMDID_MACHINE_TYPE
    bool getPressParameter(PressParameter& param);                  //!< CMDID_PRESS_PARAMETER
    bool getTempParameter(TempParameter& param);                   //!< CMDID_TEMP_PARAMETER
    bool getOtherInfo(OtherInfo& info);                             //!< CMDID_OTHER_INFO

    // ==================== 压力参数命令（单值/索引） ====================
    bool getPressStepCount(uint8_t& step);                          //!< CMDID_PRESS_STEP
    bool setPressStepCount(uint8_t step);
    bool getCalibrationValue(float& value);                         //!< CMDID_PRESS_CALIBRATE
    bool setCalibrationValue(float value);
    bool getModeType(uint8_t& type);                                //!< CMDID_MOLD_TYPE
    bool setModeType(uint8_t type);
    bool getCircle(float& diameter);                                //!< CMDID_CIRCLE_D
    bool setCircle(float diameter);
    bool getRectangle(float& a, float& b);                          //!< CMDID_RECT_AB
    bool setRectangle(float a, float b);
    bool getAirTime(uint8_t& time);                                 //!< CMDID_AIR_TIME
    bool setAirTime(uint8_t time);
    bool getPressValueIndex(uint8_t index, float& value);           //!< CMDID_INDEX_PRESS
    bool setPressValueIndex(uint8_t index, float value);
    bool getAfterValueIndex(uint8_t index, float& value);           //!< CMDID_INDEX_AFTER
    bool setAfterValueIndex(uint8_t index, float value);
    bool getKeepPressTime(uint8_t index, int32_t& time);            //!< CMDID_INDEX_KPTIME（毫秒）
    bool setKeepPressTime(uint8_t index, int32_t time);

    // ==================== 温度参数命令（单值/索引） ====================
    bool getTempStepCount(uint8_t& step);                           //!< CMDID_TEMP_STEP
    bool setTempStepCount(uint8_t step);
    bool getTempSpeedMold(uint8_t& mold);                           //!< CMDID_SPEED_MOLD
    bool setTempSpeedMold(uint8_t mold);
    bool getStartTempIndex(uint8_t index, float& value);            //!< CMDID_INDEX_TEMP
    bool setTempIndex(uint8_t index, float value);
    bool getTempSpeedIndex(uint8_t index, float& value);            //!< CMDID_INDEX_SPEED
    bool setTempSpeedIndex(uint8_t index, float value);
    bool getKeepTempTime(uint8_t index, int32_t& time);             //!< CMDID_INDEX_KTTIME（毫秒）
    bool setKeepTempTime(uint8_t index, int32_t time);
    bool getMaxWaterValue(float& value);                            //!< CMDID_WATER_UP
    bool setMaxWaterValue(float value);
    bool getMinWaterValue(float& value);                            //!< CMDID_WATER_DOWN
    bool setMinWaterValue(float value);

    // ==================== 混合模式参数命令（单值/索引） ====================
    bool getMixStepCount(uint8_t& step);                            //!< CMDID_MIX_STEP
    bool setMixStepCount(uint8_t step);
    bool getMixMoldType(uint8_t& type);                             //!< CMDID_MIX_MOLD_TYPE
    bool setMixMoldType(uint8_t type);
    bool getMixRectangle(float& a, float& b);                       //!< CMDID_MIX_RECT_AB
    bool setMixRectangle(float a, float b);
    bool getMixCircle(float& diameter);                             //!< CMDID_MIX_CIRCLE_D
    bool setMixCircle(float diameter);
    bool getMixIsPressIndex(uint8_t index, bool& isPress);          //!< CMDID_MIX_INDEX_ISPRESS
    bool setMixIsPressIndex(uint8_t index, bool isPress);
    bool getMixPressIndex(uint8_t index, float& value);             //!< CMDID_MIX_INDEX_PRESS
    bool setMixPressIndex(uint8_t index, float value);
    bool getMixAfterPressIndex(uint8_t index, float& value);        //!< CMDID_MIX_INDEX_AFTER
    bool setMixAfterPressIndex(uint8_t index, float value);
    bool getMixIsHeatUpIndex(uint8_t index, bool& isHeatUp);        //!< CMDID_MIX_INDEX_ISHEAT_UP
    bool setMixIsHeatUpIndex(uint8_t index, bool isHeatUp);
    bool getMixHeatUpIndex(uint8_t index, float& value);            //!< CMDID_MIX_INDEX_TEMP_UP
    bool setMixHeatUpIndex(uint8_t index, float value);
    bool getMixIsHeatDownIndex(uint8_t index, bool& isHeatDown);    //!< CMDID_MIX_INDEX_ISHEAT_DOWN
    bool setMixIsHeatDownIndex(uint8_t index, bool isHeatDown);
    bool getMixHeatDownIndex(uint8_t index, float& value);          //!< CMDID_MIX_INDEX_TEMP_DOWN
    bool setMixHeatDownIndex(uint8_t index, float value);
    bool getMixKeepTime(uint8_t index, int32_t& time);              //!< CMDID_MIX_INDEX_KTIME（毫秒）
    bool setMixKeepTime(uint8_t index, int32_t time);

    // ==================== 段参数命令 ====================
    bool getPressStepParameter(uint8_t index, PressStepParameter& param);   //!< 0x010A
    bool setPressStepParameter(uint8_t index, const PressStepParameter& param);
    bool getTempStepParameter(uint8_t index, TempStepParameter& param);     //!< 0x0205
    bool setTempStepParameter(uint8_t index, const TempStepParameter& param);
    bool getMixStepParameter(uint8_t index, MixStepParameter& param);       //!< 0x030D
    bool setMixStepParameter(uint8_t index, const MixStepParameter& param);

    // ==================== 系统命令 ====================
    bool getDateTime(DateTimeInfo& dateTime);                       //!< CMDID_DATE_TIME
    bool setDateTime(const DateTimeInfo& dateTime);
    bool getBrightness(uint8_t& brightness);                       //!< CMDID_BRIGHTNESS
    bool setBrightness(uint8_t brightness);
    bool getButtonSound(bool& enabled);                            //!< CMDID_BUTTON_SOUND
    bool setButtonSound(bool enabled);
    bool getLanguage(uint8_t& language);                           //!< CMDID_LANGUAGE
    bool setLanguage(uint8_t language);

    // ==================== 状态与控制命令 ====================
    bool getMachineState(MachineState& state);                     //!< CMDID_GET_CURRENT_STATE
    bool setStartHeat(bool isHeating);                              //!< CMDID_SET_START_HEAT
    bool setStart(bool isStart);                                    //!< CMDID_SET_START（全部电机）
    bool selectModelState(uint8_t model);                           //!< CMDID_SELECT_MODEL_STATE（1/2）

    // ==================== JSON 批量命令（字符串形式）====================
    //! 读取全部温度参数 JSON
    bool getTempDataJson(std::string& json, bool isCompressed = false);
    //! 写入全部温度参数 JSON
    bool setTempDataJson(const std::string& json, bool isCompressed = false);
    //! 读取全部混合模式参数 JSON
    bool getMixDataJson(std::string& json, bool isCompressed = false);
    //! 写入全部混合模式参数 JSON
    bool setMixDataJson(const std::string& json, bool isCompressed = false);

    //! 设置单次请求超时（毫秒），0 恢复默认 300ms，上限 60000ms
    void setRequestTimeoutMs(std::uint32_t ms);
    std::uint32_t requestTimeoutMs() const;

    void run();
    void stop();
    bool isRunning() const;

    static std::vector<std::string> getPortList();
    const char* getLastErrorInfo() const;

    void registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds = 1);
    void unregisterDataInterface();

    JsonData getMachineData() const;

private:
    //! 启动调度线程，幂等：若已启动则直接返回
    void _startScheduler();
    //! 停止调度线程并 join，幂等：若未启动则直接返回
    void _stopScheduler();

    //! 启动实时数据定时线程，每 intervalSeconds 秒获取一次 RealTimeData；
    //! 若已启动则以新周期重启
    void _startRealTimeTimer(int intervalSeconds);
    //! 停止实时数据定时线程并 join，幂等：若未启动则直接返回
    void _stopRealTimeTimer();

    //! 关闭端口并清零注册号（调用方需持有 m_LifecycleMutex）
    void _closePort();

    //! 注销回调并停止定时线程（调用方需持有 m_LifecycleMutex）
    void _unregisterInterfaceLocked();

    std::vector<uint8_t> _buildSendFrame(const FrameData& frameData);
    MachineError _parseResponseFrame(const std::vector<uint8_t>& response, uint16_t command, FrameData& out);
    bool _requestCommand(uint16_t command, const std::vector<uint8_t>& listData, std::vector<uint8_t>& responseListData);
    void _setError(MachineError err);
    std::shared_ptr<PressDataInterface> _safeGetInterface();
    bool _handleSetResponse(uint16_t cmd, const std::vector<uint8_t>& responseListData);

    std::mutex m_RequestSerialMutex{};

    std::mutex m_CommandMutex{};
    std::queue<FrameData> m_CommandQueue = {};
    std::condition_variable m_CommandCond{};

    std::mutex m_ResponseMutex{};
    std::queue<FrameData> m_ResponseQueue = {};
    std::condition_variable m_ResponseCond{};

    std::unique_ptr<std::thread> mpRunThread = nullptr;
    std::atomic<bool> m_IsRunning{false};

    std::atomic<uint64_t> m_RegisterNo{0};
    std::atomic<uint64_t> m_RequestSeq{0};     //!< 请求序号，超时后丢弃迟到响应
    std::unique_ptr<PortBase> mpPort = nullptr;

    std::atomic<MachineError> m_LastError{MachineError::None};
    std::atomic<std::uint32_t> m_RequestTimeoutMs{300};  //!< 单次请求超时（毫秒），可配置

    JsonData m_JsonData{};
    std::shared_ptr<PressDataInterface> mpMachineDataInterface;
    std::mutex m_MachineDataMutex{};

    std::unique_ptr<std::thread> mpRealTimeTimerThread = nullptr;
    std::atomic<bool> m_TimerRunning{false};
    std::atomic<int> m_TimerInterval{0};      //!< 实时数据定时周期（秒），<=0 表示未启用
    std::atomic<int> m_TimerConsecutiveFailures{0};  //!< 定时采集连续失败计数（M7 断线上报）
    std::mutex m_TimerMutex{};
    std::condition_variable m_TimerCond{};

    mutable std::mutex m_JsonDataMutex{};     //!< 保护 m_JsonData 的解析与拷贝
    std::mutex m_LifecycleMutex{};            //!< 保护 connect/disconnect/run/stop/注册回调等生命周期操作
};

#endif
