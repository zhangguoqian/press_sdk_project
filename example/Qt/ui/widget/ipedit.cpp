//
// Created by 11518 on 2026/10/7.
//

#include "ipedit.h"

#include <QRegularExpression>

IpValidator::IpValidator(QObject *parent) :
    QValidator(parent)
{
}

QValidator::State IpValidator::validate(QString &input, int &pos) const
{
    Q_UNUSED(pos)

    if (input.isEmpty()) {
        return Intermediate;
    }

    // 段数不能超过 4
    int dots = input.count('.');
    if (dots > 3) {
        return Invalid;
    }

    // 末尾如果是点号，表示用户正在继续输入下一段，按 3 段处理也是合法中间态，
    // 所以这里允许末尾点号存在。
    QString trimmed = input;
    if (trimmed.endsWith('.')) {
        trimmed.chop(1);
    }

    const QStringList parts = trimmed.split('.');
    const int count = parts.size();

    if (count < 1 || count > 4) {
        return Invalid;
    }

    // 每段必须是纯数字，且数值在 0–255 之间。
    // 前导零是允许的（如 "192.168.001.01"），最终由调用方决定是否规范化。
    static const QRegularExpression octetRe("^\\d{1,3}$");
    for (int i = 0; i < count; ++i) {
        const QString &p = parts.at(i);
        if (!octetRe.match(p).hasMatch()) {
            return Invalid;
        }
        bool ok = false;
        const int v = p.toInt(&ok);
        if (!ok || v < 0 || v > 255) {
            return Invalid;
        }
    }

    if (count == 4) {
        return Acceptable;
    }
    return Intermediate;
}

// -------------------------------------------------------------------------

IpEdit::IpEdit(QWidget *parent) :
    QLineEdit(parent)
{
    setValidator(new IpValidator(this));
    setPlaceholderText(tr("0.0.0.0"));
}

IpEdit::~IpEdit()
{
}

bool IpEdit::isValid() const
{
    QString current = text();
    int pos = cursorPosition();
    return validator()->validate(current, pos) == QValidator::Acceptable;
}

QString IpEdit::ip() const
{
    return isValid() ? text() : QString();
}

bool IpEdit::setIp(const QString &ip)
{
    QString candidate = ip.trimmed();
    int pos = 0;
    if (validator()->validate(candidate, pos) != QValidator::Acceptable) {
        return false;
    }
    setText(candidate);
    return true;
}
