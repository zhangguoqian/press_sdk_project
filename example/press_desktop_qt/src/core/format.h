#pragma once

#include <QString>
#include <QtGlobal>

inline QString fmtNumber(double value, int decimals = 1)
{
    return QString::number(value, 'f', decimals);
}

inline QString fmtTime(int seconds)
{
    seconds = qMax(0, seconds);
    return QStringLiteral("%1:%2")
        .arg(seconds / 60, 2, 10, QLatin1Char('0'))
        .arg(seconds % 60, 2, 10, QLatin1Char('0'));
}
