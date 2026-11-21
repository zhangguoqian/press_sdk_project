#pragma once

#include <QString>
#include <QtGlobal>

// 计时（保压/保温）允许的最大秒数，设备字段与界面输入统一受此约束
constexpr int MaxHoldSeconds = 99999999;

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
