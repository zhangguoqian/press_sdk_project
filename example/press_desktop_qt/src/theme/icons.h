#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QWidget>

class QPainter;

// 界面使用的线性图标（与原型 SVG sprite 对应）
enum class AppIcon {
    Press,
    Link,
    Gear,
    Gauge,
    Flame,
    Temp,
    Chart,
    Power,
    Air,
    Clock,
    Info,
    Alarm,
    Stop,
    Bolt,
    Trend,
    Layers
};

namespace Icons {

void paint(QPainter *p, AppIcon icon, const QRectF &rect,
           const QColor &color, qreal strokeWidth = 1.7);
QPixmap pixmap(AppIcon icon, int size, const QColor &color,
               qreal strokeWidth = 1.7, qreal dpr = 2.0);
QIcon icon(AppIcon id, const QColor &color, int size = 18, qreal strokeWidth = 1.7);

} // namespace Icons

// 固定在布局中的图标控件
class IconWidget : public QWidget
{
    Q_OBJECT
public:
    IconWidget(AppIcon icon, const QColor &color, int size = 17, QWidget *parent = nullptr);

    void setColor(const QColor &color);
    void setIcon(AppIcon icon);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    AppIcon m_icon;
    QColor m_color;
    int m_size;
};
