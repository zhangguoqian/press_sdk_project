#pragma once

#include <QWidget>

class MachineState;
class KvList;
class MoldPanel;
class RealtimeChart;
class Segmented;
class StageTable;
class StageTools;
class QLabel;
class QResizeEvent;
class UnitInput;

// 单独模式：压力参数 / 压力阶段 + 温度阶段 + 曲线 / 温控设置
class SinglePage : public QWidget
{
    Q_OBJECT
public:
    explicit SinglePage(MachineState *state, QWidget *parent = nullptr);

    void refreshLive();
    void refreshSettings();
    void onTick();
    void resetChart();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateColumnWidth();
    QWidget *buildLeftColumn();
    QWidget *buildMiddleColumn();
    QWidget *buildRightColumn();
    QWidget *buildPressureCard();
    QWidget *buildTemperatureCard();
    QWidget *buildChartCard();
    QWidget *buildTemperatureControlCard();
    QWidget *buildMoldGroup();

    void rebuildPressureTable();
    void rebuildTemperatureTable();
    void updateLegend();

    MachineState *m_state;
    QWidget *m_leftColumn = nullptr;
    QWidget *m_rightColumn = nullptr;
    KvList *m_pressureKv = nullptr;
    KvList *m_temperatureKv = nullptr;
    StageTable *m_pressureTable = nullptr;
    StageTable *m_temperatureTable = nullptr;
    StageTools *m_pressureTools = nullptr;
    StageTools *m_temperatureTools = nullptr;
    RealtimeChart *m_chart = nullptr;
    QLabel *m_legend = nullptr;
    MoldPanel *m_mold = nullptr;
    Segmented *m_rateSegmented = nullptr;
    QLabel *m_rateTip = nullptr;
    QWidget *m_rateGroup = nullptr;
    QWidget *m_waterGroup = nullptr;
    QWidget *m_tempDivider = nullptr;
    UnitInput *m_coolInput = nullptr;
    UnitInput *m_warmInput = nullptr;
};
