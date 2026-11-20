#include "widgets/realtimechart.h"
#include "theme/theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QtMath>

RealtimeChart::RealtimeChart(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 初始曲线：填充最近 180 s 的上一轮完整循环（加压 3 段 + 升温 + 泄压回落）
    static const double pressureCurve[] = {1.0, 2.0, 3.0};
    for (int i = 0; i < kWindow; ++i) {
        const double k = double(i) / kWindow;
        const double seg = k * 3.0;
        const int stage = qMin(2, int(seg));
        const double previous = stage == 0 ? 0.0 : pressureCurve[stage - 1];
        double pv = previous + (pressureCurve[stage] - previous) * qMin(1.0, (seg - stage) * 8.0);
        if (k > 0.9)
            pv *= qMax(0.0, 1.0 - (k - 0.9) / 0.1);
        const double tv = k < 0.78 ? 24.5 + (k / 0.78) * 25.5
                                   : 50.0 - ((k - 0.78) / 0.22) * 25.5;
        m_pressure.append(pv);
        m_up.append(tv);
        m_down.append(tv - 2.5);
    }
}

void RealtimeChart::addSample(double pressure, double upTemp, double downTemp)
{
    m_pressure.append(qMax(0.0, pressure));
    m_up.append(qMax(0.0, upTemp));
    m_down.append(qMax(0.0, downTemp));
    while (m_pressure.size() > kWindow) {
        m_pressure.removeFirst();
        m_up.removeFirst();
        m_down.removeFirst();
    }
    update();
}

void RealtimeChart::clear()
{
    m_pressure.clear();
    m_up.clear();
    m_down.clear();
    update();
}

QSize RealtimeChart::sizeHint() const
{
    return QSize(420, 230);
}

double RealtimeChart::temperatureAxisMax() const
{
    const double lastUp = m_up.isEmpty() ? 0.0 : m_up.last();
    const double lastDown = m_down.isEmpty() ? 0.0 : m_down.last();
    const double value = qMax(lastUp, lastDown);
    return qMax(80.0, qCeil((value * 1.2) / 20.0) * 20.0);
}

void RealtimeChart::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), Qt::white);

    const qreal padLeft = 34.0;
    const qreal padRight = 40.0;
    const qreal padTop = 12.0;
    const qreal padBottom = 22.0;
    const qreal plotWidth = width() - padLeft - padRight;
    const qreal plotHeight = height() - padTop - padBottom;
    if (plotWidth <= 12 || plotHeight <= 12)
        return;

    const QRectF plot(padLeft, padTop, plotWidth, plotHeight);
    const double pMax = 30.0;
    const double tMax = temperatureAxisMax();
    const int rows = 5;

    // 水平网格 + 双轴刻度
    QFont labelFont = Theme::num(9);
    p.setFont(labelFont);
    for (int i = 0; i <= rows; ++i) {
        const qreal y = padTop + (plotHeight / rows) * i;
        p.setPen(QColor(0xED, 0xF2, 0xF7));
        p.drawLine(QPointF(padLeft, y + 0.5), QPointF(padLeft + plotWidth, y + 0.5));

        p.setPen(QColor(0x8C, 0xA2, 0xB3));
        p.drawText(QRectF(0, y - 7, padLeft - 7, 14),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(pMax - (pMax / rows) * i, 'f', 0));

        p.setPen(QColor(0xC0, 0x8A, 0x5A));
        p.drawText(QRectF(padLeft + plotWidth + 7, y - 7, padRight - 9, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(tMax - (tMax / rows) * i, 'f', 0));
    }

    // 垂直网格
    p.setPen(QColor(0xF4, 0xF8, 0xFB));
    for (int i = 1; i < 6; ++i) {
        const qreal x = padLeft + (plotWidth / 6) * i;
        p.drawLine(QPointF(x + 0.5, padTop), QPointF(x + 0.5, padTop + plotHeight));
    }

    const int count = m_pressure.size();
    if (count == 0)
        return;

    auto xAt = [&](int index) {
        return padLeft + (plotWidth * index) / qreal(kWindow - 1);
    };

    p.save();
    p.setClipRect(plot.adjusted(-1, -1, 1, 1));

    // 压力曲线 + 面积渐变
    {
        QLinearGradient gradient(0, padTop, 0, padTop + plotHeight);
        gradient.setColorAt(0.0, QColor(0, 161, 233, 46));
        gradient.setColorAt(1.0, QColor(0, 161, 233, 0));

        QPainterPath area;
        area.moveTo(xAt(0), padTop + plotHeight);
        for (int i = 0; i < count; ++i) {
            const double y = padTop + plotHeight
                - qBound(0.0, m_pressure.at(i), pMax) / pMax * plotHeight;
            area.lineTo(xAt(i), y);
        }
        area.lineTo(xAt(count - 1), padTop + plotHeight);
        area.closeSubpath();
        p.fillPath(area, gradient);

        QPainterPath line;
        for (int i = 0; i < count; ++i) {
            const double y = padTop + plotHeight
                - qBound(0.0, m_pressure.at(i), pMax) / pMax * plotHeight;
            if (i == 0)
                line.moveTo(xAt(i), y);
            else
                line.lineTo(xAt(i), y);
        }
        p.setPen(QPen(Theme::Brand, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(line);

        const QPointF head(xAt(count - 1),
                           padTop + plotHeight
                               - qBound(0.0, m_pressure.last(), pMax) / pMax * plotHeight);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 161, 233, 40));
        p.drawEllipse(head, 6.5, 6.5);
        p.setBrush(Theme::Brand);
        p.drawEllipse(head, 3.4, 3.4);
    }

    // 温度曲线（右轴）
    auto drawTemperature = [&](const QVector<double> &data, const QColor &color,
                               const QColor &glow) {
        if (data.isEmpty())
            return;
        QPainterPath line;
        for (int i = 0; i < data.size(); ++i) {
            const double y = padTop + plotHeight
                - qBound(0.0, data.at(i), tMax) / tMax * plotHeight;
            if (i == 0)
                line.moveTo(xAt(i), y);
            else
                line.lineTo(xAt(i), y);
        }
        p.setPen(QPen(color, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(line);

        const QPointF head(xAt(data.size() - 1),
                           padTop + plotHeight - qBound(0.0, data.last(), tMax) / tMax * plotHeight);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(head, 6.5, 6.5);
        p.setBrush(color);
        p.drawEllipse(head, 3.4, 3.4);
    };

    drawTemperature(m_up, QColor(0xC8, 0x10, 0x2E), QColor(200, 16, 46, 40));
    drawTemperature(m_down, QColor(0xE8, 0x73, 0x0C), QColor(232, 115, 12, 40));

    p.restore();
}
