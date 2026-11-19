#include "widgets/logpanel.h"
#include "theme/theme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTextStream>
#include <QUrl>
#include <QVBoxLayout>

namespace {

constexpr int kMaxEntries = 500;
constexpr qint64 kMergeWindowMs = 5000;
constexpr qint64 kMaxFileBytes = 5 * 1024 * 1024;
constexpr int kRetentionDays = 7;

LogLevel detectLevel(const QString &text)
{
    if (text.contains(QStringLiteral("失败"))
        || text.contains(QStringLiteral("错误"))
        || text.contains(QStringLiteral("异常"))
        || text.contains(QStringLiteral("未生效"))) {
        return LogLevel::Error;
    }
    if (text.contains(QStringLiteral("警告"))
        || text.contains(QStringLiteral("未连接"))
        || text.contains(QStringLiteral("请先"))
        || text.contains(QStringLiteral("超时"))
        || text.contains(QStringLiteral("无法"))
        || text.contains(QStringLiteral("中断"))
        || text.contains(QStringLiteral("断开"))) {
        return LogLevel::Warning;
    }
    return LogLevel::Info;
}

QString levelName(LogLevel level)
{
    switch (level) {
    case LogLevel::Error:
        return QStringLiteral("ERROR");
    case LogLevel::Warning:
        return QStringLiteral("WARN ");
    default:
        return QStringLiteral("INFO ");
    }
}

QString levelColor(LogLevel level)
{
    switch (level) {
    case LogLevel::Error:
        return QStringLiteral("#C8102E");
    case LogLevel::Warning:
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

    // 自动落盘目录：<AppData>/logs，按天一个文件
    m_logDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/logs");
    QDir().mkpath(m_logDir);
    pruneOldLogs();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("logHeader"));
    m_header->setAttribute(Qt::WA_StyledBackground, true);
    m_header->setFixedHeight(38);
    m_header->setCursor(Qt::SizeVerCursor);
    m_header->installEventFilter(this);

    auto *headerLayout = new QHBoxLayout(m_header);
    headerLayout->setContentsMargins(18, 0, 12, 0);
    headerLayout->setSpacing(8);

    auto *title = new QLabel(QStringLiteral("运行日志"), m_header);
    title->setObjectName(QStringLiteral("logTitle"));
    title->setFont(Theme::ui(12, QFont::DemiBold));

    m_count = new QLabel(QStringLiteral("0 条"), m_header);
    m_count->setObjectName(QStringLiteral("logCount"));
    m_count->setFont(Theme::num(10));

    m_level = new QComboBox(m_header);
    m_level->setObjectName(QStringLiteral("logLevel"));
    m_level->addItem(QStringLiteral("全部级别"));
    m_level->addItem(QStringLiteral("警告及以上"));
    m_level->addItem(QStringLiteral("仅错误"));
    m_level->setFixedSize(96, 24);
    m_level->setFont(Theme::ui(12));

    m_search = new QLineEdit(m_header);
    m_search->setObjectName(QStringLiteral("logSearch"));
    m_search->setPlaceholderText(QStringLiteral("搜索日志"));
    m_search->setClearButtonEnabled(true);
    m_search->setFixedSize(150, 24);
    m_search->setFont(Theme::ui(12));

    m_autoScroll = new QCheckBox(QStringLiteral("自动滚动"), m_header);
    m_autoScroll->setObjectName(QStringLiteral("logAutoScroll"));
    m_autoScroll->setChecked(true);
    m_autoScroll->setFont(Theme::ui(12));

    auto *clearButton = new QPushButton(QStringLiteral("清空"), m_header);
    clearButton->setObjectName(QStringLiteral("logTool"));
    clearButton->setCursor(Qt::PointingHandCursor);
    clearButton->setFixedHeight(24);
    clearButton->setFont(Theme::ui(12));

    auto *exportButton = new QPushButton(QStringLiteral("导出"), m_header);
    exportButton->setObjectName(QStringLiteral("logTool"));
    exportButton->setCursor(Qt::PointingHandCursor);
    exportButton->setFixedHeight(24);
    exportButton->setFont(Theme::ui(12));

    auto *collapse = new QPushButton(QStringLiteral("收起"), m_header);
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
    layout->addWidget(m_header);

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
    append(message, detectLevel(message));
}

void LogPanel::append(const QString &message, LogLevel level)
{
    if (!m_text)
        return;

    const QDateTime now = QDateTime::currentDateTime();
    Entry entry;
    entry.time = now.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
    entry.level = level;
    entry.text = message;
    entry.timeMs = now.toMSecsSinceEpoch();

    // 5 s 内相同消息合并为一条，显示重复次数，避免通讯错误刷屏
    if (!m_entries.isEmpty()) {
        Entry &last = m_entries.last();
        if (last.text == message && entry.timeMs - last.timeMs <= kMergeWindowMs) {
            ++last.repeat;
            writeToFile(entry);
            if (matches(last))
                rebuild();
            else
                updateCount();
            return;
        }
    }

    m_entries.append(entry);
    if (m_entries.size() > kMaxEntries)
        m_entries.removeFirst();

    writeToFile(entry);

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

bool LogPanel::eventFilter(QObject *watched, QEvent *event)
{
    // 拖动标题栏调整面板高度
    if (watched == m_header) {
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                m_dragging = true;
                m_dragStartY = mouse->globalY();
                m_dragStartHeight = height();
                return true;
            }
        } else if (event->type() == QEvent::MouseMove && m_dragging) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            const int next = m_dragStartHeight + (m_dragStartY - mouse->globalY());
            setFixedHeight(qBound(110, next, 520));
            return true;
        } else if (event->type() == QEvent::MouseButtonRelease && m_dragging) {
            m_dragging = false;
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

bool LogPanel::isFilterActive() const
{
    return (m_level && m_level->currentIndex() != 0)
        || (m_search && !m_search->text().trimmed().isEmpty());
}

bool LogPanel::matches(const Entry &entry) const
{
    if (m_level) {
        const int filter = m_level->currentIndex();
        if (filter == 1 && entry.level == LogLevel::Info)
            return false;
        if (filter == 2 && entry.level != LogLevel::Error)
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
    const QString message = entry.repeat > 1
        ? QStringLiteral("%1（×%2）").arg(entry.text).arg(entry.repeat)
        : entry.text;
    m_text->appendHtml(QStringLiteral("<span style=\"color:%1\">[%2] [%3] %4</span>")
                           .arg(levelColor(entry.level), entry.time, levelName(entry.level),
                                message.toHtmlEscaped()));
}

void LogPanel::rebuild()
{
    if (!m_text)
        return;
    auto *bar = m_text->verticalScrollBar();
    const int previous = bar->value();
    m_text->clear();
    for (const Entry &entry : m_entries) {
        if (matches(entry))
            appendToView(entry);
    }
    if (m_autoScroll && m_autoScroll->isChecked()) {
        bar->setValue(bar->maximum());
    } else {
        // 自动滚动关闭时尽量保持用户当前的查看位置
        bar->setValue(qMin(previous, bar->maximum()));
    }
    updateCount();
}

void LogPanel::updateCount()
{
    if (!m_count)
        return;
    if (!isFilterActive()) {
        m_count->setText(QStringLiteral("%1 条").arg(m_entries.size()));
        return;
    }
    int matched = 0;
    for (const Entry &entry : m_entries) {
        if (matches(entry))
            ++matched;
    }
    m_count->setText(QStringLiteral("匹配 %1 / 共 %2").arg(matched).arg(m_entries.size()));
}

void LogPanel::exportLog()
{
    if (m_entries.isEmpty()) {
        emit statusMessage(QStringLiteral("没有可导出的日志"));
        return;
    }

    const QString name = QStringLiteral("press_log_%1.log")
                             .arg(QDateTime::currentDateTime().toString(
                                 QStringLiteral("yyyyMMdd_HHmmss")));
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出运行日志"), name,
        QStringLiteral("日志文件 (*.log *.txt);;所有文件 (*.*)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit statusMessage(QStringLiteral("日志导出失败：%1").arg(file.errorString()));
        return;
    }

    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    for (const Entry &entry : m_entries) {
        const QString suffix = entry.repeat > 1
            ? QStringLiteral("（×%1）").arg(entry.repeat)
            : QString();
        stream << QStringLiteral("[%1] [%2] %3%4\n")
                      .arg(entry.time, levelName(entry.level), entry.text, suffix);
    }
    file.close();

    emit statusMessage(QStringLiteral("日志已导出：%1").arg(QFileInfo(path).absoluteFilePath()));
    QDesktopServices::openUrl(
        QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
}

void LogPanel::writeToFile(const Entry &entry)
{
    if (m_logDir.isEmpty())
        return;

    const QString date = QDate::currentDate().toString(QStringLiteral("yyyyMMdd"));
    QString path = QStringLiteral("%1/%2.log").arg(m_logDir, date);
    QFileInfo info(path);
    if (info.exists() && info.size() >= kMaxFileBytes) {
        // 超过 5 MB 轮转一次，最多保留当天两个文件
        const QString backup = QStringLiteral("%1/%2.1.log").arg(m_logDir, date);
        QFile::remove(backup);
        QFile::rename(path, backup);
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
        return;
    QTextStream stream(&file);
    stream.setCodec("UTF-8");
    stream << QStringLiteral("[%1] [%2] %3\n")
                  .arg(entry.time, levelName(entry.level), entry.text);
}

void LogPanel::pruneOldLogs()
{
    QDir dir(m_logDir);
    if (!dir.exists())
        return;
    const QDateTime cutoff = QDateTime::currentDateTime().addDays(-kRetentionDays);
    const QFileInfoList files = dir.entryInfoList(
        {QStringLiteral("*.log")}, QDir::Files, QDir::Time);
    for (const QFileInfo &info : files) {
        if (info.lastModified() < cutoff)
            QFile::remove(info.absoluteFilePath());
    }
}
