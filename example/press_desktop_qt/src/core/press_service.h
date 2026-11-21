#pragma once

#include "core/loglevel.h"

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>

class Press;
class QTimer;
class SdkAdapter;

/**
 * 一次实时数据快照（由 SDK 定时回调解析得到）。
 * 字段与设备 RT 报文对应，界面只消费该结构，不直接依赖 SDK 类型。
 */
struct PressRtData
{
    // 设备工作模式（RT ModelState），用于区分单独模式 / 合作模式
    int modelState = 0;
    bool pressOn = false;       // 加压中
    bool heatOn = false;        // 加热中
    bool airAllowed = false;    // IsAir：设备是否允许排气（1 允许）
    int pressStep = 0;          // 当前压力段（与下位机一致，从 1 开始）
    int tempStep = 0;           // 当前温度段（与下位机一致，从 1 开始）
    double pressure = 0.0;      // 实时压力 Ton
    double tempUp = 0.0;        // 上板温度 °C
    double tempDown = 0.0;      // 下板温度 °C
    int pressRemainSec = 0;     // 压力倒计时 s（JSON 时间值为秒）
    int tempRemainSec = 0;      // 温度倒计时 s
    bool mixOn = false;         // IsMStart：合作流程启动中（合作模式有效）
    int mixStep = 0;            // CMStep：合作当前步骤（与下位机一致，从 1 开始）
    int mixRemainSec = 0;       // MTime：合作倒计时 s
    // 参数变更标记：0 无操作，1 压力 JSON 变更，2 温度 JSON 变更，3 合作 JSON 变更
    int flowChange = 0;
};
Q_DECLARE_METATYPE(PressRtData)

/**
 * 设备只读参数（连接后从 ReadOnlyData JSON 解析）。
 * 用于“关于本机”等界面展示设备固有信息。
 */
struct PressReadOnlyData
{
    QString nameZh;
    QString nameEn;
    QString type;
    QString serialNumber;

    int maxPStep = 0;         // 最大压力段数
    int maxTStep = 0;         // 最大温度段数
    double maxPLimit = 0.0;   // 压力上限 Ton
    double minPLimit = 0.0;   // 压力下限 Ton
    double autoBoost = 0.0;   // 自动补压差值 Ton（Max_Min）
    double diameter = 0.0;    // 油缸直径 mm
    double maxTLimit = 0.0;   // 温度上限 °C
    double minTLimit = 0.0;   // 温度下限 °C

    int pDecimal = 1;         // PDecimal：压力（Ton）显示小数位
    int pressDecimal = 2;     // PressDecimal：模具压强（MPa）显示小数位
    int tDecimal = 1;         // TDecimal：温度（°C）显示小数位

    bool hasWater = false;    // 支持水冷
    bool hasSpeed = false;    // 支持速度模式
    bool network = false;     // 网络模块
    int startDelay = 0;       // 启动延迟 s
    int versionType = 2;      // 版本模式：0 仅单独，1 仅合作，2 双模式
};
Q_DECLARE_METATYPE(PressReadOnlyData)

/**
 * 设备固件版本（连接握手时缓存的 VersionInfo，不额外发请求）。
 */
struct DeviceVersionInfo
{
    bool valid = false;
    int appType = 0;   // AppType：1 自动热压，2 自动压片，3 自动加热，4 手动压片
    int major = 0;
    int minor = 0;
    int patch = 0;
};
Q_DECLARE_METATYPE(DeviceVersionInfo)

/**
 * 压力工艺参数（连接后从 PressData JSON 解析）。
 * setP / afterP / keepSec 为 30 段数组，stepCount 为设备当前使用的段数。
 */
struct PressProgramData
{
    int stepCount = 0;      // PStep：当前压力段数
    int type = 0;           // Type：压制类型
    double a = 0.0;         // 模具参数 A
    double b = 0.0;         // 模具参数 B
    double d = 0.0;         // 模具参数 D
    double checkValue = 0.0;// 校准值
    int airTime = 0;        // 排气时间 s

    QVector<double> setP;   // SetPValue：每段设定压力 Ton
    QVector<double> afterP; // AfterValue：每段补压设定 Ton
    QVector<int> keepSec;   // KPTime：每段保压时间 s
};
Q_DECLARE_METATYPE(PressProgramData)

/**
 * 温度工艺参数（TempData JSON）。
 */
struct PressTempProgramData
{
    int stepCount = 0;          // TStep：当前温度段数
    int type = 0;               // Type
    double minWater = 0.0;      // MinWater
    double maxWater = 0.0;      // MaxWater

    QVector<double> startTemp;  // StartTemp：每段目标温度 °C
    QVector<double> speed;      // Speed：每段升温速率 °C/min
    QVector<int> keepSec;       // KTTime：每段保温时间 s
};
Q_DECLARE_METATYPE(PressTempProgramData)

/**
 * 合作工艺参数（MixData JSON）。
 */
struct PressMixProgramData
{
    int stepCount = 0;          // Step：当前合作段数
    int type = 0;               // Type
    double a = 0.0;             // 模具参数 A
    double b = 0.0;             // 模具参数 B
    double d = 0.0;             // 模具参数 D

    QVector<int> isPress;       // IsPress：是否加压
    QVector<double> setP;       // SetPValue：设定压力 Ton
    QVector<double> afterP;     // AfterValue：补压设定 Ton
    QVector<int> isUpHeat;      // IsUpHeat：是否加热上板
    QVector<double> upTemp;     // UpTemp：上板温度 °C
    QVector<int> isDownHeat;    // IsDownHeat：是否加热下板
    QVector<double> downTemp;   // DownTemp：下板温度 °C
    QVector<int> keepSec;       // KTime：计时 s
};
Q_DECLARE_METATYPE(PressMixProgramData)

/**
 * SDK 通讯桥接层。
 *
 * 连接设备后通过 SDK 的 registerDataInterface(this, 1) 启动 1s 周期采集，
 * SDK 内部线程回调 -> 解析 JSON -> emit realTimeUpdated，
 * 跨线程自动排队到界面线程。控制与参数下发使用同步 API。
 *
 * 头文件不包含 SDK 的 press.hpp，避免其全局 MachineState 与本工程同名类型冲突；
 * SDK 适配器 SdkAdapter 在 .cpp 中实现。
 */
class PressService : public QObject
{
    Q_OBJECT
public:
    explicit PressService(QObject *parent = nullptr);
    ~PressService() override;

    // 枚举本机可用串口（用于连接对话框下拉列表）
    static QStringList availablePorts();

    // 异步连接：立即返回，结果通过 connectionChanged / sdkError 通知
    void connectDevice(const QString &endpoint, bool tcp);
    // 断开连接；若正在连接则取消本次连接，若处于自动重连则停止重连
    void disconnectDevice();
    bool isConnected() const { return m_connected; }
    bool isConnecting() const { return m_connecting.load(); }
    // 已连接但超过 3 s 未收到实时数据（通讯中断，可能正在自动重连）
    bool isStale() const { return m_stale; }
    // 是否有控制指令正在等待设备状态确认
    bool isCommandPending() const { return !m_pendingCommands.isEmpty(); }
    QString endpoint() const { return m_endpoint; }

    // ---- 运行控制 ----
    bool setPressing(bool on, QString *error = nullptr);
    bool setStartHeat(bool on, QString *error = nullptr);
    bool setStart(bool on, QString *error = nullptr);
    bool startAir(QString *error = nullptr);
    bool selectModel(int model);   // 1 合作模式，2 单独模式

    // ---- 参数下发 ----
    bool pushStepCounts(int pressSteps, int tempSteps);
    bool pushPressureStage(int index, double pressure, double boost, int holdSeconds);
    bool pushTemperatureStage(int index, double temperature, double rate, int holdSeconds);
    bool pushCoopCount(int steps);
    bool pushCoopStage(int index, bool press, double pressure, double boost,
                       bool heatUp, double tempUp, bool heatDown, double tempDown,
                       int holdSeconds);
    bool pushMoldCircle(double diameter);
    bool pushMoldRect(double length, double width);
    bool pushMoldType(int type);
    bool pushCalibrationValue(double value);
    bool pushAirTime(int seconds);
    bool pushCoopMoldCircle(double diameter);
    bool pushCoopMoldRect(double length, double width);
    bool pushCoopMoldType(int type);
    bool pushRateMode(int mode);   // 0 普通模式，1 速率模式（TempData.Type）

    // ---- 信息查询 ----
    QString sdkVersion() const;

signals:
    void connectionChanged(bool connected, const QString &endpoint);
    void realTimeUpdated(const PressRtData &data);
    // 连接后主动读取的静态参数
    void readOnlyDataUpdated(const PressReadOnlyData &data);
    void versionInfoUpdated(const DeviceVersionInfo &info);
    void pressProgramUpdated(const PressProgramData &data);
    void tempProgramUpdated(const PressTempProgramData &data);
    void mixProgramUpdated(const PressMixProgramData &data);
    void sdkError(const QString &message);
    // 运行日志（连接、参数读取、指令下发等明细）
    void logMessage(const QString &message, LogLevel level = LogLevel::Info);
    // 通讯中断状态变化：true 超过 3 s 未收到实时数据，false 恢复
    void staleChanged(bool stale);
    // 已安排自动重连（第 attempt 次，delaySeconds 秒后执行）
    void reconnectScheduled(int attempt, int delaySeconds);
    // 有控制指令正在等待设备状态确认（期间界面禁用控制按钮）
    void commandPendingChanged(bool pending);

private:
    friend class SdkAdapter;
    // 需要设备 RT 状态确认的控制指令
    enum class CommandKind {
        PressOn,
        PressOff,
        HeatOn,
        HeatOff,
        CoopOn,
        CoopOff,
        Model
    };

    struct PendingCommand
    {
        CommandKind kind;
        QString name;
        int model = 0;
        qint64 deadlineMs = 0;
    };

    // 参数写入去抖：同一 key 只保留最后一次值，400 ms 后批量下发
    struct PendingWrite
    {
        QString key;
        QString name;
        std::function<bool()> apply;
    };

    void handleRealTime(int errorCode, const std::string &json);
    void handleFlowChange(int flowChange);
    void handleReadOnlyData(uint64_t registerNo, const std::string &json);
    void handlePressData(const std::string &json);
    void handleTempData(const std::string &json);
    void handleMixData(const std::string &json);

    void connectEndpoint(const QString &endpoint, bool tcp, bool automatic);
    void startWatchdog();
    void stopWatchdog();
    void checkWatchdog();
    void scheduleReconnect();
    void setStale(bool stale);

    void trackCommand(CommandKind kind, const QString &name, int model = 0);
    void checkPendingCommands(const PressRtData &data);
    void checkCommandTimeouts();
    void clearPendingCommands();
    bool commandSatisfied(const PendingCommand &command, const PressRtData &data) const;
    QString describeActual(const PendingCommand &command) const;

    void queueWrite(const QString &key, const QString &name, std::function<bool()> apply);
    bool flushWrites();
    void clearPendingWrites();

    QString lastError() const;
    bool check(bool ok, QString *error, const QString &action);
    bool report(bool ok, const QString &action);

    std::unique_ptr<Press> m_press;
    std::unique_ptr<SdkAdapter> m_adapter;
    std::future<void> m_connectTask;
    std::atomic_bool m_connecting{false};
    std::atomic_bool m_cancelConnect{false};
    std::atomic<qint64> m_lastRtMs{0};
    bool m_connected = false;
    bool m_loggedFirstRt = false;
    bool m_stale = false;
    bool m_autoReconnect = true;
    int m_reconnectAttempt = 0;
    QString m_endpoint;
    QTimer *m_watchdog = nullptr;
    QTimer *m_reconnectTimer = nullptr;
    QTimer *m_commandTimer = nullptr;
    QTimer *m_writeTimer = nullptr;
    QVector<PendingCommand> m_pendingCommands;
    QVector<PendingWrite> m_pendingWrites;
    PressRtData m_lastRt;
    std::atomic<int> m_lastFlowChange{0};
};
