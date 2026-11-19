#pragma once

#include <QWidget>

class QLabel;
class QPlainTextEdit;

// 底部运行日志面板：显示时间戳消息，可折叠
class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);

    void append(const QString &message);

signals:
    void collapseRequested();

private:
    QPlainTextEdit *m_text = nullptr;
    QLabel *m_count = nullptr;
};
