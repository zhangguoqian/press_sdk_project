//
// Created by 11518 on 2026/9/14.
//

#include "machinedata.h"
#include "tool/jsontovalue.h"
#include <algorithm>


ReadOnlyData MachineData::getReadOnlyData() const
{
    return m_ReadOnlyData;
}

PressData MachineData::getPressData() const
{
    return m_PressData;
}

RealTimeData MachineData::getRealTimeData() const
{
    return m_RealTimeData;
}

void MachineData::setReadOnlyData(const ReadOnlyData& data)
{
    m_ReadOnlyData = data;
}

void MachineData::setPressData(const PressData& data)
{
    m_PressData = data;
}

void MachineData::setRealTimeData(const RealTimeData& data)
{
    m_RealTimeData = data;
}

int MachineData::jsonToReadOnlyData(const std::string& json) // NOLINT(*-convert-member-functions-to-static)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json, root, false))
    {
        return -1;
    }
    RetValueCode code = RetValueCode::VALUE_NO_ERROR;
    code = JsonToValue::getStringValue("NameZH", root, m_ReadOnlyData.m_NameZH);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 1;
    }
    code = JsonToValue::getStringValue("NameEN", root, m_ReadOnlyData.m_NameEN);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 2;
    }
    code = JsonToValue::getStringValue("Type", root, m_ReadOnlyData.m_Type);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 3;
    }
    code = JsonToValue::getUInt8Value("Screenshot", root, m_ReadOnlyData.m_Screenshot, 0, 1);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 4;
    }
    code = JsonToValue::getUInt8Value("StartDelay", root, m_ReadOnlyData.m_StartDelay, 0, 10);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 5;
    }
    code = JsonToValue::getUInt8Value("VersionType", root, m_ReadOnlyData.m_VersionType, 0, 4);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 6;
    }
    code = JsonToValue::getUInt8Value("IsHideLang", root, m_ReadOnlyData.m_IsHideLang, 0, 1);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 7;
    }
    code = JsonToValue::getUInt8Value("Remote", root, m_ReadOnlyData.m_Remote, 0, 4);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 8;
    }
    code = JsonToValue::getUInt8Value("Network", root, m_ReadOnlyData.m_Network, 0, 4);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 9;
    }

    code = JsonToValue::getUInt8Value("FontZH", root, m_ReadOnlyData.m_FontZH, 0, 32);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 10;
    }
    code = JsonToValue::getUInt8Value("FontEN", root, m_ReadOnlyData.m_FontEN, 0, 32);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 11;
    }
    /**压力相关**/
    code = JsonToValue::getUInt8Value("MaxPStep", root, m_ReadOnlyData.m_MaxPStep, 0, 100);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 12;
    }
    code = JsonToValue::getFloatValue("MaxPLimit", root, m_ReadOnlyData.m_MaxPLimit, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 14;
    }
    code = JsonToValue::getFloatValue("MinPLimit", root, m_ReadOnlyData.m_MinPLimit, 0.0f, 10.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 15;
    }
    code = JsonToValue::getFloatValue("Max_Min", root, m_ReadOnlyData.m_Max_Min, 0.0f, 10.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 16;
    }
    code = JsonToValue::getFloatValue("Diameter", root, m_ReadOnlyData.m_Diameter, 0, 200);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 17;
    }
    code = JsonToValue::getUInt8Value("PDecimal", root, m_ReadOnlyData.m_PDecimal, 0, 10);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 18;
    }
    code = JsonToValue::getUInt8Value("PressDecimal", root, m_ReadOnlyData.m_PressDecimal, 0, 10);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 19;
    }
    code = JsonToValue::getUInt8Value("PModel", root, m_ReadOnlyData.m_PModel, 0, 10);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 20;
    }
    code = JsonToValue::getUInt8Value("OutType", root, m_ReadOnlyData.m_OutType, 0, 10);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 21;
    }

    /**温度相关**/
    code = JsonToValue::getUInt8Value("MaxTStep", root, m_ReadOnlyData.m_MaxTStep, 0, 100);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 22;
    }
    code = JsonToValue::getFloatValue("MaxTLimit", root, m_ReadOnlyData.m_MaxTLimit, 0.0f, 510.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 23;
    }
    code = JsonToValue::getFloatValue("MinTLimit", root, m_ReadOnlyData.m_MinTLimit, 0.0f, 510.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 24;
    }
    code = JsonToValue::getUInt8Value("TDecimal", root, m_ReadOnlyData.m_TDecimal, 0, 10);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 25;
    }
    code = JsonToValue::getUInt8Value("IsHasWater", root, m_ReadOnlyData.m_IsHasWater, 0, 1);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 26;
    }
    code = JsonToValue::getUInt8Value("IsHasSpeed", root, m_ReadOnlyData.m_IsHasSpeed, 0, 1);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 27;
    }
    return 0;
}

int MachineData::jsonToRealTimeData(const std::string& json)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json, root, false))
    {
        return -1;
    }
    RetValueCode code = RetValueCode::VALUE_NO_ERROR;
    code = JsonToValue::getUInt8Value("ModelState", root, m_RealTimeData.m_ModelState, 0, 2);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 1;
    }
    code = JsonToValue::getUInt8Value("PressState", root, m_RealTimeData.m_PressState, 0, 2);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 2;
    }
    code = JsonToValue::getUInt8Value("CPStep", root, m_RealTimeData.m_CPStep, 0, 100);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 3;
    }
    code = JsonToValue::getFloatValue("PressValue", root, m_RealTimeData.m_PressValue, 0.0f, 400.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 4;
    }
    code = JsonToValue::getUInt32Value("PTime", root, m_RealTimeData.m_PTime, 0, 0xFFFFFFFF);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 5;
    }
    code = JsonToValue::getUInt8Value("PdChanged", root, m_RealTimeData.m_PdChanged, 0, 1);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 6;
    }
    return 0;
}

int MachineData::jsonToPressData(const std::string& json) // NOLINT(*-convert-member-functions-to-static)
{
    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(json, root, false))
    {
        return -1;
    }
    RetValueCode code = RetValueCode::VALUE_NO_ERROR;
    code = JsonToValue::getUInt8Value("PStep", root, m_PressData.m_PStep, 0, 30);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 1;
    }
    code = JsonToValue::getUInt8Value("Type", root, m_PressData.m_Type, 0, 4);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 2;
    }
    code = JsonToValue::getFloatValue("A", root, m_PressData.m_A, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 3;
    }
    code = JsonToValue::getFloatValue("B", root, m_PressData.m_B, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 4;
    }
    code = JsonToValue::getFloatValue("D", root, m_PressData.m_D, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 5;
    }
    code = JsonToValue::getFloatValue("OuterD", root, m_PressData.m_OuterD, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 6;
    }
    code = JsonToValue::getFloatValue("InnerD", root, m_PressData.m_InnerD, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 7;
    }
    code = JsonToValue::getFloatValue("CheckValue", root, m_PressData.m_CheckValue, 0.0f, 5.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 8;
    }

    code = JsonToValue::getUInt8Value("Speed", root, m_PressData.m_Speed, 0, 1);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 9;
    }
    code = JsonToValue::getFloatValue("DemoldValue", root, m_PressData.m_DemoldValue, 0.0f, 200.0f);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 10;
    }

    std::vector<float> tmpFv;
    code = JsonToValue::getFloatVector("SetPValue", root, tmpFv, 0.0f, 200.0f, 30);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 11;
    }
    std::copy(tmpFv.begin(), tmpFv.end(), m_PressData.m_SetPValue.begin());

    tmpFv.clear();
    code = JsonToValue::getFloatVector("AfterValue", root, tmpFv, 0.0f, 200.0f, 30);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 12;
    }
    std::copy(tmpFv.begin(), tmpFv.end(), m_PressData.m_AfterValue.begin());

    std::vector<uint32_t> tmpU32;
    code = JsonToValue::getUInt32Vector("KPTime", root, tmpU32, 0, 0xFFFFFFFF, 30);
    if (code != RetValueCode::VALUE_NO_ERROR)
    {
        return 13;
    }
    std::copy(tmpU32.begin(), tmpU32.end(), m_PressData.m_KPTime.begin());
    return 0;
}

int MachineData::jsonToReadOnlyData(const std::vector<uint8_t>& data)
{
    std::string str(data.begin(), data.end());
    return jsonToReadOnlyData(str);
}

int MachineData::jsonToRealTimeData(const std::vector<uint8_t>& data)
{
    std::string str(data.begin(), data.end());
    return jsonToRealTimeData(str);
}

int MachineData::jsonToPressData(const std::vector<uint8_t>& data)
{
    std::string str(data.begin(), data.end());
    return jsonToPressData(str);
}

std::string ReadOnlyData::toJsonString() const
{
    Json::Value root{};

    root["NameZH"]       = m_NameZH;
    root["NameEN"]       = m_NameEN;
    root["Type"]         = m_Type;
    root["SerialNumber"] = m_SerialNumber;

    root["Screenshot"]   = m_Screenshot;
    root["StartDelay"]   = m_StartDelay;
    root["VersionType"]  = m_VersionType;
    root["IsHideLang"]   = m_IsHideLang;
    root["Remote"]       = m_Remote;
    root["Network"]      = m_Network;
    root["FontZH"]       = m_FontZH;
    root["FontEN"]       = m_FontEN;

    root["MaxPStep"]     = m_MaxPStep;
    root["MaxPLimit"]    = m_MaxPLimit;
    root["MinPLimit"]    = m_MinPLimit;
    root["Max_Min"]      = m_Max_Min;
    root["Diameter"]     = m_Diameter;
    root["PDecimal"]     = m_PDecimal;
    root["PressDecimal"] = m_PressDecimal;
    root["PModel"]       = m_PModel;
    root["OutType"]      = m_OutType;

    root["MaxTStep"]     = m_MaxTStep;
    root["MaxTLimit"]    = m_MaxTLimit;
    root["MinTLimit"]    = m_MinTLimit;
    root["TDecimal"]     = m_TDecimal;
    root["IsHasWater"]   = m_IsHasWater;
    root["IsHasSpeed"]   = m_IsHasSpeed;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["precision"] = 4;
    return Json::writeString(builder, root);
}

std::vector<uint8_t> PressData::toFrameData() const
{
    std::string str = toJsonString();
    return {str.begin(), str.end()};
}

std::string PressData::toJsonString() const
{
    Json::Value root{};

    root["PStep"] = m_PStep;
    root["Type"] = m_Type;
    //	root["PDecimal"] = m_PDecimal;
    //	root["PressDecimal"] = m_PressDecimal;
    root["A"] = m_A;
    root["B"] = m_B;
    root["D"] = m_D;
    root["OuterD"] = m_OuterD;
    root["InnerD"] = m_InnerD;
    root["CheckValue"] = m_CheckValue;
    root["Speed"] = m_Speed;
    root["DemoldValue"] = m_DemoldValue;

    Json::Value array;
    for(const auto &var:m_SetPValue){
        array.append(var);
    }
    root["SetPValue"] = array;

    array.clear();
    for(const auto &var:m_AfterValue){
        array.append(var);
    }
    root["AfterValue"] = array;

    array.clear();
    for(const auto &var:m_KPTime){
        array.append(var);
    }
    root["KPTime"] = array;

    array.clear();
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["precision"] = 4;
    return Json::writeString(builder, root);
}

std::string RealTimeData::toJsonString() const
{
    Json::Value root{};

    root["ModelState"] = m_ModelState;
    root["PressState"] = m_PressState;
    root["CPStep"]     = m_CPStep;
    root["PressValue"] = m_PressValue;
    root["PTime"]      = m_PTime;
    root["PdChanged"]  = m_PdChanged;

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    builder["precision"] = 4;
    return Json::writeString(builder, root);
}