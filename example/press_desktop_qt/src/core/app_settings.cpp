#include "core/app_settings.h"

#include "core/app_config.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

AppSettings &AppSettings::instance()
{
    static AppSettings settings;
    return settings;
}

AppSettings::AppSettings()
{
    const QString dir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    m_path = dir + QStringLiteral("/config.ini");
    ensureDefaults();
}

int AppSettings::value(const char *key, int fallback, int minValue, int maxValue) const
{
    QSettings settings(m_path, QSettings::IniFormat);
    return qBound(minValue, settings.value(QLatin1String(key), fallback).toInt(),
                  maxValue);
}

void AppSettings::ensureDefaults()
{
    QSettings settings(m_path, QSettings::IniFormat);
    const struct {
        const char *key;
        int fallback;
    } defaults[] = {
        {"runtime/tickIntervalMs", AppConfig::TickIntervalMs},
        {"runtime/watchdogIntervalMs", AppConfig::WatchdogIntervalMs},
        {"runtime/staleTimeoutMs", AppConfig::StaleTimeoutMs},
        {"runtime/reconnectTriggerMs", AppConfig::ReconnectTriggerMs},
        {"runtime/commandTimeoutMs", AppConfig::CommandTimeoutMs},
        {"runtime/writeDebounceMs", AppConfig::WriteDebounceMs},
        {"logs/maxEntries", AppConfig::LogMaxEntries},
        {"logs/mergeWindowMs", AppConfig::LogMergeWindowMs},
        {"logs/maxFileBytes", AppConfig::LogMaxFileBytes},
        {"logs/retentionDays", AppConfig::LogRetentionDays},
    };

    bool changed = false;
    for (const auto &item : defaults) {
        const QString key = QLatin1String(item.key);
        if (!settings.contains(key)) {
            settings.setValue(key, item.fallback);
            changed = true;
        }
    }
    if (changed)
        settings.sync();
}

int AppSettings::tickIntervalMs() const
{
    return value("runtime/tickIntervalMs", AppConfig::TickIntervalMs, 100, 5000);
}

int AppSettings::watchdogIntervalMs() const
{
    return value("runtime/watchdogIntervalMs", AppConfig::WatchdogIntervalMs, 200, 5000);
}

int AppSettings::staleTimeoutMs() const
{
    return value("runtime/staleTimeoutMs", AppConfig::StaleTimeoutMs, 1000, 60000);
}

int AppSettings::reconnectTriggerMs() const
{
    return value("runtime/reconnectTriggerMs", AppConfig::ReconnectTriggerMs,
                 1000, 300000);
}

int AppSettings::commandTimeoutMs() const
{
    return value("runtime/commandTimeoutMs", AppConfig::CommandTimeoutMs, 500, 30000);
}

int AppSettings::writeDebounceMs() const
{
    return value("runtime/writeDebounceMs", AppConfig::WriteDebounceMs, 0, 5000);
}

int AppSettings::logMaxEntries() const
{
    return value("logs/maxEntries", AppConfig::LogMaxEntries, 100, 100000);
}

int AppSettings::logMergeWindowMs() const
{
    return value("logs/mergeWindowMs", AppConfig::LogMergeWindowMs, 0, 60000);
}

int AppSettings::logMaxFileBytes() const
{
    return value("logs/maxFileBytes", AppConfig::LogMaxFileBytes,
                 1024 * 1024, 100 * 1024 * 1024);
}

int AppSettings::logRetentionDays() const
{
    return value("logs/retentionDays", AppConfig::LogRetentionDays, 1, 365);
}
