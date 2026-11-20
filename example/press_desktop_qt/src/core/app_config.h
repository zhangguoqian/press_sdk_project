#pragma once

/**
 * 上位机运行参数（集中管理，避免散落魔法数字）。
 */
namespace AppConfig {

constexpr int TickIntervalMs = 500;        // 界面节拍 / 本地模拟步长
constexpr int WatchdogIntervalMs = 1000;   // 通讯看门狗检查周期
constexpr int StaleTimeoutMs = 3000;       // 超过该时长未收到 RT 判为中断
constexpr int ReconnectTriggerMs = 10000;  // 中断持续该时长后触发自动重连
constexpr int CommandTimeoutMs = 3000;     // 控制指令等待设备确认的超时
constexpr int CommandCheckMs = 200;        // 指令超时检查周期
constexpr int WriteDebounceMs = 400;       // 参数写入去抖

constexpr int LogMaxEntries = 500;         // 界面日志最大条数
constexpr int LogMergeWindowMs = 5000;     // 相同日志合并窗口
constexpr int LogMaxFileBytes = 5 * 1024 * 1024; // 单个日志文件上限
constexpr int LogRetentionDays = 7;        // 日志保留天数

} // namespace AppConfig
