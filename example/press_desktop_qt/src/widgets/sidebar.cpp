#include "widgets/sidebar.h"
#include "theme/icons.h"
#include "theme/theme.h"

#include <QAbstractButton>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QVBoxLayout>

namespace {

// 品牌图标块（渐变圆角 + 压机图标）
class BrandMark : public QWidget
{
public:
    explicit BrandMark(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(34, 34); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        QLinearGradient gradient(0, 0, width(), height());
        gradient.setColorAt(0.0, Theme::Brand);
        gradient.setColorAt(1.0, QColor(0x00, 0x69, 0xA3));
        p.setPen(Qt::NoPen);
        p.setBrush(gradient);
        p.drawRoundedRect(rect(), 9, 9);
        Icons::paint(&p, AppIcon::Press, QRectF(7, 7, 20, 20), Qt::white, 1.8);
    }
};

} // namespace

class StatusDot : public QWidget
{
public:
    explicit StatusDot(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(9, 9); }

    void setOnline(bool online)
    {
        if (m_online == online)
            return;
        m_online = online;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(Qt::NoPen);
        const QColor color = m_online ? Theme::Green : QColor(0x7C, 0x90, 0xA2);
        QColor ring = color;
        ring.setAlpha(m_online ? 40 : 30);
        p.setBrush(ring);
        p.drawEllipse(QPointF(rect().center()), 4.4, 4.4);
        p.setBrush(color);
        p.drawEllipse(QPointF(rect().center()), 3.0, 3.0);
    }

private:
    bool m_online = true;
};

class RailButton : public QAbstractButton
{
public:
    RailButton(AppIcon icon, const QString &text, QWidget *parent = nullptr)
        : QAbstractButton(parent), m_icon(icon), m_text(text)
    {
        setFixedHeight(40);
        setCursor(Qt::PointingHandCursor);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_Hover, true);
    }

    void setActive(bool active)
    {
        if (m_active == active)
            return;
        m_active = active;
        update();
    }

    void setLocked(bool locked)
    {
        if (m_locked == locked)
            return;
        m_locked = locked;
        update();
    }

protected:
    void enterEvent(QEvent *) override { m_hover = true; update(); }
    void leaveEvent(QEvent *) override { m_hover = false; update(); }

    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setOpacity(m_locked ? 0.4 : 1.0);

        const QRectF r = rect().adjusted(0, 0, 0, 0);
        if (m_active) {
            QLinearGradient gradient(r.topLeft(), r.topRight());
            gradient.setColorAt(0.0, QColor(0, 161, 233, 66));
            gradient.setColorAt(1.0, QColor(0, 161, 233, 13));
            p.setPen(Qt::NoPen);
            p.setBrush(gradient);
            p.drawRoundedRect(r, 8, 8);

            p.setPen(Qt::NoPen);
            p.setBrush(Theme::Brand);
            p.drawRoundedRect(QRectF(0, 9, 3, height() - 18), 1.5, 1.5);
        } else if (m_hover) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(255, 255, 255, 15));
            p.drawRoundedRect(r, 8, 8);
        }

        const QColor iconColor = m_active ? Theme::Brand : QColor(0xB4, 0xCB, 0xD9);
        Icons::paint(&p, m_icon, QRectF(11, (height() - 18) / 2.0, 18, 18), iconColor, 1.7);

        QFont font = Theme::ui(13, m_active ? QFont::DemiBold : QFont::Normal);
        font.setLetterSpacing(QFont::AbsoluteSpacing, 0.3);
        p.setFont(font);
        p.setPen(m_active ? QColor(Qt::white) : QColor(0xB4, 0xCB, 0xD9));
        p.drawText(QRectF(40, 0, width() - 46, height()),
                   Qt::AlignLeft | Qt::AlignVCenter, m_text);
    }

private:
    AppIcon m_icon;
    QString m_text;
    bool m_active = false;
    bool m_hover = false;
    bool m_locked = false;
};

Sidebar::Sidebar(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("sidebar"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedWidth(206);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 12);
    layout->setSpacing(0);

    layout->addWidget(createBrand());

    auto addGroup = [&](const QString &label) {
        auto *group = new QLabel(label, this);
        group->setObjectName(QStringLiteral("railGroupLabel"));
        QFont f = Theme::ui(10, QFont::DemiBold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        group->setFont(f);
        group->setContentsMargins(10, 10, 10, 6);
        layout->addWidget(group);
    };

    auto addItem = [&](const QString &key, AppIcon icon, const QString &text) {
        auto *item = new RailButton(icon, text, this);
        connect(item, &QAbstractButton::clicked, this, [this, key] {
            emit navRequested(key);
        });
        m_items.insert(key, item);
        layout->addWidget(item);
    };

    addGroup(QStringLiteral("模式切换"));
    addItem(QStringLiteral("single"), AppIcon::Press, QStringLiteral("单独模式"));
    addItem(QStringLiteral("coop"), AppIcon::Link, QStringLiteral("合作模式"));

    addGroup(QStringLiteral("系统"));
    addItem(QStringLiteral("settings"), AppIcon::Gear, QStringLiteral("系统设置"));

    layout->addStretch(1);
    layout->addWidget(createFooter());

    setActive(QStringLiteral("single"));
}

QWidget *Sidebar::createBrand()
{
    auto *brand = new QWidget(this);
    auto *layout = new QHBoxLayout(brand);
    layout->setContentsMargins(8, 16, 8, 18);
    layout->setSpacing(10);

    auto *mark = new BrandMark(brand);

    auto *texts = new QWidget(brand);
    auto *textLayout = new QVBoxLayout(texts);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    auto *name = new QLabel(QStringLiteral("自动热压机"), texts);
    name->setObjectName(QStringLiteral("brandName"));
    QFont nameFont = Theme::ui(13, QFont::Bold);
    nameFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
    name->setFont(nameFont);

    auto *sub = new QLabel(QStringLiteral("AUTO HOT PRESS · V3.1.4"), texts);
    sub->setObjectName(QStringLiteral("brandSub"));
    QFont subFont = Theme::num(8);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    sub->setFont(subFont);

    textLayout->addWidget(name);
    textLayout->addWidget(sub);

    layout->addWidget(mark, 0, Qt::AlignTop);
    layout->addWidget(texts, 1, Qt::AlignVCenter);
    return brand;
}

QWidget *Sidebar::createFooter()
{
    auto *footer = new QWidget(this);
    auto *outer = new QVBoxLayout(footer);
    outer->setContentsMargins(8, 12, 8, 0);
    outer->setSpacing(10);

    auto *line = new QFrame(footer);
    line->setFixedHeight(1);
    line->setStyleSheet(QStringLiteral("background: rgba(255,255,255,0.07);"));
    outer->addWidget(line);

    auto *row = new QWidget(footer);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto *dot = new StatusDot(row);
    m_linkDot = dot;
    auto *texts = new QWidget(row);
    auto *textLayout = new QVBoxLayout(texts);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(1);

    auto *title = new QLabel(QStringLiteral("未连接"), texts);
    title->setObjectName(QStringLiteral("railStatText"));
    title->setFont(Theme::ui(11));
    m_linkTitle = title;
    auto *detail = new QLabel(QStringLiteral("本地模拟运行"), texts);
    detail->setObjectName(QStringLiteral("railStatSub"));
    detail->setFont(Theme::num(10));
    m_linkDetail = detail;

    textLayout->addWidget(title);
    textLayout->addWidget(detail);

    layout->addWidget(dot, 0, Qt::AlignTop);
    layout->addWidget(texts, 1);
    outer->addWidget(row);

    return footer;
}

void Sidebar::setLinkStatus(bool connected, const QString &detail)
{
    if (m_linkTitle)
        m_linkTitle->setText(connected ? QStringLiteral("通讯正常")
                                       : QStringLiteral("未连接"));
    if (m_linkDetail) {
        m_linkDetail->setText(detail.isEmpty()
                                  ? (connected ? QStringLiteral("SDK 已连接")
                                               : QStringLiteral("本地模拟运行"))
                                  : detail);
    }
    if (m_linkDot)
        m_linkDot->setOnline(connected);
}

void Sidebar::setLinkWarning(const QString &detail)
{
    if (m_linkTitle)
        m_linkTitle->setText(QStringLiteral("通讯中断"));
    if (m_linkDetail)
        m_linkDetail->setText(detail.isEmpty() ? QStringLiteral("正在尝试自动重连")
                                               : detail);
    if (m_linkDot)
        m_linkDot->setOnline(false);
}

void Sidebar::setModeVisible(const QString &key, bool visible)
{
    if (auto *item = m_items.value(key))
        item->setVisible(visible);
}

void Sidebar::setActive(const QString &key)
{
    for (auto it = m_items.begin(); it != m_items.end(); ++it)
        it.value()->setActive(it.key() == key);
}

void Sidebar::setLocked(const QString &currentKey, bool running)
{
    for (auto it = m_items.begin(); it != m_items.end(); ++it)
        it.value()->setLocked(running && it.key() != currentKey);
}
