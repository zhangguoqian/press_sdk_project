#pragma once

#include <QMetaType>

// 日志级别：界面按级别着色，同时用于文件落盘
enum class LogLevel
{
    Info,
    Warning,
    Error
};

Q_DECLARE_METATYPE(LogLevel)
