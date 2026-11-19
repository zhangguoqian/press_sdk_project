#pragma once

#include <QWidget>
#include "theme/icons.h"

class MachineState;
class QLabel;
class QPushButton;
class QSizeGrip;

// 底部操作栏：运行提示 + 启动 / 排气 / 启动加热（或合作启动）
class ActionBar : public QWidget
{
    Q_OBJECT
public:
    enum class Mode { Run, Coop, Settings };

    explicit ActionBar(MachineState *state, QWidget *parent = nullptr);

    void setMode(Mode mode);
    void refresh();
    // 日志面板隐藏时的未读条数，0 表示不显示角标
    void setLogBadge(int count);
    // 通讯中断期间禁用启动/排气等控制按钮
    void setControlsEnabled(bool enabled);

signals:
    void pressRequested();
    void heatRequested();
    void ventRequested();
    void coopRequested();
    void logToggled();

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    QPushButton *createButton(const QString &text, const QString &objectName,
                              AppIcon icon, const QColor &iconColor);

    MachineState *m_state;
    QLabel *m_note = nullptr;
    QWidget *m_runGroup = nullptr;
    QWidget *m_coopGroup = nullptr;
    QPushButton *m_pressButton = nullptr;
    QPushButton *m_heatButton = nullptr;
    QPushButton *m_ventButton = nullptr;
    QPushButton *m_coopButton = nullptr;
    QPushButton *m_logButton = nullptr;
    QSizeGrip *m_sizeGrip = nullptr;
    Mode m_mode = Mode::Run;
};
