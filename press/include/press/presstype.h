/******************************************************************************
 * presstype.h — 压片机数据结构与公开命令常量
 *
 *  本文件定义 SDK 对外暴露的核心数据结构：
 *      PortType       — 通信端口类型枚举
 *      ReadOnlyData   — 设备静态身份信息（不可变）
 *      PressData      — 压力控制参数（多步压制曲线）
 *      RealTimeData   — 设备实时运行状态
 *
 *  以及预定义的便捷命令常量宏 (GET_MACHINE_TYPE / SET_PRESS 等)。
 *
 *  兼容性:
 *    - C 编译器: 仅 struct + enum + #define，纯 C 兼容
 *    - C++11+:   额外提供 static constexpr 常量和 operator<< 流输出
 *    - Qt 环境:  额外提供 QDebug operator<< (QT_CORE_LIB)
 *
 *  外部依赖: typeprivate.h (命令 ID 定义), <stdint.h>, <cstddef>
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_PRESSTYPE_H
#define PRESS_SDK_PROJECT_PRESSTYPE_H

#include "typeprivate.h"

#define MAX_PStep 30

#if defined(__cplusplus) && __cplusplus >= 201103L

static constexpr uint8_t MAX_PStep_V = MAX_PStep;
#endif

/******************************************************************************
 * Public Command Constants / 公开命令常量
 *****************************************************************************/

//! Read the machine model / 读取机器型号
#define GET_MACHINE_TYPE CMDID_MACHINE_TYPE

//! Read all read-only parameters (JSON) / 读取全部只读参数(Json格式)
#define GET_ROD_JSON CMDID_ROD_JSON

//! Read all pressure parameters (JSON) / 读取全部压力参数(Json格式)
#define GET_PD_JSON CMDID_PD_JSON

//! Write all pressure parameters (JSON) / 写入全部压力参数(Json格式)
#define SET_PD_JSON SET_CMD(CMDID_PD_JSON)

//! Read all real-time state (JSON) / 读取全部实时状态 (Json 格式)
#define GET_RT_JSON CMDID_GET_CURRENT_STATE_JSON

//! Start or stop pressing / 启动或停止加压
#define SET_PRESS CMDID_SET_START_PRESS

//! Start or stop demolding / 启动或停止脱模
#define SET_DEMOLD CMDID_SET_START_DEMOLD

#if defined(__cplusplus) && __cplusplus >= 201103L
static constexpr uint16_t GET_MACHINE_TYPE_V = GET_MACHINE_TYPE;
static constexpr uint16_t GET_ROD_JSON_V     = GET_ROD_JSON;
static constexpr uint16_t GET_PD_JSON_V      = GET_PD_JSON;
static constexpr uint16_t SET_PD_JSON_V      = SET_PD_JSON;
static constexpr uint16_t GET_RT_JSON_V      = GET_RT_JSON;
static constexpr uint16_t SET_PRESS_V        = SET_PRESS;
static constexpr uint16_t SET_DEMOLD_V       = SET_DEMOLD;
#endif


/******************************************************************************
 * PortType — Communication Port Type / 通信端口类型枚举
 *****************************************************************************/

enum PortType
{
    SerialPortType,    //!< Serial port (COM / /dev/tty*) / 串口
    TcpSocketPortType, //!< TCP socket / TCP 套接字
};


/******************************************************************************
 * ReadOnlyData — Immutable device information / 只读设备信息
 *
 *  捕获运行期间不变的静态配置：机器身份标识（名、型号、序列号）、
 *  压力/温度限制、界面设置（字体、语言菜单、远程/网络开关）等。
 *  所有字符串均为固定长度 char[64]，以 '\0' 结尾。
 *****************************************************************************/

struct ReadOnlyData
{
    char m_NameZH[64];              //!< Chinese device name / 中文设备名
    char m_NameEN[64];              //!< English device name / 英文设备名
    char m_Type[64];                //!< Device model / 设备型号
    char m_SerialNumber[64];        //!< Hardware serial number / 硬件序列号

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
    float    m_Diameter;            //!< Cylinder diameter (mm) / 油缸直径 (毫米)
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
};


/******************************************************************************
 * PressData — Pressure control parameters / 压力控制参数
 *
 *  定义多步压制曲线：步数、模具参数、以及每步的设定压力、保压值和保压时间。
 *  m_SetPValue / m_AfterValue / m_KPTime 三个数组长度均为 MAX_PStep (30)。
 *  m_Speed: 0 = 等静压 (isostatic), 1 = 普通压制 (normal)。
 *****************************************************************************/

struct PressData
{
    uint8_t m_PStep;                //!< Current pressure step count / 压力步数
    uint8_t m_Type;                 //!< Press type selector / 压制类型选择
    float   m_A;                    //!< Mold parameter A / 模具参数 A
    float   m_B;                    //!< Mold parameter B / 模具参数 B
    float   m_D;                    //!< Mold parameter D / 模具参数 D
    float   m_OuterD;               //!< Ring outer diameter / 环形外径
    float   m_InnerD;               //!< Ring inner diameter / 环形内径
    float   m_CheckValue;           //!< Calibration / check value / 校准值

    uint8_t m_Speed;                //!< 0 = isostatic, 1 = normal / 0 等静压, 1 普通
    float   m_DemoldValue;          //!< Demolding pressure / 脱模压力值

    float    m_SetPValue[MAX_PStep];   //!< Target pressure per step / 每步设定压力
    float    m_AfterValue[MAX_PStep];   //!< Hold pressure per step / 每步保压值
    uint32_t m_KPTime[MAX_PStep];       //!< Keep-pressure time per step (ms) / 每步保压时间 (毫秒)
};


/******************************************************************************
 * RealTimeData — Live machine state / 实时机器状态
 *
 *  设备运行时持续更新，反映当前工作模式、加压状态、活动步骤、实时压力
 *  和倒计时。m_PdChanged 标志通知宿主程序 PressData 是否已被设备端修改。
 *****************************************************************************/

struct RealTimeData
{
    uint8_t  m_ModelState;      //!< 0=normal, 1=speed, 2=3-speed mode / 0 普通模式 / 1 速度模式 / 2 三速模式
    uint8_t  m_PressState;      //!< 0=idle, 1=pressing, 2=demolding / 0 未加压 / 1 加压中 / 2 脱模中
    uint8_t  m_CPStep;          //!< Current active step index / 当前执行步骤
    float    m_PressValue;      //!< Live pressure reading / 实时压力数据
    uint32_t m_PTime;           //!< Countdown remaining (ms) / 剩余倒计时 (毫秒)
    uint8_t  m_PdChanged;       //!< PressData modification flag / 压力数据是否已变更
};

#if defined(__cplusplus)
#include <iostream>

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
    os << "SetPValue size: " << MAX_PStep << std::endl;
    os << "AfterValue size: " << MAX_PStep << std::endl;
    os << "KPTime size: " << MAX_PStep << std::endl;
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
    os << "SetPValue size: " << MAX_PStep;
    os << "AfterValue size: " << MAX_PStep;
    os << "KPTime size: " << MAX_PStep;
    os << '}';
    return os;
}

inline QDebug operator<<(QDebug os, const ReadOnlyData& readOnlyData)
{
    os << "NameZH: " << QString::fromUtf8(readOnlyData.m_NameZH);
    os << "NameEN: " << QString::fromUtf8(readOnlyData.m_NameEN);
    os << "Type: " << QString::fromUtf8(readOnlyData.m_Type);
    os << "SerialNumber: " << QString::fromUtf8(readOnlyData.m_SerialNumber);
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

#endif // QT_CORE_LIB
#endif // __cplusplus

#endif