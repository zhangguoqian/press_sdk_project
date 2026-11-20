#pragma once

#include "core/loglevel.h"

#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

// 底部运行日志面板：分级着色、日期时间、级别/关键字过滤、清空、导出、自动落盘
class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);

    // 追加一条日志：按内容推断级别
    void append(const QString &message);
    // 追加一条日志：使用调用方指定的级别
    void append(const QString &message, LogLevel level);
    void clear();
    void clearUnseen();
    int unseenCount() const { return m_unseen; }

signals:
    void collapseRequested();
    void unseenChanged(int count);
    // 导出等操作结果，由主窗口用 Toast 呈现
    void statusMessage(const QString &text);

protected:
    void showEvent(QShowEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    struct Entry
    {
        QString time;      // yyyy-MM-dd HH:mm:ss.zzz
        LogLevel level = LogLevel::Info;
        QString text;
        int repeat = 1;    // 合并窗口内相同消息合并次数
        qint64 timeMs = 0; // 用于重复判定
    };

    bool matches(const Entry &entry) const;
    bool isFilterActive() const;
    void appendToView(const Entry &entry);
    void rebuild();
    void updateCount();
    void exportLog();
    void writeToFile(const Entry &entry);
    void pruneOldLogs();

    QPlainTextEdit *m_text = nullptr;
    QLabel *m_count = nullptr;
    QComboBox *m_level = nullptr;
    QLineEdit *m_search = nullptr;
    QCheckBox *m_autoScroll = nullptr;
    QWidget *m_header = nullptr;
    QVector<Entry> m_entries;
    QString m_logDir;
    int m_unseen = 0;
    // config.ini 中的运行参数（启动时读取一次）
    int m_maxEntries = 500;
    int m_mergeWindowMs = 5000;
    qint64 m_maxFileBytes = 5 * 1024 * 1024;
    int m_retentionDays = 7;

    bool m_dragging = false;
    int m_dragStartY = 0;
    int m_dragStartHeight = 0;
};
