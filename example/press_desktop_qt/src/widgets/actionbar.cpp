#include "widgets/actionbar.h"
#include "core/machine_state.h"
#include "theme/icons.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QSizeGrip>
#include <QStyle>
#include <QVariant>

ActionBar::ActionBar(MachineState *state, QWidget *parent)
    : QWidget(parent), m_state(state)
{
    setObjectName(QStringLiteral("actionBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(64);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(18, 0, 18, 0);
    layout->setSpacing(12);

    auto *info = new IconWidget(AppIcon::Info, Theme::Brand, 16, this);
    m_note = new QLabel(this);
    m_note->setObjectName(QStringLiteral("actionNote"));
    m_note->setTextFormat(Qt::RichText);
    m_note->setFont(Theme::ui(12));
    m_note->setMinimumWidth(200);

    layout->addWidget(info, 0, Qt::AlignVCenter);
    layout->addWidget(m_note, 1, Qt::AlignVCenter);

    m_runGroup = new QWidget(this);
    auto *runLayout = new QHBoxLayout(m_runGroup);
    runLayout->setContentsMargins(0, 0, 0, 0);
    runLayout->setSpacing(10);

    m_pressButton = createButton(QStringLiteral("启动加压"), QStringLiteral("btnGreen"),
                                 AppIcon::Power, Qt::white);
    m_ventButton = createButton(QStringLiteral("排气"), QStringLiteral("btnDangerGhost"),
                                AppIcon::Air, Theme::Red600);
    m_heatButton = createButton(QStringLiteral("启动加热"), QStringLiteral("btnGreen"),
                                AppIcon::Flame, Qt::white);
    runLayout->addWidget(m_pressButton);
    runLayout->addWidget(m_ventButton);
    runLayout->addWidget(m_heatButton);

    m_coopGroup = new QWidget(this);
    auto *coopLayout = new QHBoxLayout(m_coopGroup);
    coopLayout->setContentsMargins(0, 0, 0, 0);
    coopLayout->setSpacing(10);
    m_coopButton = createButton(QStringLiteral("启动"), QStringLiteral("btnGreen"),
                                AppIcon::Power, Qt::white);
    coopLayout->addWidget(m_coopButton);

    layout->addWidget(m_runGroup, 0, Qt::AlignVCenter);
    layout->addWidget(m_coopGroup, 0, Qt::AlignVCenter);

    m_logButton = createButton(QStringLiteral("日志"), QStringLiteral("btnLogToggle"),
                               AppIcon::Info, Theme::Brand);
    layout->addWidget(m_logButton, 0, Qt::AlignVCenter);

    connect(m_pressButton, &QPushButton::clicked, this, &ActionBar::pressRequested);
    connect(m_heatButton, &QPushButton::clicked, this, &ActionBar::heatRequested);
    connect(m_ventButton, &QPushButton::clicked, this, &ActionBar::ventRequested);
    connect(m_coopButton, &QPushButton::clicked, this, &ActionBar::coopRequested);
    connect(m_logButton, &QPushButton::clicked, this, &ActionBar::logToggled);

    // 无边框窗口的右下角缩放手柄
    m_sizeGrip = new QSizeGrip(this);
    m_sizeGrip->setFixedSize(14, 14);
    m_sizeGrip->raise();

    setMode(Mode::Run);
    refresh();
}

void ActionBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_sizeGrip)
        m_sizeGrip->move(width() - m_sizeGrip->width(), height() - m_sizeGrip->height());
}

QPushButton *ActionBar::createButton(const QString &text, const QString &objectName,
                                     AppIcon icon, const QColor &iconColor)
{
    auto *button = new QPushButton(text, this);
    button->setObjectName(objectName);
    button->setIcon(Icons::icon(icon, iconColor, 19, 1.7));
    button->setIconSize(QSize(19, 19));
    button->setFixedHeight(42);
    button->setCursor(Qt::PointingHandCursor);
    QFont font = Theme::ui(14, QFont::DemiBold);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    button->setFont(font);
    return button;
}

void ActionBar::setLogBadge(int count)
{
    if (!m_logButton)
        return;
    m_logButton->setText(count > 0 ? QStringLiteral("日志 · %1")
                                  : QStringLiteral("日志"));
}

void ActionBar::setControlsEnabled(bool enabled)
{
    if (m_runGroup)
        m_runGroup->setEnabled(enabled);
    if (m_coopGroup)
        m_coopGroup->setEnabled(enabled);
}

void ActionBar::setMode(Mode mode)
{
    m_mode = mode;
    m_runGroup->setVisible(mode == Mode::Run);
    m_coopGroup->setVisible(mode == Mode::Coop);
    refresh();
}

void ActionBar::refresh()
{
    const bool running = m_state->running();

    m_pressButton->setText(m_state->press ? QStringLiteral("停止加压")
                                          : QStringLiteral("启动加压"));
    m_heatButton->setText(m_state->heat ? QStringLiteral("停止加热")
                                        : QStringLiteral("启动加热"));
    m_coopButton->setText(m_state->coopRunning ? QStringLiteral("停止")
                                               : QStringLiteral("启动"));

    auto setRunning = [](QPushButton *button, bool on) {
        if (button->property("running").toBool() == on)
            return;
        button->setProperty("running", on);
        button->style()->unpolish(button);
        button->style()->polish(button);
    };
    setRunning(m_pressButton, m_state->press);
    setRunning(m_heatButton, m_state->heat);
    setRunning(m_coopButton, m_state->coopRunning);

    QString note;
    if (m_state->press && m_state->heat) {
        note = QStringLiteral("<b>加压 + 加热中</b>");
    } else if (m_state->press) {
        note = QStringLiteral("<b>加压中</b>");
    } else if (m_state->heat) {
        note = QStringLiteral("<b>加热中</b>");
    } else if (m_state->coopRunning) {
        note = QStringLiteral("<b>合作运行中</b>");
    } else if (running) {
        note = QStringLiteral("<b>运行中</b>");
    } else {
        note = QStringLiteral("<b>待机</b> · 启动加热前请确认上 / 下板温度与模具已就位");
    }
    m_note->setText(note);
}
