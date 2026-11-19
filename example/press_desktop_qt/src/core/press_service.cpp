#include "core/press_service.h"

#include "core/format.h"
#include "press/press.hpp"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

using PortType = PressPortType;

namespace {

double jsonNumber(const QJsonObject &object, const char *key)
{
    return object.value(QLatin1String(key)).toDouble();
}

int jsonInt(const QJsonObject &object, const char *key)
{
    return object.value(QLatin1String(key)).toInt();
}

PressReadOnlyData parseReadOnly(const std::string &json)
{
    PressReadOnlyData data;
    const QJsonObject object = QJsonDocument::fromJson(
        QByteArray::fromStdString(json)).object();

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
    const QJsonObject object = QJsonDocument::fromJson(
        QByteArray::fromStdString(json)).object();

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
    const QJsonObject object = QJsonDocument::fromJson(
        QByteArray::fromStdString(json)).object();

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
    const QJsonObject object = QJsonDocument::fromJson(
        QByteArray::fromStdString(json)).object();

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
    const QJsonObject object = QJsonDocument::fromJson(
        QByteArray::fromStdString(json)).object();

    data.modelState = jsonInt(object, "ModelState");
    data.pressOn = jsonInt(object, "IsPStart") != 0;
    data.heatOn = jsonInt(object, "IsTStart") != 0 || jsonInt(object, "IsT1Start") != 0;
    data.air = jsonInt(object, "IsAir") != 0;
    data.pressStep = jsonInt(object, "CPStep");
    data.tempStep = jsonInt(object, "CTStep");
    data.pressure = jsonNumber(object, "PValue");
    data.moldPressure = jsonNumber(object, "MoldPressure");
    data.tempUp = jsonNumber(object, "TUpValue");
    data.tempDown = jsonNumber(object, "TDownValue");
    data.pressRemainSec = jsonInt(object, "PTime");
    data.tempRemainSec = jsonInt(object, "TTime");
    return data;
}

} // namespace

/**
 * SDK 回调适配器：只负责把 SDK 线程的回调转给 PressService，
 * 使 press.hpp 不出现在应用头文件中。
 */
class SdkAdapter : public PressDataInterface
{
public:
    explicit SdkAdapter(PressService *owner) : m_owner(owner) {}

    void onReadOnlyData(int, uint64_t registerNo, const std::string &json) override
    {
        m_owner->handleReadOnlyData(registerNo, json);
    }
    void onRealTimeData(int errorCode, const std::string &json) override
    {
        m_owner->handleRealTime(errorCode, json);
    }
    void onPressData(int, const std::string &json) override
    {
        m_owner->handlePressData(json);
    }
    void onTempData(int, const std::string &json) override
    {
        m_owner->handleTempData(json);
    }
    void onMixData(int, const std::string &json) override
    {
        m_owner->handleMixData(json);
    }
    void onError(uint16_t cmdCode, std::vector<uint8_t>) override
    {
        emit m_owner->sdkError(QStringLiteral("设备返回错误：命令 0x%1")
                                   .arg(cmdCode, 4, 16, QLatin1Char('0'))
                                   .toUpper());
    }

private:
    PressService *m_owner;
};

PressService::PressService(QObject *parent)
    : QObject(parent)
    , m_press(new Press())
    , m_adapter(new SdkAdapter(this))
{
    qRegisterMetaType<PressRtData>("PressRtData");
    qRegisterMetaType<PressReadOnlyData>("PressReadOnlyData");
    qRegisterMetaType<PressProgramData>("PressProgramData");
    qRegisterMetaType<PressTempProgramData>("PressTempProgramData");
    qRegisterMetaType<PressMixProgramData>("PressMixProgramData");
    m_press->setRequestTimeout(1500);
}

PressService::~PressService()
{
    // 等待在途连接结束，避免后台线程访问已析构的 Press
    if (m_connectTask.valid())
        m_connectTask.wait();
    m_connecting = false;
    disconnectDevice();
}

void PressService::handleRealTime(int errorCode, const std::string &json)
{
    if (errorCode != 0 || json.empty()) {
        emit sdkError(QStringLiteral("实时数据读取失败（%1）").arg(errorCode));
        return;
    }
    if (!m_loggedFirstRt) {
        m_loggedFirstRt = true;
        emit logMessage(QStringLiteral("实时数据已开始上报，周期采集运行中（1 s）"));
    }
    emit realTimeUpdated(parseRealTime(json));
}

void PressService::handleReadOnlyData(uint64_t registerNo, const std::string &json)
{
    Q_UNUSED(registerNo)
    if (json.empty())
        return;
    const PressReadOnlyData data = parseReadOnly(json);
    emit logMessage(QStringLiteral(
        "ReadOnlyData：%1（%2），SN %3，VersionType=%4，压力段上限 %5，温度段上限 %6")
        .arg(data.nameZh, data.type, data.serialNumber)
        .arg(data.versionType)
        .arg(data.maxPStep)
        .arg(data.maxTStep));
    emit readOnlyDataUpdated(data);
}

void PressService::handlePressData(const std::string &json)
{
    if (json.empty())
        return;
    const PressProgramData data = parsePressProgram(json);
    emit logMessage(QStringLiteral("PressData：%1 段，模具类型 %2")
                        .arg(data.stepCount)
                        .arg(data.type));
    emit pressProgramUpdated(data);
}

void PressService::handleTempData(const std::string &json)
{
    if (json.empty())
        return;
    const PressTempProgramData data = parseTempProgram(json);
    emit logMessage(QStringLiteral("TempData：%1 段，降温水温 %2 °C，升温水温 %3 °C，模式 %4")
                        .arg(data.stepCount)
                        .arg(fmtNumber(data.minWater))
                        .arg(fmtNumber(data.maxWater))
                        .arg(data.type == 0 ? QStringLiteral("普通")
                                            : QStringLiteral("速率")));
    emit tempProgramUpdated(data);
}

void PressService::handleMixData(const std::string &json)
{
    if (json.empty())
        return;
    const PressMixProgramData data = parseMixProgram(json);
    emit logMessage(QStringLiteral("MixData：%1 段，模具类型 %2")
                        .arg(data.stepCount)
                        .arg(data.type));
    emit mixProgramUpdated(data);
}

QStringList PressService::availablePorts()
{
    QStringList ports;
    for (const std::string &name : Press::getPortList())
        ports.append(QString::fromStdString(name));
    return ports;
}

QString PressService::lastError() const
{
    const char *text = m_press->getLastErrorInfo();
    return text ? QString::fromUtf8(text) : QStringLiteral("未知错误");
}

void PressService::connectDevice(const QString &endpoint, bool tcp)
{
    const QString target = endpoint.trimmed();
    if (target.isEmpty() || m_connecting.exchange(true))
        return;

    // 重新连接前先释放旧连接
    if (m_connected) {
        m_press->unregisterDataInterface();
        m_press->disconnect();
        m_connected = false;
    }
    m_endpoint = target;
    m_loggedFirstRt = false;
    emit logMessage(QStringLiteral("正在连接设备 %1（%2）…")
                        .arg(target, tcp ? QStringLiteral("TCP")
                                         : QStringLiteral("串口")));

    // 连接可能阻塞数秒（TCP 握手 / 串口打开），放到后台线程执行
    m_connectTask = std::async(std::launch::async, [this, target, tcp] {
        const QByteArray name = target.toUtf8();
        const bool ok = m_press->connect(name.constData(),
                                         tcp ? PortType::TcpSocketPortType
                                             : PortType::SerialPortType);
        QString error;
        std::string firstRt;
        if (ok) {
            // 连接成功后启动周期采集：1s 一次，回调在 SDK 线程执行
            m_press->registerDataInterface(m_adapter.get(), 1);

            // 按 ROD → PD → TD → MD 顺序读取静态参数（get 接口同步触发对应回调），
            // 最后再读取一次实时数据
            std::string json;
            m_press->getReadOnlyDataJson(json);
            m_press->getPressDataJson(json);
            m_press->getTempDataJson(json);
            m_press->getMixDataJson(json);

            std::string rt;
            if (m_press->getRealTimeDataJson(rt))
                firstRt = rt;
        } else {
            const char *text = m_press->getLastErrorInfo();
            error = QStringLiteral("连接失败：%1")
                        .arg(text ? QString::fromUtf8(text) : QStringLiteral("未知错误"));
        }

        QMetaObject::invokeMethod(this, [this, ok, error, firstRt] {
            m_connecting = false;
            m_connected = ok;
            emit connectionChanged(ok, m_endpoint);
            if (!ok) {
                emit logMessage(QStringLiteral("设备连接失败：%1").arg(error));
                emit sdkError(error);
                return;
            }
            emit logMessage(QStringLiteral("设备连接成功，已注册 1 s 周期采集，开始读取设备参数"));
            // 周期采集启用时 getRealTimeDataJson 不触发回调，这里补发首次实时数据
            if (!firstRt.empty())
                handleRealTime(0, firstRt);
        }, Qt::QueuedConnection);
    });
}

void PressService::disconnectDevice()
{
    if (m_connecting.load())
        return;
    if (!m_connected && !m_press->isConnected())
        return;
    m_press->unregisterDataInterface();
    m_press->disconnect();
    m_connected = false;
    emit logMessage(QStringLiteral("设备连接已断开：%1").arg(m_endpoint));
    emit connectionChanged(false, m_endpoint);
}

bool PressService::setPressing(bool on, QString *error)
{
    return report(check(m_press->setPressing(on), error,
                        on ? QStringLiteral("启动加压") : QStringLiteral("停止加压")),
                  on ? QStringLiteral("指令：启动加压") : QStringLiteral("指令：停止加压"));
}

bool PressService::setStartHeat(bool on, QString *error)
{
    return report(check(m_press->setStartHeat(on), error,
                        on ? QStringLiteral("启动加热") : QStringLiteral("停止加热")),
                  on ? QStringLiteral("指令：启动加热") : QStringLiteral("指令：停止加热"));
}

bool PressService::setStart(bool on, QString *error)
{
    return report(check(m_press->setStart(on), error,
                        on ? QStringLiteral("启动合作流程") : QStringLiteral("停止合作流程")),
                  on ? QStringLiteral("指令：启动合作流程")
                     : QStringLiteral("指令：停止合作流程"));
}

bool PressService::selectModel(int model)
{
    const QString name = model == 1 ? QStringLiteral("合作模式")
                                    : QStringLiteral("单独模式");
    return report(m_press->selectModelState(static_cast<uint8_t>(model)),
                  QStringLiteral("切换为%1").arg(name));
}

bool PressService::pushStepCounts(int pressSteps, int tempSteps)
{
    const bool ok = m_press->setPressStepCount(static_cast<uint8_t>(qBound(1, pressSteps, 30)))
        && m_press->setTempStepCount(static_cast<uint8_t>(qBound(1, tempSteps, 30)));
    return report(ok, QStringLiteral("下发阶段数：压力 %1 段 / 温度 %2 段")
                          .arg(pressSteps)
                          .arg(tempSteps));
}

bool PressService::pushPressureStage(int index, double pressure, double boost, int holdSeconds)
{
    if (index < 0)
        return false;
    const quint8 i = static_cast<quint8>(index);
    const bool ok = m_press->setPressValueIndex(i, static_cast<float>(pressure))
        && m_press->setAfterValueIndex(i, static_cast<float>(boost))
        && m_press->setKeepPressTime(i, holdSeconds);
    return report(ok, QStringLiteral("下发压力段 %1：%2 Ton，补压 %3 Ton，保压 %4")
                          .arg(index + 1)
                          .arg(fmtNumber(pressure))
                          .arg(fmtNumber(boost))
                          .arg(fmtTime(holdSeconds)));
}

bool PressService::pushTemperatureStage(int index, double temperature, double rate, int holdSeconds)
{
    if (index < 0)
        return false;
    const quint8 i = static_cast<quint8>(index);
    const bool ok = m_press->setTempIndex(i, static_cast<float>(temperature))
        && m_press->setTempSpeedIndex(i, static_cast<float>(rate))
        && m_press->setKeepTempTime(i, holdSeconds);
    return report(ok, QStringLiteral("下发温度段 %1：%2 °C，速率 %3，保温 %4")
                          .arg(index + 1)
                          .arg(fmtNumber(temperature))
                          .arg(fmtNumber(rate))
                          .arg(fmtTime(holdSeconds)));
}

bool PressService::pushCoopCount(int steps)
{
    return report(m_press->setMixStepCount(static_cast<uint8_t>(qBound(1, steps, 30))),
                  QStringLiteral("下发合作阶段数：%1 段").arg(steps));
}

bool PressService::pushCoopStage(int index, bool press, double pressure, double boost,
                                 bool heatUp, double tempUp, bool heatDown, double tempDown,
                                 int holdSeconds)
{
    if (index < 0)
        return false;
    const quint8 i = static_cast<quint8>(index);
    const bool ok = m_press->setMixIsPressIndex(i, press)
        && m_press->setMixPressIndex(i, static_cast<float>(pressure))
        && m_press->setMixAfterPressIndex(i, static_cast<float>(boost))
        && m_press->setMixIsHeatUpIndex(i, heatUp)
        && m_press->setMixHeatUpIndex(i, static_cast<float>(tempUp))
        && m_press->setMixIsHeatDownIndex(i, heatDown)
        && m_press->setMixHeatDownIndex(i, static_cast<float>(tempDown))
        && m_press->setMixKeepTime(i, holdSeconds);
    return report(ok, QStringLiteral(
                          "下发合作段 %1：%2，%3 Ton，上板 %4 °C，下板 %5 °C，计时 %6")
                          .arg(index + 1)
                          .arg(press ? QStringLiteral("加压") : QStringLiteral("不加压"))
                          .arg(fmtNumber(pressure))
                          .arg(heatUp ? fmtNumber(tempUp) : QStringLiteral("关"))
                          .arg(heatDown ? fmtNumber(tempDown) : QStringLiteral("关"))
                          .arg(fmtTime(holdSeconds)));
}

bool PressService::pushMoldCircle(double diameter)
{
    return report(m_press->setCircle(static_cast<float>(diameter)),
                  QStringLiteral("下发模具：圆形 D=%1 mm").arg(fmtNumber(diameter)));
}

bool PressService::pushMoldRect(double length, double width)
{
    return report(m_press->setRectangle(static_cast<float>(length),
                                        static_cast<float>(width)),
                  QStringLiteral("下发模具：矩形 %1 × %2 mm")
                      .arg(fmtNumber(length), fmtNumber(width)));
}

bool PressService::pushCoopMoldCircle(double diameter)
{
    return report(m_press->setMixCircle(static_cast<float>(diameter)),
                  QStringLiteral("下发合作模具：圆形 D=%1 mm").arg(fmtNumber(diameter)));
}

bool PressService::pushCoopMoldRect(double length, double width)
{
    return report(m_press->setMixRectangle(static_cast<float>(length),
                                           static_cast<float>(width)),
                  QStringLiteral("下发合作模具：矩形 %1 × %2 mm")
                      .arg(fmtNumber(length), fmtNumber(width)));
}

bool PressService::pushCoopMoldType(int type)
{
    return report(m_press->setMixMoldType(static_cast<uint8_t>(type)),
                  QStringLiteral("下发合作模具类型：%1").arg(type));
}

bool PressService::report(bool ok, const QString &action)
{
    if (ok)
        emit logMessage(QStringLiteral("%1成功").arg(action));
    else
        emit logMessage(QStringLiteral("%1失败：%2").arg(action, lastError()));
    return ok;
}

bool PressService::check(bool ok, QString *error, const QString &action)
{
    if (ok)
        return true;
    const QString message = QStringLiteral("%1失败：%2").arg(action, lastError());
    if (error)
        *error = message;
    return false;
}
