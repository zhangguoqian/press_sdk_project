#include "widgets/card.h"
#include "theme/theme.h"

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainterPath>
#include <QRegion>
#include <QResizeEvent>
#include <QStyle>
#include <QVBoxLayout>

namespace {

/**
 * 卡片内容容器：按卡片圆角裁切子控件。
 * 表格、曲线等子控件铺满卡片底部时会用直角背景盖住卡片的圆角，
 * 这里把内容区裁成「上直下圆」，效果等同网页卡片上的 overflow: hidden。
 */
class RoundedBody : public QWidget
{
public:
    explicit RoundedBody(QWidget *parent = nullptr) : QWidget(parent) {}

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);

        constexpr qreal radius = 13.0;
        // 左右/底部让出 1px，保留卡片自身的描边，避免内容盖住边框
        const QRectF area = QRectF(rect()).adjusted(1, 0, -1, -1);
        const qreal x0 = area.left();
        const qreal y0 = area.top();
        const qreal x1 = area.right();
        const qreal y1 = area.bottom();
        QPainterPath clip;
        clip.moveTo(x0, y0);
        clip.lineTo(x1, y0);
        clip.lineTo(x1, y1 - radius);
        clip.arcTo(QRectF(x1 - 2 * radius, y1 - 2 * radius, 2 * radius, 2 * radius), 0, -90);
        clip.lineTo(x0 + radius, y1);
        clip.arcTo(QRectF(x0, y1 - 2 * radius, 2 * radius, 2 * radius), 270, -90);
        clip.closeSubpath();
        setMask(QRegion(clip.toFillPolygon().toPolygon()));
    }
};

} // namespace

Card::Card(const QString &title, AppIcon icon, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("card"));
    setAttribute(Qt::WA_StyledBackground, true);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    m_header = new QWidget(this);
    m_header->setObjectName(QStringLiteral("cardHeaderBordered"));
    m_header->setAttribute(Qt::WA_StyledBackground, true);
    m_header->setFixedHeight(44);
    m_headerLayout = new QHBoxLayout(m_header);
    m_headerLayout->setContentsMargins(14, 0, 14, 0);
    m_headerLayout->setSpacing(10);

    if (!title.isEmpty()) {
        auto *iconWidget = new IconWidget(icon, Theme::Brand, 17, m_header);
        auto *titleLabel = new QLabel(title, m_header);
        titleLabel->setObjectName(QStringLiteral("cardTitle"));
        QFont f = Theme::ui(13, QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
        titleLabel->setFont(f);
        m_headerLayout->addWidget(iconWidget);
        m_headerLayout->addWidget(titleLabel);
    }
    m_headerLayout->addStretch(1);

    outer->addWidget(m_header);

    auto *body = new RoundedBody(this);
    m_bodyLayout = new QVBoxLayout(body);
    m_bodyLayout->setContentsMargins(0, 0, 0, 0);
    m_bodyLayout->setSpacing(0);
    outer->addWidget(body, 1);

    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 2);
    shadow->setColor(QColor(15, 40, 62, 26));
    setGraphicsEffect(shadow);
}

void Card::addHeaderWidget(QWidget *widget)
{
    m_headerLayout->addWidget(widget, 0, Qt::AlignVCenter);
}

void Card::addHeaderSpacing(int px)
{
    m_headerLayout->addSpacing(px);
}

void Card::setHeaderVisible(bool visible)
{
    m_header->setVisible(visible);
}

void Card::setHeaderHeight(int height)
{
    m_header->setFixedHeight(height);
}

void Card::setHeaderBordered(bool bordered)
{
    m_header->setObjectName(bordered ? QStringLiteral("cardHeaderBordered")
                                     : QStringLiteral("cardHeader"));
    m_header->style()->unpolish(m_header);
    m_header->style()->polish(m_header);
}

void Card::setBodyMargins(int left, int top, int right, int bottom)
{
    m_bodyLayout->setContentsMargins(left, top, right, bottom);
}
