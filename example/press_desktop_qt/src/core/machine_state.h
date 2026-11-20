#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "core/press_service.h"

class QTimer;

struct PressureStage {
    double p = 1.0;      // 设定压力 Ton
    double boost = 0.7;  // 补压设定 Ton
    int hold = 600;      // 保压计时 s
};

struct TemperatureStage {
    double t = 30.0;     // 温度 °C
    double rate = 3.0;   // 速率 °C/min
    int hold = 500;      // 计时 s
};

struct CoopStage {
    bool press = true;
    double p = 1.0;
    double boost = 0.7;
    bool heatUp = true;
    double tUp = 30.0;
    bool heatDown = true;
    double tDown = 30.0;
    int hold = 600;
};

enum class MoldShape { Special, Circle, Rect };
enum class RateMode { Normal, Fast };

/**
 * 设备状态与流程数据（界面唯一数据源）。
 * 以 500ms 周期推进一次模拟，等价于原型 app.js 中的 setInterval 逻辑。
 */
class MachineState : public QObject
{
    Q_OBJECT
public:
    explicit MachineState(QObject *parent = nullptr);

    // ---- 流程数据 ----
    QVector<PressureStage> pStages;
    QVector<TemperatureStage> tStages;
    QVector<CoopStage> coopRows;

    int pCount = 30;
    int tCount = 30;
    int coopCount = 30;

    // ---- 模具 / 温控设置 ----
    MoldShape moldShape = MoldShape::Circle;
    double moldDia = 100.0;   // mm
    double moldLen = 100.0;   // mm
    double moldWid = 100.0;   // mm

    // 合作模式模具（来自 MixData，与单独模式相互独立）
    MoldShape coopMoldShape = MoldShape::Circle;
    double coopMoldDia = 100.0;   // mm
    double coopMoldLen = 100.0;   // mm
    double coopMoldWid = 100.0;   // mm
    RateMode rateMode = RateMode::Normal;
    double coolT = 15.0;
    double warmT = 45.0;

    // ---- 设备能力（ReadOnlyData，未连接时保留完整界面）----
    bool hasSpeed = true;   // 是否支持升温速率
    bool hasWater = true;   // 是否支持水冷回路
    int versionType = 2;    // 0 仅单独模式，1 仅合作模式，2 双模式
    bool singleAllowed = true;
    bool coopAllowed = true;

    int pDecimals = 1;     // PDecimal：压力（Ton）显示小数位
    int moldDecimals = 2;  // PressDecimal：模具压强（MPa）显示小数位
    double autoBoost = 0.3; // Max_Min：自动补压差（Ton）

    // ---- 运行状态 ----
    bool press = false;
    bool heat = false;
    bool coopRunning = false;
    int modelState = 0;   // 设备工作模式（1 单独 / 2 合作），来自 RT ModelState

    int pStage = 0;   // 1 起，0 = 空闲
    int tStage = 0;
    int coopStage = 0;

    int pRemain = 600;
    int tRemain = 500;
    int coopRemain = 0;

    double liveP = 0.0;
    double upT = -1.0;
    double downT = 0.0;

    double runtime = 0.0;
    int output = 0;

    // ---- 查询 ----
    bool running() const { return press || heat || coopRunning; }
    // 已连接或通讯中断（自动重连中）都按设备模式处理，避免退回本地模拟
    bool sdkMode() const
    {
        return m_service && (m_service->isConnected() || m_service->isStale());
    }
    int pressureIndex() const;     // 当前压力阶段下标（0 起）
    int temperatureIndex() const;
    int coopIndex() const;

    double moldArea() const;             // mm²；异形返回 0
    double moldPressure(double ton) const; // MPa；异形返回 -1
    QString moldAreaText() const;        // cm² 文本

    double coopMoldArea() const;
    double coopMoldPressure(double ton) const;
    QString coopMoldAreaText() const;

    // ---- SDK ----
    void attachService(PressService *service);
    void commitPressureStage(int index);
    void commitTemperatureStage(int index);
    void commitCoopStage(int index);
    void commitMold();
    void commitCoopMold();
    void applyPressProgram(const PressProgramData &data);
    void applyTempProgram(const PressTempProgramData &data);
    void applyMixProgram(const PressMixProgramData &data);

    // ---- 操作 ----
    void setPCount(int value);
    void setTCount(int value);
    void setCoopCount(int value);

    void startPress();
    void stopPress();
    void startHeat();
    void stopHeat();
    void toggleCoop();
    void vent();

    void setMoldShape(MoldShape shape);
    void setMoldDiameter(double mm);
    void setMoldLength(double mm);
    void setMoldWidth(double mm);
    void setCoopMoldShape(MoldShape shape);
    void setCoopMoldDiameter(double mm);
    void setCoopMoldLength(double mm);
    void setCoopMoldWidth(double mm);
    void setRateMode(RateMode mode);
    void notifySettingsChanged();

signals:
    void changed();          // 任意状态变化（用于刷新界面）
    void ticked();           // 500ms 采样节拍
    void settingsChanged();  // 模具 / 速率等设定变化
    void pressProgramChanged();  // 设备压力工艺参数已同步
    void tempProgramChanged();   // 设备温度工艺参数已同步
    void mixProgramChanged();    // 设备合作工艺参数已同步
    void linkChanged(bool connected, const QString &endpoint);
    void modelStateChanged(int modelState);  // 设备工作模式变化
    void sdkError(const QString &message);

private slots:
    void tick();
    void applyRealTime(const PressRtData &data);

private:
    QTimer *m_timer = nullptr;
    PressService *m_service = nullptr;
    bool m_coopRequested = false;
    bool m_coopSeenRunning = false;
    int m_coopIdleFrames = 0;
    double m_sdkMoldPressure = -1.0;
};
