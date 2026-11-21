#include "core/press_json.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace {

double jsonNumber(const QJsonObject &object, const char *key)
{
    return object.value(QLatin1String(key)).toDouble();
}

int jsonInt(const QJsonObject &object, const char *key)
{
    return object.value(QLatin1String(key)).toInt();
}

QJsonObject parseObject(const std::string &json)
{
    return QJsonDocument::fromJson(QByteArray::fromStdString(json)).object();
}

} // namespace

namespace PressJson {

PressReadOnlyData parseReadOnly(const std::string &json)
{
    PressReadOnlyData data;
    const QJsonObject object = parseObject(json);

    data.nameZh = object.value(QLatin1String("NameZH")).toString();
    data.nameEn = object.value(QLatin1String("NameEN")).toString();
    data.type = object.value(QLatin1String("Type")).toString();
    data.serialNumber = object.value(QLatin1String("SerialNumber")).toString();

    data.maxPStep = jsonInt(object, "MaxPStep");
    data.maxTStep = jsonInt(object, "MaxTStep");
    data.maxPLimit = jsonNumber(object, "MaxPLimit");
    data.minPLimit = jsonNumber(object, "MinPLimit");
    data.autoBoost = jsonNumber(object, "Max_Min");
    data.diameter = jsonNumber(object, "Diameter");
    data.maxTLimit = jsonNumber(object, "MaxTLimit");
    data.minTLimit = jsonNumber(object, "MinTLimit");

    data.pDecimal = jsonInt(object, "PDecimal");
    data.pressDecimal = jsonInt(object, "PressDecimal");
    data.tDecimal = jsonInt(object, "TDecimal");

    data.hasWater = jsonInt(object, "IsHasWater") != 0;
    data.hasSpeed = jsonInt(object, "IsHasSpeed") != 0;
    data.network = jsonInt(object, "Network") != 0;
    data.startDelay = jsonInt(object, "StartDelay");
    data.versionType = jsonInt(object, "VersionType");
    return data;
}

PressProgramData parsePressProgram(const std::string &json)
{
    PressProgramData data;
    const QJsonObject object = parseObject(json);

    data.stepCount = qBound(1, jsonInt(object, "PStep"), 30);
    data.type = jsonInt(object, "Type");
    data.a = jsonNumber(object, "A");
    data.b = jsonNumber(object, "B");
    data.d = jsonNumber(object, "D");
    data.checkValue = jsonNumber(object, "CheckValue");
    data.airTime = jsonInt(object, "AirTime");

    const auto readNumbers = [&object](const char *key) {
        QVector<double> values;
        const QJsonArray array = object.value(QLatin1String(key)).toArray();
        values.reserve(array.size());
        for (const QJsonValue &value : array)
            values.append(value.toDouble());
        return values;
    };
    data.setP = readNumbers("SetPValue");
    data.afterP = readNumbers("AfterValue");

    const QJsonArray keep = object.value(QLatin1String("KPTime")).toArray();
    data.keepSec.reserve(keep.size());
    for (const QJsonValue &value : keep)
        data.keepSec.append(value.toInt());
    return data;
}

PressTempProgramData parseTempProgram(const std::string &json)
{
    PressTempProgramData data;
    const QJsonObject object = parseObject(json);

    data.stepCount = qBound(1, jsonInt(object, "TStep"), 30);
    data.type = jsonInt(object, "Type");
    data.minWater = jsonNumber(object, "MinWater");
    data.maxWater = jsonNumber(object, "MaxWater");

    const auto readNumbers = [&object](const char *key) {
        QVector<double> values;
        const QJsonArray array = object.value(QLatin1String(key)).toArray();
        values.reserve(array.size());
        for (const QJsonValue &value : array)
            values.append(value.toDouble());
        return values;
    };
    data.startTemp = readNumbers("StartTemp");
    data.speed = readNumbers("Speed");

    const QJsonArray keep = object.value(QLatin1String("KTTime")).toArray();
    data.keepSec.reserve(keep.size());
    for (const QJsonValue &value : keep)
        data.keepSec.append(value.toInt());
    return data;
}

PressMixProgramData parseMixProgram(const std::string &json)
{
    PressMixProgramData data;
    const QJsonObject object = parseObject(json);

    data.stepCount = qBound(1, jsonInt(object, "Step"), 30);
    data.type = jsonInt(object, "Type");
    data.a = jsonNumber(object, "A");
    data.b = jsonNumber(object, "B");
    data.d = jsonNumber(object, "D");

    const auto readNumbers = [&object](const char *key) {
        QVector<double> values;
        const QJsonArray array = object.value(QLatin1String(key)).toArray();
        values.reserve(array.size());
        for (const QJsonValue &value : array)
            values.append(value.toDouble());
        return values;
    };
    const auto readInts = [&object](const char *key) {
        QVector<int> values;
        const QJsonArray array = object.value(QLatin1String(key)).toArray();
        values.reserve(array.size());
        for (const QJsonValue &value : array)
            values.append(value.toInt());
        return values;
    };

    data.isPress = readInts("IsPress");
    data.setP = readNumbers("SetPValue");
    data.afterP = readNumbers("AfterValue");
    data.isUpHeat = readInts("IsUpHeat");
    data.upTemp = readNumbers("UpTemp");
    data.isDownHeat = readInts("IsDownHeat");
    data.downTemp = readNumbers("DownTemp");
    data.keepSec = readInts("KTime");
    return data;
}

PressRtData parseRealTime(const std::string &json)
{
    PressRtData data;
    const QJsonObject object = parseObject(json);

    data.modelState = jsonInt(object, "ModelState");
    data.pressOn = jsonInt(object, "IsPStart") != 0;
    data.heatOn = jsonInt(object, "IsTStart") != 0;
    data.airAllowed = jsonInt(object, "IsAir") != 0;
    data.pressStep = jsonInt(object, "CPStep");
    data.tempStep = jsonInt(object, "CTStep");
    data.mixOn = jsonInt(object, "IsMStart") != 0;
    data.mixStep = jsonInt(object, "CMStep");
    data.mixRemainSec = jsonInt(object, "MTime");
    data.pressure = jsonNumber(object, "PValue");
    data.tempUp = jsonNumber(object, "TUpValue");
    data.tempDown = jsonNumber(object, "TDownValue");
    data.pressRemainSec = jsonInt(object, "PTime");
    data.tempRemainSec = jsonInt(object, "TTime");
    data.flowChange = jsonInt(object, "FlowChange");
    return data;
}

} // namespace PressJson
