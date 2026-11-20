#pragma once

#include <QWidget>

class MachineState;
class QFrame;
class QLabel;
class Segmented;
class UnitInput;

// 模具类型设置（单独模式 / 合作模式各自独立）
class MoldPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MoldPanel(MachineState *state, QWidget *parent = nullptr, bool coop = false);

    void syncFromState();

private:
    QWidget *createMeasureRow(const QString &label, UnitInput *input);

    MachineState *m_state;
    bool m_coop = false;
    Segmented *m_seg = nullptr;
    UnitInput *m_calibration = nullptr;  // 校准值 Ton（单独模式）
    UnitInput *m_airTime = nullptr;      // 排气时间 s（单独模式）
    QWidget *m_specialPane = nullptr;
    QWidget *m_circlePane = nullptr;
    QWidget *m_rectPane = nullptr;
    UnitInput *m_diameter = nullptr;
    UnitInput *m_length = nullptr;
    UnitInput *m_width = nullptr;
    QFrame *m_area = nullptr;
    QLabel *m_areaValue = nullptr;
    bool m_syncing = false;
};
