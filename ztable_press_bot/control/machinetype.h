//
// Created by 11518 on 2026/9/13.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_MACHINETYPE_H
#define ZTABLE_PRESS_BOT_PROJECT_MACHINETYPE_H


#include <cstdint>
#include <utility>
#include <vector>
#include <string>
#include <iostream>
#include "typeprivate.h"

/**机器参数获取**/
static const std::pair<uint16_t, std::vector<uint8_t>> GET_VERSION = {CMDID_VERSION, {0x00}};
static const std::pair<uint16_t, std::vector<uint8_t>> GET_ZH_NAME = {CMDID_NAME, {0x01}};
static const std::pair<uint16_t, std::vector<uint8_t>> GET_EN_NAME = {CMDID_NAME, {0x02}};
static constexpr uint16_t GET_MACHINE_TYPE = CMDID_MACHINE_TYPE;
static const std::pair<uint16_t, std::vector<uint8_t>> GET_SERIAL_NUMBER = {CMDID_SERIAL_NUMBER, {0x00}};
static const std::pair<uint16_t, std::vector<uint8_t>> GET_PRESS_PARAMETER = {CMDID_PRESS_PARAMETER, {0x00}};
static const std::pair<uint16_t, std::vector<uint8_t>> GET_OTHER_INFO = {CMDID_OTHER_INFO, {0x00}};

static constexpr uint16_t GET_ROD_JSON = CMDID_ROD_JSON;
/**压力参数获取**/
static constexpr uint16_t GET_PD_JSON = CMDID_PD_JSON;
/**实时参数获取**/
static constexpr uint16_t GET_RT_JSON = CMDID_GET_CURRENT_STATE_JSON;



/**设置**/
static const std::pair<uint16_t, std::vector<uint8_t>> SET_PRESS_START = {CMDID_SET_START_PRESS, {0x01}};
static const std::pair<uint16_t, std::vector<uint8_t>> SET_PRESS_STOP = {CMDID_SET_START_PRESS, {0x00}};
static const std::pair<uint16_t, std::vector<uint8_t>> SET_DEMOLD_START = {CMDID_SET_START_DEMOLD, {0x01}};
static const std::pair<uint16_t, std::vector<uint8_t>> SET_DEMOLD_STOP = {CMDID_SET_START_DEMOLD, {0x00}};

// CMDID_PRESS_STEP       		= 0x0100,
// CMDID_PRESS_CALIBRATE		= 0x0101,
// CMDID_MOLD_TYPE             = 0x0102,
// CMDID_CIRCLE_D              = 0x0103,
// CMDID_RECT_AB    			= 0x0104,
// CMDID_RING_OUT_IN 			= 0x0105,
// CMDID_AIR_TIME              = 0x0106,
// CMDID_INDEX_PRESS           = 0x0107,
// CMDID_INDEX_AFTER           = 0x0108,
// CMDID_INDEX_KPTIME          = 0x0109,
// CMDID_INDEX_PSTEP_PARAMETER = 0x010A,
// CMDID_PRESS_DECIMAL 		= 0x010B,
// CMDID_PRESSURE_DECIMAL 		= 0x010C,
// CMDID_PMODEL 				= 0x010D,
// CMDID_DEMOLD_VALUE 			= 0x010E,

enum RetCommand
{
    RET_SUCCESS = 0,
    RET_PORT_CONNECTED = -1,
    RET_WRITE_ERROR = -2,
    RET_READ_TOO_SHORT = -3,
    RET_INVALID_CHECKSUM = -4,
    RET_TIME_TOO_LONG = 1,
};


struct ReadOnlyData
{
    std::string m_NameZH;
    std::string m_NameEN;
    std::string m_Type;
    std::string m_SerialNumber;
    uint8_t m_Screenshot;
    uint8_t m_StartDelay;
    uint8_t m_VersionType;
    uint8_t m_IsHideLang;
    uint8_t m_Remote;
    uint8_t m_Network;
    uint8_t m_FontZH;
    uint8_t m_FontEN;

    /**压力相关**/
    uint8_t m_MaxPStep;
    float m_MaxPLimit;
    float m_MinPLimit;
    float m_Max_Min;
    float m_Diameter;
    uint8_t m_PDecimal;
    uint8_t m_PressDecimal; //压强
    uint8_t m_PModel; //是否有压力模式切换
    uint8_t m_OutType;

    /**温度相关**/
    uint8_t m_MaxTStep;
    float m_MaxTLimit;
    float m_MinTLimit;
    uint8_t m_TDecimal;
    uint8_t m_IsHasWater;
    uint8_t m_IsHasSpeed;
};

struct PressData
{
    uint8_t m_PStep;
    uint8_t m_Type;
    float m_A;
    float m_B;
    float m_D;
    float m_OuterD;
    float m_InnerD;
    float m_CheckValue;

    uint8_t m_Speed; //0 等静压，1普通
    float m_DemoldValue;

    std::vector<float> m_SetPValue;
    std::vector<float> m_AfterValue;
    std::vector<uint32_t> m_KPTime;

    std::string toJsonString() const;
};

static std::pair<uint16_t, std::vector<uint8_t>> SET_PRESS_STEP(const PressData& pressData)
{
    std::string json = pressData.toJsonString();
    // uint8_t *p = json.c_str();
    std::vector<uint8_t> pressStep;
    pressStep.insert(pressStep.begin(), 0x00);
    pressStep.insert(pressStep.end(), json.data(), json.data() + json.size());
    return {SET_CMD(CMDID_PD_JSON), pressStep};
}


struct RealTimeData
{
    uint8_t m_ModelState{ 0 }; //切换普通模式0/速度模式1/三速模式2
    uint8_t m_PressState{ 0 }; //0未加压，1：加压 2：脱模
    uint8_t m_CPStep{ 1 }; //当前步骤
    float m_PressValue{ .0f }; //压力数据
    uint32_t m_PTime{ 0 }; //倒计时
    uint8_t m_PdChanged{ 0 }; //压力数据是否改变
};


class MachineDataInterface
{
public:
    virtual ~MachineDataInterface() = default;
    virtual void onReadOnlyData(int errorCode,uint64_t registerNo, const ReadOnlyData& readOnlyData) = 0;
    virtual void onRealTimeData(int errorCode, const RealTimeData& realTimeData) = 0;
    virtual void onPressData(int errorCode, const PressData& pressData) = 0;
    virtual void onDataError(uint16_t cmdCode,std::vector<uint8_t> response) = 0;
    virtual void onCommandError(RetCommand errorCode, uint16_t cmdCode) = 0;
    virtual void onCommandPassWarningError(uint16_t cmdCode) = 0;
};


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
    os << "ModelState: " << static_cast<int>(realTimeData.m_ModelState) << std::endl;
    os << "PressState: " << static_cast<int>(realTimeData.m_PressState) << std::endl;
    os << "CPStep: " << static_cast<int>(realTimeData.m_CPStep) << std::endl;
    os << "PressValue: " << realTimeData.m_PressValue << std::endl;
    os << "PTime: " << realTimeData.m_PTime << std::endl;
    os << "PdChanged: " << static_cast<int>(realTimeData.m_PdChanged) << std::endl;
    os << '}' << std::endl;
    return os;
}

#ifdef QT_CORE_LIB
#include <QDebug>

inline QDebug& operator<<(QDebug& os, const RealTimeData& realTimeData)
{
    os << '{';
    os << "ModelState: " << static_cast<int>(realTimeData.m_ModelState);
    os << "PressState: " << static_cast<int>(realTimeData.m_PressState);
    os << "CPStep: " << static_cast<int>(realTimeData.m_CPStep);
    os << "PressValue: " << realTimeData.m_PressValue;
    os << "PTime: " << realTimeData.m_PTime;
    os << "PdChanged: " << realTimeData.m_PdChanged;
    os << '}';
    return os;
}

inline QDebug& operator<<(QDebug& os, const PressData& pressData)
{
    os << '{';
    os << "PStep: " << pressData.m_PStep;
    os << "Type: " << pressData.m_Type;
    os << "A: " << pressData.m_A;
    os << "B: " << pressData.m_B;
    os << "D: " << pressData.m_D;
    os << "OuterD: " << pressData.m_OuterD;
    os << "InnerD: " << pressData.m_InnerD;
    os << "CheckValue: " << pressData.m_CheckValue;
    os << "Speed: " << pressData.m_Speed;
    os << "DemoldValue: " << pressData.m_DemoldValue;
    os << "SetPValue size: " << pressData.m_SetPValue.size();
    os << "AfterValue size: " << pressData.m_AfterValue.size();
    os << "KPTime size: " << pressData.m_KPTime.size();
    os << '}';
    return os;
}

inline QDebug& operator<<(QDebug& os, const ReadOnlyData& readOnlyData)
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
