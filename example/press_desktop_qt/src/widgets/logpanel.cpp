#include "widgets/logpanel.h"
#include "theme/theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>

namespace {

constexpr int kMaxEntries = 500;

LogPanel::Level detectLevel(const QString &text)
{
    if (text.contains(QStringLiteral("失败"))
        || text.contains(QStringLiteral("错误"))
        || text.contains(QStringLiteral("异常"))) {
        return LogPanel::Level::Error;
    }
    if (text.contains(QStringLiteral("警告"))
        || text.contains(QStringLiteral("未连接"))
        || text.contains(QStringLiteral("请先"))
        || text.contains(QStringLiteral("超时"))
        || text.contains(QStringLiteral("无法"))
        || text.contains(QStringLiteral("断开"))) {
        return LogPanel::Level::Warning;
    }
    return LogPanel::Level::Info;
}

QString levelName(LogPanel::Level level)
{
    switch (level) {
    case LogPanel::Level::Error:
        return QStringLiteral("ERROR");
    case LogPanel::Level::Warning:
        return QStringLiteral("WARN ");
    default:
        return QStringLiteral("INFO ");
    }
}

QString levelColor(LogPanel::Level level)
{
    switch (level) {
    case LogPanel::Level::Error:
        return QStringLiteral("#C8102E");
    case LogPanel::Level::Warning:
        return QStringLiteral("#B26A00");
    default:
        return QStringLiteral("#3E5568");
    }
}

} // namespace

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
    header->setFixedHeight(38);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 12, 0);
    headerLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("运行日志"), header);
    title->setObjectName(QStringLiteral("logTitle"));
    title->setFont(Theme::ui(12, QFont::DemiBold));

    m_count = new QLabel(QStringLiteral("0 条"), header);
    m_count->setObjectName(QStringLiteral("logCount"));
    m_count->setFont(Theme::num(10));

    m_level = new QComboBox(header);
    m_level->setObjectName(QStringLiteral("logLevel"));
    m_level->addItem(QStringLiteral("全部级别"));
    m_level->addItem(QStringLiteral("警告及以上"));
    m_level->addItem(QStringLiteral("仅错误"));
    m_level->setFixedSize(96, 24);
    m_level->setFont(Theme::ui(12));

    m_search = new QLineEdit(header);
    m_search->setObjectName(QStringLiteral("logSearch"));
    m_search->setPlaceholderText(QStringLiteral("搜索日志"));
    m_search->setClearButtonEnabled(true);
    m_search->setFixedSize(150, 24);
    m_search->setFont(Theme::ui(12));

    m_autoScroll = new QCheckBox(QStringLiteral("自动滚动"), header);
    m_autoScroll->setObjectName(QStringLiteral("logAutoScroll"));
    m_autoScroll->setChecked(true);
    m_autoScroll->setFont(Theme::ui(12));

    auto *clearButton = new QPushButton(QStringLiteral("清空"), header);
    clearButton->setObjectName(QStringLiteral("logTool"));
    clearButton->setCursor(Qt::PointingHandCursor);
    clearButton->setFixedHeight(24);
    clearButton->setFont(Theme::ui(12));

    auto *exportButton = new QPushButton(QStringLiteral("导出"), header);
    exportButton->setObjectName(QStringLiteral("logTool"));
    exportButton->setCursor(Qt::PointingHandCursor);
    exportButton->setFixedHeight(24);
    exportButton->setFont(Theme::ui(12));

    auto *collapse = new QPushButton(QStringLiteral("收起"), header);
    collapse->setObjectName(QStringLiteral("logCollapse"));
    collapse->setCursor(Qt::PointingHandCursor);
    collapse->setFixedHeight(24);

    headerLayout->addWidget(title);
    headerLayout->addWidget(m_count);
    headerLayout->addStretch(1);
    headerLayout->addWidget(m_level);
    headerLayout->addWidget(m_search);
    headerLayout->addWidget(m_autoScroll);
    headerLayout->addWidget(clearButton);
    headerLayout->addWidget(exportButton);
    headerLayout->addWidget(collapse);
    layout->addWidget(header);

    m_text = new QPlainTextEdit(this);
    m_text->setObjectName(QStringLiteral("logText"));
    m_text->setReadOnly(true);
    m_text->setFrameShape(QFrame::NoFrame);
    m_text->setMaximumBlockCount(kMaxEntries);
    m_text->setFont(Theme::num(11));
    layout->addWidget(m_text, 1);

    connect(collapse, &QPushButton::clicked, this, &LogPanel::collapseRequested);
    connect(clearButton, &QPushButton::clicked, this, &LogPanel::clear);
    connect(exportButton, &QPushButton::clicked, this, &LogPanel::exportLog);
    connect(m_level, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            [this](int) { rebuild(); });
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &) { rebuild(); });
}

void LogPanel::append(const QString &message)
{
    if (!m_text)
        return;

    Entry entry;
    entry.time = QTime::currentTime().toString(QStringLiteral("HH:mm:ss.zzz"));
    entry.level = detectLevel(message);
    entry.text = message;

    m_entries.append(entry);
    if (m_entries.size() > kMaxEntries)
        m_entries.removeFirst();

    // 面板隐藏时累计未读条数（窗口尚未显示时不计）
    const bool windowReady = window() && window()->isVisible();
    if (!isVisible() && windowReady) {
        ++m_unseen;
        emit unseenChanged(m_unseen);
    }

    if (matches(entry))
        appendToView(entry);
    if (m_autoScroll && m_autoScroll->isChecked() && isVisible()) {
        auto *bar = m_text->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
    updateCount();
}

void LogPanel::clear()
{
    m_entries.clear();
    if (m_text)
        m_text->clear();
    updateCount();
}

void LogPanel::clearUnseen()
{
    if (m_unseen == 0)
        return;
    m_unseen = 0;
    emit unseenChanged(0);
}

void LogPanel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (m_autoScroll && m_autoScroll->isChecked() && m_text) {
        auto *bar = m_text->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
}

bool LogPanel::matches(const Entry &entry) const
{
    if (m_level) {
        const int filter = m_level->currentIndex();
        if (filter == 1 && entry.level == Level::Info)
            return false;
        if (filter == 2 && entry.level != Level::Error)
            return false;
    }
    if (m_search) {
        const QString keyword = m_search->text().trimmed();
        if (!keyword.isEmpty() && !entry.text.contains(keyword, Qt::CaseInsensitive))
            return false;
    }
    return true;
}

void LogPanel::appendToView(const Entry &entry)
{
    m_text->appendHtml(QStringLiteral("<span style=\"color:%1\">[%2] [%3] %4</span>")
                           .arg(levelColor(entry.level), entry.time, levelName(entry.level),
                                entry.text.toHtmlEscaped()));
}

void LogPanel::rebuild()
{
    if (!m_text)
        return;
    m_text->clear();
    for (const Entry &entry : m_entries) {
        if (matches(entry))
            appendToView(entry);
    }
    if (m_autoScroll && m_autoScroll->isChecked()) {
        auto *bar = m_text->verticalScrollBar();
        bar->setValue(bar->maximum());
    }
    updateCount();
}

void LogPanel::updateCount()
{
    if (m_count)
        m_count->setText(QStringLiteral("%1 条").arg(m_entries.size()));
}

void LogPanel::exportLog()
{
    const QString name = QStringLiteral("press_log_%1.log")
                             .arg(QDateTime::currentDateTime().toString(
                                 QStringLiteral("yyyyMMdd_HHmmss")));
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出运行日志"), name,
        QStringLiteral("日志文件 (*.log *.txt);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    for (const Entry &entry : m_entries) {
        stream << QStringLiteral("[%1] [%2] %3\n")
                      .arg(entry.time, levelName(entry.level), entry.text);
    }
}
