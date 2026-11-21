#include "core/press_service.h"

#include "core/app_config.h"
#include "core/app_settings.h"
#include "core/format.h"
#include "core/press_json.h"
#include "press/press.hpp"

#include <QByteArray>
#include <QDateTime>
#include <QTimer>

using PortType = PressPortType;

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
    qRegisterMetaType<DeviceVersionInfo>("DeviceVersionInfo");
    qRegisterMetaType<PressProgramData>("PressProgramData");
    qRegisterMetaType<PressTempProgramData>("PressTempProgramData");
    qRegisterMetaType<PressMixProgramData>("PressMixProgramData");
    qRegisterMetaType<LogLevel>("LogLevel");
    m_press->setRequestTimeout(1500);

    // 看门狗：1 s 检查一次实时数据是否超时
    m_watchdog = new QTimer(this);
    m_watchdog->setInterval(AppSettings::instance().watchdogIntervalMs());
    connect(m_watchdog, &QTimer::timeout, this, &PressService::checkWatchdog);

    // 自动重连定时器（单次触发，每次失败后重新计算退避时间）
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setSingleShot(true);
    connect(m_reconnectTimer, &QTimer::timeout, this, [this] {
        if (!m_autoReconnect || m_endpoint.isEmpty())
            return;
        if (m_connecting.load()) {
            m_reconnectTimer->start(2000);
            return;
        }
        const bool tcp = m_endpoint.contains(QLatin1Char(':'));
        connectEndpoint(m_endpoint, tcp, true);
    });

    // 控制指令确认：200 ms 检查一次超时，RT 到达时校验设备状态
    m_commandTimer = new QTimer(this);
    m_commandTimer->setInterval(AppConfig::CommandCheckMs);
    connect(m_commandTimer, &QTimer::timeout, this, &PressService::checkCommandTimeouts);
    connect(this, &PressService::realTimeUpdated, this,
            &PressService::checkPendingCommands, Qt::QueuedConnection);

    // 参数写入去抖：400 ms 内的连续修改合并为一次下发
    m_writeTimer = new QTimer(this);
    m_writeTimer->setSingleShot(true);
    m_writeTimer->setInterval(AppSettings::instance().writeDebounceMs());
    connect(m_writeTimer, &QTimer::timeout, this, [this] { flushWrites(); });
}

PressService::~PressService()
{
    // 等待在途连接结束，避免后台线程访问已析构的 Press
    m_autoReconnect = false;
    if (m_reconnectTimer)
        m_reconnectTimer->stop();
    if (m_watchdog)
        m_watchdog->stop();
    if (m_connectTask.valid())
        m_connectTask.wait();
    m_connecting = false;
    disconnectDevice();
}

void PressService::startWatchdog()
{
    if (m_watchdog && !m_watchdog->isActive())
        m_watchdog->start();
}

void PressService::stopWatchdog()
{
    if (m_watchdog)
        m_watchdog->stop();
}

void PressService::setStale(bool stale)
{
    if (m_stale == stale)
        return;
    m_stale = stale;
    emit staleChanged(m_stale);
}

void PressService::checkWatchdog()
{
    // 自动重连期间 m_connected 为 false，由重连定时器接管，不再重复判定
    if (!m_connected)
        return;

    const qint64 idle = QDateTime::currentMSecsSinceEpoch() - m_lastRtMs.load();
    if (idle < AppSettings::instance().staleTimeoutMs()) {
        if (m_stale) {
            setStale(false);
            m_reconnectTimer->stop();
            m_reconnectAttempt = 0;
            emit logMessage(QStringLiteral("通讯已恢复，实时数据重新上报"));
        }
        return;
    }

    if (!m_stale) {
        setStale(true);
        emit logMessage(QStringLiteral("通讯中断：超过 3 s 未收到实时数据"),
                        LogLevel::Warning);
        emit sdkError(QStringLiteral("通讯中断：超过 3 秒未收到设备实时数据"));
    }

    if (idle >= AppSettings::instance().reconnectTriggerMs())
        scheduleReconnect();
}

void PressService::scheduleReconnect()
{
    if (!m_autoReconnect || m_connecting.load() || m_reconnectTimer->isActive()
        || m_endpoint.isEmpty()) {
        return;
    }

    static const int kDelays[] = {2, 5, 10, 30};
    const int delay = kDelays[qMin(m_reconnectAttempt, 3)];
    ++m_reconnectAttempt;
    m_reconnectTimer->start(delay * 1000);
    emit reconnectScheduled(m_reconnectAttempt, delay);
    emit logMessage(QStringLiteral("将在 %1 s 后自动重连（第 %2 次）")
                        .arg(delay)
                        .arg(m_reconnectAttempt),
                    LogLevel::Warning);
}

void PressService::trackCommand(CommandKind kind, const QString &name, int model)
{
    if (!m_commandTimer)
        return;

    // 同一类指令只保留最后一次，避免重复确认
    for (int i = m_pendingCommands.size() - 1; i >= 0; --i) {
        if (m_pendingCommands.at(i).kind == kind)
            m_pendingCommands.removeAt(i);
    }

    PendingCommand command;
    command.kind = kind;
    command.name = name;
    command.model = model;
    command.deadlineMs = QDateTime::currentMSecsSinceEpoch()
        + AppSettings::instance().commandTimeoutMs();
    m_pendingCommands.append(command);

    if (!m_commandTimer->isActive())
        m_commandTimer->start();
    emit commandPendingChanged(true);
}

bool PressService::commandSatisfied(const PendingCommand &command,
                                    const PressRtData &data) const
{
    switch (command.kind) {
    case CommandKind::PressOn:
        return data.pressOn;
    case CommandKind::PressOff:
        return !data.pressOn;
    case CommandKind::HeatOn:
        return data.heatOn;
    case CommandKind::HeatOff:
        return !data.heatOn;
    case CommandKind::CoopOn:
        return data.pressOn || data.heatOn;
    case CommandKind::CoopOff:
        return !data.pressOn && !data.heatOn;
    case CommandKind::Model:
        return data.modelState == command.model;
    }
    return false;
}

QString PressService::describeActual(const PendingCommand &command) const
{
    switch (command.kind) {
    case CommandKind::PressOn:
    case CommandKind::PressOff:
        return m_lastRt.pressOn ? QStringLiteral("加压中") : QStringLiteral("未加压");
    case CommandKind::HeatOn:
    case CommandKind::HeatOff:
        return m_lastRt.heatOn ? QStringLiteral("加热中") : QStringLiteral("未加热");
    case CommandKind::CoopOn:
    case CommandKind::CoopOff:
        return m_lastRt.pressOn || m_lastRt.heatOn ? QStringLiteral("合作流程运行中")
                                                    : QStringLiteral("合作流程未运行");
    case CommandKind::Model:
        return m_lastRt.modelState == 1 ? QStringLiteral("合作模式")
             : m_lastRt.modelState == 2 ? QStringLiteral("单独模式")
                                        : QStringLiteral("未知模式（%1）").arg(m_lastRt.modelState);
    }
    return QStringLiteral("未知");
}

void PressService::checkPendingCommands(const PressRtData &data)
{
    m_lastRt = data;
    if (m_pendingCommands.isEmpty())
        return;

    bool removed = false;
    for (int i = m_pendingCommands.size() - 1; i >= 0; --i) {
        const PendingCommand &command = m_pendingCommands.at(i);
        if (!commandSatisfied(command, data))
            continue;
        emit logMessage(QStringLiteral("指令已生效：%1").arg(command.name));
        m_pendingCommands.removeAt(i);
        removed = true;
    }

    if (removed && m_pendingCommands.isEmpty()) {
        m_commandTimer->stop();
        emit commandPendingChanged(false);
    }
}

void PressService::checkCommandTimeouts()
{
    if (m_pendingCommands.isEmpty()) {
        m_commandTimer->stop();
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (int i = m_pendingCommands.size() - 1; i >= 0; --i) {
        const PendingCommand &command = m_pendingCommands.at(i);
        if (now < command.deadlineMs)
            continue;
        const QString actual = describeActual(command);
        m_pendingCommands.removeAt(i);
        emit sdkError(QStringLiteral("指令未生效：%1（3 s 内未检测到状态变化，实际：%2）")
                          .arg(command.name, actual));
    }

    if (m_pendingCommands.isEmpty()) {
        m_commandTimer->stop();
        emit commandPendingChanged(false);
    }
}

void PressService::clearPendingCommands()
{
    if (m_pendingCommands.isEmpty())
        return;
    m_pendingCommands.clear();
    if (m_commandTimer)
        m_commandTimer->stop();
    emit commandPendingChanged(false);
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
    m_lastRtMs.store(QDateTime::currentMSecsSinceEpoch());
    const PressRtData data = PressJson::parseRealTime(json);
    handleFlowChange(data.flowChange);
    emit realTimeUpdated(data);
}

void PressService::handleFlowChange(int flowChange)
{
    // 0：无变更；非 0 值重复上报时只处理一次，回到 0 后重新武装
    if (flowChange == 0) {
        m_lastFlowChange.store(0);
        return;
    }
    if (flowChange == m_lastFlowChange.load())
        return;
    m_lastFlowChange.store(flowChange);

    // 读取 JSON 为同步请求，RT 回调运行在 SDK 定时线程，切回界面线程执行
    QMetaObject::invokeMethod(this, [this, flowChange] {
        std::string json;
        switch (flowChange) {
        case 1:
            emit logMessage(
                QStringLiteral("设备控压状态已变更（FlowChange=1），重新读取 PressData"));
            if (!m_press->getPressDataJson(json))
                emit logMessage(QStringLiteral("重新读取 PressData 失败：%1").arg(lastError()),
                                LogLevel::Warning);
            break;
        case 2:
            emit logMessage(
                QStringLiteral("设备温度参数已变更（FlowChange=2），重新读取 TempData"));
            if (!m_press->getTempDataJson(json))
                emit logMessage(QStringLiteral("重新读取 TempData 失败：%1").arg(lastError()),
                                LogLevel::Warning);
            break;
        case 3:
            emit logMessage(
                QStringLiteral("设备合作参数已变更（FlowChange=3），重新读取 MixData"));
            if (!m_press->getMixDataJson(json))
                emit logMessage(QStringLiteral("重新读取 MixData 失败：%1").arg(lastError()),
                                LogLevel::Warning);
            break;
        default:
            emit logMessage(QStringLiteral("收到未知的参数变更标记 FlowChange=%1")
                                .arg(flowChange),
                            LogLevel::Warning);
            break;
        }
    }, Qt::QueuedConnection);
}

void PressService::handleReadOnlyData(uint64_t registerNo, const std::string &json)
{
    Q_UNUSED(registerNo)
    if (json.empty())
        return;
    const PressReadOnlyData data = PressJson::parseReadOnly(json);
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
    const PressProgramData data = PressJson::parsePressProgram(json);
    emit logMessage(QStringLiteral("PressData：%1 段，模具类型 %2")
                        .arg(data.stepCount)
                        .arg(data.type));
    emit pressProgramUpdated(data);
}

void PressService::handleTempData(const std::string &json)
{
    if (json.empty())
        return;
    const PressTempProgramData data = PressJson::parseTempProgram(json);
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
    const PressMixProgramData data = PressJson::parseMixProgram(json);
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
    if (m_connecting.load()) {
        emit logMessage(QStringLiteral("连接请求被忽略：正在连接中"), LogLevel::Warning);
        emit sdkError(QStringLiteral("正在连接设备，请稍候或先取消当前连接"));
        return;
    }
    m_autoReconnect = true;
    m_reconnectAttempt = 0;
    if (m_reconnectTimer)
        m_reconnectTimer->stop();
    connectEndpoint(endpoint, tcp, false);
}

void PressService::connectEndpoint(const QString &endpoint, bool tcp, bool automatic)
{
    const QString target = endpoint.trimmed();
    if (target.isEmpty() || m_connecting.exchange(true))
        return;

    m_cancelConnect = false;
    clearPendingCommands();
    clearPendingWrites();
    // 重新连接前先释放旧连接
    if (m_connected) {
        m_press->unregisterDataInterface();
        m_press->disconnect();
        m_connected = false;
    }
    m_endpoint = target;
    m_loggedFirstRt = false;
    m_lastFlowChange.store(0);
    emit logMessage(automatic
                        ? QStringLiteral("正在自动重连设备 %1（%2）…")
                              .arg(target, tcp ? QStringLiteral("TCP")
                                               : QStringLiteral("串口"))
                        : QStringLiteral("正在连接设备 %1（%2）…")
                              .arg(target, tcp ? QStringLiteral("TCP")
                                               : QStringLiteral("串口")));

    // 连接可能阻塞数秒（TCP 握手 / 串口打开），放到后台线程执行
    m_connectTask = std::async(std::launch::async, [this, target, tcp, automatic] {
        const QByteArray name = target.toUtf8();
        const bool ok = m_press->connect(name.constData(),
                                         tcp ? PortType::TcpSocketPortType
                                             : PortType::SerialPortType);
        QString error;
        std::string firstRt;
        VersionInfo sdkVersion{};
        const bool hasVersion = ok && m_press->getDeviceVersionInfo(sdkVersion);
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

        QMetaObject::invokeMethod(this, [this, ok, error, firstRt, automatic,
                                         hasVersion, sdkVersion] {
            m_connecting = false;

            // 连接过程中用户点击了断开/取消
            if (m_cancelConnect.exchange(false)) {
                if (ok) {
                    m_press->unregisterDataInterface();
                    m_press->disconnect();
                }
                m_connected = false;
                stopWatchdog();
                clearPendingCommands();
                clearPendingWrites();
                if (m_reconnectTimer)
                    m_reconnectTimer->stop();
                setStale(false);
                emit logMessage(QStringLiteral("连接已取消"), LogLevel::Warning);
                emit connectionChanged(false, m_endpoint);
                return;
            }

            m_connected = ok;
            if (ok) {
                m_reconnectAttempt = 0;
                setStale(false);
                m_lastRtMs = QDateTime::currentMSecsSinceEpoch();
                startWatchdog();
                emit logMessage(automatic
                                    ? QStringLiteral("自动重连成功：%1").arg(m_endpoint)
                                    : QStringLiteral("设备连接成功，已注册 1 s 周期采集，开始读取设备参数"));
                emit connectionChanged(true, m_endpoint);
                if (hasVersion) {
                    DeviceVersionInfo info;
                    info.valid = true;
                    info.appType = static_cast<int>(sdkVersion.m_AppNo);
                    info.major = sdkVersion.m_Major;
                    info.minor = sdkVersion.m_Minor;
                    info.patch = sdkVersion.m_Patch;
                    emit versionInfoUpdated(info);
                }
                // 周期采集启用时 getRealTimeDataJson 不触发回调，这里补发首次实时数据
                if (!firstRt.empty())
                    handleRealTime(0, firstRt);
                return;
            }

            if (automatic) {
                // 自动重连失败不打断界面，仅记录并安排下一次
                emit logMessage(QStringLiteral("自动重连失败：%1").arg(error),
                                LogLevel::Error);
                scheduleReconnect();
                return;
            }

            stopWatchdog();
            setStale(false);
            emit logMessage(QStringLiteral("设备连接失败：%1").arg(error), LogLevel::Error);
            emit sdkError(error);
            emit connectionChanged(false, m_endpoint);
        }, Qt::QueuedConnection);
    });
}

void PressService::disconnectDevice()
{
    // 用户主动断开：关闭自动重连，并取消在途连接
    m_autoReconnect = false;
    if (m_reconnectTimer)
        m_reconnectTimer->stop();

    if (m_connecting.load()) {
        m_cancelConnect = true;
        emit logMessage(QStringLiteral("正在取消连接…"), LogLevel::Warning);
        return;
    }

    stopWatchdog();
    clearPendingCommands();
    clearPendingWrites();
    const bool hadLink = m_connected || m_stale || m_press->isConnected();
    if (m_connected || m_press->isConnected()) {
        m_press->unregisterDataInterface();
        m_press->disconnect();
    }
    m_connected = false;
    setStale(false);
    m_lastFlowChange.store(0);
    if (hadLink) {
        emit logMessage(QStringLiteral("设备连接已断开：%1").arg(m_endpoint),
                        LogLevel::Warning);
        emit connectionChanged(false, m_endpoint);
    }
}

bool PressService::setPressing(bool on, QString *error)
{
    // 启动前先把未下发的参数写入设备，失败则不启动
    if (on && !flushWrites()) {
        if (error)
            *error = QStringLiteral("参数下发失败，已取消启动加压");
        return false;
    }
    const bool ok = report(check(m_press->setPressing(on), error,
                                 on ? QStringLiteral("启动加压") : QStringLiteral("停止加压")),
                           on ? QStringLiteral("指令：启动加压") : QStringLiteral("指令：停止加压"));
    if (ok) {
        trackCommand(on ? CommandKind::PressOn : CommandKind::PressOff,
                     on ? QStringLiteral("启动加压") : QStringLiteral("停止加压"));
    }
    return ok;
}

bool PressService::setStartHeat(bool on, QString *error)
{
    if (on && !flushWrites()) {
        if (error)
            *error = QStringLiteral("参数下发失败，已取消启动加热");
        return false;
    }
    const bool ok = report(check(m_press->setStartHeat(on), error,
                                 on ? QStringLiteral("启动加热") : QStringLiteral("停止加热")),
                           on ? QStringLiteral("指令：启动加热") : QStringLiteral("指令：停止加热"));
    if (ok) {
        trackCommand(on ? CommandKind::HeatOn : CommandKind::HeatOff,
                     on ? QStringLiteral("启动加热") : QStringLiteral("停止加热"));
    }
    return ok;
}

bool PressService::startAir(QString *error)
{
    return report(check(m_press->startAir(), error, QStringLiteral("启动排气")),
                  QStringLiteral("指令：启动排气"));
}

bool PressService::setStart(bool on, QString *error)
{
    if (on && !flushWrites()) {
        if (error)
            *error = QStringLiteral("参数下发失败，已取消启动合作流程");
        return false;
    }
    const bool ok = report(check(m_press->setStart(on), error,
                                 on ? QStringLiteral("启动合作流程") : QStringLiteral("停止合作流程")),
                           on ? QStringLiteral("指令：启动合作流程")
                              : QStringLiteral("指令：停止合作流程"));
    if (ok) {
        trackCommand(on ? CommandKind::CoopOn : CommandKind::CoopOff,
                     on ? QStringLiteral("启动合作流程") : QStringLiteral("停止合作流程"));
    }
    return ok;
}

bool PressService::selectModel(int model)
{
    // 模式切换前先落盘参数，避免写入落到切换后的模式
    if (!flushWrites())
        return false;
    const QString name = model == 1 ? QStringLiteral("合作模式")
                                    : QStringLiteral("单独模式");
    const bool ok = report(m_press->selectModelState(static_cast<uint8_t>(model)),
                           QStringLiteral("切换为%1").arg(name));
    if (ok) {
        trackCommand(CommandKind::Model, QStringLiteral("切换为%1").arg(name), model);
    }
    return ok;
}

void PressService::queueWrite(const QString &key, const QString &name,
                              std::function<bool()> apply)
{
    if (!m_writeTimer)
        return;

    for (int i = 0; i < m_pendingWrites.size(); ++i) {
        if (m_pendingWrites.at(i).key != key)
            continue;
        // 同一参数连续修改：只保留最后一次值并重新计时
        m_pendingWrites[i].name = name;
        m_pendingWrites[i].apply = std::move(apply);
        m_writeTimer->start();
        return;
    }

    m_pendingWrites.append({key, name, std::move(apply)});
    if (!m_writeTimer->isActive())
        m_writeTimer->start();
}

bool PressService::flushWrites()
{
    m_writeTimer->stop();
    if (m_pendingWrites.isEmpty())
        return true;

    const QVector<PendingWrite> writes = m_pendingWrites;
    m_pendingWrites.clear();

    QStringList applied;
    bool allOk = true;
    for (const PendingWrite &write : writes) {
        bool ok = write.apply();
        QString error;
        if (!ok) {
            error = lastError();
            // 参数写入失败重试一次
            ok = write.apply();
            if (!ok)
                error = lastError();
        }
        if (ok) {
            applied.append(write.name);
        } else {
            allOk = false;
            emit sdkError(QStringLiteral("参数下发失败：%1（%2）").arg(write.name, error));
        }
    }

    if (!applied.isEmpty())
        emit logMessage(QStringLiteral("参数已下发：%1")
                            .arg(applied.join(QStringLiteral("、"))));
    return allOk;
}

void PressService::clearPendingWrites()
{
    if (m_writeTimer)
        m_writeTimer->stop();
    m_pendingWrites.clear();
}

bool PressService::pushStepCounts(int pressSteps, int tempSteps)
{
    if (!m_connected)
        return false;
    const int press = qBound(1, pressSteps, 30);
    const int temp = qBound(1, tempSteps, 30);
    queueWrite(QStringLiteral("step-counts"), QStringLiteral("压力/温度段数"),
               [this, press, temp] {
                   return m_press->setPressStepCount(static_cast<uint8_t>(press))
                       && m_press->setTempStepCount(static_cast<uint8_t>(temp));
               });
    return true;
}

bool PressService::pushPressureStage(int index, double pressure, double boost, int holdSeconds)
{
    if (!m_connected || index < 0)
        return false;
    const quint8 i = static_cast<quint8>(index);
    queueWrite(QStringLiteral("press-stage-%1").arg(index),
               QStringLiteral("压力段 %1").arg(index + 1),
               [this, i, pressure, boost, holdSeconds] {
                   return m_press->setPressValueIndex(i, static_cast<float>(pressure))
                       && m_press->setAfterValueIndex(i, static_cast<float>(boost))
                       && m_press->setKeepPressTime(i, holdSeconds);
               });
    return true;
}

bool PressService::pushTemperatureStage(int index, double temperature, double rate,
                                        int holdSeconds)
{
    if (!m_connected || index < 0)
        return false;
    const quint8 i = static_cast<quint8>(index);
    queueWrite(QStringLiteral("temp-stage-%1").arg(index),
               QStringLiteral("温度段 %1").arg(index + 1),
               [this, i, temperature, rate, holdSeconds] {
                   return m_press->setTempIndex(i, static_cast<float>(temperature))
                       && m_press->setTempSpeedIndex(i, static_cast<float>(rate))
                       && m_press->setKeepTempTime(i, holdSeconds);
               });
    return true;
}

bool PressService::pushCoopCount(int steps)
{
    if (!m_connected)
        return false;
    const int count = qBound(1, steps, 30);
    queueWrite(QStringLiteral("coop-count"), QStringLiteral("合作段数"),
               [this, count] {
                   return m_press->setMixStepCount(static_cast<uint8_t>(count));
               });
    return true;
}

bool PressService::pushCoopStage(int index, bool press, double pressure, double boost,
                                 bool heatUp, double tempUp, bool heatDown, double tempDown,
                                 int holdSeconds)
{
    if (!m_connected || index < 0)
        return false;
    const quint8 i = static_cast<quint8>(index);
    queueWrite(QStringLiteral("coop-stage-%1").arg(index),
               QStringLiteral("合作段 %1").arg(index + 1),
               [this, i, press, pressure, boost, heatUp, tempUp, heatDown, tempDown,
                holdSeconds] {
                   return m_press->setMixIsPressIndex(i, press)
                       && m_press->setMixPressIndex(i, static_cast<float>(pressure))
                       && m_press->setMixAfterPressIndex(i, static_cast<float>(boost))
                       && m_press->setMixIsHeatUpIndex(i, heatUp)
                       && m_press->setMixHeatUpIndex(i, static_cast<float>(tempUp))
                       && m_press->setMixIsHeatDownIndex(i, heatDown)
                       && m_press->setMixHeatDownIndex(i, static_cast<float>(tempDown))
                       && m_press->setMixKeepTime(i, holdSeconds);
               });
    return true;
}

bool PressService::pushMoldCircle(double diameter)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("press-mold"), QStringLiteral("单独模具"),
               [this, diameter] {
                   return m_press->setCircle(static_cast<float>(diameter));
               });
    return true;
}

bool PressService::pushMoldRect(double length, double width)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("press-mold"), QStringLiteral("单独模具"),
               [this, length, width] {
                   return m_press->setRectangle(static_cast<float>(length),
                                                static_cast<float>(width));
               });
    return true;
}

bool PressService::pushMoldType(int type)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("press-mold-type"), QStringLiteral("单独模具类型"),
               [this, type] {
                   return m_press->setMoldType(static_cast<uint8_t>(type));
               });
    return true;
}

bool PressService::pushCalibrationValue(double value)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("press-calibration"), QStringLiteral("压力校准值"),
               [this, value] {
                   return m_press->setCalibrationValue(static_cast<float>(value));
               });
    return true;
}

bool PressService::pushAirTime(int seconds)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("press-air-time"), QStringLiteral("排气时间"),
               [this, seconds] {
                   return m_press->setAirTime(static_cast<uint8_t>(seconds));
               });
    return true;
}

bool PressService::pushCoopMoldCircle(double diameter)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("coop-mold-shape"), QStringLiteral("合作模具尺寸"),
               [this, diameter] {
                   return m_press->setMixCircle(static_cast<float>(diameter));
               });
    return true;
}

bool PressService::pushCoopMoldRect(double length, double width)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("coop-mold-shape"), QStringLiteral("合作模具尺寸"),
               [this, length, width] {
                   return m_press->setMixRectangle(static_cast<float>(length),
                                                   static_cast<float>(width));
               });
    return true;
}

bool PressService::pushCoopMoldType(int type)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("coop-mold-type"), QStringLiteral("合作模具类型"),
               [this, type] {
                   return m_press->setMixMoldType(static_cast<uint8_t>(type));
               });
    return true;
}

bool PressService::pushRateMode(int mode)
{
    if (!m_connected)
        return false;
    queueWrite(QStringLiteral("temp-rate-mode"), QStringLiteral("温度速率模式"),
               [this, mode] {
                   return m_press->setTempSpeedMold(static_cast<uint8_t>(mode != 0 ? 1 : 0));
               });
    return true;
}

QString PressService::sdkVersion() const
{
    return QString::fromStdString(Press::version());
}

bool PressService::report(bool ok, const QString &action)
{
    if (ok)
        emit logMessage(QStringLiteral("%1成功").arg(action));
    else
        emit logMessage(QStringLiteral("%1失败：%2").arg(action, lastError()),
                        LogLevel::Error);
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
