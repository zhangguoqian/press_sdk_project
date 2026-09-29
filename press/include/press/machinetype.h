//
// Created by 11518 on 2026/9/13.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_MACHINETYPE_H
#define ZTABLE_PRESS_BOT_PROJECT_MACHINETYPE_H


#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include "typeprivate.h"

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


/******************************************************************************
 * Public Command Constants / 公开命令常量
 *****************************************************************************/

//! Read the machine model / 读取机器型号
static constexpr uint16_t GET_MACHINE_TYPE = CMDID_MACHINE_TYPE;

//! Read all read-only parameters (JSON) / 读取全部只读参数(Json格式)
static constexpr uint16_t GET_ROD_JSON = CMDID_ROD_JSON;

//! Read all pressure parameters (JSON) / 读取全部压力参数(Json格式)
static constexpr uint16_t GET_PD_JSON = CMDID_PD_JSON;

//! Write all pressure parameters (JSON) / 写入全部压力参数(Json格式)
static constexpr uint16_t SET_PD_JSON = SET_CMD(CMDID_PD_JSON);

//! Read all real-time state (JSON) / 读取全部实时状态(Json格式)
static constexpr uint16_t GET_RT_JSON = CMDID_GET_CURRENT_STATE_JSON;

//! Start or stop pressing / 启动或停止加压
static constexpr uint16_t SET_PRESS = CMDID_SET_START_PRESS;

//! Start or stop demolding / 启动或停止脱模
static constexpr uint16_t SET_DEMOLD = CMDID_SET_START_DEMOLD;


/******************************************************************************
 * PortType — Communication Port Type / 通信端口类型枚举
 *****************************************************************************/

enum PortType
{
    SerialPortType,    //!< Serial port (COM / /dev/tty*) / 串口
    TcpSocketPortType, //!< TCP socket / TCP套接字
};


/******************************************************************************
 * ReadOnlyData — Immutable device information / 只读设备信息
 *
 * Captures static configuration that does not change during operation,
 * such as machine identity, pressure/temperature limits, and UI settings.
 * 捕获运行期间不变的静态配置，如机器身份、压力/温度限制和界面设置。
 *****************************************************************************/

struct PRESS_EXPORT ReadOnlyData
{
    std::string m_NameZH;           //!< Chinese device name / 中文设备名
    std::string m_NameEN;           //!< English device name / 英文设备名
    std::string m_Type;             //!< Device model / 设备型号
    std::string m_SerialNumber;     //!< Hardware serial number / 硬件序列号

    uint8_t m_Screenshot;           //!< Screenshot enable flag / 截屏使能标志
    uint8_t m_StartDelay;           //!< Start-up delay (seconds) / 启动延迟(秒)
    uint8_t m_VersionType;          //!< Firmware version type / 固件版本类型
    uint8_t m_IsHideLang;           //!< Hide language menu flag / 隐藏语言菜单标志
    uint8_t m_Remote;               //!< Remote control enable flag / 远程控制使能标志
    uint8_t m_Network;              //!< Network module enable flag / 网络模块使能标志
    uint8_t m_FontZH;               //!< Chinese font variant / 中文字体变体
    uint8_t m_FontEN;               //!< English font variant / 英文字体变体

    // Pressure-related fields / 压力相关字段
    uint8_t  m_MaxPStep;            //!< Maximum pressure steps / 最大压力步数
    float    m_MaxPLimit;           //!< Upper pressure limit / 压力上限
    float    m_MinPLimit;           //!< Lower pressure limit / 压力下限
    float    m_Max_Min;             //!< Max - Min difference / 最大与最小压力差
    float    m_Diameter;            //!< Cylinder diameter (mm) / 油缸直径(毫米)
    uint8_t  m_PDecimal;            //!< Pressure value decimal places / 压力值小数位数
    uint8_t  m_PressDecimal;        //!< Pressure unit decimal places / 压强单位小数位数
    uint8_t  m_PModel;              //!< Pressure mode switch flag / 是否有压力模式切换
    uint8_t  m_OutType;             //!< Output signal type / 输出信号类型

    // Temperature-related fields / 温度相关字段
    uint8_t  m_MaxTStep;            //!< Maximum temperature steps / 最大温度步数
    float    m_MaxTLimit;           //!< Upper temperature limit / 温度上限
    float    m_MinTLimit;           //!< Lower temperature limit / 温度下限
    uint8_t  m_TDecimal;            //!< Temperature value decimal places / 温度值小数位数
    uint8_t  m_IsHasWater;          //!< Has water cooling flag / 是否有冷却
    uint8_t  m_IsHasSpeed;          //!< Has speed mode flag / 是否有速度模式

    //! Serialize to JSON string / 序列化为Json字符串
    std::string toJsonString() const;
};


/******************************************************************************
 * PressData — Pressure control parameters / 压力控制参数
 *
 * Defines the multi-step press curve including step count, pressure values,
 * after-pressure (hold), and keep-pressure time for each step.
 * 定义多步压制曲线，包含步数、压力值、保压和每步保压时间。
 *****************************************************************************/

struct PRESS_EXPORT PressData
{
    uint8_t m_PStep;                //!< Current pressure step count / 压力步数
    uint8_t m_Type;                 //!< Press type selector / 压制类型选择
    float   m_A;                    //!< Mold parameter A / 模具参数A
    float   m_B;                    //!< Mold parameter B / 模具参数B
    float   m_D;                    //!< Mold parameter D / 模具参数D
    float   m_OuterD;               //!< Ring outer diameter / 环形外径
    float   m_InnerD;               //!< Ring inner diameter / 环形内径
    float   m_CheckValue;           //!< Calibration / check value / 校准值

    uint8_t m_Speed;                //!< 0 = isostatic, 1 = normal / 0等静压，1普通
    float   m_DemoldValue;          //!< Demolding pressure / 脱模压力值

    std::vector<float>  m_SetPValue; //!< Target pressure per step / 每步设定压力
    std::vector<float>  m_AfterValue;//!< Hold pressure per step / 每步保压值
    std::vector<uint32_t> m_KPTime; //!< Keep-pressure time per step (ms) / 每步保压时间(毫秒)

    //! Serialize to frame data (wire format) / 序列化为帧数据(传输格式)
    std::vector<uint8_t> toFrameData() const;
    //! Serialize to JSON string / 序列化为Json字符串
    std::string toJsonString() const;
};


/******************************************************************************
 * RealTimeData — Live machine state / 实时机器状态
 *
 * Updated continuously by the device while running, reflecting the current
 * work mode, press state, active step, live pressure, and countdown timer.
 * 设备运行时持续更新，反映当前工作模式、加压状态、活动步骤、实时压力和倒计时。
 *****************************************************************************/

struct PRESS_EXPORT RealTimeData
{
    uint8_t m_ModelState{ 0 };      //!< 0=normal, 1=speed, 2=3-speed mode / 切换普通模式/速度模式1/三速模式
    uint8_t m_PressState{ 0 };      //!< 0=idle, 1=pressing, 2=demolding / 0未加压,1加压中,2脱模中
    uint8_t m_CPStep{ 1 };          //!< Current active step index / 当前执行步骤
    float   m_PressValue{ .0f };    //!< Live pressure reading / 实时压力数据
    uint32_t m_PTime{ 0 };          //!< Countdown remaining (ms) / 剩余倒计时(毫秒)
    uint8_t m_PdChanged{ 0 };       //!< PressData modification flag / 压力数据是否已变更

    //! Serialize to JSON string / 序列化为Json字符串
    std::string toJsonString() const;
};


/******************************************************************************
 * MachineDataInterface — Asynchronous data callback interface / 异步数据回调接口
 *
 * Implement this interface and register it with Machine to receive
 * push-style notifications when read-only, press, or real-time data changes.
 * 实现此接口并注册到Machine，即可在只读数据、压力数据或实时数据变化时
 * 收到推送式通知。
 *****************************************************************************/

class MachineDataInterface
{
public:
    virtual ~MachineDataInterface() = default;

    //! Read-only data callback
    //! @param errorCode  0 on success, non-zero on failure
    //! @param registerNo Device register number (unique per connection)
    //! @param readOnlyData Decoded read-only data
    //! 只读数据回调
    virtual void onReadOnlyData(int errorCode, uint64_t registerNo,
                                const ReadOnlyData& readOnlyData) = 0;

    //! Real-time data callback
    //! @param errorCode  0 on success, non-zero on failure
    //! @param realTimeData Decoded real-time state
    //! 实时数据回调
    virtual void onRealTimeData(int errorCode, const RealTimeData& realTimeData) = 0;

    //! Pressure parameter callback
    //! @param errorCode  0 on success, non-zero on failure
    //! @param pressData Decoded pressure parameters
    //! 压力参数回调
    virtual void onPressData(int errorCode, const PressData& pressData) = 0;

    //! Error / unhandled response callback
    //! @param cmdCode    Command that produced the response
    //! @param response   Raw response bytes from device
    //! 错误/未处理响应回调
    virtual void onError(uint16_t cmdCode, std::vector<uint8_t> response) = 0;
};


/******************************************************************************
 * Stream operators for debugging / 调试流输出操作符
 *****************************************************************************/

inline std::ostream& operator<<(std::ostream& os, const ReadOnlyData& readOnlyData)
{
    os << "NameZH: " << readOnlyData.m_NameZH << std::endl;
    os << "NameEN: " << readOnlyData.m_NameEN << std::endl;
    os << "Type: " << readOnlyData.m_Type << std::endl;
    os << "SerialNumber: " << readOnlyData.m_SerialNumber << std::endl;
    os << "Screenshot: " << readOnlyData.m_Screenshot << std::endl;
    os << "StartDelay: " << readOnlyData.m_StartDelay << std::endl;
    os << "VersionType: " << readOnlyData.m_VersionType << std::endl;
    os << "IsHideLang: " << readOnlyData.m_IsHideLang << std::endl;
    os << "Remote: " << readOnlyData.m_Remote << std::endl;
    os << "Network: " << readOnlyData.m_Network << std::endl;
    os << "FontZH: " << readOnlyData.m_FontZH << std::endl;
    os << "FontEN: " << readOnlyData.m_FontEN << std::endl;
    os << "MaxPStep: " << readOnlyData.m_MaxPStep << std::endl;
    os << "MaxPLimit: " << readOnlyData.m_MaxPLimit << std::endl;
    os << "MinPLimit: " << readOnlyData.m_MinPLimit << std::endl;
    os << "Max_Min: " << readOnlyData.m_Max_Min << std::endl;
    os << "Diameter: " << readOnlyData.m_Diameter << std::endl;
    os << "PDecimal: " << readOnlyData.m_PDecimal << std::endl;
    os << "PressDecimal: " << readOnlyData.m_PressDecimal << std::endl;
    os << "PModel: " << readOnlyData.m_PModel << std::endl;
    os << "OutType: " << readOnlyData.m_OutType << std::endl;
    os << "MaxTStep: " << readOnlyData.m_MaxTStep << std::endl;
    os << "MaxTLimit: " << readOnlyData.m_MaxTLimit << std::endl;
    os << "MinTLimit: " << readOnlyData.m_MinTLimit << std::endl;
    os << "TDecimal: " << readOnlyData.m_TDecimal << std::endl;
    os << "IsHasWater: " << readOnlyData.m_IsHasWater << std::endl;
    os << "IsHasSpeed: " << readOnlyData.m_IsHasSpeed << std::endl;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const PressData& pressData)
{
    os << "PStep: " << pressData.m_PStep << std::endl;
    os << "Type: " << pressData.m_Type << std::endl;
    os << "A: " << pressData.m_A << std::endl;
    os << "B: " << pressData.m_B << std::endl;
    os << "D: " << pressData.m_D << std::endl;
    os << "OuterD: " << pressData.m_OuterD << std::endl;
    os << "InnerD: " << pressData.m_InnerD << std::endl;
    os << "CheckValue: " << pressData.m_CheckValue << std::endl;
    os << "Speed: " << pressData.m_Speed << std::endl;
    os << "DemoldValue: " << pressData.m_DemoldValue << std::endl;
    os << "SetPValue size: " << pressData.m_SetPValue.size() << std::endl;
    os << "AfterValue size: " << pressData.m_AfterValue.size() << std::endl;
    os << "KPTime size: " << pressData.m_KPTime.size() << std::endl;
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const RealTimeData& realTimeData)
{
    os << '{' << std::endl;
    os << "ModelState: "  << static_cast<int>(realTimeData.m_ModelState) << std::endl;
    os << "PressState: "  << static_cast<int>(realTimeData.m_PressState) << std::endl;
    os << "CPStep: "      << static_cast<int>(realTimeData.m_CPStep) << std::endl;
    os << "PressValue: "  << realTimeData.m_PressValue << std::endl;
    os << "PTime: "       << realTimeData.m_PTime << std::endl;
    os << "PdChanged: "   << static_cast<int>(realTimeData.m_PdChanged) << std::endl;
    os << '}' << std::endl;
    return os;
}


#ifdef QT_CORE_LIB
#include <QDebug>

inline QDebug operator<<(QDebug os, const RealTimeData& realTimeData)
{
    os << '{';
    os << "ModelState: "  << static_cast<int>(realTimeData.m_ModelState);
    os << "PressState: "  << static_cast<int>(realTimeData.m_PressState);
    os << "CPStep: "      << static_cast<int>(realTimeData.m_CPStep);
    os << "PressValue: "  << realTimeData.m_PressValue;
    os << "PTime: "       << realTimeData.m_PTime;
    os << "PdChanged: "   << realTimeData.m_PdChanged;
    os << '}';
    return os;
}

inline QDebug operator<<(QDebug os, const PressData& pressData)
{
    os << '{';
    os << "PStep: "  << pressData.m_PStep;
    os << "Type: "   << pressData.m_Type;
    os << "A: "      << pressData.m_A;
    os << "B: "      << pressData.m_B;
    os << "D: "      << pressData.m_D;
    os << "OuterD: " << pressData.m_OuterD;
    os << "InnerD: " << pressData.m_InnerD;
    os << "CheckValue: " << pressData.m_CheckValue;
    os << "Speed: "  << pressData.m_Speed;
    os << "DemoldValue: " << pressData.m_DemoldValue;
    os << "SetPValue size: " << pressData.m_SetPValue.size();
    os << "AfterValue size: " << pressData.m_AfterValue.size();
    os << "KPTime size: " << pressData.m_KPTime.size();
    os << '}';
    return os;
}

inline QDebug operator<<(QDebug os, const ReadOnlyData& readOnlyData)
{
    os << "NameZH: " << QString::fromStdString(readOnlyData.m_NameZH);
    os << "NameEN: " << QString::fromStdString(readOnlyData.m_NameEN);
    os << "Type: " << QString::fromStdString(readOnlyData.m_Type);
    os << "SerialNumber: " << QString::fromStdString(readOnlyData.m_SerialNumber);
    os << "Screenshot: " << readOnlyData.m_Screenshot;
    os << "StartDelay: " << readOnlyData.m_StartDelay;
    os << "VersionType: " << readOnlyData.m_VersionType;
    os << "IsHideLang: " << readOnlyData.m_IsHideLang;
    os << "Remote: " << readOnlyData.m_Remote;
    os << "Network: " << readOnlyData.m_Network;
    os << "FontZH: " << readOnlyData.m_FontZH;
    os << "FontEN: " << readOnlyData.m_FontEN;
    os << "MaxPStep: " << readOnlyData.m_MaxPStep;
    os << "MaxPLimit: " << readOnlyData.m_MaxPLimit;
    os << "MinPLimit: " << readOnlyData.m_MinPLimit;
    os << "Max_Min: " << readOnlyData.m_Max_Min;
    os << "Diameter: " << readOnlyData.m_Diameter;
    os << "PDecimal: " << readOnlyData.m_PDecimal;
    os << "PressDecimal: " << readOnlyData.m_PressDecimal;
    os << "PModel: " << readOnlyData.m_PModel;
    os << "OutType: " << readOnlyData.m_OutType;
    os << "MaxTStep: " << readOnlyData.m_MaxTStep;
    os << "MaxTLimit: " << readOnlyData.m_MaxTLimit;
    os << "MinTLimit: " << readOnlyData.m_MinTLimit;
    os << "TDecimal: " << readOnlyData.m_TDecimal;
    os << "IsHasWater: " << readOnlyData.m_IsHasWater;
    os << "IsHasSpeed: " << readOnlyData.m_IsHasSpeed;
    return os;
}

#endif
#endif