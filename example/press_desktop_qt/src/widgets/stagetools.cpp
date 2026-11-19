#include "widgets/stagetools.h"
#include "theme/theme.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

StageTools::StageTools(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(24);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *countLabel = new QLabel(QStringLiteral("阶段数"), this);
    countLabel->setObjectName(QStringLiteral("countLabel"));
    countLabel->setFont(Theme::ui(11));
    layout->addWidget(countLabel);

    auto *spin = new QFrame(this);
    spin->setObjectName(QStringLiteral("spin"));
    spin->setAttribute(Qt::WA_StyledBackground, true);
    spin->setFixedSize(72, 24);
    auto *spinLayout = new QHBoxLayout(spin);
    spinLayout->setContentsMargins(0, 0, 0, 0);
    spinLayout->setSpacing(0);

    auto *minus = new QPushButton(QStringLiteral("−"), spin);
    minus->setObjectName(QStringLiteral("spinBtn"));
    minus->setFixedSize(20, 22);
    minus->setCursor(Qt::PointingHandCursor);

    m_edit = new QLineEdit(spin);
    m_edit->setObjectName(QStringLiteral("spinEdit"));
    m_edit->setFixedSize(32, 22);
    m_edit->setAlignment(Qt::AlignCenter);
    m_edit->setFont(Theme::num(12));
    m_edit->setValidator(new QIntValidator(1, 30, m_edit));
    m_edit->setText(QString::number(m_total));

    auto *plus = new QPushButton(QStringLiteral("+"), spin);
    plus->setObjectName(QStringLiteral("spinBtn"));
    plus->setFixedSize(20, 22);
    plus->setCursor(Qt::PointingHandCursor);

    spinLayout->addWidget(minus);
    spinLayout->addWidget(m_edit);
    spinLayout->addWidget(plus);
    layout->addWidget(spin);

    auto *pill = new QFrame(this);
    pill->setObjectName(QStringLiteral("stagePill"));
    pill->setAttribute(Qt::WA_StyledBackground, true);
    pill->setFixedHeight(24);
    auto *pillLayout = new QHBoxLayout(pill);
    pillLayout->setContentsMargins(10, 0, 10, 0);
    m_pill = new QLabel(pill);
    m_pill->setObjectName(QStringLiteral("stagePillText"));
    m_pill->setTextFormat(Qt::RichText);
    pillLayout->addWidget(m_pill);
    layout->addWidget(pill);

    connect(minus, &QPushButton::clicked, this, [this] { applyCount(m_total - 1); });
    connect(plus, &QPushButton::clicked, this, [this] { applyCount(m_total + 1); });
    connect(m_edit, &QLineEdit::editingFinished, this, [this] {
        applyCount(m_edit->text().toInt());
    });
    connect(m_edit, &QLineEdit::returnPressed, this, [this] {
        applyCount(m_edit->text().toInt());
    });

    updatePill();
}

void StageTools::setTotal(int total)
{
    m_total = qBound(1, total, 30);
    m_current = qMin(m_current, m_total);
    if (m_edit->text().toInt() != m_total)
        m_edit->setText(QString::number(m_total));
    updatePill();
}

void StageTools::setCurrent(int current)
{
    m_current = qBound(0, current, m_total);
    updatePill();
}

void StageTools::updatePill()
{
    m_pill->setText(QStringLiteral(
        "<span style=\"font-size:11px;color:#0079B4;\">当前阶段</span>&nbsp;"
        "<span style=\"font-size:14px;font-weight:700;color:#0079B4;\">%1</span>"
        "<span style=\"font-size:11px;color:#0079B4;\">/%2</span>")
        .arg(m_current)
        .arg(m_total));
}

void StageTools::applyCount(int value)
{
    const int count = qBound(1, value, 30);
    m_edit->setText(QString::number(count));
    if (count != m_total) {
        m_total = count;
        updatePill();
        emit countChanged(count);
    }
}
