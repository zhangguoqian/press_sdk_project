#pragma once

#include <QWidget>

class MachineState;
class QFrame;
class QLabel;
class Segmented;
class UnitInput;

// 模具类型设置（单独模式 / 合作模式共用同一份设定）
class MoldPanel : public QWidget
{
    Q_OBJECT
public:
    explicit MoldPanel(MachineState *state, QWidget *parent = nullptr);

    void syncFromState();

private:
    QWidget *createMeasureRow(const QString &label, UnitInput *input);

    MachineState *m_state;
    Segmented *m_seg = nullptr;
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
