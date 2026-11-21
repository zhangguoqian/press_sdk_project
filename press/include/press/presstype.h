/******************************************************************************
 * presstype.h — public data model and command constants for the press SDK
 * presstype.h — 压片机 SDK 的公开数据模型与命令常量
 *
 *  This file exposes the public data structures that describe the machine state,
 *  configured pressure curve, and communication endpoint type. It also defines
 *  the high-level command constants used by the protocol layer.
 *  该文件对外暴露了描述机器状态、压力曲线和通信端口类型的数据结构，同时
 *  定义了协议层所使用的便捷命令常量。
 *
 *  Public entities / 对外实体:
 *      PortType       — communication port type / 通信端口类型
 *      ReadOnlyData   — immutable device identity and limits / 设备静态身份及限制信息
 *      PressData      — multi-step pressure control profile / 多步压力控制参数
 *      RealTimeData   — live machine state / 实时运行状态
 *
 *  Compatibility / 兼容性:
 *    - C compiler / C 编译器: uses plain struct/enum/define only; safe for C code
 *      仅使用 struct / enum / #define，兼容纯 C
 *    - C++ compiler / C++ 编译器: adds stream insertion operators and constexpr helper values
 *      提供 stream 输出重载和 constexpr 辅助值
 *    - Qt environment / Qt 环境: adds QDebug overloads when QT_CORE_LIB is defined
 *      在定义 QT_CORE_LIB 时提供 QDebug 重载
 *
 *  External dependencies / 外部依赖:
 *    - <cstdint>
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_PRESSTYPE_H
#define PRESS_SDK_PROJECT_PRESSTYPE_H

#include <cstdint>

//! Maximum number of pressure/temperature steps in PressData / PressData 最大段数
#define MAX_PStep 30


/******************************************************************************
 * PortType — Communication Port Type / 通信端口类型枚举
 *****************************************************************************/

enum PressPortType
{
    SerialPortType,    //!< Serial port (COMx or /dev/tty*) / 串口
    TcpSocketPortType, //!< TCP socket ("host:port") / TCP 套接字
};

//! Application / firmware type reported by the device / 设备上报的应用类型
enum AppType
{
    AUTO_HOT_PRESS = 0x01, //!< Automatic hot press / 自动热压
    AUTO_PRESS     = 0x02, //!< Automatic press / 自动压片
    AUTO_HOT       = 0x03, //!< Automatic heating / 自动加热
    MANUAL_PRESS   = 0x04, //!< Manual press / 手动压片
};

/******************************************************************************
 * ReadOnlyData — Immutable device information / 只读设备信息
 *
 *  捕获运行期间不变的静态配置：机器身份标识（名、型号、序列号）、
 *  压力/温度限制、界面设置（字体、语言菜单、远程/网络开关）等。
 *  所有字符串均为固定长度 char[64]，以 '\0' 结尾。
 *
 *  @note The struct has no user-defined constructor. Zero-initialize it before
 *        use (e.g. `ReadOnlyData data{};`) so unused fields do not contain
 *        indeterminate values. / 结构体没有自定义构造函数，使用前请零初始化
 *        （如 `ReadOnlyData data{};`），避免未使用字段为不确定值。
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
    float    m_Max_Min;             //!< Auto repress offset / 自动补压值（非最大最小压力差）
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

    // 固件 PD JSON 携带的排气时间（尾部追加，旧字段偏移不变）
    uint8_t  m_AirTime;                //!< Exhaust time (seconds) / 排气时间(秒)
};


/******************************************************************************
 * RealTimeData — Live machine state / 实时机器状态
 *
 *  设备运行时持续更新，反映当前工作模式、加压/加热状态、活动步骤、实时
 *  压力/温度和倒计时。m_PdChanged 为兼容字段，固件当前不提供该信息，恒为 0；
 *  如需感知 PressData 变化，请重新调用 getPressDataJson()。
 *****************************************************************************/

struct RealTimeData
{
    uint8_t  m_ModelState;      //!< 0=normal, 1=speed, 2=3-speed mode / 0 普通模式 / 1 速度模式 / 2 三速模式
    uint8_t  m_PressState;      //!< Derived from m_IsPStart / 由加压标志推导(0=未加压, 1=加压中)
    uint8_t  m_CPStep;          //!< Current active step index / 当前执行步骤
    float    m_PressValue;      //!< Live pressure reading / 实时压力数据
    uint32_t m_PTime;           //!< Countdown remaining (ms) / 剩余倒计时 (毫秒)
    uint8_t  m_PdChanged;       //!< Always 0, kept for compatibility / 固件无此字段，恒为 0（兼容保留）

    // 固件 RT JSON 对齐字段（尾部追加，上方字段偏移不变，旧消费者不受影响）
    // Firmware RT JSON aligned fields (appended; offsets above unchanged)
    uint8_t  m_IsPStart;        //!< Press start flag (0/1) / 加压启动标志
    uint8_t  m_IsTStart;        //!< Heat start flag (0/1) / 加热启动标志
    uint8_t  m_IsMStart;        //!< Mix/cooperation start flag (0/1) / 合作流程启动标志
    uint8_t  m_CTStep;          //!< Current temperature step / 当前温度步骤
    uint8_t  m_CMStep;          //!< Current mix/cooperation step / 当前合作步骤
    float    m_MoldPressure;    //!< Mold pressure / 模具压力
    float    m_TUpValue;        //!< Upper heat realtime temperature / 上加热实时温度
    float    m_TDownValue;      //!< Lower heat realtime temperature / 下加热实时温度
    uint32_t m_TTime;           //!< Temperature countdown remaining (ms) / 保温剩余倒计时 (毫秒)
    uint32_t m_MTime;           //!< Mix/cooperation countdown remaining (ms) / 合作剩余倒计时 (毫秒)
    uint8_t  m_IsAir;           //!< Exhaust flag (0/1) / 排气标志
};

//! Firmware version reported by the device / 设备上报的固件版本
struct VersionInfo
{
    AppType m_AppNo;   //!< Application type / 应用类型
    uint8_t m_Major;   //!< Major version / 主版本号
    uint8_t m_Minor;   //!< Minor version / 次版本号
    uint8_t m_Patch;   //!< Patch version / 修订号
};

//! Pressure parameter block (CMDID_PRESS_PARAMETER, 21 bytes) / 压力参数块
//! 布局: [MaxPLimit f4][MinPLimit f4][Max_Min f4][Diameter f4]
//!       [MaxPStep u1][PDecimal u1][PressDecimal u1][PModel u1][OutType u1]
struct PressParameter
{
    float   m_MaxPLimit;      //!< Upper pressure limit / 压力上限
    float   m_MinPLimit;      //!< Lower pressure limit / 压力下限
    float   m_MaxMin;         //!< Auto repress offset / 自动补压值（非最大最小压力差）
    float   m_Diameter;       //!< Cylinder diameter (mm) / 油缸直径
    uint8_t m_MaxPStep;       //!< Maximum pressure steps / 最大压力步数
    uint8_t m_PDecimal;       //!< Pressure value decimal places / 压力值小数位
    uint8_t m_PressDecimal;   //!< Pressure unit decimal places / 压强单位小数位
    uint8_t m_PModel;         //!< Pressure mode switch flag / 压力模式切换标志
    uint8_t m_OutType;        //!< Output signal type / 输出信号类型
};

//! Temperature parameter block (CMDID_TEMP_PARAMETER, 12 bytes) / 温度参数块
//! 布局: [MaxTLimit f4][MinTLimit f4][MaxTStep u1][TDecimal u1][IsHasWater u1][IsHasSpeed u1]
struct TempParameter
{
    float   m_MaxTLimit;     //!< Upper temperature limit / 温度上限
    float   m_MinTLimit;     //!< Lower temperature limit / 温度下限
    uint8_t m_MaxTStep;      //!< Maximum temperature steps / 最大温度步数
    uint8_t m_TDecimal;      //!< Temperature decimal places / 温度小数位
    uint8_t m_IsHasWater;    //!< Has water cooling flag / 是否有冷却
    uint8_t m_IsHasSpeed;    //!< Has speed mode flag / 是否有速度模式
};

//! Miscellaneous info block (CMDID_OTHER_INFO, 5 bytes) / 其他信息块
//! 布局: [Screenshot u1][StartDelay u1][IsHideLang u1][Remote u1][VersionType u1]
struct OtherInfo
{
    uint8_t m_Screenshot;    //!< Screenshot enable flag / 截屏使能标志
    uint8_t m_StartDelay;    //!< Start-up delay (seconds) / 启动延迟(秒)
    uint8_t m_IsHideLang;    //!< Hide language menu flag / 隐藏语言菜单标志
    uint8_t m_Remote;        //!< Remote control enable flag / 远程控制使能标志
    uint8_t m_VersionType;   //!< Firmware version type / 固件版本类型
};

//! Device date & time (CMDID_DATE_TIME, 6 bytes) / 设备日期时间
//! 布局: [Second u1][Minute u1][Hour u1][Day u1][Month u1][Year u1]（下位机 tm 字段顺序）
struct DateTimeInfo
{
    uint8_t m_Second;    //!< Seconds (0-59) / 秒
    uint8_t m_Minute;    //!< Minutes (0-59) / 分
    uint8_t m_Hour;      //!< Hours (0-23) / 时
    uint8_t m_Day;       //!< Day of month (1-31) / 日
    uint8_t m_Month;     //!< Month (1-12) / 月
    uint8_t m_Year;      //!< Year since 1900 (tm_year, e.g. 126 = 2026) / 自 1900 起的年份偏移（tm_year，126 表示 2026）
};

//! Machine state (CMDID_GET_CURRENT_STATE, variable length) / 机器状态
//! 下位机 RTData::getRtData 二进制布局（字段数依模式分支）:
//!   [model u1][flags u1][CPStep u1][CTStep u1?][CMStep u1?]
//!   [PValue f4][TUpValue f4][TDownValue f4]
//!   [PTime u4][TTime u4?][MTime u4?][FlowChange u1]
//! flags 位组合: bit0=IsPStart(加压) bit1=IsTStart(加热) bit2=IsMStart(合作)
//! m_Count 为实际 payload 字节数（20/25/30），据此判断哪些字段有效。
struct MachineState
{
    uint8_t m_ModelState;     //!< Work mode / 工作模式
    uint8_t m_Flags;          //!< Start flags bit-combined / 启动标志位组合
    uint8_t m_PressStep;      //!< Current press step / 当前压力步骤
    uint8_t m_TempStep;       //!< Current temperature step / 当前温度步骤
    uint8_t m_MixStep;        //!< Mix/cooperation step (P_T_T mode) / 合作步骤

    // 实时值 / live values (appended, aligned with firmware getRtData)
    float    m_PressValue;    //!< Live pressure / 实时压力
    float    m_TempUpValue;   //!< Upper heat realtime temperature / 上加热实时温度
    float    m_TempDownValue; //!< Lower heat realtime temperature / 下加热实时温度
    uint32_t m_PTime;         //!< Pressure countdown remaining (ms) / 压力剩余倒计时(毫秒)
    uint32_t m_TTime;         //!< Temperature countdown remaining (ms) / 保温剩余倒计时(毫秒)
    uint32_t m_MTime;         //!< Mix/cooperation countdown remaining (ms) / 合作剩余倒计时(毫秒)
    uint8_t  m_FlowChange;    //!< Flow change flag (firmware raw byte) / 流量变化原始字节
    uint8_t  m_Count;         //!< Number of parsed payload bytes (20/25/30) / 实际解析字节数
};

//! Pressure step parameter (CMDID_INDEX_PSTEP_PARAMETER, index + 12 bytes) / 压力步骤段参数
//! 帧内布局: [index u1][SetPValue f4][AfterValue f4][KPTime i4]
struct PressStepParameter
{
    float    m_SetPValue;     //!< Target pressure / 设定压力
    float    m_AfterValue;    //!< Hold pressure / 保压值
    uint32_t m_KPTime;        //!< Keep-pressure time (ms) / 保压时间(毫秒)
};

//! Temperature step parameter (CMDID_INDEX_TSTEP_PARAMETER, index + 12 bytes) / 温度步骤段参数
//! 帧内布局: [index u1][TempValue f4][SpeedValue f4][KTTime i4]
struct TempStepParameter
{
    float    m_TempValue;     //!< Target temperature / 设定温度
    float    m_SpeedValue;    //!< Speed value / 速度值
    uint32_t m_KTTime;        //!< Keep-temperature time (ms) / 保温时间(毫秒)
};

//! Mixed-mode step parameter (CMDID_MIX_INDEX_STEP_PARAMETER, index + 23 bytes) / 混合模式步骤段参数
//! 帧内布局: [index u1][IsPress u1][SetPValue f4][AfterValue f4]
//!           [IsUpHeat u1][UpTemp f4][IsDownHeat u1][DownTemp f4][KTime i4]
//! @note 帧长度为 24 字节（index + 23）。旧固件按 23 字节校验会拒绝写入；
//!       如写入返回长度错误，请升级固件。
struct MixStepParameter
{
    uint8_t  m_IsPress;       //!< Press flag (0/1) / 是否加压
    float    m_SetPValue;     //!< Target pressure / 设定压力
    float    m_AfterValue;    //!< Hold pressure / 保压值
    uint8_t  m_IsUpHeat;      //!< Heat-up flag (0/1) / 是否加热
    float    m_UpTemp;        //!< Heat-up temperature / 加热温度
    uint8_t  m_IsDownHeat;    //!< Cool-down flag (0/1) / 是否冷却
    float    m_DownTemp;      //!< Cool-down temperature / 冷却温度
    uint32_t m_KTime;         //!< Keep time (ms) / 保持时间(毫秒)
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