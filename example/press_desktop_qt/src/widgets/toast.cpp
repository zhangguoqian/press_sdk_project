#include "widgets/toast.h"
#include "theme/theme.h"

#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPropertyAnimation>
#include <QTimer>

Toast::Toast(QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("toast"));
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "QFrame#toast { background: rgba(10, 34, 51, 240); border-radius: 10px; }"
        "QLabel { color: #FFFFFF; background: transparent; }"));
    setFixedHeight(40);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(20, 0, 20, 0);
    layout->setSpacing(9);

    auto *dot = new QLabel(QStringLiteral("●"), this);
    dot->setStyleSheet(QStringLiteral("color:#37D06B; font-size:9px;"));
    dot->setAlignment(Qt::AlignVCenter);

    m_text = new QLabel(this);
    m_text->setFont(Theme::ui(12));
    m_text->setAlignment(Qt::AlignVCenter);

    layout->addWidget(dot, 0, Qt::AlignVCenter);
    layout->addWidget(m_text, 0, Qt::AlignVCenter);

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(0.0);
    setGraphicsEffect(m_opacity);

    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, [this] {
        auto *animation = new QPropertyAnimation(m_opacity, "opacity", this);
        animation->setDuration(200);
        animation->setStartValue(1.0);
        animation->setEndValue(0.0);
        connect(animation, &QPropertyAnimation::finished, this, &QWidget::hide);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    });

    hide();
}

void Toast::showMessage(const QString &text)
{
    m_text->setText(text);
    adjustSize();
    setFixedHeight(40);
    show();
    raise();

    auto *animation = new QPropertyAnimation(m_opacity, "opacity", this);
    animation->setDuration(160);
    animation->setStartValue(m_opacity->opacity());
    animation->setEndValue(1.0);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    m_timer->start(2200);
}
