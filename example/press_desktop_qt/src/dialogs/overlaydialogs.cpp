#include "dialogs/overlaydialogs.h"
#include "core/format.h"
#include "theme/theme.h"

#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QShowEvent>
#include <QVBoxLayout>

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

} // namespace Dialogs
