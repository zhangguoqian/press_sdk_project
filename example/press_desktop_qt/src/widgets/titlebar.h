#pragma once

#include <QPoint>
#include <QWidget>

class QLabel;

// 自定义标题栏：品牌标识、标题、窗口按钮，支持拖动与双击最大化
class TitleBar : public QWidget
{
    Q_OBJECT
public:
    explicit TitleBar(QWidget *parent = nullptr);

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
    bool m_dragging = false;
    QPoint m_dragOffset;
};
