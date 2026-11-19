#include "dialogs/overlaydialogs.h"
#include "core/format.h"
#include "core/press_service.h"
#include "theme/theme.h"

#include <QButtonGroup>
#include <QComboBox>
#include <QCoreApplication>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QShowEvent>
#include <QVBoxLayout>

#include <memory>

namespace {

QPushButton *makeFooterButton(const QString &text, const QString &objectName, QWidget *parent)
{
    auto *button = new QPushButton(text, parent);
    button->setObjectName(objectName);
    button->setFixedHeight(44);
    button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(Theme::ui(13, QFont::DemiBold));
    return button;
}

QColor toneColor(DialogTone tone)
{
    switch (tone) {
    case DialogTone::Warn: return Theme::Amber600;
    case DialogTone::Bad:  return Theme::Red;
    case DialogTone::Info:
    default:               return Theme::Brand700;
    }
}

QString toneName(DialogTone tone)
{
    switch (tone) {
    case DialogTone::Warn: return QStringLiteral("warn");
    case DialogTone::Bad:  return QStringLiteral("bad");
    case DialogTone::Info:
    default:               return QStringLiteral("info");
    }
}

// QSS 接管 QComboBox 后系统下拉箭头不再绘制，这里补一个细描边箭头
class PortComboBox : public QComboBox
{
public:
    using QComboBox::QComboBox;

protected:
    void paintEvent(QPaintEvent *event) override
    {
        QComboBox::paintEvent(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(isEnabled() ? QColor(0x5F, 0x74, 0x88) : QColor(0x93, 0xA7, 0xB7), 1.6);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);
        const qreal cx = width() - 14;
        const qreal cy = height() / 2.0;
        QPainterPath chevron;
        chevron.moveTo(cx - 4, cy - 2.5);
        chevron.lineTo(cx, cy + 1.5);
        chevron.lineTo(cx + 4, cy - 2.5);
        painter.drawPath(chevron);
    }
};

} // namespace

OverlayDialog::OverlayDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setModal(true);

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);
    outer->addStretch(1);

    auto *row = new QHBoxLayout;
    row->setContentsMargins(0, 0, 0, 0);
    row->addStretch(1);

    m_card = new QFrame(this);
    m_card->setObjectName(QStringLiteral("modalCard"));
    m_card->setAttribute(Qt::WA_StyledBackground, true);
    m_card->setFixedWidth(420);
    m_card->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);

    row->addWidget(m_card, 0, Qt::AlignCenter);
    row->addStretch(1);
    outer->addLayout(row);
    outer->addStretch(1);

    auto *shadow = new QGraphicsDropShadowEffect(m_card);
    shadow->setBlurRadius(48);
    shadow->setOffset(0, 12);
    shadow->setColor(QColor(6, 24, 38, 80));
    m_card->setGraphicsEffect(shadow);
}

QWidget *OverlayDialog::buildHeader(AppIcon icon, const QString &title, DialogTone tone)
{
    auto *header = new QWidget(m_card);
    auto *layout = new QHBoxLayout(header);
    layout->setContentsMargins(20, 18, 20, 0);
    layout->setSpacing(10);

    auto *iconBox = new QFrame(header);
    iconBox->setObjectName(QStringLiteral("modalIcon"));
    iconBox->setAttribute(Qt::WA_StyledBackground, true);
    iconBox->setProperty("tone", toneName(tone));
    iconBox->setFixedSize(30, 30);
    auto *iconLayout = new QHBoxLayout(iconBox);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->addWidget(new IconWidget(icon, toneColor(tone), 17, iconBox), 0,
                          Qt::AlignCenter);

    auto *titleLabel = new QLabel(title, header);
    titleLabel->setObjectName(QStringLiteral("modalTitle"));
    titleLabel->setFont(Theme::ui(15, QFont::Bold));

    layout->addWidget(iconBox);
    layout->addWidget(titleLabel);
    layout->addStretch(1);
    return header;
}

void OverlayDialog::showEvent(QShowEvent *event)
{
    if (parentWidget()) {
        QWidget *window = parentWidget()->window();
        if (window)
            setGeometry(window->frameGeometry());
    }
    QDialog::showEvent(event);
}

void OverlayDialog::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.fillRect(rect(), QColor(6, 24, 38, 117));
}

void OverlayDialog::mousePressEvent(QMouseEvent *event)
{
    if (m_card) {
        const QPoint topLeft = m_card->mapTo(this, QPoint(0, 0));
        const QRect cardRect(topLeft, m_card->size());
        if (!cardRect.contains(event->pos())) {
            reject();
            return;
        }
    }
    QDialog::mousePressEvent(event);
}

void OverlayDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}

namespace Dialogs {

bool confirm(QWidget *parent, const QString &title, const QString &text,
             const QString &okText, DialogTone tone, AppIcon icon)
{
    OverlayDialog dialog(parent);
    auto *layout = new QVBoxLayout(dialog.card());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(dialog.buildHeader(icon, title, tone));

    auto *body = new QLabel(text, dialog.card());
    body->setObjectName(QStringLiteral("modalBody"));
    body->setWordWrap(true);
    body->setFont(Theme::ui(13));
    body->setContentsMargins(20, 12, 20, 22);
    body->setMinimumHeight(70);
    layout->addWidget(body);

    auto *footer = new QWidget(dialog.card());
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 20);
    footerLayout->setSpacing(10);

    auto *cancel = makeFooterButton(QStringLiteral("返回"), QStringLiteral("btnGhost"), footer);
    const QString okStyle = tone == DialogTone::Bad ? QStringLiteral("btnRed")
                          : tone == DialogTone::Warn ? QStringLiteral("btnAmber")
                                                     : QStringLiteral("btnBrand");
    auto *ok = makeFooterButton(okText, okStyle, footer);
    ok->setDefault(true);

    footerLayout->addWidget(cancel, 1);
    footerLayout->addWidget(ok, 1);
    layout->addWidget(footer);

    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    return dialog.exec() == QDialog::Accepted;
}

bool editTiming(QWidget *parent, int seconds, int *result)
{
    OverlayDialog dialog(parent);
    auto *layout = new QVBoxLayout(dialog.card());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(dialog.buildHeader(AppIcon::Clock, QStringLiteral("设置计时"),
                                         DialogTone::Info));

    auto *body = new QWidget(dialog.card());
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(20, 12, 20, 22);
    bodyLayout->setSpacing(0);

    auto *preview = new QLabel(body);
    preview->setObjectName(QStringLiteral("timePreview"));
    preview->setAlignment(Qt::AlignCenter);
    preview->setFont(Theme::num(42, QFont::Bold));
    bodyLayout->addWidget(preview);
    bodyLayout->addSpacing(18);

    auto *minimum = new QLineEdit(body);
    minimum->setObjectName(QStringLiteral("tpNum"));
    minimum->setFixedSize(108, 60);
    minimum->setAlignment(Qt::AlignCenter);
    minimum->setFont(Theme::num(28, QFont::Bold));
    minimum->setValidator(new QIntValidator(0, 599, minimum));
    minimum->setFrame(false);
    minimum->setProperty("maxValue", 599);

    auto *secondsEdit = new QLineEdit(body);
    secondsEdit->setObjectName(QStringLiteral("tpNum"));
    secondsEdit->setFixedSize(108, 60);
    secondsEdit->setAlignment(Qt::AlignCenter);
    secondsEdit->setFont(Theme::num(28, QFont::Bold));
    secondsEdit->setValidator(new QIntValidator(0, 59, secondsEdit));
    secondsEdit->setFrame(false);
    secondsEdit->setProperty("maxValue", 59);

    auto makeColumn = [&](QLineEdit *edit, const QString &unit) {
        auto *column = new QWidget(body);
        auto *columnLayout = new QVBoxLayout(column);
        columnLayout->setContentsMargins(0, 0, 0, 0);
        columnLayout->setSpacing(8);

        auto *plus = new QPushButton(QStringLiteral("+"), column);
        plus->setObjectName(QStringLiteral("tpStep"));
        plus->setFixedSize(108, 32);
        plus->setCursor(Qt::PointingHandCursor);

        auto *unitLabel = new QLabel(unit, column);
        unitLabel->setObjectName(QStringLiteral("tpUnit"));
        unitLabel->setAlignment(Qt::AlignCenter);

        auto *minus = new QPushButton(QStringLiteral("−"), column);
        minus->setObjectName(QStringLiteral("tpStep"));
        minus->setFixedSize(108, 32);
        minus->setCursor(Qt::PointingHandCursor);

        columnLayout->addWidget(plus, 0, Qt::AlignHCenter);
        columnLayout->addWidget(edit, 0, Qt::AlignHCenter);
        columnLayout->addWidget(unitLabel, 0, Qt::AlignHCenter);
        columnLayout->addWidget(minus, 0, Qt::AlignHCenter);

        QObject::connect(plus, &QPushButton::clicked, edit, [edit] {
            const int maxValue = edit->property("maxValue").toInt();
            const int value = edit->text().toInt();
            edit->setText(QString::number(qMin(maxValue, value + 1)));
        });
        QObject::connect(minus, &QPushButton::clicked, edit, [edit] {
            const int value = edit->text().toInt();
            edit->setText(QString::number(qMax(0, value - 1)));
        });
        return column;
    };

    auto *picker = new QWidget(body);
    auto *pickerLayout = new QHBoxLayout(picker);
    pickerLayout->setContentsMargins(0, 0, 0, 0);
    pickerLayout->setSpacing(16);
    pickerLayout->addStretch(1);
    pickerLayout->addWidget(makeColumn(minimum, QStringLiteral("分")), 0, Qt::AlignTop);

    auto *colon = new QLabel(QStringLiteral(":"), body);
    colon->setFont(Theme::num(28, QFont::Bold));
    colon->setStyleSheet(QStringLiteral("color:#7C90A2;"));
    colon->setAlignment(Qt::AlignCenter);

    auto *colonWrap = new QWidget(body);
    auto *colonLayout = new QVBoxLayout(colonWrap);
    colonLayout->setContentsMargins(0, 0, 0, 0);
    colonLayout->setSpacing(0);
    colonLayout->addSpacing(56);
    colonLayout->addWidget(colon, 0, Qt::AlignHCenter);
    colonLayout->addStretch(1);
    pickerLayout->addWidget(colonWrap, 0, Qt::AlignTop);

    pickerLayout->addWidget(makeColumn(secondsEdit, QStringLiteral("秒")), 0, Qt::AlignTop);
    pickerLayout->addStretch(1);
    bodyLayout->addWidget(picker);
    bodyLayout->addSpacing(18);

    auto *presets = new QWidget(body);
    auto *presetLayout = new QHBoxLayout(presets);
    presetLayout->setContentsMargins(0, 0, 0, 0);
    presetLayout->setSpacing(8);
    presetLayout->addStretch(1);
    const QList<QPair<QString, int>> presetValues = {
        {QStringLiteral("1 分"), 60}, {QStringLiteral("3 分"), 180},
        {QStringLiteral("5 分"), 300}, {QStringLiteral("10 分"), 600}};
    for (const auto &preset : presetValues) {
        auto *button = new QPushButton(preset.first, presets);
        button->setObjectName(QStringLiteral("tpPreset"));
        button->setFixedHeight(28);
        button->setCursor(Qt::PointingHandCursor);
        const int total = preset.second;
        QObject::connect(button, &QPushButton::clicked, body, [minimum, secondsEdit, total] {
            minimum->setText(QString::number(total / 60));
            secondsEdit->setText(QString::number(total % 60));
        });
        presetLayout->addWidget(button);
    }
    presetLayout->addStretch(1);
    bodyLayout->addWidget(presets);

    layout->addWidget(body);

    auto *footer = new QWidget(dialog.card());
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 20);
    footerLayout->setSpacing(10);
    auto *cancel = makeFooterButton(QStringLiteral("取消"), QStringLiteral("btnGhost"), footer);
    auto *ok = makeFooterButton(QStringLiteral("确定"), QStringLiteral("btnBrand"), footer);
    ok->setDefault(true);
    footerLayout->addWidget(cancel, 1);
    footerLayout->addWidget(ok, 1);
    layout->addWidget(footer);

    auto clampText = [](QLineEdit *edit, int maxValue) {
        bool ok = false;
        int value = edit->text().toInt(&ok);
        if (!ok)
            value = 0;
        value = qBound(0, value, maxValue);
        if (edit->text() != QString::number(value) && !edit->hasFocus())
            edit->setText(QString::number(value));
        return value;
    };

    auto refresh = [minimum, secondsEdit, preview, clampText] {
        const int m = clampText(minimum, 599);
        const int s = clampText(secondsEdit, 59);
        preview->setText(fmtTime(m * 60 + s));
    };

    QObject::connect(minimum, &QLineEdit::textChanged, body, [refresh] { refresh(); });
    QObject::connect(secondsEdit, &QLineEdit::textChanged, body, [refresh] { refresh(); });
    QObject::connect(minimum, &QLineEdit::editingFinished, body, [refresh] { refresh(); });
    QObject::connect(secondsEdit, &QLineEdit::editingFinished, body, [refresh] { refresh(); });
    QObject::connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    const int initial = qBound(0, seconds, 35999);
    minimum->setText(QString::number(initial / 60));
    secondsEdit->setText(QString::number(initial % 60));
    refresh();

    if (dialog.exec() != QDialog::Accepted)
        return false;

    const int m = clampText(minimum, 599);
    const int s = clampText(secondsEdit, 59);
    if (result)
        *result = qMin(35999, m * 60 + s);
    return true;
}

void connectDevice(QWidget *parent, PressService *service)
{
    if (!service)
        return;

    OverlayDialog dialog(parent);
    auto *layout = new QVBoxLayout(dialog.card());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    layout->addWidget(dialog.buildHeader(AppIcon::Link, QStringLiteral("设备连接"),
                                         DialogTone::Info));

    auto *body = new QWidget(dialog.card());
    auto *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(20, 14, 20, 6);
    bodyLayout->setSpacing(10);

    QSettings settings(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"));
    QString mode = settings.value(QStringLiteral("link/mode"), QStringLiteral("tcp")).toString();
    QString serialName = settings.value(QStringLiteral("link/serial"), QStringLiteral("COM4"))
                             .toString();
    QString host = settings.value(QStringLiteral("link/host"), QStringLiteral("192.168.0.100"))
                       .toString();
    int port = settings.value(QStringLiteral("link/port"), 8010).toInt();

    // 已连接时优先回显当前端点
    const QString current = service->endpoint();
    if (service->isConnected() && !current.isEmpty()) {
        const int colon = current.lastIndexOf(QLatin1Char(':'));
        if (colon > 0 && !current.startsWith(QStringLiteral("/dev/"))) {
            mode = QStringLiteral("tcp");
            host = current.left(colon);
            const int value = current.mid(colon + 1).toInt();
            if (value > 0)
                port = value;
        } else {
            mode = QStringLiteral("serial");
            serialName = current;
        }
    }

    // 串口行
    auto *serialRow = new QFrame(body);
    serialRow->setObjectName(QStringLiteral("linkOption"));
    serialRow->setAttribute(Qt::WA_StyledBackground, true);
    auto *serialLayout = new QHBoxLayout(serialRow);
    serialLayout->setContentsMargins(12, 8, 12, 8);
    serialLayout->setSpacing(12);

    auto *serialRadio = new QRadioButton(QStringLiteral("串口"), serialRow);
    serialRadio->setObjectName(QStringLiteral("linkRadio"));
    serialRadio->setFont(Theme::ui(13));
    serialRadio->setCursor(Qt::PointingHandCursor);
    serialRadio->setFixedWidth(62);

    auto *serialCombo = new PortComboBox(serialRow);
    serialCombo->setObjectName(QStringLiteral("linkCombo"));
    serialCombo->setFixedHeight(36);
    serialCombo->setCursor(Qt::PointingHandCursor);
    QStringList ports = PressService::availablePorts();
    ports.removeAll(QString());
    if (!serialName.isEmpty() && !ports.contains(serialName))
        ports.prepend(serialName);
    serialCombo->addItems(ports);
    if (!serialName.isEmpty())
        serialCombo->setCurrentText(serialName);

    serialLayout->addWidget(serialRadio);
    serialLayout->addWidget(serialCombo, 1);

    // 网络行
    auto *networkRow = new QFrame(body);
    networkRow->setObjectName(QStringLiteral("linkOption"));
    networkRow->setAttribute(Qt::WA_StyledBackground, true);
    auto *networkLayout = new QHBoxLayout(networkRow);
    networkLayout->setContentsMargins(12, 8, 12, 8);
    networkLayout->setSpacing(12);

    auto *networkRadio = new QRadioButton(QStringLiteral("网络"), networkRow);
    networkRadio->setObjectName(QStringLiteral("linkRadio"));
    networkRadio->setFont(Theme::ui(13));
    networkRadio->setCursor(Qt::PointingHandCursor);
    networkRadio->setFixedWidth(62);

    auto *ipEdit = new QLineEdit(networkRow);
    ipEdit->setObjectName(QStringLiteral("linkField"));
    ipEdit->setFixedHeight(36);
    ipEdit->setText(host);
    ipEdit->setPlaceholderText(QStringLiteral("设备 IP 地址"));

    auto *portEdit = new QLineEdit(networkRow);
    portEdit->setObjectName(QStringLiteral("linkField"));
    portEdit->setFixedHeight(36);
    portEdit->setFixedWidth(78);
    portEdit->setAlignment(Qt::AlignCenter);
    portEdit->setValidator(new QIntValidator(1, 65535, portEdit));
    portEdit->setText(QString::number(port));
    portEdit->setPlaceholderText(QStringLiteral("端口"));

    networkLayout->addWidget(networkRadio);
    networkLayout->addWidget(ipEdit, 1);
    networkLayout->addWidget(portEdit);

    // 本地模拟行
    auto *localRow = new QFrame(body);
    localRow->setObjectName(QStringLiteral("linkOption"));
    localRow->setAttribute(Qt::WA_StyledBackground, true);
    auto *localLayout = new QHBoxLayout(localRow);
    localLayout->setContentsMargins(12, 8, 12, 8);
    localLayout->setSpacing(12);

    auto *localRadio = new QRadioButton(QStringLiteral("本地模拟"), localRow);
    localRadio->setObjectName(QStringLiteral("linkRadio"));
    localRadio->setFont(Theme::ui(13));
    localRadio->setCursor(Qt::PointingHandCursor);
    localRadio->setFixedWidth(62);

    auto *localHint = new QLabel(QStringLiteral("不连接设备，使用本地模拟数据运行"), localRow);
    localHint->setFont(Theme::ui(12));
    localHint->setStyleSheet(QStringLiteral("color:#5F7488;"));

    localLayout->addWidget(localRadio);
    localLayout->addWidget(localHint, 1);

    // 三个单选按钮分处不同容器，需要按钮组保证互斥
    auto *group = new QButtonGroup(body);
    group->addButton(serialRadio);
    group->addButton(networkRadio);
    group->addButton(localRadio);
    if (mode == QStringLiteral("serial"))
        serialRadio->setChecked(true);
    else if (mode == QStringLiteral("local"))
        localRadio->setChecked(true);
    else
        networkRadio->setChecked(true);

    auto *status = new QLabel(body);
    status->setObjectName(QStringLiteral("linkStatus"));
    status->setFont(Theme::ui(12));
    status->setMinimumHeight(18);
    status->setContentsMargins(2, 0, 2, 0);

    auto setStatus = [status](const QString &text, const QColor &color, const QString &tip = QString()) {
        status->setText(text);
        status->setStyleSheet(QStringLiteral("color:%1;").arg(color.name()));
        status->setToolTip(tip.isEmpty() ? text : tip);
    };

    bodyLayout->addWidget(serialRow);
    bodyLayout->addWidget(networkRow);
    bodyLayout->addWidget(localRow);
    bodyLayout->addWidget(status);
    layout->addWidget(body);

    auto *footer = new QWidget(dialog.card());
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(20, 0, 20, 20);
    footerLayout->setSpacing(10);
    auto *exitButton = makeFooterButton(QStringLiteral("退出"), QStringLiteral("btnGhost"),
                                        footer);
    auto *disconnectButton = makeFooterButton(QStringLiteral("断开"), QStringLiteral("btnGhost"),
                                              footer);
    auto *connectButton = makeFooterButton(QStringLiteral("连接"), QStringLiteral("btnBrand"),
                                           footer);
    connectButton->setDefault(true);
    disconnectButton->setEnabled(service->isConnected() || service->isConnecting());
    if (service->isConnecting())
        disconnectButton->setText(QStringLiteral("取消"));
    footerLayout->addWidget(exitButton, 1);
    footerLayout->addWidget(disconnectButton, 1);
    footerLayout->addWidget(connectButton, 1);
    layout->addWidget(footer);

    auto applyMode = [serialRadio, networkRadio, localRadio, serialCombo, ipEdit, portEdit,
                      connectButton] {
        const bool local = localRadio->isChecked();
        const bool serial = serialRadio->isChecked();
        serialCombo->setEnabled(!local && serial);
        ipEdit->setEnabled(!local && !serial);
        portEdit->setEnabled(!local && !serial);
        if (!connectButton->isEnabled())
            return;
        connectButton->setText(local ? QStringLiteral("开始运行")
                                     : QStringLiteral("连接"));
    };
    QObject::connect(serialRadio, &QRadioButton::toggled, body, [applyMode](bool) { applyMode(); });
    QObject::connect(networkRadio, &QRadioButton::toggled, body, [applyMode](bool) { applyMode(); });
    QObject::connect(localRadio, &QRadioButton::toggled, body, [applyMode](bool) { applyMode(); });
    QObject::connect(localRadio, &QRadioButton::toggled, body,
                     [setStatus](bool checked) {
                         if (checked)
                             setStatus(QStringLiteral("本地模拟运行，不访问真实设备"),
                                       Theme::Muted);
                     });
    applyMode();

    if (service->isConnected())
        setStatus(QStringLiteral("当前已连接：%1").arg(current), Theme::Green);
    else if (service->isConnecting())
        setStatus(QStringLiteral("正在连接…"), Theme::Brand700);
    else if (localRadio->isChecked())
        setStatus(QStringLiteral("本地模拟运行，不访问真实设备"), Theme::Muted);
    else
        setStatus(QStringLiteral("选择串口或网络端点后点击连接"), Theme::Muted);

    if (service->isConnecting()) {
        connectButton->setText(QStringLiteral("连接中…"));
        connectButton->setEnabled(false);
    }

    // 退出：关闭对话框并终止整个软件
    QObject::connect(exitButton, &QPushButton::clicked, &dialog, [&dialog] {
        dialog.reject();
        QCoreApplication::quit();
    });

    // 区分“用户主动断开”与“连接失败”，两者都会触发 connectionChanged(false)
    auto disconnectRequested = std::make_shared<bool>(false);
    QObject::connect(disconnectButton, &QPushButton::clicked, &dialog,
                     [service, disconnectRequested, disconnectButton, connectButton, setStatus] {
        if (!service->isConnected() && !service->isConnecting())
            return;
        *disconnectRequested = true;
        const bool canceling = service->isConnecting();
        service->disconnectDevice();
        disconnectButton->setEnabled(false);
        connectButton->setText(QStringLiteral("连接"));
        if (canceling)
            setStatus(QStringLiteral("正在取消连接…"), Theme::Brand700);
    });

    QObject::connect(connectButton, &QPushButton::clicked, &dialog,
                     [service, &dialog, networkRadio, serialRadio, localRadio, serialCombo, ipEdit,
                      portEdit, connectButton, disconnectButton, setStatus] {
        if (service->isConnecting())
            return;

        // 本地模拟：不连接设备，直接进入主界面
        if (localRadio->isChecked()) {
            QSettings store(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"));
            store.setValue(QStringLiteral("link/mode"), QStringLiteral("local"));
            if (service->isConnected())
                service->disconnectDevice();
            dialog.accept();
            return;
        }

        const bool tcp = networkRadio->isChecked();
        QString endpoint;
        if (tcp) {
            const QString ip = ipEdit->text().trimmed();
            const int portValue = portEdit->text().toInt();
            if (ip.isEmpty()) {
                setStatus(QStringLiteral("请输入设备 IP 地址"), Theme::Red);
                ipEdit->setFocus();
                return;
            }
            if (portValue <= 0) {
                setStatus(QStringLiteral("请输入有效的端口号"), Theme::Red);
                portEdit->setFocus();
                return;
            }
            endpoint = QStringLiteral("%1:%2").arg(ip).arg(portValue);
        } else {
            endpoint = serialCombo->currentText().trimmed();
            if (endpoint.isEmpty()) {
                setStatus(QStringLiteral("未检测到可用串口"), Theme::Red);
                return;
            }
        }

        QSettings store(QStringLiteral("AutoHotPress"), QStringLiteral("HMI"));
        store.setValue(QStringLiteral("link/mode"),
                       tcp ? QStringLiteral("tcp") : QStringLiteral("serial"));
        store.setValue(QStringLiteral("link/serial"), serialCombo->currentText());
        store.setValue(QStringLiteral("link/host"), ipEdit->text().trimmed());
        store.setValue(QStringLiteral("link/port"), portEdit->text().toInt());
        store.setValue(QStringLiteral("link/endpoint"), endpoint);

        setStatus(QStringLiteral("正在连接…"), Theme::Brand700);
        connectButton->setText(QStringLiteral("连接中…"));
        connectButton->setEnabled(false);
        disconnectButton->setText(QStringLiteral("取消"));
        disconnectButton->setEnabled(true);
        service->connectDevice(endpoint, tcp);
    });

    QObject::connect(service, &PressService::connectionChanged, &dialog,
                     [service, &dialog, connectButton, disconnectButton, setStatus,
                      disconnectRequested](bool connected, const QString &) {
        if (connected) {
            dialog.accept();
            return;
        }
        if (*disconnectRequested) {
            *disconnectRequested = false;
            disconnectButton->setEnabled(false);
            disconnectButton->setText(QStringLiteral("断开"));
            connectButton->setText(QStringLiteral("连接"));
            connectButton->setEnabled(true);
            setStatus(QStringLiteral("已断开连接"), Theme::Muted);
            return;
        }
        if (service->isConnecting())
            return;
        disconnectButton->setEnabled(false);
        disconnectButton->setText(QStringLiteral("断开"));
        connectButton->setText(QStringLiteral("重试"));
        connectButton->setEnabled(true);
        setStatus(QStringLiteral("连接失败，请检查端口后重试"), Theme::Red);
    });
    QObject::connect(service, &PressService::sdkError, &dialog,
                     [service, setStatus](const QString &message) {
        if (service->isConnected())
            return;
        setStatus(QStringLiteral("连接失败，请检查端口后重试"), Theme::Red, message);
    });

    if (serialRadio->isChecked())
        serialCombo->setFocus();
    else
        ipEdit->setFocus();

    dialog.exec();
}

} // namespace Dialogs
