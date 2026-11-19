#pragma once

#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;

// 底部运行日志面板：分级着色、时间戳、级别/关键字过滤、清空与导出
class LogPanel : public QWidget
{
    Q_OBJECT
public:
    enum class Level { Info, Warning, Error };

    explicit LogPanel(QWidget *parent = nullptr);

    // 追加一条日志，级别按内容自动判定（失败/错误 → 错误，未连接/请先 → 警告）
    void append(const QString &message);
    void clear();
    void clearUnseen();
    int unseenCount() const { return m_unseen; }

signals:
    void collapseRequested();
    void unseenChanged(int count);

protected:
    void showEvent(QShowEvent *event) override;

private:
    struct Entry
    {
        QString time;
        Level level;
        QString text;
    };

    bool matches(const Entry &entry) const;
    void appendToView(const Entry &entry);
    void rebuild();
    void updateCount();
    void exportLog();

    QPlainTextEdit *m_text = nullptr;
    QLabel *m_count = nullptr;
    QComboBox *m_level = nullptr;
    QLineEdit *m_search = nullptr;
    QCheckBox *m_autoScroll = nullptr;
    QVector<Entry> m_entries;
    int m_unseen = 0;
};
