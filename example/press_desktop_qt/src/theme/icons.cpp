#include "theme/icons.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QtMath>

namespace Icons {

void paint(QPainter *p, AppIcon icon, const QRectF &rect,
           const QColor &color, qreal strokeWidth)
{
    if (!p || rect.width() <= 0 || rect.height() <= 0)
        return;

    p->save();
    p->setRenderHint(QPainter::Antialiasing, true);
    p->translate(rect.topLeft());
    p->scale(rect.width() / 24.0, rect.height() / 24.0);

    QPen pen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    p->setPen(pen);
    p->setBrush(Qt::NoBrush);

    switch (icon) {
    case AppIcon::Press: {
        p->drawRoundedRect(QRectF(3.5, 3, 17, 4.2), 1.2, 1.2);
        QPainterPath path;
        path.moveTo(8, 7.2);
        path.lineTo(8, 10.3);
        path.lineTo(16, 10.3);
        path.lineTo(16, 7.2);
        p->drawPath(path);
        p->drawRoundedRect(QRectF(9.6, 10.3, 4.8, 3.4), 0.8, 0.8);
        p->drawRoundedRect(QRectF(4.6, 13.7, 14.8, 4), 1.2, 1.2);
        p->drawLine(QPointF(6.5, 17.7), QPointF(6.5, 20.7));
        p->drawLine(QPointF(17.5, 17.7), QPointF(17.5, 20.7));
        break;
    }
    case AppIcon::Link: {
        p->save();
        p->translate(12, 12);
        p->rotate(-45);
        p->translate(-12, -12);
        p->drawRoundedRect(QRectF(2.6, 9.2, 9.2, 5.6), 2.8, 2.8);
        p->drawRoundedRect(QRectF(12.2, 9.2, 9.2, 5.6), 2.8, 2.8);
        p->drawLine(QPointF(10.2, 12), QPointF(13.8, 12));
        p->restore();
        break;
    }
    case AppIcon::Gear: {
        p->drawEllipse(QPointF(12, 12), 3.2, 3.2);
        static const qreal angles[] = {0, 45, 90, 135, 180, 225, 270, 315};
        for (qreal a : angles) {
            const qreal rad = qDegreesToRadians(a);
            const QPointF from(12 + qCos(rad) * 6.4, 12 + qSin(rad) * 6.4);
            const QPointF to(12 + qCos(rad) * 9.2, 12 + qSin(rad) * 9.2);
            p->drawLine(from, to);
        }
        break;
    }
    case AppIcon::Gauge: {
        p->drawArc(QRectF(4, 9.4, 16, 16), 0 * 16, 180 * 16);
        p->drawLine(QPointF(12, 17.5), QPointF(16, 11));
        p->setBrush(color);
        p->drawEllipse(QPointF(12, 17.5), 1.4, 1.4);
        break;
    }
    case AppIcon::Flame: {
        QPainterPath path;
        path.moveTo(12, 2.8);
        path.cubicTo(14.6, 5.8, 17, 8.4, 17, 11.8);
        path.cubicTo(17, 14.56, 14.76, 16.8, 12, 16.8);
        path.cubicTo(9.24, 16.8, 7, 14.56, 7, 11.8);
        path.cubicTo(7, 9.8, 8.2, 8.1, 9.3, 6.7);
        path.cubicTo(9.8, 8.1, 10.7, 8.8, 12.1, 9.4);
        path.cubicTo(11.4, 7.2, 11, 5, 12, 2.8);
        path.closeSubpath();
        p->drawPath(path);
        break;
    }
    case AppIcon::Temp: {
        p->drawRoundedRect(QRectF(10, 3.4, 4, 10.4), 2, 2);
        p->drawEllipse(QRectF(8, 13.3, 8, 8));
        p->drawLine(QPointF(12, 9), QPointF(12, 15.4));
        break;
    }
    case AppIcon::Chart: {
        p->drawLine(QPointF(4, 4), QPointF(4, 20));
        p->drawLine(QPointF(4, 20), QPointF(20, 20));
        QPainterPath path;
        path.moveTo(7, 15.2);
        path.lineTo(10.6, 11.1);
        path.lineTo(13.6, 13.6);
        path.lineTo(19.2, 7);
        p->drawPath(path);
        break;
    }
    case AppIcon::Power: {
        p->drawLine(QPointF(12, 3.2), QPointF(12, 11.6));
        QPainterPath path;
        path.arcMoveTo(QRectF(4.6, 5.0, 14.8, 14.8), 135);
        path.arcTo(QRectF(4.6, 5.0, 14.8, 14.8), 135, -270);
        p->drawPath(path);
        break;
    }
    case AppIcon::Air: {
        QPainterPath p1;
        p1.moveTo(3.5, 8.5);
        p1.lineTo(14.3, 8.5);
        p1.cubicTo(16.9, 8.5, 16.9, 4.4, 14.3, 4.4);
        p1.cubicTo(13.2, 4.4, 12.3, 5.1, 12.0, 6.1);
        p->drawPath(p1);
        QPainterPath p2;
        p2.moveTo(3.5, 12.5);
        p2.lineTo(16.7, 12.5);
        p2.cubicTo(19.3, 12.5, 19.3, 16.6, 16.7, 16.6);
        p2.cubicTo(15.6, 16.6, 14.7, 15.9, 14.4, 14.9);
        p->drawPath(p2);
        p->drawLine(QPointF(3.5, 16.5), QPointF(10.3, 16.5));
        break;
    }
    case AppIcon::Clock: {
        p->drawEllipse(QPointF(12, 12), 8.4, 8.4);
        p->drawLine(QPointF(12, 7.4), QPointF(12, 12));
        p->drawLine(QPointF(12, 12), QPointF(15.2, 14));
        break;
    }
    case AppIcon::Info: {
        p->drawEllipse(QPointF(12, 12), 8.6, 8.6);
        p->drawLine(QPointF(12, 11), QPointF(12, 16.4));
        p->setBrush(color);
        p->drawEllipse(QPointF(12, 8.3), 0.9, 0.9);
        break;
    }
    case AppIcon::Alarm: {
        QPainterPath path;
        path.moveTo(18, 8.8);
        path.arcTo(QRectF(6, 2.8, 12, 12), 0, 180);
        path.lineTo(3.8, 16);
        path.lineTo(20.2, 16);
        path.closeSubpath();
        p->drawPath(path);
        QPainterPath clapper;
        clapper.moveTo(10.2, 19.4);
        clapper.cubicTo(10.6, 20.8, 13.4, 20.8, 13.8, 19.4);
        p->drawPath(clapper);
        break;
    }
    case AppIcon::Stop: {
        p->drawRoundedRect(QRectF(5.5, 5.5, 13, 13), 2.4, 2.4);
        break;
    }
    case AppIcon::Bolt: {
        QPainterPath path;
        path.moveTo(13.5, 2.5);
        path.lineTo(5, 13.6);
        path.lineTo(10.6, 13.6);
        path.lineTo(10, 21.5);
        path.lineTo(19, 10.4);
        path.lineTo(13.4, 10.4);
        path.closeSubpath();
        p->drawPath(path);
        break;
    }
    case AppIcon::Trend: {
        p->drawLine(QPointF(4, 20), QPointF(20, 20));
        QPainterPath path;
        path.moveTo(5, 16);
        path.lineTo(10, 11);
        path.lineTo(13, 13.5);
        path.lineTo(19.5, 6.5);
        p->drawPath(path);
        break;
    }
    case AppIcon::Layers: {
        QPainterPath p1;
        p1.moveTo(12, 3.2);
        p1.lineTo(3.4, 7.6);
        p1.lineTo(12, 12);
        p1.lineTo(20.6, 7.6);
        p1.closeSubpath();
        p->drawPath(p1);
        p->drawLine(QPointF(3.4, 12.4), QPointF(12, 16.8));
        p->drawLine(QPointF(12, 16.8), QPointF(20.6, 12.4));
        break;
    }
    }

    p->restore();
}

QPixmap pixmap(AppIcon icon, int size, const QColor &color,
               qreal strokeWidth, qreal dpr)
{
    const int px = qMax(1, int(size * dpr));
    QPixmap p(px, px);
    p.setDevicePixelRatio(dpr);
    p.fill(Qt::transparent);
    QPainter painter(&p);
    paint(&painter, icon, QRectF(0, 0, size, size), color, strokeWidth);
    painter.end();
    return p;
}

QIcon icon(AppIcon id, const QColor &color, int size, qreal strokeWidth)
{
    QIcon result;
    result.addPixmap(pixmap(id, size, color, strokeWidth, 1.0), QIcon::Normal);
    result.addPixmap(pixmap(id, size, color.lighter(120), strokeWidth, 1.0), QIcon::Active);
    result.addPixmap(pixmap(id, size, color, strokeWidth, 1.0), QIcon::Selected);
    return result;
}

} // namespace Icons

IconWidget::IconWidget(AppIcon icon, const QColor &color, int size, QWidget *parent)
    : QWidget(parent), m_icon(icon), m_color(color), m_size(size)
{
    setFixedSize(size, size);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
}

void IconWidget::setColor(const QColor &color)
{
    if (m_color == color)
        return;
    m_color = color;
    update();
}

void IconWidget::setIcon(AppIcon icon)
{
    if (m_icon == icon)
        return;
    m_icon = icon;
    update();
}

QSize IconWidget::sizeHint() const
{
    return QSize(m_size, m_size);
}

void IconWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter p(this);
    Icons::paint(&p, m_icon, QRectF(0, 0, m_size, m_size), m_color, 1.7);
}
