#pragma once

#include <QPoint>
#include <QString>
#include <QWidget>

class QLabel;

// 自定义标题栏：品牌标识、标题、窗口按钮，支持拖动与双击最大化
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

    // 生成标题：<设备名> · 上位机 (SDK x.y.z)
    static QString appTitle(const QString &deviceName = QString());

    // 连接设备后用设备名称更新标题
    void setTitle(const QString &text);

signals:
    void minimizeRequested();
    void maximizeRequested();
    void closeRequested();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
    QLabel *m_title = nullptr;
    bool m_dragging = false;
    QPoint m_dragOffset;
};
