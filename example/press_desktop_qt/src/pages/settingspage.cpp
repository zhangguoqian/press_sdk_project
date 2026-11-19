#include "pages/settingspage.h"
#include "theme/icons.h"
#include "theme/theme.h"
#include "widgets/card.h"
#include "widgets/segmented.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QVariant>

namespace {

// 关于页品牌图标（大号渐变圆角）
class AboutLogo : public QWidget
{
public:
    explicit AboutLogo(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(74, 74); }

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
        p.drawRoundedRect(rect(), 20, 20);
        Icons::paint(&p, AppIcon::Press, QRectF(17, 17, 40, 40), Qt::white, 1.5);
    }
};

QWidget *makeRow(const QString &key, const QString &value, bool last, QWidget *parent)
{
    auto *row = new QFrame(parent);
    row->setObjectName(QStringLiteral("aboutRow"));
    row->setAttribute(Qt::WA_StyledBackground, true);
    row->setProperty("last", last);
    row->setFixedHeight(42);

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(14, 0, 14, 0);
    layout->setSpacing(12);

    auto *keyLabel = new QLabel(key, row);
    keyLabel->setObjectName(QStringLiteral("aboutKey"));
    keyLabel->setFont(Theme::ui(12));

    auto *valueLabel = new QLabel(value, row);
    valueLabel->setObjectName(QStringLiteral("aboutVal"));
    valueLabel->setFont(Theme::num(12, QFont::DemiBold));
    valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(keyLabel);
    layout->addStretch(1);
    layout->addWidget(valueLabel);
    return row;
}

} // namespace

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("page"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    layout->addWidget(buildTabCard());

    auto *content = new QWidget(this);
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);
    contentLayout->addWidget(buildHeroCard(), 1);
    contentLayout->addWidget(buildAboutCard(), 1);
    layout->addWidget(content, 1);
}

QWidget *SettingsPage::buildTabCard()
{
    auto *card = new Card(QString(), AppIcon::Info, this);
    card->setHeaderVisible(false);
    card->setFixedHeight(54);
    card->setBodyMargins(14, 10, 14, 10);

    auto *tabs = new Segmented({QStringLiteral("关于本机")}, card);
    tabs->setFixedWidth(118);
    card->contentLayout()->addWidget(tabs, 0, Qt::AlignLeft | Qt::AlignVCenter);
    return card;
}

QWidget *SettingsPage::buildHeroCard()
{
    auto *card = new Card(QString(), AppIcon::Info, this);
    card->setHeaderVisible(false);

    auto *hero = new QWidget(card);
    auto *layout = new QVBoxLayout(hero);
    layout->setContentsMargins(24, 24, 24, 24);
    layout->setSpacing(14);
    layout->setAlignment(Qt::AlignCenter);

    layout->addStretch(1);
    layout->addWidget(new AboutLogo(hero), 0, Qt::AlignHCenter);

    auto *title = new QLabel(QStringLiteral("自动热压机"), hero);
    title->setObjectName(QStringLiteral("heroTitle"));
    QFont titleFont = Theme::ui(19, QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);

    auto *subtitle = new QLabel(QStringLiteral("AUTO HOT PRESS"), hero);
    subtitle->setObjectName(QStringLiteral("heroSub"));
    QFont subFont = Theme::num(12);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    subtitle->setFont(subFont);
    subtitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitle);

    auto *chip = new QLabel(QStringLiteral("已授权 · 单机永久许可"), hero);
    chip->setObjectName(QStringLiteral("chipBrand"));
    chip->setFont(Theme::ui(11));
    chip->setAlignment(Qt::AlignCenter);
    chip->setFixedHeight(26);
    layout->addWidget(chip, 0, Qt::AlignHCenter);

    auto *meta = new QFrame(hero);
    meta->setObjectName(QStringLiteral("heroMeta"));
    meta->setAttribute(Qt::WA_StyledBackground, true);
    meta->setFixedHeight(62);
    auto *metaLayout = new QVBoxLayout(meta);
    metaLayout->setContentsMargins(0, 14, 0, 0);
    metaLayout->setSpacing(5);
    auto *metaKey = new QLabel(QStringLiteral("通讯状态"), meta);
    metaKey->setObjectName(QStringLiteral("heroMetaKey"));
    metaKey->setAlignment(Qt::AlignCenter);
    auto *metaValue = new QLabel(QStringLiteral("已连接"), meta);
    metaValue->setObjectName(QStringLiteral("heroMetaVal"));
    metaValue->setFont(Theme::num(13, QFont::Bold));
    metaValue->setAlignment(Qt::AlignCenter);
    metaLayout->addWidget(metaKey);
    metaLayout->addWidget(metaValue);
    layout->addWidget(meta);

    layout->addStretch(1);

    card->contentLayout()->addWidget(hero, 1);
    return card;
}

QWidget *SettingsPage::buildAboutCard()
{
    auto *card = new Card(QString(), AppIcon::Info, this);
    card->setHeaderVisible(false);
    card->setBodyMargins(16, 16, 16, 16);
    card->contentLayout()->setSpacing(14);

    const QVector<QPair<QString, QString>> firstRows = {
        {QStringLiteral("仪器名称"), QStringLiteral("自动热压机")},
        {QStringLiteral("仪器型号"), QStringLiteral("TYPE")},
        {QStringLiteral("油缸直径"), QStringLiteral("110.0 mm")},
        {QStringLiteral("最大压力"), QStringLiteral("30.0 Ton")},
        {QStringLiteral("最小压力"), QStringLiteral("0.5 Ton")},
        {QStringLiteral("最高温度"), QStringLiteral("310.0 °C")}};

    const QVector<QPair<QString, QString>> secondRows = {
        {QStringLiteral("压力显示精度"), QStringLiteral("0.1 Ton")},
        {QStringLiteral("压强显示精度"), QStringLiteral("0.01 MPa")},
        {QStringLiteral("温度显示精度"), QStringLiteral("0.1 °C")},
        {QStringLiteral("自动补压差值"), QStringLiteral("0.3 Ton")},
        {QStringLiteral("本机序列号"), QStringLiteral("ASD123")}};

    card->contentLayout()->addWidget(buildAboutList(firstRows));
    card->contentLayout()->addWidget(buildAboutList(secondRows));
    card->contentLayout()->addStretch(1);
    return card;
}

QWidget *SettingsPage::buildAboutList(const QVector<QPair<QString, QString>> &rows)
{
    auto *list = new QFrame(this);
    list->setObjectName(QStringLiteral("aboutList"));
    list->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(list);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (int i = 0; i < rows.size(); ++i)
        layout->addWidget(makeRow(rows.at(i).first, rows.at(i).second,
                                  i == rows.size() - 1, list));
    return list;
}
