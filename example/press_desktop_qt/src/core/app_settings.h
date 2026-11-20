#pragma once

#include <QString>

/**
 * 运行参数配置（config.ini）。
 *
 * 位置：<AppDataLocation>/config.ini，首次启动自动写入默认值。
 * 修改后重启上位机生效；取值范围在读取时自动收敛。
 */
class AppSettings
{
public:
    static AppSettings &instance();

    int tickIntervalMs() const;
    int watchdogIntervalMs() const;
    int staleTimeoutMs() const;
    int reconnectTriggerMs() const;
    int commandTimeoutMs() const;
    int writeDebounceMs() const;

    int logMaxEntries() const;
    int logMergeWindowMs() const;
    int logMaxFileBytes() const;
    int logRetentionDays() const;

    QString filePath() const { return m_path; }

private:
    AppSettings();

    int value(const char *key, int fallback, int minValue, int maxValue) const;
    void ensureDefaults();

    QString m_path;
};
