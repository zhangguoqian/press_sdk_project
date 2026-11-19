#include "widgets/unitinput.h"
#include "theme/theme.h"

#include <QDoubleValidator>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QStyle>
#include <QtMath>

UnitInput::UnitInput(const QString &suffix, QWidget *parent)
    : QFrame(parent)
{
    setObjectName(QStringLiteral("unitInput"));
    setAttribute(Qt::WA_StyledBackground, true);
    setFixedHeight(34);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_edit = new QLineEdit(this);
    m_edit->setObjectName(QStringLiteral("plainEdit"));
    m_edit->setFont(Theme::num(14, QFont::DemiBold));
    m_edit->setFrame(false);
    m_edit->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_edit->installEventFilter(this);

    auto *validator = new QDoubleValidator(m_minimum, m_maximum, m_decimals, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    m_edit->setValidator(validator);

    m_suffix = new QLabel(suffix, this);
    m_suffix->setObjectName(QStringLiteral("unitSuffix"));
    m_suffix->setFont(Theme::ui(11));
    m_suffix->setAlignment(Qt::AlignCenter);
    m_suffix->setAttribute(Qt::WA_TransparentForMouseEvents, true);

    layout->addWidget(m_edit, 1);
    layout->addWidget(m_suffix);

    connect(m_edit, &QLineEdit::editingFinished, this, &UnitInput::commit);
    connect(m_edit, &QLineEdit::returnPressed, this, &UnitInput::commit);
}

void UnitInput::setRange(double minimum, double maximum, int decimals)
{
    m_minimum = minimum;
    m_maximum = maximum;
    m_decimals = decimals;
    auto *validator = new QDoubleValidator(minimum, maximum, decimals, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    validator->setLocale(QLocale::c());
    m_edit->setValidator(validator);
}

void UnitInput::setValue(double value)
{
    m_edit->setText(QString::number(value, 'f', m_decimals));
}

double UnitInput::value() const
{
    bool ok = false;
    const double v = m_edit->text().toDouble(&ok);
    return ok ? v : m_minimum;
}

void UnitInput::commit()
{
    bool ok = false;
    double v = m_edit->text().toDouble(&ok);
    if (!ok)
        v = m_minimum;
    v = qBound(m_minimum, v, m_maximum);
    v = qRound(v * qPow(10.0, m_decimals)) / qPow(10.0, m_decimals);
    m_edit->setText(QString::number(v, 'f', m_decimals));
    emit valueChanged(v);
}

bool UnitInput::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_edit) {
        if (event->type() == QEvent::FocusIn)
            applyFocusState(true);
        else if (event->type() == QEvent::FocusOut)
            applyFocusState(false);
    }
    return QFrame::eventFilter(watched, event);
}

void UnitInput::applyFocusState(bool focused)
{
    setProperty("focused", focused);
    style()->unpolish(this);
    style()->polish(this);
}
