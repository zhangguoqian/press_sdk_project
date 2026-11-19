#pragma once

#include <QMetaType>
#include <QObject>
#include <QString>
#include <QStringList>

#include <atomic>
#include <future>
#include <memory>
#include <string>

class Press;
class SdkAdapter;

/**
 * 一次实时数据快照（由 SDK 定时回调解析得到）。
 * 字段与设备 RT 报文对应，界面只消费该结构，不直接依赖 SDK 类型。
 */
struct PressRtData
{
    bool pressOn = false;       // 加压中
    bool heatOn = false;        // 加热中
    bool air = false;           // 排气中
    int pressStep = 0;          // 当前压力段（SDK 从 0 开始）
    int tempStep = 0;           // 当前温度段（SDK 从 0 开始）
    double pressure = 0.0;      // 实时压力 Ton
    double moldPressure = 0.0;  // 模具压强 MPa
    double tempUp = 0.0;        // 上板温度 °C
    double tempDown = 0.0;      // 下板温度 °C
    int pressRemainMs = 0;      // 压力倒计时 ms
    int tempRemainMs = 0;       // 温度倒计时 ms
};
Q_DECLARE_METATYPE(PressRtData)

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
    void disconnectDevice();
    bool isConnected() const { return m_connected; }
    bool isConnecting() const { return m_connecting.load(); }
    QString endpoint() const { return m_endpoint; }

    // ---- 运行控制 ----
    bool setPressing(bool on, QString *error = nullptr);
    bool setStartHeat(bool on, QString *error = nullptr);
    bool setStart(bool on, QString *error = nullptr);

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

signals:
    void connectionChanged(bool connected, const QString &endpoint);
    void realTimeUpdated(const PressRtData &data);
    // 连接后主动读取的静态参数（原始 JSON）
    void readOnlyDataReceived(const QString &json);
    void pressDataReceived(const QString &json);
    void tempDataReceived(const QString &json);
    void mixDataReceived(const QString &json);
    void sdkError(const QString &message);

private:
    friend class SdkAdapter;
    void handleRealTime(int errorCode, const std::string &json);
    void handleReadOnlyData(uint64_t registerNo, const std::string &json);
    void handlePressData(const std::string &json);
    void handleTempData(const std::string &json);
    void handleMixData(const std::string &json);

    QString lastError() const;
    bool check(bool ok, QString *error, const QString &action);

    std::unique_ptr<Press> m_press;
    std::unique_ptr<SdkAdapter> m_adapter;
    std::future<void> m_connectTask;
    std::atomic_bool m_connecting{false};
    bool m_connected = false;
    QString m_endpoint;
};
