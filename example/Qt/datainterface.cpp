#include "datainterface.h"
#include "ui/uihome.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <cstring>

namespace
{
//! 拷贝 JSON 字符串字段到定长缓冲区 / copy a JSON string field into a fixed buffer
void copyString(char* dest, size_t size, const QJsonObject& object, const char* key)
{
    const QByteArray value = object.value(QLatin1String(key)).toString().toUtf8();
    std::strncpy(dest, value.constData(), size - 1);
    dest[size - 1] = '\0';
}

uint8_t toByte(const QJsonObject& object, const char* key)
{
    return static_cast<uint8_t>(object.value(QLatin1String(key)).toInt());
}

float toFloat(const QJsonObject& object, const char* key)
{
    return static_cast<float>(object.value(QLatin1String(key)).toDouble());
}

ReadOnlyData parseReadOnlyDataJson(const std::string& json)
{
    ReadOnlyData data{};
    const QJsonObject object = QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();

    copyString(data.m_NameZH, sizeof(data.m_NameZH), object, "NameZH");
    copyString(data.m_NameEN, sizeof(data.m_NameEN), object, "NameEN");
    copyString(data.m_Type, sizeof(data.m_Type), object, "Type");
    copyString(data.m_SerialNumber, sizeof(data.m_SerialNumber), object, "SerialNumber");

    data.m_Screenshot = toByte(object, "Screenshot");
    data.m_StartDelay = toByte(object, "StartDelay");
    data.m_VersionType = toByte(object, "VersionType");
    data.m_IsHideLang = toByte(object, "IsHideLang");
    data.m_Remote = toByte(object, "Remote");
    data.m_Network = toByte(object, "Network");

    data.m_MaxPStep = toByte(object, "MaxPStep");
    data.m_MaxPLimit = toFloat(object, "MaxPLimit");
    data.m_MinPLimit = toFloat(object, "MinPLimit");
    data.m_Max_Min = toFloat(object, "Max_Min");
    data.m_Diameter = toFloat(object, "Diameter");
    data.m_PDecimal = toByte(object, "PDecimal");
    data.m_PressDecimal = toByte(object, "PressDecimal");
    data.m_PModel = toByte(object, "PModel");
    data.m_OutType = toByte(object, "OutType");

    data.m_MaxTStep = toByte(object, "MaxTStep");
    data.m_MaxTLimit = toFloat(object, "MaxTLimit");
    data.m_MinTLimit = toFloat(object, "MinTLimit");
    data.m_TDecimal = toByte(object, "TDecimal");
    data.m_IsHasWater = toByte(object, "IsHasWater");
    data.m_IsHasSpeed = toByte(object, "IsHasSpeed");
    return data;
}

PressData parsePressDataJson(const std::string& json)
{
    PressData data{};
    const QJsonObject object = QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();

    data.m_PStep = toByte(object, "PStep");
    data.m_Type = toByte(object, "Type");
    data.m_A = toFloat(object, "A");
    data.m_B = toFloat(object, "B");
    data.m_D = toFloat(object, "D");
    data.m_CheckValue = toFloat(object, "CheckValue");
    data.m_AirTime = toByte(object, "AirTime");

    const QJsonArray setPValue = object.value(QStringLiteral("SetPValue")).toArray();
    const QJsonArray afterValue = object.value(QStringLiteral("AfterValue")).toArray();
    const QJsonArray kpTime = object.value(QStringLiteral("KPTime")).toArray();
    for (int i = 0; i < MAX_PStep; ++i)
    {
        data.m_SetPValue[i] = static_cast<float>(setPValue.at(i).toDouble());
        data.m_AfterValue[i] = static_cast<float>(afterValue.at(i).toDouble());
        data.m_KPTime[i] = static_cast<uint32_t>(kpTime.at(i).toDouble());
    }
    return data;
}

RealTimeData parseRealTimeDataJson(const std::string& json)
{
    RealTimeData data{};
    const QJsonObject object = QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();

    data.m_ModelState = toByte(object, "ModelState");
    data.m_IsPStart = toByte(object, "IsPStart");
    data.m_IsTStart = toByte(object, "IsTStart");
    data.m_IsT1Start = toByte(object, "IsT1Start");
    data.m_CPStep = toByte(object, "CPStep");
    data.m_CTStep = toByte(object, "CTStep");
    data.m_CT1Step = toByte(object, "CT1Step");
    data.m_PressValue = toFloat(object, "PValue");
    data.m_MoldPressure = toFloat(object, "MoldPressure");
    data.m_TUpValue = toFloat(object, "TUpValue");
    data.m_TDownValue = toFloat(object, "TDownValue");
    data.m_PTime = static_cast<uint32_t>(object.value(QStringLiteral("PTime")).toDouble());
    data.m_TTime = static_cast<uint32_t>(object.value(QStringLiteral("TTime")).toDouble());
    data.m_T1Time = static_cast<uint32_t>(object.value(QStringLiteral("T1Time")).toDouble());
    data.m_IsAir = toByte(object, "IsAir");

    // 推导兼容字段 / derived compatibility fields
    data.m_PressState = data.m_IsPStart != 0 ? 1 : 0;
    data.m_PdChanged = 0;
    return data;
}
} // namespace

DataInterface::DataInterface(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<ReadOnlyData>("ReadOnlyData");
    qRegisterMetaType<RealTimeData>("RealTimeData");
    qRegisterMetaType<PressData>("PressData");
    qRegisterMetaType<std::vector<uint8_t>>("std::vector<uint8_t>");
}

DataInterface::~DataInterface() = default;

ReadOnlyData DataInterface::parseReadOnlyData(const std::string& json)
{
    return parseReadOnlyDataJson(json);
}

PressData DataInterface::parsePressData(const std::string& json)
{
    return parsePressDataJson(json);
}

RealTimeData DataInterface::parseRealTimeData(const std::string& json)
{
    return parseRealTimeDataJson(json);
}

std::string DataInterface::serializePressData(const PressData& data)
{
    QJsonObject object;
    object.insert(QStringLiteral("PStep"), static_cast<int>(data.m_PStep));
    object.insert(QStringLiteral("Type"), static_cast<int>(data.m_Type));
    object.insert(QStringLiteral("A"), data.m_A);
    object.insert(QStringLiteral("B"), data.m_B);
    object.insert(QStringLiteral("D"), data.m_D);
    object.insert(QStringLiteral("CheckValue"), data.m_CheckValue);
    object.insert(QStringLiteral("AirTime"), static_cast<int>(data.m_AirTime));

    QJsonArray setPValue;
    QJsonArray afterValue;
    QJsonArray kpTime;
    for (int i = 0; i < MAX_PStep; ++i)
    {
        setPValue.append(data.m_SetPValue[i]);
        afterValue.append(data.m_AfterValue[i]);
        kpTime.append(static_cast<int>(data.m_KPTime[i]));
    }
    object.insert(QStringLiteral("SetPValue"), setPValue);
    object.insert(QStringLiteral("AfterValue"), afterValue);
    object.insert(QStringLiteral("KPTime"), kpTime);

    return QJsonDocument(object).toJson(QJsonDocument::Compact).toStdString();
}


void DataInterface::onReadOnlyData(int errorCode, uint64_t registerNo, const std::string& json)
{
    emit signalReadOnlyData(errorCode, registerNo, parseReadOnlyDataJson(json));
}

void DataInterface::onRealTimeData(int errorCode, const std::string& json)
{
    emit signalRealTimeData(errorCode, parseRealTimeDataJson(json));
}

void DataInterface::onPressData(int errorCode, const std::string& json)
{
    emit signalPressData(errorCode, parsePressDataJson(json));
}

void DataInterface::onError(uint16_t cmdCode, std::vector<uint8_t> response)
{
    emit signalError(cmdCode, response);
}

void DataInterface::onTempData(int errorCode, const std::string& json)
{
    emit signalTempData(errorCode, json);
}

void DataInterface::onMixData(int errorCode, const std::string& json)
{
    emit signalMixData(errorCode, json);
}
