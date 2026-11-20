#include "widgets/kvlist.h"
#include "theme/theme.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>
#include <QVariant>
#include <QtMath>

namespace {

// 运行指示圆点：呼吸光晕
class DotWidget : public QWidget
{
public:
    explicit DotWidget(const QColor &color, QWidget *parent = nullptr)
        : QWidget(parent), m_color(color), m_phase(0.0)
    {
        setFixedSize(9, 9);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
        auto *timer = new QTimer(this);
        timer->setInterval(90);
        connect(timer, &QTimer::timeout, this, [this] {
            m_phase += 0.14;
            update();
        });
        timer->start();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        const QPointF center(width() / 2.0, height() / 2.0);
        const double pulse = (qSin(m_phase) + 1.0) / 2.0;
        QColor glow = m_color;
        glow.setAlphaF(0.10 + 0.14 * pulse);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(center, 4.4, 4.4);
        p.setBrush(m_color);
        p.drawEllipse(center, 3.0, 3.0);
    }

private:
    QColor m_color;
    double m_phase;
};

} // namespace

KvList::KvList(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void KvList::setRowHeight(int height)
{
    m_rowHeight = qMax(24, height);
}

void KvList::addRow(const QString &key, const QString &label,
                    const QString &unit, const QColor &dot)
{
    auto *row = new QWidget(this);
    row->setObjectName(QStringLiteral("kvRow"));
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setFixedHeight(m_rowHeight);

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(7);

    if (dot.isValid()) {
        auto *dotWidget = new DotWidget(dot, row);
        layout->addWidget(dotWidget, 0, Qt::AlignVCenter);
    }

    QString html = QStringLiteral("<span style=\"font-size:12px;color:#3B5770;\">%1</span>")
                       .arg(label.toHtmlEscaped());
    if (!unit.isEmpty()) {
        html += QStringLiteral("<span style=\"font-size:10px;color:#7C90A2;\">&nbsp;%1</span>")
                    .arg(unit.toHtmlEscaped());
    }
    auto *labelWidget = new QLabel(html, row);
    labelWidget->setTextFormat(Qt::RichText);
    labelWidget->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    auto *valueWidget = new QLabel(QStringLiteral("0.0"), row);
    valueWidget->setObjectName(QStringLiteral("kvValue"));
    valueWidget->setFont(Theme::num(23, QFont::DemiBold));
    valueWidget->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(labelWidget, 0, Qt::AlignVCenter);
    layout->addStretch(1);
    layout->addWidget(valueWidget, 0, Qt::AlignVCenter);

    // 前一行取消“末行”标记
    if (m_rowCount > 0) {
        for (auto it = m_rows.begin(); it != m_rows.end(); ++it) {
            it.value()->setProperty("last", false);
            it.value()->style()->unpolish(it.value());
            it.value()->style()->polish(it.value());
        }
    }
    row->setProperty("last", true);

    auto *containerLayout = qobject_cast<QVBoxLayout *>(this->layout());
    containerLayout->addWidget(row);

    m_values.insert(key, valueWidget);
    m_rows.insert(key, row);
    ++m_rowCount;
}

void KvList::setValue(const QString &key, const QString &text)
{
    auto it = m_values.find(key);
    if (it != m_values.end() && it.value()->text() != text)
        it.value()->setText(text);
}

void KvList::setValueColor(const QString &key, const QColor &color)
{
    auto it = m_values.find(key);
    if (it == m_values.end())
        return;
    it.value()->setStyleSheet(QStringLiteral("color: %1;").arg(color.name()));
}

QString KvList::value(const QString &key) const
{
    auto it = m_values.constFind(key);
    return it == m_values.constEnd() ? QString() : it.value()->text();
}
