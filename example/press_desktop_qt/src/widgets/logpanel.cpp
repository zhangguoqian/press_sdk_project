#include "widgets/logpanel.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTime>
#include <QVBoxLayout>

LogPanel::LogPanel(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("logPanel"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(156);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    header->setObjectName(QStringLiteral("logHeader"));
    header->setAttribute(Qt::WA_StyledBackground, true);
    header->setFixedHeight(34);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 12, 0);
    headerLayout->setSpacing(10);

    auto *title = new QLabel(QStringLiteral("运行日志"), header);
    title->setObjectName(QStringLiteral("logTitle"));
    title->setFont(Theme::ui(12, QFont::DemiBold));

    m_count = new QLabel(QStringLiteral("0 条"), header);
    m_count->setObjectName(QStringLiteral("logCount"));
    m_count->setFont(Theme::num(10));

    auto *collapse = new QPushButton(QStringLiteral("收起"), header);
    collapse->setObjectName(QStringLiteral("logCollapse"));
    collapse->setCursor(Qt::PointingHandCursor);
    collapse->setFixedHeight(24);

    headerLayout->addWidget(title);
    headerLayout->addWidget(m_count);
    headerLayout->addStretch(1);
    headerLayout->addWidget(collapse);
    layout->addWidget(header);

    m_text = new QPlainTextEdit(this);
    m_text->setObjectName(QStringLiteral("logText"));
    m_text->setReadOnly(true);
    m_text->setFrameShape(QFrame::NoFrame);
    m_text->setMaximumBlockCount(500);
    m_text->setFont(Theme::num(11));
    layout->addWidget(m_text, 1);

    connect(collapse, &QPushButton::clicked, this, &LogPanel::collapseRequested);
}

void LogPanel::append(const QString &message)
{
    if (!m_text)
        return;
    const QString line = QStringLiteral("[%1] %2")
                             .arg(QTime::currentTime().toString(QStringLiteral("HH:mm:ss")),
                                  message);
    m_text->appendPlainText(line);
    m_text->verticalScrollBar()->setValue(m_text->verticalScrollBar()->maximum());

    if (m_count) {
        const int blocks = m_text->blockCount() - 1;
        m_count->setText(QStringLiteral("%1 条").arg(qMax(0, blocks)));
    }
}
