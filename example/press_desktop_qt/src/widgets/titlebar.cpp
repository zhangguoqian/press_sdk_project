#include "widgets/titlebar.h"
#include "theme/icons.h"
#include "theme/theme.h"

#include <QAbstractButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>

namespace {

class CaptionButton : public QAbstractButton
{
public:
    enum Kind { Minimize, Maximize, Close };

    explicit CaptionButton(Kind kind, QWidget *parent = nullptr)
        : QAbstractButton(parent), m_kind(kind)
    {
        setFixedSize(44, 34);
        setCursor(Qt::ArrowCursor);
        setFocusPolicy(Qt::NoFocus);
    }

protected:
    void enterEvent(QEvent *) override { m_hover = true; update(); }
    void leaveEvent(QEvent *) override { m_hover = false; update(); }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);

        if (m_hover || isDown()) {
            const QColor bg = (m_kind == Close)
                ? QColor(0xC4, 0x2B, 0x1C)
                : QColor(255, 255, 255, isDown() ? 34 : 23);
            p.fillRect(rect(), bg);
        }

        p.setPen(QPen((m_hover || isDown()) ? QColor(Qt::white) : QColor(0x9D, 0xB9, 0xCA),
                      1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);

        const QPointF c(width() / 2.0, height() / 2.0);
        switch (m_kind) {
        case Minimize:
            p.drawLine(c + QPointF(-4, 0), c + QPointF(4, 0));
            break;
        case Maximize:
            p.drawRect(QRectF(c.x() - 3.8, c.y() - 3.8, 7.6, 7.6));
            break;
        case Close:
            p.drawLine(c + QPointF(-3.4, -3.4), c + QPointF(3.4, 3.4));
            p.drawLine(c + QPointF(3.4, -3.4), c + QPointF(-3.4, 3.4));
            break;
        }
    }

private:
    Kind m_kind;
    bool m_hover = false;
};

} // namespace

TitleBar::TitleBar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("titleBar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(34);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 4, 0);
    layout->setSpacing(10);

    auto *mark = new IconWidget(AppIcon::Press, Theme::Brand, 16, this);
    auto *title = new QLabel(QStringLiteral("自动热压机 · 上位机"), this);
    title->setObjectName(QStringLiteral("titleText"));
    title->setFont(Theme::ui(12));

    layout->addWidget(mark);
    layout->addWidget(title);
    layout->addStretch(1);

    auto *minimize = new CaptionButton(CaptionButton::Minimize, this);
    auto *maximize = new CaptionButton(CaptionButton::Maximize, this);
    auto *close = new CaptionButton(CaptionButton::Close, this);

    layout->addWidget(minimize);
    layout->addWidget(maximize);
    layout->addWidget(close);

    connect(minimize, &QAbstractButton::clicked, this, &TitleBar::minimizeRequested);
    connect(maximize, &QAbstractButton::clicked, this, &TitleBar::maximizeRequested);
    connect(close, &QAbstractButton::clicked, this, &TitleBar::closeRequested);
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragOffset = event->globalPos() - window()->frameGeometry().topLeft();
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        window()->move(event->globalPos() - m_dragOffset);
        event->accept();
        return;
    }
    QWidget::mouseMoveEvent(event);
}

void TitleBar::mouseReleaseEvent(QMouseEvent *event)
{
    m_dragging = false;
    QWidget::mouseReleaseEvent(event);
}

void TitleBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit maximizeRequested();
        event->accept();
        return;
    }
    QWidget::mouseDoubleClickEvent(event);
}
