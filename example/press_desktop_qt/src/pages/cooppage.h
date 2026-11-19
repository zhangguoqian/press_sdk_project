#pragma once

#include <QWidget>

class MachineState;
class KvList;
class MoldPanel;
class RealtimeChart;
class StageTable;
class StageTools;
class QLabel;
class QResizeEvent;

// 合作模式：实时状态 + 模具类型 / 合作流程明细 + 曲线
class CoopPage : public QWidget
{
    Q_OBJECT
public:
    explicit CoopPage(MachineState *state, QWidget *parent = nullptr);

    void refreshLive();
    void onTick();
    void resetChart();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateColumnWidth();
    QWidget *buildLeftColumn();
    QWidget *buildRightColumn();
    QWidget *buildTableCard();
    QWidget *buildChartCard();
    void rebuildTable();
    void updateLegend();

    MachineState *m_state;
    QWidget *m_leftColumn = nullptr;
    KvList *m_kv = nullptr;
    MoldPanel *m_mold = nullptr;
    StageTable *m_table = nullptr;
    StageTools *m_tools = nullptr;
    RealtimeChart *m_chart = nullptr;
    QLabel *m_legend = nullptr;
};
