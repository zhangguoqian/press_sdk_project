#include "pages/settingspage.h"
#include "core/machine_state.h"
#include "core/press_service.h"
#include "dialogs/overlaydialogs.h"
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
    connect(m_service, &PressService::versionInfoUpdated,
            this, &SettingsPage::applyVersionInfo);
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
    m_endpoint->setReadOnly(true);
    m_endpoint->setText(QSettings(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"))
                            .value(QStringLiteral("link/endpoint"),
                                   QStringLiteral("192.168.1.20:9000"))
                            .toString());

    m_linkButton = new QPushButton(QStringLiteral("连接设备"), row);
    m_linkButton->setObjectName(QStringLiteral("btnBrand"));
    m_linkButton->setFixedSize(96, 32);
    m_linkButton->setCursor(Qt::PointingHandCursor);

    layout->addWidget(m_linkState);
    layout->addWidget(m_endpoint, 1);
    layout->addWidget(m_linkButton);
    card->contentLayout()->addWidget(row);

    connect(m_linkButton, &QPushButton::clicked, this, &SettingsPage::connectOrDisconnect);
    connect(m_service, &PressService::connectionChanged, this,
            [this](bool connected, const QString &) {
                updateLinkUi(connected);
                // 断开后固件信息失效，恢复占位符
                if (!connected) {
                    if (m_devAppType)
                        m_devAppType->setText(QStringLiteral("—"));
                    if (m_devVersion)
                        m_devVersion->setText(QStringLiteral("—"));
                }
            });
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
        m_linkButton->setText(connected ? QStringLiteral("断开连接")
                                        : QStringLiteral("连接设备"));
        m_linkButton->setEnabled(!m_service->isConnecting());
    }
    if (m_endpoint) {
        // 端点统一在连接对话框中修改，这里只读展示
        m_endpoint->setEnabled(false);
        const QString saved = QSettings(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"))
                                  .value(QStringLiteral("link/endpoint"))
                                  .toString();
        const QString shown = connected && !m_service->endpoint().isEmpty()
            ? m_service->endpoint()
            : saved;
        if (!shown.isEmpty())
            m_endpoint->setText(shown);
    }
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

    if (m_state->running()) {
        m_linkState->setText(QStringLiteral("运行中"));
        m_linkState->setStyleSheet(QStringLiteral("color:#E8730C;"));
        m_linkState->setToolTip(QStringLiteral("设备运行中，请先停止后再切换连接"));
        return;
    }

    m_linkState->setToolTip(QString());
    // 断开当前连接后回到设备连接对话框（可选串口、网络或本地模拟）；
    // 对话框打开期间隐藏主界面，关闭后再恢复
    if (m_service->isConnected())
        m_service->disconnectDevice();

    QWidget *host = window();
    if (host)
        host->hide();
    const bool exitRequested = Dialogs::connectDevice(host ? host : this, m_service);
    if (exitRequested)
        return;
    if (host) {
        host->show();
        host->raise();
        host->activateWindow();
    }
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

    m_heroTitle = new QLabel(QStringLiteral("自动热压机"), hero);
    m_heroTitle->setObjectName(QStringLiteral("heroTitle"));
    QFont titleFont = Theme::ui(19, QFont::Bold);
    titleFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.0);
    m_heroTitle->setFont(titleFont);
    m_heroTitle->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_heroTitle);

    m_heroSub = new QLabel(QStringLiteral("AUTO HOT PRESS"), hero);
    m_heroSub->setObjectName(QStringLiteral("heroSub"));
    QFont subFont = Theme::num(12);
    subFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
    m_heroSub->setFont(subFont);
    m_heroSub->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_heroSub);

    auto *chip = new QLabel(
        QStringLiteral("PressSDK(%1)").arg(m_service ? m_service->sdkVersion() : QString()), hero);
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
    card->setBodyMargins(16, 12, 16, 12);
    card->contentLayout()->setSpacing(10);

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

    // 只读信息下方：固件版本
    const QVector<QPair<QString, QString>> versionRows = {
        {QStringLiteral("应用类型"), QStringLiteral("—")},
        {QStringLiteral("固件版本"), QStringLiteral("—")}};

    card->contentLayout()->addWidget(buildAboutList(
        firstRows, {&m_devName, &m_devType, &m_devSerial,
                    &m_devDiameter, &m_devMaxP, &m_devMinP}));
    card->contentLayout()->addWidget(buildAboutList(
        secondRows, {&m_devMaxT, &m_devMinT, &m_devBoost,
                     &m_devPrecision, &m_devPStep, &m_devTStep}));
    card->contentLayout()->addWidget(buildAboutList(
        versionRows, {&m_devAppType, &m_devVersion}));
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
    // 品牌区标题与窗口标题、侧栏一致，使用设备上报的名称
    if (m_heroTitle) {
        const QString heroName = data.nameZh.trimmed().isEmpty()
            ? data.nameEn.trimmed()
            : data.nameZh.trimmed();
        if (!heroName.isEmpty())
            m_heroTitle->setText(heroName);
    }
    // 品牌区副标题显示设备型号，与侧栏品牌区一致
    if (m_heroSub && !data.type.trimmed().isEmpty())
        m_heroSub->setText(data.type.trimmed());
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

void SettingsPage::applyVersionInfo(const DeviceVersionInfo &info)
{
    if (!info.valid)
        return;

    QString appType;
    switch (info.appType) {
    case 1:
        appType = QStringLiteral("自动热压");
        break;
    case 2:
        appType = QStringLiteral("自动压片");
        break;
    case 3:
        appType = QStringLiteral("自动加热");
        break;
    case 4:
        appType = QStringLiteral("手动压片");
        break;
    default:
        appType = QStringLiteral("未知（%1）").arg(info.appType);
        break;
    }

    if (m_devAppType)
        m_devAppType->setText(appType);
    if (m_devVersion) {
        m_devVersion->setText(QStringLiteral("V%1.%2.%3")
                                  .arg(info.major)
                                  .arg(info.minor)
                                  .arg(info.patch));
    }
}
