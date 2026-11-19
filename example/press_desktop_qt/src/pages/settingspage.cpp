#include "pages/settingspage.h"
#include "core/machine_state.h"
#include "core/press_service.h"
#include "theme/icons.h"
#include "theme/theme.h"
#include "widgets/card.h"
#include "widgets/segmented.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QVariant>

#include <cmath>

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

QWidget *makeRow(const QString &key, const QString &value, bool last, QWidget *parent,
                 QLabel **valueOut = nullptr)
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
    if (valueOut)
        *valueOut = valueLabel;
    return row;
}

QString number1(double value)
{
    return QString::number(value, 'f', 1);
}

QString numberTrim(double value, int decimals = 2)
{
    QString text = QString::number(value, 'f', decimals);
    while (text.contains(QLatin1Char('.'))
           && (text.endsWith(QLatin1Char('0')) || text.endsWith(QLatin1Char('.')))) {
        text.chop(1);
    }
    return text;
}

// 小数位数 -> 显示精度文本，如 2 -> "0.01"
QString precisionText(int decimals)
{
    const int digits = qBound(0, decimals, 4);
    return QString::number(1.0 / std::pow(10.0, digits), 'f', digits);
}

} // namespace

SettingsPage::SettingsPage(MachineState *state, PressService *service, QWidget *parent)
    : QWidget(parent)
    , m_state(state)
    , m_service(service)
{
    setObjectName(QStringLiteral("page"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(14);

    layout->addWidget(buildTabCard());
    layout->addWidget(buildLinkCard());

    auto *content = new QWidget(this);
    auto *contentLayout = new QHBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(14);
    contentLayout->addWidget(buildHeroCard(), 1);
    contentLayout->addWidget(buildAboutCard(), 1);
    layout->addWidget(content, 1);

    // 连接设备后，用 ReadOnlyData 填充“关于本机”
    connect(m_service, &PressService::readOnlyDataUpdated,
            this, &SettingsPage::applyReadOnlyData);
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

QWidget *SettingsPage::buildLinkCard()
{
    auto *card = new Card(QStringLiteral("设备通讯"), AppIcon::Link, this);
    card->setFixedHeight(94);
    card->setBodyMargins(14, 8, 14, 8);

    auto *row = new QWidget(card);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    m_linkState = new QLabel(QStringLiteral("未连接"), row);
    m_linkState->setFont(Theme::ui(12, QFont::DemiBold));
    m_linkState->setMinimumWidth(52);

    m_endpoint = new QLineEdit(row);
    m_endpoint->setObjectName(QStringLiteral("plainEdit"));
    m_endpoint->setPlaceholderText(QStringLiteral("IP:端口（TCP）或 COM3（串口）"));
    m_endpoint->setFixedHeight(32);
    m_endpoint->setText(QSettings(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"))
                            .value(QStringLiteral("link/endpoint"),
                                   QStringLiteral("192.168.1.20:9000"))
                            .toString());

    m_linkButton = new QPushButton(QStringLiteral("连接"), row);
    m_linkButton->setObjectName(QStringLiteral("btnBrand"));
    m_linkButton->setFixedSize(96, 32);
    m_linkButton->setCursor(Qt::PointingHandCursor);

    layout->addWidget(m_linkState);
    layout->addWidget(m_endpoint, 1);
    layout->addWidget(m_linkButton);
    card->contentLayout()->addWidget(row);

    connect(m_linkButton, &QPushButton::clicked, this, &SettingsPage::connectOrDisconnect);
    connect(m_endpoint, &QLineEdit::returnPressed, this, &SettingsPage::connectOrDisconnect);
    connect(m_service, &PressService::connectionChanged, this,
            [this](bool connected, const QString &) { updateLinkUi(connected); });
    connect(m_service, &PressService::sdkError, this, [this](const QString &message) {
        if (m_service->isConnected() || m_service->isConnecting())
            return;
        m_linkState->setText(QStringLiteral("连接失败"));
        m_linkState->setStyleSheet(QStringLiteral("color:#C8102E;"));
        m_linkState->setToolTip(message);
    });

    updateLinkUi(m_service->isConnected());
    return card;
}

void SettingsPage::updateLinkUi(bool connected)
{
    if (m_linkButton) {
        m_linkButton->setText(connected ? QStringLiteral("断开") : QStringLiteral("连接"));
        m_linkButton->setEnabled(!m_service->isConnecting());
    }
    if (m_endpoint)
        m_endpoint->setEnabled(!connected && !m_service->isConnecting());
    if (m_linkState) {
        m_linkState->setText(connected ? QStringLiteral("已连接") : QStringLiteral("未连接"));
        m_linkState->setStyleSheet(connected ? QStringLiteral("color:#00A32A;")
                                             : QStringLiteral("color:#5F7488;"));
    }
    if (m_heroLinkValue)
        m_heroLinkValue->setText(connected ? QStringLiteral("已连接")
                                           : QStringLiteral("未连接"));
}

void SettingsPage::connectOrDisconnect()
{
    if (!m_service || !m_state || m_service->isConnecting())
        return;

    if (m_service->isConnected()) {
        m_service->disconnectDevice();
        return;
    }

    if (m_state->running()) {
        m_linkState->setText(QStringLiteral("运行中"));
        m_linkState->setStyleSheet(QStringLiteral("color:#E8730C;"));
        m_linkState->setToolTip(QStringLiteral("设备运行中，请先停止后再切换连接"));
        return;
    }

    const QString endpoint = m_endpoint->text().trimmed();
    if (endpoint.isEmpty())
        return;

    QSettings(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"))
        .setValue(QStringLiteral("link/endpoint"), endpoint);

    m_linkState->setText(QStringLiteral("连接中…"));
    m_linkState->setStyleSheet(QStringLiteral("color:#0079B4;"));
    m_linkState->setToolTip(QString());
    m_linkButton->setEnabled(false);

    // 带端口号的地址走 TCP，否则按串口处理；连接在后台线程执行
    m_service->connectDevice(endpoint, endpoint.contains(QLatin1Char(':')));
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
    auto *metaValue = new QLabel(m_service->isConnected() ? QStringLiteral("已连接")
                                                         : QStringLiteral("未连接"),
                                 meta);
    metaValue->setObjectName(QStringLiteral("heroMetaVal"));
    metaValue->setFont(Theme::num(13, QFont::Bold));
    metaValue->setAlignment(Qt::AlignCenter);
    m_heroLinkValue = metaValue;
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

    // 数值由 ReadOnlyData 回调填充，未连接时显示占位符
    const QVector<QPair<QString, QString>> firstRows = {
        {QStringLiteral("设备名称"), QStringLiteral("—")},
        {QStringLiteral("设备型号"), QStringLiteral("—")},
        {QStringLiteral("本机序列号"), QStringLiteral("—")},
        {QStringLiteral("油缸直径"), QStringLiteral("—")},
        {QStringLiteral("最大压力"), QStringLiteral("—")},
        {QStringLiteral("最小压力"), QStringLiteral("—")}};

    const QVector<QPair<QString, QString>> secondRows = {
        {QStringLiteral("最高温度"), QStringLiteral("—")},
        {QStringLiteral("最低温度"), QStringLiteral("—")},
        {QStringLiteral("自动补压差值"), QStringLiteral("—")},
        {QStringLiteral("显示精度"), QStringLiteral("—")},
        {QStringLiteral("压力段数"), QStringLiteral("—")},
        {QStringLiteral("温度段数"), QStringLiteral("—")}};

    card->contentLayout()->addWidget(buildAboutList(
        firstRows, {&m_devName, &m_devType, &m_devSerial,
                    &m_devDiameter, &m_devMaxP, &m_devMinP}));
    card->contentLayout()->addWidget(buildAboutList(
        secondRows, {&m_devMaxT, &m_devMinT, &m_devBoost,
                     &m_devPrecision, &m_devPStep, &m_devTStep}));
    card->contentLayout()->addStretch(1);
    return card;
}

QWidget *SettingsPage::buildAboutList(const QVector<QPair<QString, QString>> &rows,
                                      const QVector<QLabel **> &outs)
{
    auto *list = new QFrame(this);
    list->setObjectName(QStringLiteral("aboutList"));
    list->setAttribute(Qt::WA_StyledBackground, true);

    auto *layout = new QVBoxLayout(list);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (int i = 0; i < rows.size(); ++i) {
        QLabel *valueLabel = nullptr;
        layout->addWidget(makeRow(rows.at(i).first, rows.at(i).second,
                                  i == rows.size() - 1, list, &valueLabel));
        if (i < outs.size() && outs.at(i))
            *outs.at(i) = valueLabel;
    }
    return list;
}

void SettingsPage::applyReadOnlyData(const PressReadOnlyData &data)
{
    auto setText = [](QLabel *label, const QString &text) {
        if (!label)
            return;
        label->setText(text.isEmpty() ? QStringLiteral("—") : text);
    };

    setText(m_devName, data.nameZh.isEmpty() ? data.nameEn : data.nameZh);
    if (m_devName)
        m_devName->setToolTip(data.nameEn);
    setText(m_devType, data.type);
    setText(m_devSerial, data.serialNumber);
    setText(m_devDiameter, QStringLiteral("%1 mm").arg(number1(data.diameter)));
    setText(m_devMaxP, QStringLiteral("%1 Ton").arg(number1(data.maxPLimit)));
    setText(m_devMinP, QStringLiteral("%1 Ton").arg(number1(data.minPLimit)));
    setText(m_devMaxT, QStringLiteral("%1 °C").arg(number1(data.maxTLimit)));
    setText(m_devMinT, QStringLiteral("%1 °C").arg(number1(data.minTLimit)));
    setText(m_devBoost, QStringLiteral("%1 Ton").arg(numberTrim(data.autoBoost)));
    setText(m_devPrecision,
            QStringLiteral("%1 Ton · %2 MPa · %3 °C")
                .arg(precisionText(data.pDecimal))
                .arg(precisionText(data.pressDecimal))
                .arg(precisionText(data.tDecimal)));
    setText(m_devPStep, QStringLiteral("%1 段").arg(data.maxPStep));
    setText(m_devTStep, QStringLiteral("%1 段").arg(data.maxTStep));
}
