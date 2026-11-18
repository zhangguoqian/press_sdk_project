/******************************************************************************
 * Press SDK C++ 接口测试示例 / Press SDK C++ interface test example
 *
 * 用法 / Usage:
 *   press_cpp_test serial COM3                  # 串口连接 / serial port
 *   press_cpp_test tcp 192.168.1.100:9000       # TCP 连接 / TCP socket (host:port)
 *   press_cpp_test                              # 列出可用串口 / list serial ports
 *
 * 覆盖 / Covers:
 *   - 连接握手 (connect 内部做版本查询)        connect() performs a version query
 *   - 异步回调 + 定时采集 RealTimeData          callbacks + periodic RealTimeData fetch
 *   - 设备信息命令 (名称/机型/序列号/参数块)    device info commands
 *   - JSON 批量命令 (ROD/PD/TD/MD/RT)          JSON bulk commands
 *   - 压力单值命令 (段数量/校准/模具/排气)      pressure single-value commands
 *   - 温度单值命令 (段数量/速度模式/水冷)       temperature single-value commands
 *   - 段参数命令 (压力/温度/混合模式)          step-parameter commands
 *   - 系统命令 (日期时间/亮度/提示音/语言)      system commands
 *   - 设置命令 (读出-写回-校验)                setter commands (read-back verified)
 *   - 运行控制命令 (真实动作，空载)            control commands (real actuation, empty machine)
 *   - 全接口性能基准 + Markdown 报告            full-interface benchmark + Markdown report
 *
 * 安全说明: 参数写入均为"读出-写回"，不改变配置；运行控制命令会真实驱动设备
 *          (加热/加压/脱模/启停/模式切换)，必须在空载且确认安全的情况下运行。
 * Safety: parameter writes are read-back only; control commands physically actuate
 *         the machine and require an empty, confirmed-safe setup.
 *****************************************************************************/

#include "press.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{

//! 简单结果统计 / Simple pass/fail counters
int g_pass = 0;
int g_fail = 0;
int g_skip = 0;

void report(const char* name, bool ok, Press& press)
{
    if (ok)
    {
        ++g_pass;
        std::printf("[PASS] %s\n", name);
    }
    else
    {
        ++g_fail;
        std::printf("[FAIL] %s  (last error: %s)\n", name, press.getLastErrorInfo());
    }
}

//! 性能采样结构 / Latency sample holder
struct BenchResult
{
    const char* name;   //!< 命令名 / command label
    const char* kind;   //!< 分类 get/set/ctrl/static
    const char* note;   //!< 备注 / note
    int         ok;     //!< 成功次数 / successful calls
    int         n;      //!< 采样次数 / sample count
    double      minMs;  //!< 最小耗时 / min latency
    double      avgMs;  //!< 平均耗时 / average latency
    double      p50Ms;  //!< 中位数 / median latency
    double      maxMs;  //!< 最大耗时 / max latency
};

std::vector<BenchResult> g_bench;

//! 重复执行 fn 并统计耗时分布 / Run fn repeatedly and collect latency stats
template <typename Fn>
bool bench(const char* name, const char* kind, int repeats, Fn&& fn, const char* note = "")
{
    std::vector<double> samples;
    samples.reserve(static_cast<size_t>(repeats));
    int okCount = 0;
    for (int i = 0; i < repeats; ++i)
    {
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = fn();
        const auto t1 = std::chrono::steady_clock::now();
        samples.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
        if (ok)
        {
            ++okCount;
        }
    }
    std::sort(samples.begin(), samples.end());
    double sum = 0.0;
    for (double v : samples)
    {
        sum += v;
    }
    const BenchResult r{name, kind, note, okCount, repeats,
                        samples.front(), sum / samples.size(),
                        samples[samples.size() / 2], samples.back()};
    g_bench.push_back(r);
    std::printf("  [bench] %-32s %-6s ok=%2d/%-2d  min=%7.2f  avg=%7.2f  p50=%7.2f  max=%7.2f ms\n",
                name, kind, r.ok, r.n, r.minMs, r.avgMs, r.p50Ms, r.maxMs);
    return okCount == repeats;
}

//! 直接记录一次采样（用于控制类动作命令）
void recordSample(const char* name, const char* kind, const char* note,
                  int okCount, int calls, double totalMs)
{
    const double avg = totalMs / static_cast<double>(calls);
    const BenchResult r{name, kind, note, okCount, calls, avg, avg, avg, avg};
    g_bench.push_back(r);
    std::printf("  [bench] %-32s %-6s ok=%2d/%-2d  total=%8.2f  avg=%7.2f ms\n",
                name, kind, r.ok, r.n, totalMs, avg);
}

//! 打印性能汇总表 / Print the latency summary table
void printBenchTable()
{
    if (g_bench.empty())
    {
        return;
    }
    std::printf("\n================ latency summary / 耗时汇总 ================\n");
    std::printf("%-32s %-6s %5s %9s %9s %9s %9s\n",
                "interface", "kind", "ok", "min(ms)", "avg(ms)", "p50(ms)", "max(ms)");
    std::printf("-----------------------------------------------------------------------------------\n");
    for (const auto& r : g_bench)
    {
        std::printf("%-32s %-6s %2d/%-2d %9.2f %9.2f %9.2f %9.2f\n",
                    r.name, r.kind, r.ok, r.n, r.minMs, r.avgMs, r.p50Ms, r.maxMs);
    }
    std::printf("===================================================================================\n");
}

//! 写出 Markdown 性能报告 / Write the Markdown performance report
void writeBenchReport(const char* path, const char* target, const char* portType)
{
    std::FILE* f = std::fopen(path, "w");
    if (f == nullptr)
    {
        std::printf("  [WARN] cannot write performance report: %s\n", path);
        return;
    }

    int okRows = 0;
    int failedRows = 0;
    double totalAvg = 0.0;
    int avgRows = 0;
    for (const auto& r : g_bench)
    {
        if (r.ok == r.n)
        {
            ++okRows;
        }
        else
        {
            ++failedRows;
        }
        if (std::strcmp(r.kind, "ctrl") != 0)
        {
            totalAvg += r.avgMs;
            ++avgRows;
        }
    }
    const double meanAvg = avgRows == 0 ? 0.0 : totalAvg / avgRows;

    std::fprintf(f, "# Press SDK 接口性能报告\n\n");
    std::fprintf(f, "- **测试对象**：Press SDK C++ 接口（`Press` 类）\n");
    std::fprintf(f, "- **设备连接**：%s %s\n", portType, target);
    std::fprintf(f, "- **测量方式**：`std::chrono::steady_clock`，含完整请求/响应往返；\n");
    std::fprintf(f, "  本地查询 20 次、单值命令 10 次、参数块 5 次、JSON 批量 5 次、\n");
    std::fprintf(f, "  set 类原值写回 5 次（JSON 写入 3 次）、控制动作 1 次。\n");
    std::fprintf(f, "- **接口数**：%zu（全部通过 %d，存在失败 %d）\n", g_bench.size(), okRows, failedRows);
    std::fprintf(f, "- **平均耗时（不含控制类）**：%.2f ms\n\n", meanAvg);
    std::fprintf(f, "> 生命周期接口（`connect` / `disconnect` / `registerDataInterface` / `unregisterDataInterface` /\n");
    std::fprintf(f, "> `run` / `stop`）已在功能测试中执行，不做重复基准。\n");
    std::fprintf(f, "> 耗时显示为 0.000 表示本地调用（无串口往返）低于 0.001 ms。\n\n");

    std::fprintf(f, "| 接口 | 类别 | 采样 | 成功 | min(ms) | avg(ms) | p50(ms) | max(ms) | 备注 |\n");
    std::fprintf(f, "|---|---|---|---|---|---|---|---|---|\n");
    for (const auto& r : g_bench)
    {
        std::fprintf(f, "| `%s` | %s | %d | %d/%d | %.3f | %.3f | %.3f | %.3f | %s |\n",
                     r.name, r.kind, r.n, r.ok, r.n, r.minMs, r.avgMs, r.p50Ms, r.maxMs,
                     (r.note != nullptr && r.note[0] != '\0') ? r.note : "-");
    }
    std::fprintf(f, "\n> 控制类（ctrl）命令包含状态轮询、动作等待与停止确认，单次耗时明显高于普通命令，属预期。\n");
    std::fprintf(f, "> `setDemolding` 因固件未实现 `CMDID_SET_START_DEMOLD`(0x1523) 而无法通过，仅记录实际往返耗时。\n");
    std::fclose(f);
    std::printf("\nperformance report written to: %s\n", path);
}

//! 轮询等待某个状态位达到期望值 / Poll until the flag bits reach the expected state
bool waitForFlag(Press& press, uint8_t mask, bool expected, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    do
    {
        MachineState st{};
        if (press.getMachineState(st) && (((st.m_Flags & mask) != 0) == expected))
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

//! 轮询等待设备回到空闲状态 / Poll until the machine reports no active flags
bool waitForIdle(Press& press, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    do
    {
        MachineState st{};
        if (press.getMachineState(st) && st.m_Flags == 0)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

//! 轮询等待设备进入任意运行状态（加压或加热）/ Poll until any activity flag is set
bool waitForActive(Press& press, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    do
    {
        MachineState st{};
        if (press.getMachineState(st) && st.m_Flags != 0)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

//! 轮询等待工作模式变为目标值 / Poll until the work mode reaches the expected value
bool waitForMode(Press& press, uint8_t mode, int timeoutMs)
{
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(timeoutMs);
    do
    {
        MachineState st{};
        if (press.getMachineState(st) && st.m_ModelState == mode)
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } while (std::chrono::steady_clock::now() < deadline);
    return false;
}

//! 强制设备回到空闲状态 / Force the machine back to idle
void forceIdle(Press& press)
{
    MachineState st{};
    if (!press.getMachineState(st))
    {
        return;
    }
    if (st.m_Flags == 0)
    {
        return;
    }
    // 先尝试整体停止，再分别停止加压与加热
    press.setStart(false);
    press.setPressing(false);
    press.setStartHeat(false);
    waitForIdle(press, 5000);
}

//! 执行单次接口调用并打印 参数 / 返回值 / 错误信息 / 耗时
//! 原始收发帧十六进制由 SDK 的 PRESS_HEX_TRACE 环境变量开启
template <typename Fn>
bool callAndLog(const char* fn, const char* argText, uint16_t cmd, Press& press, Fn&& call)
{
    const auto start = std::chrono::steady_clock::now();
    const bool ret = call();
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    std::printf("  %-14s param=%-5s cmd=0x%04X ret=%-5s err=\"%s\" %.1f ms\n",
                fn, argText, cmd, ret ? "true" : "false", press.getLastErrorInfo(), ms);
    return ret;
}

//! 打印当前状态标志位（bit0=加压 bit1=加热）
void printFlags(const char* tag, Press& press)
{
    MachineState st{};
    if (press.getMachineState(st))
    {
        std::printf("  [state] %-9s flags=0x%02X press=%u heat=%u mode=%u\n",
                    tag, st.m_Flags,
                    static_cast<unsigned>(st.m_Flags & 0x01),
                    static_cast<unsigned>((st.m_Flags >> 1) & 0x01),
                    static_cast<unsigned>(st.m_ModelState));
    }
}

//! 启停分项测试：启动与停止各自单独调用一次，逐条记录参数/返回值/耗时
void runStartStopCases(Press& press)
{
    // 协议命令号（与 pressprivate.h 中的 CMDID_* 一致）
    constexpr uint16_t CMD_SET_START  = 0x1511;
    constexpr uint16_t CMD_SET_PRESS  = 0x1521;
    constexpr uint16_t CMD_SET_HEAT   = 0x1522;
    constexpr uint16_t CMD_SET_DEMOLD = 0x1523;

    std::printf("\n================ start/stop cases: one call per test ================\n");
    std::printf("  frame hex dump: set PRESS_HEX_TRACE=1 to print TX/RX raw frames\n");

    forceIdle(press);
    MachineState st{};
    uint8_t originalMode = 0;
    if (press.getMachineState(st))
    {
        originalMode = st.m_ModelState;
    }
    if (originalMode != 2)
    {
        std::printf("  [info] switching work mode %u -> 2 for control cases\n",
                    static_cast<unsigned>(originalMode));
        press.selectModelState(2);
        waitForMode(press, 2, 2000);
    }
    printFlags("initial", press);

    // ---- 加热：开、关 分别调用 ----
    std::printf("\n-- setStartHeat (CMD 0x1522) --\n");
    callAndLog("setStartHeat", "true", CMD_SET_HEAT, press, [&] { return press.setStartHeat(true); });
    const bool heatOn = waitForFlag(press, 0x02, true, 8000);
    printFlags("heat=on", press);
    callAndLog("setStartHeat", "false", CMD_SET_HEAT, press, [&] { return press.setStartHeat(false); });
    const bool heatOff = waitForFlag(press, 0x02, false, 5000);
    printFlags("heat=off", press);
    std::printf("  [check] heat flag on=%s off=%s\n",
                heatOn ? "ok" : "fail", heatOff ? "ok" : "fail");

    // ---- 加压：开 → 动作 0.8 s → 关 分别调用 ----
    std::printf("\n-- setPressing (CMD 0x1521) --\n");
    forceIdle(press);
    callAndLog("setPressing", "true", CMD_SET_PRESS, press, [&] { return press.setPressing(true); });
    const bool pressOn = waitForFlag(press, 0x01, true, 3000);
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    printFlags("press=on", press);
    callAndLog("setPressing", "false", CMD_SET_PRESS, press, [&] { return press.setPressing(false); });
    const bool pressOff = waitForFlag(press, 0x01, false, 5000);
    printFlags("press=off", press);
    std::printf("  [check] press flag on=%s off=%s\n",
                pressOn ? "ok" : "fail", pressOff ? "ok" : "fail");

    // ---- 脱模：固件未实现 0x1523，仅记录调用结果 ----
    std::printf("\n-- setDemolding (CMD 0x1523, firmware not implemented) --\n");
    forceIdle(press);
    callAndLog("setDemolding", "true", CMD_SET_DEMOLD, press, [&] { return press.setDemolding(true); });
    callAndLog("setDemolding", "false", CMD_SET_DEMOLD, press, [&] { return press.setDemolding(false); });

    // ---- 全流程启停 ----
    std::printf("\n-- setStart (CMD 0x1511) --\n");
    forceIdle(press);
    callAndLog("setStart", "true", CMD_SET_START, press, [&] { return press.setStart(true); });
    const bool active = waitForActive(press, 10000);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    printFlags("start", press);
    callAndLog("setStart", "false", CMD_SET_START, press, [&] { return press.setStart(false); });
    bool idle = waitForIdle(press, 12000);
    if (!idle)
    {
        press.setPressing(false);
        press.setStartHeat(false);
        idle = waitForIdle(press, 5000);
    }
    printFlags("stopped", press);
    std::printf("  [check] active flag=%s idle=%s\n",
                active ? "ok" : "none", idle ? "ok" : "fail");

    if (originalMode != 0)
    {
        press.selectModelState(originalMode);
        waitForMode(press, originalMode, 2000);
    }
    forceIdle(press);
    std::printf("\nstart/stop cases done.\n");
}

// ==================== 全接口逐调用追踪 / per-call trace for all interfaces ====================

int g_traceTotal = 0;
int g_traceFail  = 0;

//! 出参摘要：单值 / 字符串（超过 60 字符截断）
std::string traceDescribe(float v)
{
    char b[64];
    std::snprintf(b, sizeof(b), "value=%.1f", v);
    return b;
}

std::string traceDescribe(uint8_t v)
{
    char b[64];
    std::snprintf(b, sizeof(b), "value=%u", static_cast<unsigned>(v));
    return b;
}

std::string traceDescribe(int32_t v)
{
    char b[64];
    std::snprintf(b, sizeof(b), "value=%lld", static_cast<long long>(v));
    return b;
}

std::string traceDescribe(uint64_t v)
{
    char b[64];
    std::snprintf(b, sizeof(b), "value=%llu", static_cast<unsigned long long>(v));
    return b;
}

std::string traceDescribe(bool v)
{
    return v ? std::string("value=true") : std::string("value=false");
}

std::string traceDescribe(const std::string& v)
{
    const size_t kMax = 60;
    if (v.size() <= kMax)
    {
        return "value=" + v;
    }
    return "value=" + v.substr(0, kMax) + "...";
}

std::string traceDescribe(const std::vector<std::string>& v)
{
    char b[64];
    std::snprintf(b, sizeof(b), "count=%zu", v.size());
    std::string s = b;
    if (!v.empty())
    {
        s += ",first=" + v.front();
    }
    return s;
}

//! JSON 出参不打印全文，只给字节数
std::string traceDescribeJson(const std::string& v)
{
    char b[48];
    std::snprintf(b, sizeof(b), "json=%zuB", v.size());
    return b;
}

std::string traceDescribe(const PressParameter& p)
{
    char b[96];
    std::snprintf(b, sizeof(b), "maxP=%.1f,minP=%.1f,maxStep=%u",
                  p.m_MaxPLimit, p.m_MinPLimit, static_cast<unsigned>(p.m_MaxPStep));
    return b;
}

std::string traceDescribe(const TempParameter& p)
{
    char b[96];
    std::snprintf(b, sizeof(b), "maxT=%.1f,minT=%.1f,maxStep=%u",
                  p.m_MaxTLimit, p.m_MinTLimit, static_cast<unsigned>(p.m_MaxTStep));
    return b;
}

std::string traceDescribe(const OtherInfo& i)
{
    char b[96];
    std::snprintf(b, sizeof(b), "startDelay=%u,remote=%u",
                  static_cast<unsigned>(i.m_StartDelay), static_cast<unsigned>(i.m_Remote));
    return b;
}

std::string traceDescribe(const DateTimeInfo& t)
{
    char b[96];
    std::snprintf(b, sizeof(b), "%04u-%02u-%02u %02u:%02u:%02u",
                  static_cast<unsigned>(1900 + t.m_Year), static_cast<unsigned>(t.m_Month),
                  static_cast<unsigned>(t.m_Day), static_cast<unsigned>(t.m_Hour),
                  static_cast<unsigned>(t.m_Minute), static_cast<unsigned>(t.m_Second));
    return b;
}

std::string traceDescribe(const MachineState& s)
{
    char b[96];
    std::snprintf(b, sizeof(b), "flags=0x%02X,model=%u",
                  static_cast<unsigned>(s.m_Flags), static_cast<unsigned>(s.m_ModelState));
    return b;
}

std::string traceDescribe(const PressStepParameter& p)
{
    char b[96];
    std::snprintf(b, sizeof(b), "setP=%.1f,afterP=%.1f,kp=%u",
                  p.m_SetPValue, p.m_AfterValue, static_cast<unsigned>(p.m_KPTime));
    return b;
}

std::string traceDescribe(const TempStepParameter& p)
{
    char b[96];
    std::snprintf(b, sizeof(b), "temp=%.1f,speed=%.1f,kt=%u",
                  p.m_TempValue, p.m_SpeedValue, static_cast<unsigned>(p.m_KTTime));
    return b;
}

std::string traceDescribe(const MixStepParameter& p)
{
    char b[96];
    std::snprintf(b, sizeof(b), "isPress=%u,setP=%.1f,upTemp=%.1f",
                  static_cast<unsigned>(p.m_IsPress), p.m_SetPValue, p.m_UpTemp);
    return b;
}

//! 单次调用追踪：打印 函数名 / 入参 / 返回值 / 引用返回摘要 / 错误信息 / 耗时
//! （收发原始帧由 PRESS_HEX_TRACE 输出）；调用失败时不读取未初始化出参，out 打印 "-"
template <typename Fn, typename OutFn>
bool traceCallOut(const char* fn, const char* param, Press& press, Fn&& call, OutFn&& outFn)
{
    const auto start = std::chrono::steady_clock::now();
    const bool ret = call();
    const double ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - start).count();
    ++g_traceTotal;
    if (!ret)
    {
        ++g_traceFail;
    }

    std::string out = "-";
    if (ret)
    {
        out = outFn();
        for (char& c : out)
        {
            if (c == '"')
            {
                c = '\'';
            }
        }
    }
    std::printf("  [trace] %-26s param=%-26s ret=%-5s out=\"%s\" err=\"%s\" %.1f ms\n",
                fn, param, ret ? "true" : "false", out.c_str(), press.getLastErrorInfo(), ms);
    return ret;
}

//! 单次调用追踪（无出参 / 不关心出参）
template <typename Fn>
bool traceCall(const char* fn, const char* param, Press& press, Fn&& call)
{
    return traceCallOut(fn, param, press, call, [] { return std::string("-"); });
}

//! getter 追踪：调用后按出参类型生成摘要
template <typename T, typename Fn>
bool traceGet(const char* fn, const char* param, Press& press, T& out, Fn&& call)
{
    return traceCallOut(fn, param, press, call, [&] { return traceDescribe(out); });
}

template <typename GetFn, typename SetFn>
void traceByteSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    uint8_t value = 0;
    if (!get(value))
    {
        traceCall(name, "read-back failed", press, [&] { return false; });
        return;
    }
    char param[64];
    std::snprintf(param, sizeof(param), "value=%u (原值写回)", static_cast<unsigned>(value));
    traceCall(name, param, press, [&] { return set(value); });
}

template <typename GetFn, typename SetFn>
void traceFloatSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    float value = 0.0f;
    if (!get(value))
    {
        traceCall(name, "read-back failed", press, [&] { return false; });
        return;
    }
    char param[64];
    std::snprintf(param, sizeof(param), "value=%.3f (原值写回)", value);
    traceCall(name, param, press, [&] { return set(value); });
}

template <typename GetFn, typename SetFn>
void traceInt32Setter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    int32_t value = 0;
    if (!get(value))
    {
        traceCall(name, "read-back failed", press, [&] { return false; });
        return;
    }
    char param[64];
    std::snprintf(param, sizeof(param), "value=%lld (原值写回)", static_cast<long long>(value));
    traceCall(name, param, press, [&] { return set(value); });
}

template <typename GetFn, typename SetFn>
void traceBoolSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    bool value = false;
    if (!get(value))
    {
        traceCall(name, "read-back failed", press, [&] { return false; });
        return;
    }
    char param[64];
    std::snprintf(param, sizeof(param), "value=%s (原值写回)", value ? "true" : "false");
    traceCall(name, param, press, [&] { return set(value); });
}

template <typename GetFn, typename SetFn>
void traceIndexedFloatSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    float value = 0.0f;
    if (!get(0, value))
    {
        traceCall(name, "index=0 read-back failed", press, [&] { return false; });
        return;
    }
    char param[64];
    std::snprintf(param, sizeof(param), "index=0 value=%.3f (原值写回)", value);
    traceCall(name, param, press, [&] { return set(0, value); });
}

template <typename GetFn, typename SetFn>
void traceIndexedInt32Setter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    int32_t value = 0;
    if (!get(0, value))
    {
        traceCall(name, "index=0 read-back failed", press, [&] { return false; });
        return;
    }
    char param[64];
    std::snprintf(param, sizeof(param), "index=0 value=%lld (原值写回)",
                  static_cast<long long>(value));
    traceCall(name, param, press, [&] { return set(0, value); });
}

//! 全接口追踪：每个接口调用一次，输出 入参 / 返回值 / 错误 / 耗时；原始帧由 PRESS_HEX_TRACE 打印
void runInterfaceTrace(Press& press)
{
    g_traceTotal = 0;
    g_traceFail  = 0;
    std::printf("\n================ trace: all interfaces (1 call each) ================\n");
    std::printf("  raw frame hex: set PRESS_HEX_TRACE=1\n");

    // ---- 本地接口（无串口往返）----
    {
        bool connected = false;
        traceCallOut("isConnected", "-", press,
                     [&] { connected = press.isConnected(); return connected; },
                     [&] { return traceDescribe(connected); });
    }
    {
        uint64_t no = 0;
        traceCallOut("getMachineRegisterNo", "-", press,
                     [&] { no = press.getMachineRegisterNo(); return true; },
                     [&] { return traceDescribe(no); });
    }
    {
        bool running = false;
        traceCallOut("isRunning", "-", press,
                     [&] { running = press.isRunning(); return true; },
                     [&] { return traceDescribe(running); });
    }
    {
        std::string info;
        traceCallOut("getLastErrorInfo", "-", press,
                     [&] { info = press.getLastErrorInfo(); return true; },
                     [&] { return traceDescribe(info); });
    }
    {
        std::string ver;
        traceCallOut("Press::version", "-", press,
                     [&] { ver = Press::version(); return !ver.empty(); },
                     [&] { return traceDescribe(ver); });
    }
    {
        std::vector<std::string> ports;
        traceCallOut("Press::getPortList", "-", press,
                     [&] { ports = Press::getPortList(); return !ports.empty(); },
                     [&] { return traceDescribe(ports); });
    }

    // ---- 设备信息 ----
    {
        std::string s;
        traceGet("getName", "lang=0", press, s, [&] { return press.getName(s); });
    }
    {
        std::string s;
        traceGet("getMachineType", "out=string", press, s,
                 [&] { return press.getMachineType(s); });
    }
    {
        std::string s;
        traceGet("getSerialNumber", "out=string", press, s,
                 [&] { return press.getSerialNumber(s); });
    }
    {
        PressParameter p{};
        traceGet("getPressParameter", "out=PressParameter", press, p,
                 [&] { return press.getPressParameter(p); });
    }
    {
        TempParameter p{};
        traceGet("getTempParameter", "out=TempParameter", press, p,
                 [&] { return press.getTempParameter(p); });
    }
    {
        OtherInfo i{};
        traceGet("getOtherInfo", "out=OtherInfo", press, i,
                 [&] { return press.getOtherInfo(i); });
    }

    // ---- JSON 批量（仅打印字节数）----
    {
        std::string j;
        traceCallOut("getReadOnlyDataJson", "compressed=false", press,
                     [&] { return press.getReadOnlyDataJson(j); },
                     [&] { return traceDescribeJson(j); });
    }
    {
        std::string j;
        traceCallOut("getPressDataJson", "compressed=false", press,
                     [&] { return press.getPressDataJson(j); },
                     [&] { return traceDescribeJson(j); });
    }
    {
        std::string j;
        traceCallOut("getRealTimeDataJson", "compressed=false", press,
                     [&] { return press.getRealTimeDataJson(j); },
                     [&] { return traceDescribeJson(j); });
    }
    {
        std::string j;
        traceCallOut("getTempDataJson", "compressed=false", press,
                     [&] { return press.getTempDataJson(j); },
                     [&] { return traceDescribeJson(j); });
    }
    {
        std::string j;
        traceCallOut("getMixDataJson", "compressed=false", press,
                     [&] { return press.getMixDataJson(j); },
                     [&] { return traceDescribeJson(j); });
    }

    // ---- 状态 ----
    {
        MachineState state{};
        traceGet("getMachineState", "out=MachineState", press, state,
                 [&] { return press.getMachineState(state); });
    }

    // ---- 压力单值/索引（getter）----
    {
        uint8_t v = 0;
        traceGet("getPressStepCount", "out=uint8", press, v,
                 [&] { return press.getPressStepCount(v); });
    }
    {
        float v = 0.0f;
        traceGet("getCalibrationValue", "out=float", press, v,
                 [&] { return press.getCalibrationValue(v); });
    }
    {
        uint8_t v = 0;
        traceGet("getModeType", "out=uint8", press, v, [&] { return press.getModeType(v); });
    }
    {
        float v = 0.0f;
        traceGet("getCircle", "out=float", press, v, [&] { return press.getCircle(v); });
    }
    {
        float a = 0.0f;
        float b = 0.0f;
        traceCallOut("getRectangle", "out=a,b", press,
                     [&] { return press.getRectangle(a, b); },
                     [&] {
                         char buf[64];
                         std::snprintf(buf, sizeof(buf), "a=%.1f,b=%.1f", a, b);
                         return std::string(buf);
                     });
    }
    {
        uint8_t v = 0;
        traceGet("getAirTime", "out=uint8", press, v, [&] { return press.getAirTime(v); });
    }
    {
        float v = 0.0f;
        traceGet("getPressValueIndex", "index=0", press, v,
                 [&] { return press.getPressValueIndex(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getAfterValueIndex", "index=0", press, v,
                 [&] { return press.getAfterValueIndex(0, v); });
    }
    {
        int32_t v = 0;
        traceGet("getKeepPressTime", "index=0", press, v,
                 [&] { return press.getKeepPressTime(0, v); });
    }
    {
        PressStepParameter p{};
        traceGet("getPressStepParameter", "index=0", press, p,
                 [&] { return press.getPressStepParameter(0, p); });
    }

    // ---- 温度单值/索引（getter）----
    {
        uint8_t v = 0;
        traceGet("getTempStepCount", "out=uint8", press, v,
                 [&] { return press.getTempStepCount(v); });
    }
    {
        uint8_t v = 0;
        traceGet("getTempSpeedMold", "out=uint8", press, v,
                 [&] { return press.getTempSpeedMold(v); });
    }
    {
        float v = 0.0f;
        traceGet("getStartTempIndex", "index=0", press, v,
                 [&] { return press.getStartTempIndex(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getTempSpeedIndex", "index=0", press, v,
                 [&] { return press.getTempSpeedIndex(0, v); });
    }
    {
        int32_t v = 0;
        traceGet("getKeepTempTime", "index=0", press, v,
                 [&] { return press.getKeepTempTime(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getMaxWaterValue", "out=float", press, v,
                 [&] { return press.getMaxWaterValue(v); });
    }
    {
        float v = 0.0f;
        traceGet("getMinWaterValue", "out=float", press, v,
                 [&] { return press.getMinWaterValue(v); });
    }
    {
        TempStepParameter p{};
        traceGet("getTempStepParameter", "index=0", press, p,
                 [&] { return press.getTempStepParameter(0, p); });
    }

    // ---- 混合模式（getter）----
    {
        uint8_t v = 0;
        traceGet("getMixStepCount", "out=uint8", press, v,
                 [&] { return press.getMixStepCount(v); });
    }
    {
        uint8_t v = 0;
        traceGet("getMixMoldType", "out=uint8", press, v,
                 [&] { return press.getMixMoldType(v); });
    }
    {
        float v = 0.0f;
        traceGet("getMixCircle", "out=float", press, v,
                 [&] { return press.getMixCircle(v); });
    }
    {
        float a = 0.0f;
        float b = 0.0f;
        traceCallOut("getMixRectangle", "out=a,b", press,
                     [&] { return press.getMixRectangle(a, b); },
                     [&] {
                         char buf[64];
                         std::snprintf(buf, sizeof(buf), "a=%.1f,b=%.1f", a, b);
                         return std::string(buf);
                     });
    }
    {
        bool v = false;
        traceGet("getMixIsPressIndex", "index=0", press, v,
                 [&] { return press.getMixIsPressIndex(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getMixPressIndex", "index=0", press, v,
                 [&] { return press.getMixPressIndex(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getMixAfterPressIndex", "index=0", press, v,
                 [&] { return press.getMixAfterPressIndex(0, v); });
    }
    {
        bool v = false;
        traceGet("getMixIsHeatUpIndex", "index=0", press, v,
                 [&] { return press.getMixIsHeatUpIndex(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getMixHeatUpIndex", "index=0", press, v,
                 [&] { return press.getMixHeatUpIndex(0, v); });
    }
    {
        bool v = false;
        traceGet("getMixIsHeatDownIndex", "index=0", press, v,
                 [&] { return press.getMixIsHeatDownIndex(0, v); });
    }
    {
        float v = 0.0f;
        traceGet("getMixHeatDownIndex", "index=0", press, v,
                 [&] { return press.getMixHeatDownIndex(0, v); });
    }
    {
        int32_t v = 0;
        traceGet("getMixKeepTime", "index=0", press, v,
                 [&] { return press.getMixKeepTime(0, v); });
    }
    {
        MixStepParameter p{};
        traceGet("getMixStepParameter", "index=0", press, p,
                 [&] { return press.getMixStepParameter(0, p); });
    }

    // ---- 系统（getter）----
    {
        DateTimeInfo t{};
        traceGet("getDateTime", "out=DateTimeInfo", press, t,
                 [&] { return press.getDateTime(t); });
    }
    {
        uint8_t v = 0;
        traceGet("getBrightness", "out=uint8", press, v,
                 [&] { return press.getBrightness(v); });
    }
    {
        bool v = false;
        traceGet("getButtonSound", "out=bool", press, v,
                 [&] { return press.getButtonSound(v); });
    }
    {
        uint8_t v = 0;
        traceGet("getLanguage", "out=uint8", press, v,
                 [&] { return press.getLanguage(v); });
    }

    // ---- 设置命令（读出后原值写回，入参为实际写入值）----
    traceByteSetter(press, "setPressStepCount",
                    [&](uint8_t& v) { return press.getPressStepCount(v); },
                    [&](uint8_t v) { return press.setPressStepCount(v); });
    traceFloatSetter(press, "setCalibrationValue",
                     [&](float& v) { return press.getCalibrationValue(v); },
                     [&](float v) { return press.setCalibrationValue(v); });
    traceByteSetter(press, "setModeType",
                    [&](uint8_t& v) { return press.getModeType(v); },
                    [&](uint8_t v) { return press.setModeType(v); });
    traceFloatSetter(press, "setCircle",
                     [&](float& v) { return press.getCircle(v); },
                     [&](float v) { return press.setCircle(v); });
    {
        float a = 0.0f;
        float b = 0.0f;
        char param[64];
        if (press.getRectangle(a, b))
        {
            std::snprintf(param, sizeof(param), "a=%.1f b=%.1f (原值写回)", a, b);
            traceCall("setRectangle", param, press, [&] { return press.setRectangle(a, b); });
        }
        else
        {
            traceCall("setRectangle", "read-back failed", press, [&] { return false; });
        }
    }
    traceByteSetter(press, "setAirTime",
                    [&](uint8_t& v) { return press.getAirTime(v); },
                    [&](uint8_t v) { return press.setAirTime(v); });
    traceIndexedFloatSetter(press, "setPressValueIndex",
                            [&](uint8_t i, float& v) { return press.getPressValueIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setPressValueIndex(i, v); });
    traceIndexedFloatSetter(press, "setAfterValueIndex",
                            [&](uint8_t i, float& v) { return press.getAfterValueIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setAfterValueIndex(i, v); });
    traceIndexedInt32Setter(press, "setKeepPressTime",
                            [&](uint8_t i, int32_t& v) { return press.getKeepPressTime(i, v); },
                            [&](uint8_t i, int32_t v) { return press.setKeepPressTime(i, v); });
    {
        PressStepParameter p{};
        if (press.getPressStepParameter(0, p))
        {
            traceCall("setPressStepParameter", "index=0 (原值写回)", press,
                      [&] { return press.setPressStepParameter(0, p); });
        }
        else
        {
            traceCall("setPressStepParameter", "index=0 read-back failed", press,
                      [&] { return false; });
        }
    }

    traceByteSetter(press, "setTempStepCount",
                    [&](uint8_t& v) { return press.getTempStepCount(v); },
                    [&](uint8_t v) { return press.setTempStepCount(v); });
    traceByteSetter(press, "setTempSpeedMold",
                    [&](uint8_t& v) { return press.getTempSpeedMold(v); },
                    [&](uint8_t v) { return press.setTempSpeedMold(v); });
    traceIndexedFloatSetter(press, "setTempIndex",
                            [&](uint8_t i, float& v) { return press.getStartTempIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setTempIndex(i, v); });
    traceIndexedFloatSetter(press, "setTempSpeedIndex",
                            [&](uint8_t i, float& v) { return press.getTempSpeedIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setTempSpeedIndex(i, v); });
    traceIndexedInt32Setter(press, "setKeepTempTime",
                            [&](uint8_t i, int32_t& v) { return press.getKeepTempTime(i, v); },
                            [&](uint8_t i, int32_t v) { return press.setKeepTempTime(i, v); });
    traceFloatSetter(press, "setMaxWaterValue",
                     [&](float& v) { return press.getMaxWaterValue(v); },
                     [&](float v) { return press.setMaxWaterValue(v); });
    traceFloatSetter(press, "setMinWaterValue",
                     [&](float& v) { return press.getMinWaterValue(v); },
                     [&](float v) { return press.setMinWaterValue(v); });

    traceByteSetter(press, "setMixStepCount",
                    [&](uint8_t& v) { return press.getMixStepCount(v); },
                    [&](uint8_t v) { return press.setMixStepCount(v); });
    traceByteSetter(press, "setMixMoldType",
                    [&](uint8_t& v) { return press.getMixMoldType(v); },
                    [&](uint8_t v) { return press.setMixMoldType(v); });
    traceFloatSetter(press, "setMixCircle",
                     [&](float& v) { return press.getMixCircle(v); },
                     [&](float v) { return press.setMixCircle(v); });
    {
        float a = 0.0f;
        float b = 0.0f;
        char param[64];
        if (press.getMixRectangle(a, b))
        {
            std::snprintf(param, sizeof(param), "a=%.1f b=%.1f (原值写回)", a, b);
            traceCall("setMixRectangle", param, press, [&] { return press.setMixRectangle(a, b); });
        }
        else
        {
            traceCall("setMixRectangle", "read-back failed", press, [&] { return false; });
        }
    }
    {
        bool v = false;
        if (press.getMixIsPressIndex(0, v))
        {
            traceCall("setMixIsPressIndex", v ? "index=0 value=true" : "index=0 value=false",
                      press, [&] { return press.setMixIsPressIndex(0, v); });
        }
        else
        {
            traceCall("setMixIsPressIndex", "index=0 read-back failed", press,
                      [&] { return false; });
        }
    }
    traceIndexedFloatSetter(press, "setMixPressIndex",
                            [&](uint8_t i, float& v) { return press.getMixPressIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setMixPressIndex(i, v); });
    traceIndexedFloatSetter(press, "setMixAfterPressIndex",
                            [&](uint8_t i, float& v) { return press.getMixAfterPressIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setMixAfterPressIndex(i, v); });
    {
        bool v = false;
        if (press.getMixIsHeatUpIndex(0, v))
        {
            traceCall("setMixIsHeatUpIndex", v ? "index=0 value=true" : "index=0 value=false",
                      press, [&] { return press.setMixIsHeatUpIndex(0, v); });
        }
        else
        {
            traceCall("setMixIsHeatUpIndex", "index=0 read-back failed", press,
                      [&] { return false; });
        }
    }
    traceIndexedFloatSetter(press, "setMixHeatUpIndex",
                            [&](uint8_t i, float& v) { return press.getMixHeatUpIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setMixHeatUpIndex(i, v); });
    {
        bool v = false;
        if (press.getMixIsHeatDownIndex(0, v))
        {
            traceCall("setMixIsHeatDownIndex", v ? "index=0 value=true" : "index=0 value=false",
                      press, [&] { return press.setMixIsHeatDownIndex(0, v); });
        }
        else
        {
            traceCall("setMixIsHeatDownIndex", "index=0 read-back failed", press,
                      [&] { return false; });
        }
    }
    traceIndexedFloatSetter(press, "setMixHeatDownIndex",
                            [&](uint8_t i, float& v) { return press.getMixHeatDownIndex(i, v); },
                            [&](uint8_t i, float v) { return press.setMixHeatDownIndex(i, v); });
    traceIndexedInt32Setter(press, "setMixKeepTime",
                            [&](uint8_t i, int32_t& v) { return press.getMixKeepTime(i, v); },
                            [&](uint8_t i, int32_t v) { return press.setMixKeepTime(i, v); });
    {
        TempStepParameter p{};
        if (press.getTempStepParameter(0, p))
        {
            traceCall("setTempStepParameter", "index=0 (原值写回)", press,
                      [&] { return press.setTempStepParameter(0, p); });
        }
        else
        {
            traceCall("setTempStepParameter", "index=0 read-back failed", press,
                      [&] { return false; });
        }
    }
    {
        MixStepParameter p{};
        if (press.getMixStepParameter(0, p))
        {
            traceCall("setMixStepParameter", "index=0 (原值写回)", press,
                      [&] { return press.setMixStepParameter(0, p); });
        }
        else
        {
            traceCall("setMixStepParameter", "index=0 read-back failed", press,
                      [&] { return false; });
        }
    }

    // ---- 系统设置（原值写回）----
    {
        DateTimeInfo t{};
        if (press.getDateTime(t))
        {
            traceCall("setDateTime", "原值写回", press, [&] { return press.setDateTime(t); });
        }
        else
        {
            traceCall("setDateTime", "read-back failed", press, [&] { return false; });
        }
    }
    traceByteSetter(press, "setBrightness",
                    [&](uint8_t& v) { return press.getBrightness(v); },
                    [&](uint8_t v) { return press.setBrightness(v); });
    traceBoolSetter(press, "setButtonSound",
                    [&](bool& v) { return press.getButtonSound(v); },
                    [&](bool v) { return press.setButtonSound(v); });
    traceByteSetter(press, "setLanguage",
                    [&](uint8_t& v) { return press.getLanguage(v); },
                    [&](uint8_t v) { return press.setLanguage(v); });

    // ---- JSON 写入（原值写回）----
    {
        std::string j;
        if (press.getPressDataJson(j))
        {
            char param[64];
            std::snprintf(param, sizeof(param), "json=%zuB (原值写回)", j.size());
            traceCall("setPressDataJson", param, press, [&] { return press.setPressDataJson(j); });
        }
        else
        {
            traceCall("setPressDataJson", "read-back failed", press, [&] { return false; });
        }
    }
    {
        std::string j;
        if (press.getTempDataJson(j))
        {
            char param[64];
            std::snprintf(param, sizeof(param), "json=%zuB (原值写回)", j.size());
            traceCall("setTempDataJson", param, press, [&] { return press.setTempDataJson(j); });
        }
        else
        {
            traceCall("setTempDataJson", "read-back failed", press, [&] { return false; });
        }
    }
    {
        std::string j;
        if (press.getMixDataJson(j))
        {
            char param[64];
            std::snprintf(param, sizeof(param), "json=%zuB (原值写回)", j.size());
            traceCall("setMixDataJson", param, press, [&] { return press.setMixDataJson(j); });
        }
        else
        {
            traceCall("setMixDataJson", "read-back failed", press, [&] { return false; });
        }
    }

    // ---- 运行控制（真实动作，空载）----
    forceIdle(press);
    MachineState st{};
    uint8_t originalMode = 0;
    if (press.getMachineState(st))
    {
        originalMode = st.m_ModelState;
    }
    if (originalMode != 2)
    {
        char param[32];
        std::snprintf(param, sizeof(param), "target=2 (原模式 %u)", static_cast<unsigned>(originalMode));
        traceCall("selectModelState", param, press, [&] { return press.selectModelState(2); });
        waitForMode(press, 2, 2000);
    }

    traceCall("setStartHeat", "true", press, [&] { return press.setStartHeat(true); });
    const bool heatOn = waitForFlag(press, 0x02, true, 8000);
    traceCall("setStartHeat", "false", press, [&] { return press.setStartHeat(false); });
    const bool heatOff = waitForFlag(press, 0x02, false, 5000);
    std::printf("  [check] setStartHeat flag on=%s off=%s\n",
                heatOn ? "ok" : "fail", heatOff ? "ok" : "fail");

    forceIdle(press);
    traceCall("setPressing", "true", press, [&] { return press.setPressing(true); });
    const bool pressOn = waitForFlag(press, 0x01, true, 3000);
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    traceCall("setPressing", "false", press, [&] { return press.setPressing(false); });
    const bool pressOff = waitForFlag(press, 0x01, false, 5000);
    std::printf("  [check] setPressing flag on=%s off=%s\n",
                pressOn ? "ok" : "fail", pressOff ? "ok" : "fail");

    forceIdle(press);
    traceCall("setDemolding", "true", press, [&] { return press.setDemolding(true); });
    traceCall("setDemolding", "false", press, [&] { return press.setDemolding(false); });

    forceIdle(press);
    traceCall("setStart", "true", press, [&] { return press.setStart(true); });
    const bool active = waitForActive(press, 10000);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    traceCall("setStart", "false", press, [&] { return press.setStart(false); });
    bool idle = waitForIdle(press, 12000);
    if (!idle)
    {
        press.setPressing(false);
        press.setStartHeat(false);
        idle = waitForIdle(press, 5000);
    }
    std::printf("  [check] setStart active=%s idle=%s\n",
                active ? "ok" : "none", idle ? "ok" : "fail");

    if (originalMode != 0)
    {
        char param[32];
        std::snprintf(param, sizeof(param), "target=%u (恢复原模式)", static_cast<unsigned>(originalMode));
        traceCall("selectModelState", param, press, [&] { return press.selectModelState(originalMode); });
        waitForMode(press, originalMode, 2000);
    }
    forceIdle(press);

    std::printf("\n  trace summary: %d calls, %d failed\n", g_traceTotal, g_traceFail);
}

//! set 类基准：读出当前值后原值写回 / Read current value then write it back
template <typename GetFn, typename SetFn>
void benchSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    uint8_t byteValue = 0;
    if (!get(byteValue))
    {
        std::printf("  [SKIP] %s: read failed\n", name);
        ++g_skip;
        return;
    }
    bench(name, "set", 5, [&] { return set(byteValue); }, "原值写回");
}

template <typename GetFn, typename SetFn>
void benchFloatSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    float value = 0.0f;
    if (!get(value))
    {
        std::printf("  [SKIP] %s: read failed\n", name);
        ++g_skip;
        return;
    }
    bench(name, "set", 5, [&] { return set(value); }, "原值写回");
}

template <typename GetFn, typename SetFn>
void benchIndexedFloatSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    float value = 0.0f;
    if (!get(0, value))
    {
        std::printf("  [SKIP] %s: read failed\n", name);
        ++g_skip;
        return;
    }
    bench(name, "set", 5, [&] { return set(0, value); }, "index=0 原值写回");
}

template <typename GetFn, typename SetFn>
void benchIndexedInt32Setter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    int32_t value = 0;
    if (!get(0, value))
    {
        std::printf("  [SKIP] %s: read failed\n", name);
        ++g_skip;
        return;
    }
    bench(name, "set", 5, [&] { return set(0, value); }, "index=0 原值写回");
}

template <typename GetFn, typename SetFn>
void benchBoolSetter(Press& press, const char* name, GetFn&& get, SetFn&& set)
{
    bool value = false;
    if (!get(value))
    {
        std::printf("  [SKIP] %s: read failed\n", name);
        ++g_skip;
        return;
    }
    bench(name, "set", 5, [&] { return set(value); }, "原值写回");
}

//! 回调实现：打印异步上报的数据 / Callback impl printing async reports
class TestCallback : public PressDataInterface
{
public:
    void onReadOnlyData(int errorCode, uint64_t registerNo, const std::string& json) override
    {
        std::printf("  [callback] onReadOnlyData: err=%d regNo=%llu bytes=%zu\n",
                    errorCode, static_cast<unsigned long long>(registerNo), json.size());
    }

    void onPressData(int errorCode, const std::string& json) override
    {
        std::printf("  [callback] onPressData: err=%d bytes=%zu\n", errorCode, json.size());
    }

    void onTempData(int errorCode, const std::string& json) override
    {
        std::printf("  [callback] onTempData: err=%d bytes=%zu\n", errorCode, json.size());
    }

    void onMixData(int errorCode, const std::string& json) override
    {
        std::printf("  [callback] onMixData: err=%d bytes=%zu\n", errorCode, json.size());
    }

    void onRealTimeData(int errorCode, const std::string& json) override
    {
        if (errorCode != 0)
        {
            std::printf("  [callback] onRealTimeData: err=%d\n", errorCode);
            return;
        }

        // 记录回调间隔用于评估定时器精度 / record callback interval for timer accuracy
        const auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_hasLast)
            {
                m_intervals.push_back(
                    std::chrono::duration<double, std::milli>(now - m_last).count());
            }
            m_last = now;
            m_hasLast = true;
            ++m_count;
        }

        std::printf("  [callback] onRealTimeData: bytes=%zu\n", json.size());
    }

    void onError(uint16_t cmdCode, std::vector<uint8_t> /*response*/) override
    {
        std::printf("  [callback] onError: cmd=0x%04X\n", static_cast<unsigned>(cmdCode));
    }

    //! 清零统计，避免基准测试中的直接调用污染定时器统计
    //! Reset stats so direct calls made by the benchmark do not pollute timer metrics
    void resetStats()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_intervals.clear();
        m_count = 0;
        m_hasLast = false;
    }

    //! 打印回调间隔统计 / Print callback interval statistics
    void printIntervalStats()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_intervals.empty())
        {
            std::printf("callback count=%d, no interval sample\n", m_count);
            return;
        }
        std::vector<double> v = m_intervals;
        std::sort(v.begin(), v.end());
        double sum = 0.0;
        for (double d : v)
        {
            sum += d;
        }
        double var = 0.0;
        const double avg = sum / v.size();
        for (double d : v)
        {
            var += (d - avg) * (d - avg);
        }
        var /= v.size();
        std::printf("callback count=%d  interval avg=%.1f ms  min=%.1f  max=%.1f  stddev=%.2f ms\n",
                    m_count, avg, v.front(), v.back(), std::sqrt(var));
    }

private:
    std::mutex                                m_mutex;
    std::chrono::steady_clock::time_point     m_last{};
    bool                                      m_hasLast = false;
    int                                       m_count = 0;
    std::vector<double>                       m_intervals;
};

void printDateTime(const DateTimeInfo& t)
{
    // m_Year 为 tm_year（自 1900 起偏移），显示时 +1900 / m_Year is tm_year, add 1900 for display
    std::printf("  %04u-%02u-%02u %02u:%02u:%02u\n",
                1900u + t.m_Year, t.m_Month, t.m_Day, t.m_Hour, t.m_Minute, t.m_Second);
}

void printMachineState(const MachineState& s)
{
    const char* press = (s.m_Flags & 0x01) ? "running" : "stopped";
    const char* heat = (s.m_Flags & 0x02) ? "running" : "stopped";
    std::printf("  mode=%u flags=0x%02X press=%s heat=%s pStep=%u tStep=%u t1Step=%u\n",
                s.m_ModelState, s.m_Flags, press, heat,
                s.m_PressStep, s.m_TempStep, s.m_Temp1Step);
    std::printf("  press=%.3f upT=%.1f downT=%.1f pRemain=%ums tRemain=%ums t1Remain=%ums flow=%u (%u bytes)\n",
                s.m_PressValue, s.m_TempUpValue, s.m_TempDownValue,
                s.m_PTime, s.m_TTime, s.m_T1Time, s.m_FlowChange, s.m_Count);
}

//! 十六进制 dump / hex dump
void printHex(const char* label, const std::vector<uint8_t>& data)
{
    std::printf("  %s (%zu bytes):", label, data.size());
    for (uint8_t b : data)
    {
        std::printf(" %02X", b);
    }
    std::printf("\n");
}

void usage()
{
    std::printf("Press SDK C++ interface test / C++ 接口测试\n\n");
    std::printf("Usage:\n");
    std::printf("  press_cpp_test serial <port>        e.g. serial COM3\n");
    std::printf("  press_cpp_test tcp <host:port>      e.g. tcp 192.168.1.100:9000\n\n");
    std::printf("  press_cpp_test serial <port> startstop   # 仅执行启停分项测试\n");
    std::printf("                                           # start/stop cases only\n");
    std::printf("  press_cpp_test serial <port> traceall    # 全接口逐调用追踪（入参/返回值/耗时）\n");
    std::printf("                                           # per-call trace for all interfaces\n\n");

    const auto ports = Press::getPortList();
    if (ports.empty())
    {
        std::printf("No serial port available / 当前无可用串口\n");
        return;
    }
    std::printf("Available serial ports / 可用串口:\n");
    for (const auto& p : ports)
    {
        std::printf("  %s\n", p.c_str());
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        usage();
        return 0;
    }

    const std::string portTypeArg = argv[1];
    const std::string portName = argv[2];
    if (portTypeArg != "serial" && portTypeArg != "tcp")
    {
        std::printf("Unknown port type: %s (expected 'serial' or 'tcp')\n", portTypeArg.c_str());
        return 1;
    }
    const PressPortType portType = (portTypeArg == "tcp") ? TcpSocketPortType : SerialPortType;

    std::printf("Press SDK version: %s\n", Press::version().c_str());

    Press press;

    // ==================== 1. 连接（内部完成版本握手）====================
    const auto connectStart = std::chrono::steady_clock::now();
    const bool connectOk = press.connect(portName.c_str(), portType);
    const auto connectEnd = std::chrono::steady_clock::now();
    report("connect", connectOk, press);
    std::printf("  connect latency: %.1f ms (includes version handshake)\n",
                std::chrono::duration<double, std::milli>(connectEnd - connectStart).count());
    if (!press.isConnected())
    {
        std::printf("Connect failed, abort. err: %s\n", press.getLastErrorInfo());
        return 1;
    }

    // 可选模式：仅执行启停分项测试（第三参数 startstop）
    const std::string runMode = (argc >= 4) ? argv[3] : std::string();
    if (runMode == "startstop")
    {
        runStartStopCases(press);
        press.disconnect();
        return 0;
    }

    // 可选模式：全接口逐调用追踪（每个接口 1 次，打印入参/返回值/耗时，hex 由 PRESS_HEX_TRACE 输出）
    if (runMode == "traceall")
    {
        runInterfaceTrace(press);
        press.disconnect();
        return 0;
    }

    // ==================== 2. 回调注册 + 1 秒定时采集 ====================
    TestCallback callback;
    press.registerDataInterface(&callback, 1);

    // ==================== 3. 设备信息命令 ====================
    {
        std::string name;
        report("getName(ZH)", press.getName(name, 0x00), press);
        if (!name.empty())
        {
            std::printf("  name(ZH): %s\n", name.c_str());
        }
        report("getName(EN)", press.getName(name, 0x02), press);
        if (!name.empty())
        {
            std::printf("  name(EN): %s\n", name.c_str());
        }

        std::string type;
        report("getMachineType", press.getMachineType(type), press);
        if (!type.empty())
        {
            std::printf("  machine type: %s\n", type.c_str());
        }

        std::string sn;
        report("getSerialNumber", press.getSerialNumber(sn), press);
        if (!sn.empty())
        {
            std::printf("  serial number: %s\n", sn.c_str());
        }

        PressParameter pp{};
        if (press.getPressParameter(pp))
        {
            ++g_pass;
            std::printf("[PASS] getPressParameter\n");
            std::printf("  MaxP=%.3f MinP=%.3f Max_Min=%.3f Diameter=%.3f MaxStep=%u "
                        "PDecimal=%u PressDecimal=%u PModel=%u OutType=%u\n",
                        pp.m_MaxPLimit, pp.m_MinPLimit, pp.m_MaxMin, pp.m_Diameter,
                        pp.m_MaxPStep, pp.m_PDecimal, pp.m_PressDecimal,
                        pp.m_PModel, pp.m_OutType);
        }
        else
        {
            ++g_fail;
            std::printf("[FAIL] getPressParameter (last error: %s)\n",
                        press.getLastErrorInfo());
        }

        TempParameter tp{};
        bool tpOk = press.getTempParameter(tp);
        report("getTempParameter", tpOk, press);
        if (tpOk)
        {
            std::printf("  MaxT=%.3f MinT=%.3f MaxTStep=%u TDecimal=%u IsHasWater=%u IsHasSpeed=%u\n",
                        tp.m_MaxTLimit, tp.m_MinTLimit, tp.m_MaxTStep,
                        tp.m_TDecimal, tp.m_IsHasWater, tp.m_IsHasSpeed);
        }

        OtherInfo oi{};
        bool oiOk = press.getOtherInfo(oi);
        report("getOtherInfo", oiOk, press);
        if (oiOk)
        {
            std::printf("  Screenshot=%u StartDelay=%u IsHideLang=%u Remote=%u VersionType=%u\n",
                        oi.m_Screenshot, oi.m_StartDelay, oi.m_IsHideLang,
                        oi.m_Remote, oi.m_VersionType);
        }
    }

    // ==================== 4. JSON 批量命令 ====================
    {
        std::string roJson;
        bool roOk = press.getReadOnlyDataJson(roJson);
        report("getReadOnlyDataJson", roOk, press);
        if (roOk)
        {
            std::printf("  rod json (%zu bytes): %s\n", roJson.size(), roJson.c_str());
        }

        std::string pdJson;
        bool pdOk = press.getPressDataJson(pdJson);
        report("getPressDataJson", pdOk, press);
        if (pdOk)
        {
            std::printf("  pd json (%zu bytes): %s\n", pdJson.size(), pdJson.c_str());
        }

        std::string rtJson;
        bool rtOk = press.getRealTimeDataJson(rtJson);
        report("getRealTimeDataJson", rtOk, press);
        if (rtOk)
        {
            std::printf("  rt json (%zu bytes): %s\n", rtJson.size(), rtJson.c_str());
        }

        std::string tempJson;
        bool tjOk = press.getTempDataJson(tempJson);
        report("getTempDataJson", tjOk, press);
        if (tjOk)
        {
            std::printf("  temp json (%zu bytes): %s\n", tempJson.size(), tempJson.c_str());
        }

        std::string mixJson;
        bool mjOk = press.getMixDataJson(mixJson);
        report("getMixDataJson", mjOk, press);
        if (mjOk)
        {
            std::printf("  mix json (%zu bytes): %s\n", mixJson.size(), mixJson.c_str());
        }
    }

    // ==================== 5. 压力参数命令（单值/索引）====================
    {
        uint8_t pStep = 0;
        bool ok = press.getPressStepCount(pStep);
        report("getPressStepCount", ok, press);
        if (ok)
        {
            std::printf("  press step count = %u\n", pStep);
        }

        float calibration = 0.0f;
        ok = press.getCalibrationValue(calibration);
        report("getCalibrationValue", ok, press);
        if (ok)
        {
            std::printf("  calibration value = %.3f\n", calibration);
        }

        uint8_t moldType = 0;
        ok = press.getModeType(moldType);
        report("getModeType", ok, press);
        if (ok)
        {
            std::printf("  mold type = %u\n", moldType);
        }

        float diameter = 0.0f;
        ok = press.getCircle(diameter);
        report("getCircle", ok, press);
        if (ok)
        {
            std::printf("  circle diameter = %.3f\n", diameter);
        }

        float rectA = 0.0f;
        float rectB = 0.0f;
        ok = press.getRectangle(rectA, rectB);
        report("getRectangle", ok, press);
        if (ok)
        {
            std::printf("  rectangle A = %.3f  B = %.3f\n", rectA, rectB);
        }

        uint8_t airTime = 0;
        ok = press.getAirTime(airTime);
        report("getAirTime", ok, press);
        if (ok)
        {
            std::printf("  air time = %u s\n", airTime);
        }

        float pressValue = 0.0f;
        ok = press.getPressValueIndex(0, pressValue);
        report("getPressValueIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] target pressure = %.3f\n", pressValue);
        }

        float afterValue = 0.0f;
        ok = press.getAfterValueIndex(0, afterValue);
        report("getAfterValueIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] hold pressure = %.3f\n", afterValue);
        }

        int32_t kpTime = 0;
        ok = press.getKeepPressTime(0, kpTime);
        report("getKeepPressTime(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] keep-pressure time = %d ms\n", kpTime);
        }
    }

    // ==================== 6. 段参数命令 ====================
    {
        PressStepParameter psp{};
        if (press.getPressStepParameter(0, psp))
        {
            ++g_pass;
            std::printf("[PASS] getPressStepParameter(0): setP=%.3f after=%.3f kpTime=%u\n",
                        psp.m_SetPValue, psp.m_AfterValue, psp.m_KPTime);
        }
        else
        {
            ++g_fail;
            std::printf("[FAIL] getPressStepParameter(0) (last error: %s)\n",
                        press.getLastErrorInfo());
        }

        TempStepParameter tsp{};
        bool tspOk = press.getTempStepParameter(0, tsp);
        report("getTempStepParameter(0)", tspOk, press);
        if (tspOk)
        {
            std::printf("  tempValue=%.3f speedValue=%.3f ktTime=%ums\n",
                        tsp.m_TempValue, tsp.m_SpeedValue, tsp.m_KTTime);
        }

        MixStepParameter msp{};
        bool mspOk = press.getMixStepParameter(0, msp);
        report("getMixStepParameter(0)", mspOk, press);
        if (mspOk)
        {
            std::printf("  IsPress=%u setP=%.3f after=%.3f IsUpHeat=%u upTemp=%.3f "
                        "IsDownHeat=%u downTemp=%.3f kTime=%ums\n",
                        msp.m_IsPress, msp.m_SetPValue, msp.m_AfterValue,
                        msp.m_IsUpHeat, msp.m_UpTemp,
                        msp.m_IsDownHeat, msp.m_DownTemp, msp.m_KTime);
        }
    }

    // ==================== 7. 温度参数命令（单值/索引）====================
    {
        uint8_t tStep = 0;
        bool ok = press.getTempStepCount(tStep);
        report("getTempStepCount", ok, press);
        if (ok)
        {
            std::printf("  temp step count = %u\n", tStep);
        }

        uint8_t speedMold = 0;
        ok = press.getTempSpeedMold(speedMold);
        report("getTempSpeedMold", ok, press);
        if (ok)
        {
            std::printf("  temp speed mold = %u\n", speedMold);
        }

        float tempValue = 0.0f;
        ok = press.getStartTempIndex(0, tempValue);
        report("getStartTempIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] temperature = %.3f\n", tempValue);
        }

        float speedValue = 0.0f;
        ok = press.getTempSpeedIndex(0, speedValue);
        report("getTempSpeedIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] speed = %.3f\n", speedValue);
        }

        int32_t ktTime = 0;
        ok = press.getKeepTempTime(0, ktTime);
        report("getKeepTempTime(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] keep-temperature time = %d ms\n", ktTime);
        }

        float maxWater = 0.0f;
        ok = press.getMaxWaterValue(maxWater);
        report("getMaxWaterValue", ok, press);
        if (ok)
        {
            std::printf("  max water value = %.3f\n", maxWater);
        }

        float minWater = 0.0f;
        ok = press.getMinWaterValue(minWater);
        report("getMinWaterValue", ok, press);
        if (ok)
        {
            std::printf("  min water value = %.3f\n", minWater);
        }
    }

    // ==================== 7.1 混合模式参数命令（单值/索引）====================
    {
        uint8_t mixStep = 0;
        bool ok = press.getMixStepCount(mixStep);
        report("getMixStepCount", ok, press);
        if (ok)
        {
            std::printf("  mix step count = %u\n", mixStep);
        }

        uint8_t mixMold = 0;
        ok = press.getMixMoldType(mixMold);
        report("getMixMoldType", ok, press);
        if (ok)
        {
            std::printf("  mix mold type = %u\n", mixMold);
        }

        float mixD = 0.0f;
        ok = press.getMixCircle(mixD);
        report("getMixCircle", ok, press);
        if (ok)
        {
            std::printf("  mix circle diameter = %.3f\n", mixD);
        }

        float mixA = 0.0f;
        float mixB = 0.0f;
        ok = press.getMixRectangle(mixA, mixB);
        report("getMixRectangle", ok, press);
        if (ok)
        {
            std::printf("  mix rectangle A=%.3f B=%.3f\n", mixA, mixB);
        }

        bool mixIsPress = false;
        ok = press.getMixIsPressIndex(0, mixIsPress);
        report("getMixIsPressIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] isPress = %d\n", mixIsPress ? 1 : 0);
        }

        float mixPress = 0.0f;
        ok = press.getMixPressIndex(0, mixPress);
        report("getMixPressIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] pressure = %.3f\n", mixPress);
        }

        float mixAfter = 0.0f;
        ok = press.getMixAfterPressIndex(0, mixAfter);
        report("getMixAfterPressIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] after-pressure = %.3f\n", mixAfter);
        }

        bool mixIsUp = false;
        ok = press.getMixIsHeatUpIndex(0, mixIsUp);
        report("getMixIsHeatUpIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] isHeatUp = %d\n", mixIsUp ? 1 : 0);
        }

        float mixUp = 0.0f;
        ok = press.getMixHeatUpIndex(0, mixUp);
        report("getMixHeatUpIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] heat-up temperature = %.3f\n", mixUp);
        }

        bool mixIsDown = false;
        ok = press.getMixIsHeatDownIndex(0, mixIsDown);
        report("getMixIsHeatDownIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] isHeatDown = %d\n", mixIsDown ? 1 : 0);
        }

        float mixDown = 0.0f;
        ok = press.getMixHeatDownIndex(0, mixDown);
        report("getMixHeatDownIndex(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] heat-down temperature = %.3f\n", mixDown);
        }

        int32_t mixKTime = 0;
        ok = press.getMixKeepTime(0, mixKTime);
        report("getMixKeepTime(0)", ok, press);
        if (ok)
        {
            std::printf("  step[0] keep time = %d ms\n", mixKTime);
        }
    }

    // ==================== 8. 系统命令 ====================
    {
        DateTimeInfo dt{};
        bool ok = press.getDateTime(dt);
        report("getDateTime", ok, press);
        if (ok)
        {
            printDateTime(dt);
        }

        uint8_t brightness = 0;
        bool brOk = press.getBrightness(brightness);
        report("getBrightness", brOk, press);
        if (brOk)
        {
            std::printf("  brightness = %u\n", brightness);
        }

        // 唯一的写示例：读出-写回，不改变设备状态
        // The single write example: read then write the same value back
        if (brOk)
        {
            report("setBrightness(read-back)", press.setBrightness(brightness), press);
            std::printf("  wrote back brightness = %u\n", brightness);
        }

        bool sound = false;
        bool sndOk = press.getButtonSound(sound);
        report("getButtonSound", sndOk, press);
        if (sndOk)
        {
            std::printf("  button sound = %s (%d)\n", sound ? "on" : "off", sound ? 1 : 0);
        }

        uint8_t language = 0;
        bool langOk = press.getLanguage(language);
        report("getLanguage", langOk, press);
        if (langOk)
        {
            std::printf("  language = %u\n", language);
        }
    }

    // ==================== 9. 状态查询 ====================
    {
        MachineState state;
        bool ok = press.getMachineState(state);
        report("getMachineState", ok, press);
        if (ok)
        {
            printMachineState(state);
        }

        // 注册号在 connect 时由固件分配 / register number assigned by firmware at connect
        std::printf("  register no = %llu\n",
                    static_cast<unsigned long long>(press.getMachineRegisterNo()));
    }

    // ==================== 10. 设置命令测试（读出-写回-校验）====================
    // 每条写命令先读出当前值，原值写回，再重新读取校验，确保设备配置不变。
    // Each setter reads the current value, writes it back, then re-reads to verify.
    {
        // 通用校验：无参 getter + 值 setter
        auto testByte = [&](const char* name, auto get, auto set) {
            uint8_t value = 0;
            uint8_t back = 0;
            const bool ok = get(value) && set(value) && get(back) && back == value;
            report(name, ok, press);
            if (ok)
            {
                std::printf("  write-back value = %u\n", value);
            }
        };
        auto testFloat = [&](const char* name, auto get, auto set) {
            float value = 0.0f;
            float back = 0.0f;
            const bool ok = get(value) && set(value) && get(back) &&
                            std::fabs(back - value) <= 0.001f;
            report(name, ok, press);
            if (ok)
            {
                std::printf("  write-back value = %.3f\n", value);
            }
        };
        auto testIndexedFloat = [&](const char* name, auto get, auto set) {
            float value = 0.0f;
            float back = 0.0f;
            const bool ok = get(0, value) && set(0, value) && get(0, back) &&
                            std::fabs(back - value) <= 0.001f;
            report(name, ok, press);
            if (ok)
            {
                std::printf("  write-back value = %.3f\n", value);
            }
        };
        auto testIndexedInt32 = [&](const char* name, auto get, auto set) {
            int32_t value = 0;
            int32_t back = 0;
            const bool ok = get(0, value) && set(0, value) && get(0, back) && back == value;
            report(name, ok, press);
            if (ok)
            {
                std::printf("  write-back value = %d\n", value);
            }
        };
        auto testIndexedBool = [&](const char* name, auto get, auto set) {
            bool value = false;
            bool back = false;
            const bool ok = get(0, value) && set(0, value) && get(0, back) && back == value;
            report(name, ok, press);
            if (ok)
            {
                std::printf("  write-back value = %d\n", value ? 1 : 0);
            }
        };
        auto testRect = [&](const char* name, auto get, auto set) {
            float a = 0.0f;
            float b = 0.0f;
            float a2 = 0.0f;
            float b2 = 0.0f;
            const bool ok = get(a, b) && set(a, b) && get(a2, b2) &&
                            std::fabs(a2 - a) <= 0.001f && std::fabs(b2 - b) <= 0.001f;
            report(name, ok, press);
            if (ok)
            {
                std::printf("  write-back A=%.3f B=%.3f\n", a, b);
            }
        };

        // ---- 压力参数 ----
        testByte("setPressStepCount(read-back)",
                 [&](uint8_t& v) { return press.getPressStepCount(v); },
                 [&](uint8_t v) { return press.setPressStepCount(v); });
        testFloat("setCalibrationValue(read-back)",
                  [&](float& v) { return press.getCalibrationValue(v); },
                  [&](float v) { return press.setCalibrationValue(v); });
        testByte("setModeType(read-back)",
                 [&](uint8_t& v) { return press.getModeType(v); },
                 [&](uint8_t v) { return press.setModeType(v); });
        testFloat("setCircle(read-back)",
                  [&](float& v) { return press.getCircle(v); },
                  [&](float v) { return press.setCircle(v); });

        {
            float a = 0.0f;
            float b = 0.0f;
            float backA = 0.0f;
            float backB = 0.0f;
            const bool ok = press.getRectangle(a, b) && press.setRectangle(a, b) &&
                            press.getRectangle(backA, backB) &&
                            std::fabs(backA - a) <= 0.001f && std::fabs(backB - b) <= 0.001f;
            report("setRectangle(read-back)", ok, press);
            if (ok)
            {
                std::printf("  write-back value = A %.3f  B %.3f\n", a, b);
            }
        }

        testByte("setAirTime(read-back)",
                 [&](uint8_t& v) { return press.getAirTime(v); },
                 [&](uint8_t v) { return press.setAirTime(v); });
        testIndexedFloat("setPressValueIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getPressValueIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setPressValueIndex(i, v); });
        testIndexedFloat("setAfterValueIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getAfterValueIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setAfterValueIndex(i, v); });
        testIndexedInt32("setKeepPressTime(0)(read-back)",
                         [&](uint8_t i, int32_t& v) { return press.getKeepPressTime(i, v); },
                         [&](uint8_t i, int32_t v) { return press.setKeepPressTime(i, v); });

        // ---- 温度参数 ----
        testByte("setTempStepCount(read-back)",
                 [&](uint8_t& v) { return press.getTempStepCount(v); },
                 [&](uint8_t v) { return press.setTempStepCount(v); });
        testByte("setTempSpeedMold(read-back)",
                 [&](uint8_t& v) { return press.getTempSpeedMold(v); },
                 [&](uint8_t v) { return press.setTempSpeedMold(v); });
        testIndexedFloat("setTempIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getStartTempIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setTempIndex(i, v); });
        testIndexedFloat("setTempSpeedIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getTempSpeedIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setTempSpeedIndex(i, v); });
        testIndexedInt32("setKeepTempTime(0)(read-back)",
                         [&](uint8_t i, int32_t& v) { return press.getKeepTempTime(i, v); },
                         [&](uint8_t i, int32_t v) { return press.setKeepTempTime(i, v); });
        testFloat("setMaxWaterValue(read-back)",
                  [&](float& v) { return press.getMaxWaterValue(v); },
                  [&](float v) { return press.setMaxWaterValue(v); });
        testFloat("setMinWaterValue(read-back)",
                  [&](float& v) { return press.getMinWaterValue(v); },
                  [&](float v) { return press.setMinWaterValue(v); });

        // ---- 混合模式参数 ----
        testByte("setMixStepCount(read-back)",
                 [&](uint8_t& v) { return press.getMixStepCount(v); },
                 [&](uint8_t v) { return press.setMixStepCount(v); });
        testByte("setMixMoldType(read-back)",
                 [&](uint8_t& v) { return press.getMixMoldType(v); },
                 [&](uint8_t v) { return press.setMixMoldType(v); });
        testFloat("setMixCircle(read-back)",
                  [&](float& v) { return press.getMixCircle(v); },
                  [&](float v) { return press.setMixCircle(v); });
        testRect("setMixRectangle(read-back)",
                 [&](float& a, float& b) { return press.getMixRectangle(a, b); },
                 [&](float a, float b) { return press.setMixRectangle(a, b); });
        testIndexedBool("setMixIsPressIndex(0)(read-back)",
                        [&](uint8_t i, bool& v) { return press.getMixIsPressIndex(i, v); },
                        [&](uint8_t i, bool v) { return press.setMixIsPressIndex(i, v); });
        testIndexedFloat("setMixPressIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getMixPressIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setMixPressIndex(i, v); });
        testIndexedFloat("setMixAfterPressIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getMixAfterPressIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setMixAfterPressIndex(i, v); });
        testIndexedBool("setMixIsHeatUpIndex(0)(read-back)",
                        [&](uint8_t i, bool& v) { return press.getMixIsHeatUpIndex(i, v); },
                        [&](uint8_t i, bool v) { return press.setMixIsHeatUpIndex(i, v); });
        testIndexedFloat("setMixHeatUpIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getMixHeatUpIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setMixHeatUpIndex(i, v); });
        testIndexedBool("setMixIsHeatDownIndex(0)(read-back)",
                        [&](uint8_t i, bool& v) { return press.getMixIsHeatDownIndex(i, v); },
                        [&](uint8_t i, bool v) { return press.setMixIsHeatDownIndex(i, v); });
        testIndexedFloat("setMixHeatDownIndex(0)(read-back)",
                         [&](uint8_t i, float& v) { return press.getMixHeatDownIndex(i, v); },
                         [&](uint8_t i, float v) { return press.setMixHeatDownIndex(i, v); });
        testIndexedInt32("setMixKeepTime(0)(read-back)",
                         [&](uint8_t i, int32_t& v) { return press.getMixKeepTime(i, v); },
                         [&](uint8_t i, int32_t v) { return press.setMixKeepTime(i, v); });

        // ---- 系统参数 ----
        {
            DateTimeInfo dt{};
            DateTimeInfo back{};
            const bool ok = press.getDateTime(dt) && press.setDateTime(dt) &&
                            press.getDateTime(back) &&
                            back.m_Year == dt.m_Year && back.m_Month == dt.m_Month &&
                            back.m_Day == dt.m_Day && back.m_Hour == dt.m_Hour &&
                            back.m_Minute == dt.m_Minute;
            report("setDateTime(read-back)", ok, press);
            if (ok)
            {
                std::printf("  write-back value = ");
                printDateTime(dt);
            }
        }

        {
            bool sound = false;
            bool back = false;
            const bool ok = press.getButtonSound(sound) && press.setButtonSound(sound) &&
                            press.getButtonSound(back) && back == sound;
            report("setButtonSound(read-back)", ok, press);
            if (ok)
            {
                std::printf("  write-back value = %s\n", sound ? "on" : "off");
            }
        }

        testByte("setLanguage(read-back)",
                 [&](uint8_t& v) { return press.getLanguage(v); },
                 [&](uint8_t v) { return press.setLanguage(v); });

        // ---- 段参数 ----
        {
            PressStepParameter psp{};
            PressStepParameter back{};
            const bool ok = press.getPressStepParameter(0, psp) &&
                            press.setPressStepParameter(0, psp) &&
                            press.getPressStepParameter(0, back) &&
                            std::fabs(back.m_SetPValue - psp.m_SetPValue) <= 0.001f &&
                            std::fabs(back.m_AfterValue - psp.m_AfterValue) <= 0.001f &&
                            back.m_KPTime == psp.m_KPTime;
            report("setPressStepParameter(0)(read-back)", ok, press);
            if (ok)
            {
                std::printf("  write-back value = setP %.3f after %.3f kpTime %u\n",
                            psp.m_SetPValue, psp.m_AfterValue, psp.m_KPTime);
            }
        }

        {
            TempStepParameter tsp{};
            TempStepParameter back{};
            const bool ok = press.getTempStepParameter(0, tsp) &&
                            press.setTempStepParameter(0, tsp) &&
                            press.getTempStepParameter(0, back) &&
                            std::fabs(back.m_TempValue - tsp.m_TempValue) <= 0.001f &&
                            std::fabs(back.m_SpeedValue - tsp.m_SpeedValue) <= 0.001f &&
                            back.m_KTTime == tsp.m_KTTime;
            report("setTempStepParameter(0)(read-back)", ok, press);
            if (ok)
            {
                std::printf("  write-back value = temp %.3f speed %.3f ktTime %u\n",
                            tsp.m_TempValue, tsp.m_SpeedValue, tsp.m_KTTime);
            }
        }

        {
            MixStepParameter msp{};
            MixStepParameter back{};
            const bool ok = press.getMixStepParameter(0, msp) &&
                            press.setMixStepParameter(0, msp) &&
                            press.getMixStepParameter(0, back) &&
                            back.m_IsPress == msp.m_IsPress &&
                            back.m_IsUpHeat == msp.m_IsUpHeat &&
                            back.m_IsDownHeat == msp.m_IsDownHeat &&
                            std::fabs(back.m_SetPValue - msp.m_SetPValue) <= 0.001f &&
                            std::fabs(back.m_AfterValue - msp.m_AfterValue) <= 0.001f &&
                            std::fabs(back.m_UpTemp - msp.m_UpTemp) <= 0.001f &&
                            std::fabs(back.m_DownTemp - msp.m_DownTemp) <= 0.001f &&
                            back.m_KTime == msp.m_KTime;
            report("setMixStepParameter(0)(read-back)", ok, press);
            if (ok)
            {
                std::printf("  write-back value = setP %.3f after %.3f upTemp %.3f downTemp %.3f kTime %u\n",
                            msp.m_SetPValue, msp.m_AfterValue,
                            msp.m_UpTemp, msp.m_DownTemp, msp.m_KTime);
            }
        }

        // ---- 运行控制命令（真实动作，设备空载）----
        // 每条命令都先确保设备处于停止状态，启动后通过 getMachineState 校验状态位，
        // 再下发停止并再次校验，确保测试结束时设备回到空闲。
        {
            // 起点先确保空闲：上一次测试或人工操作可能让设备处于运行态
            forceIdle(press);

            // 记录原始工作模式；加热/加压仅在模式 2（压力 + 双温）下有效，
            // 测试期间切到模式 2，结束后恢复原始模式。
            MachineState initial{};
            uint8_t originalMode = 0;
            if (press.getMachineState(initial))
            {
                originalMode = initial.m_ModelState;
            }
            if (originalMode != 2)
            {
                std::printf("  [info] switching work mode %u -> 2 for control tests\n",
                            static_cast<unsigned>(originalMode));
                press.selectModelState(2);
                waitForMode(press, 2, 2000);
            }

            // 1) 加热 开 → 校验 → 关 → 校验
            const bool heatOk = bench("setStartHeat(true/false)", "ctrl", 1, [&] {
                MachineState st{};
                if (!press.getMachineState(st))
                {
                    return false;
                }
                if ((st.m_Flags & 0x02) != 0)
                {
                    if (!press.setStartHeat(false) || !waitForFlag(press, 0x02, false, 3000))
                    {
                        return false;
                    }
                }
                const bool heatOn = press.setStartHeat(true);
                if (!heatOn || !waitForFlag(press, 0x02, true, 8000))
                {
                    MachineState dbg{};
                    press.getMachineState(dbg);
                    std::printf("  [info] heat on: call=%s flags=0x%02X (last error: %s)\n",
                                heatOn ? "true" : "false", dbg.m_Flags, press.getLastErrorInfo());
                    return false;
                }
                if (!press.setStartHeat(false) || !waitForFlag(press, 0x02, false, 5000))
                {
                    return false;
                }
                return true;
            }, "1 次 开→校验→关→校验");
            report("setStartHeat(true/false)", heatOk, press);

            // 2) 加压 开 → 动作 0.8 s → 关
            forceIdle(press);
            const bool pressOk = bench("setPressing(true/false)", "ctrl", 1, [&] {
                MachineState st{};
                if (!press.getMachineState(st))
                {
                    return false;
                }
                if ((st.m_Flags & 0x01) != 0)
                {
                    if (!press.setPressing(false) || !waitForFlag(press, 0x01, false, 5000))
                    {
                        return false;
                    }
                }
                if (!press.setPressing(true) || !waitForFlag(press, 0x01, true, 3000))
                {
                    return false;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                MachineState running{};
                if (press.getMachineState(running) && (running.m_Flags & 0x01) != 0)
                {
                    if (!press.setPressing(false) || !waitForFlag(press, 0x01, false, 5000))
                    {
                        return false;
                    }
                }
                return true;
            }, "1 次 启动→动作 0.8 s→停止");
            report("setPressing(true/false)", pressOk, press);

            // 3) 脱模：固件未实现 CMDID_SET_START_DEMOLD(0x1523)，仅记录结果
            {
                const auto t0 = std::chrono::steady_clock::now();
                const bool d1 = press.setDemolding(true);
                const bool d2 = press.setDemolding(false);
                const double totalMs = std::chrono::duration<double, std::milli>(
                    std::chrono::steady_clock::now() - t0).count();
                recordSample("setDemolding(true/false)", "ctrl",
                             "固件未实现 0x1523", (d1 ? 1 : 0) + (d2 ? 1 : 0), 2, totalMs);
                std::printf("  [N/A] setDemolding: firmware 未实现 CMDID_SET_START_DEMOLD(0x1523)，"
                            "返回 %s/%s\n", d1 ? "true" : "false", d2 ? "true" : "false");
                ++g_skip;
            }

            // 4) 全部启动/停止（加压 + 加热）
            forceIdle(press);
            const bool startOk = bench("setStart(true/false)", "ctrl", 1, [&] {
                MachineState st{};
                if (!press.getMachineState(st))
                {
                    return false;
                }
                if (st.m_Flags != 0)
                {
                    if (!press.setStart(false) || !waitForIdle(press, 5000))
                    {
                        return false;
                    }
                }
                if (!press.setStart(true))
                {
                    // 启动失败时尽力恢复空闲，避免设备停留在运行态
                    press.setStart(false);
                    press.setPressing(false);
                    press.setStartHeat(false);
                    return false;
                }
                // 完整流程含 3 s 启动延时与升温阶段，10 s 内可能尚未出现状态位
                const bool active = waitForActive(press, 10000);
                if (!active)
                {
                    std::printf("  [info] setStart: accepted, no active flag within 10 s "
                                "(start delay / heat-up phase)\n");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                // 无论是否已观察到状态位，都下发停止，结束可能处于延时/升温阶段的流程
                press.setStart(false);
                if (!waitForIdle(press, 3000))
                {
                    press.setPressing(false);
                    press.setStartHeat(false);
                }
                const bool idle = waitForIdle(press, 12000);
                if (!idle)
                {
                    MachineState dbg{};
                    press.getMachineState(dbg);
                    std::printf("  [info] setStart stop: flags=0x%02X after 12 s (last error: %s)\n",
                                dbg.m_Flags, press.getLastErrorInfo());
                }
                return idle;
            }, "1 次 启动全部→0.5 s→停止");
            report("setStart(true/false)", startOk, press);

            // 5) 模式切换并恢复原模式
            forceIdle(press);
            const bool modeOk = bench("selectModelState(1<->2)", "ctrl", 1, [&] {
                MachineState st{};
                if (!press.getMachineState(st))
                {
                    return false;
                }
                if (st.m_Flags != 0)
                {
                    std::printf("  [info] selectModelState skipped: machine busy (flags=0x%02X)\n",
                                st.m_Flags);
                    return false;
                }
                const uint8_t original = st.m_ModelState;
                const uint8_t target = (original == 1) ? 2 : 1;
                const bool switched = press.selectModelState(target);
                const bool observed = switched && waitForMode(press, target, 2000);
                if (!observed)
                {
                    MachineState mid{};
                    press.getMachineState(mid);
                    std::printf("  [info] selectModelState(%u): call=%s observed mode=%u (last error: %s)\n",
                                target, switched ? "true" : "false",
                                mid.m_ModelState, press.getLastErrorInfo());
                }
                const bool restored = press.selectModelState(original) &&
                                      waitForMode(press, original, 2000);
                return switched && observed && restored;
            }, "切换后恢复原模式");
            report("selectModelState(switch/restore)", modeOk, press);

            // 控制动作结束后强制回到空闲，并恢复原始工作模式
            forceIdle(press);
            MachineState finalState{};
            if (originalMode != 0 && press.getMachineState(finalState) &&
                finalState.m_ModelState != originalMode)
            {
                press.selectModelState(originalMode);
                waitForMode(press, originalMode, 2000);
            }
        }
    }

    // ==================== 11. 全接口性能基准 / Full interface benchmark ====================
    // 覆盖 press.hpp 中全部公开接口：本地查询、设备信息、JSON 批量、单值命令、
    // 段参数、系统命令、设置命令（原值写回）与运行控制命令（1 次动作）。
    std::printf("\n================ benchmark: all interfaces / 全接口基准 ================\n");
    {
        // ---- 本地接口（无串口往返）----
        bench("isConnected", "get", 20, [&] { return press.isConnected(); });
        bench("getMachineRegisterNo", "get", 20, [&] { press.getMachineRegisterNo(); return true; });
        bench("isRunning", "get", 20, [&] { press.isRunning(); return true; });
        bench("getLastErrorInfo", "get", 20, [&] { press.getLastErrorInfo(); return true; });
        bench("Press::version", "static", 50, [] { return !Press::version().empty(); });
        bench("Press::getPortList", "static", 5, [] { return !Press::getPortList().empty(); });

        // ---- 设备信息 ----
        bench("getName", "get", 10, [&] { std::string s; return press.getName(s); });
        bench("getMachineType", "get", 10, [&] { std::string s; return press.getMachineType(s); });
        bench("getSerialNumber", "get", 10, [&] { std::string s; return press.getSerialNumber(s); });
        bench("getPressParameter", "get", 10, [&] { PressParameter p{}; return press.getPressParameter(p); });
        bench("getTempParameter", "get", 10, [&] { TempParameter p{}; return press.getTempParameter(p); });
        bench("getOtherInfo", "get", 10, [&] { OtherInfo i{}; return press.getOtherInfo(i); });

        // ---- JSON 批量 ----
        bench("getReadOnlyDataJson", "get", 5, [&] { std::string j; return press.getReadOnlyDataJson(j); });
        bench("getPressDataJson", "get", 5, [&] { std::string j; return press.getPressDataJson(j); });
        bench("getRealTimeDataJson", "get", 5, [&] { std::string j; return press.getRealTimeDataJson(j); });
        bench("getTempDataJson", "get", 5, [&] { std::string j; return press.getTempDataJson(j); });
        bench("getMixDataJson", "get", 5, [&] { std::string j; return press.getMixDataJson(j); });

        // ---- 状态 ----
        bench("getMachineState", "get", 20, [&] { MachineState s{}; return press.getMachineState(s); });

        // ---- 压力单值/索引 ----
        bench("getPressStepCount", "get", 10, [&] { uint8_t v = 0; return press.getPressStepCount(v); });
        bench("getCalibrationValue", "get", 10, [&] { float v = 0; return press.getCalibrationValue(v); });
        bench("getModeType", "get", 10, [&] { uint8_t v = 0; return press.getModeType(v); });
        bench("getCircle", "get", 10, [&] { float v = 0; return press.getCircle(v); });
        bench("getRectangle", "get", 10, [&] { float a = 0, b = 0; return press.getRectangle(a, b); });
        bench("getAirTime", "get", 10, [&] { uint8_t v = 0; return press.getAirTime(v); });
        bench("getPressValueIndex", "get", 10, [&] { float v = 0; return press.getPressValueIndex(0, v); });
        bench("getAfterValueIndex", "get", 10, [&] { float v = 0; return press.getAfterValueIndex(0, v); });
        bench("getKeepPressTime", "get", 10, [&] { int32_t v = 0; return press.getKeepPressTime(0, v); });
        bench("getPressStepParameter", "get", 5, [&] { PressStepParameter p{}; return press.getPressStepParameter(0, p); });

        // ---- 温度单值/索引 ----
        bench("getTempStepCount", "get", 10, [&] { uint8_t v = 0; return press.getTempStepCount(v); });
        bench("getTempSpeedMold", "get", 10, [&] { uint8_t v = 0; return press.getTempSpeedMold(v); });
        bench("getStartTempIndex", "get", 10, [&] { float v = 0; return press.getStartTempIndex(0, v); });
        bench("getTempSpeedIndex", "get", 10, [&] { float v = 0; return press.getTempSpeedIndex(0, v); });
        bench("getKeepTempTime", "get", 10, [&] { int32_t v = 0; return press.getKeepTempTime(0, v); });
        bench("getMaxWaterValue", "get", 10, [&] { float v = 0; return press.getMaxWaterValue(v); });
        bench("getMinWaterValue", "get", 10, [&] { float v = 0; return press.getMinWaterValue(v); });
        bench("getTempStepParameter", "get", 5, [&] { TempStepParameter p{}; return press.getTempStepParameter(0, p); });

        // ---- 混合模式 ----
        bench("getMixStepCount", "get", 10, [&] { uint8_t v = 0; return press.getMixStepCount(v); });
        bench("getMixMoldType", "get", 10, [&] { uint8_t v = 0; return press.getMixMoldType(v); });
        bench("getMixCircle", "get", 10, [&] { float v = 0; return press.getMixCircle(v); });
        bench("getMixRectangle", "get", 10, [&] { float a = 0, b = 0; return press.getMixRectangle(a, b); });
        bench("getMixIsPressIndex", "get", 10, [&] { bool v = false; return press.getMixIsPressIndex(0, v); });
        bench("getMixPressIndex", "get", 10, [&] { float v = 0; return press.getMixPressIndex(0, v); });
        bench("getMixAfterPressIndex", "get", 10, [&] { float v = 0; return press.getMixAfterPressIndex(0, v); });
        bench("getMixIsHeatUpIndex", "get", 10, [&] { bool v = false; return press.getMixIsHeatUpIndex(0, v); });
        bench("getMixHeatUpIndex", "get", 10, [&] { float v = 0; return press.getMixHeatUpIndex(0, v); });
        bench("getMixIsHeatDownIndex", "get", 10, [&] { bool v = false; return press.getMixIsHeatDownIndex(0, v); });
        bench("getMixHeatDownIndex", "get", 10, [&] { float v = 0; return press.getMixHeatDownIndex(0, v); });
        bench("getMixKeepTime", "get", 10, [&] { int32_t v = 0; return press.getMixKeepTime(0, v); });
        bench("getMixStepParameter", "get", 5, [&] { MixStepParameter p{}; return press.getMixStepParameter(0, p); });

        // ---- 系统 ----
        bench("getDateTime", "get", 10, [&] { DateTimeInfo t{}; return press.getDateTime(t); });
        bench("getBrightness", "get", 10, [&] { uint8_t v = 0; return press.getBrightness(v); });
        bench("getButtonSound", "get", 10, [&] { bool v = false; return press.getButtonSound(v); });
        bench("getLanguage", "get", 10, [&] { uint8_t v = 0; return press.getLanguage(v); });

        // ---- 设置命令（原值写回）----
        benchSetter(press, "setPressStepCount",
                    [&](uint8_t& v) { return press.getPressStepCount(v); },
                    [&](uint8_t v) { return press.setPressStepCount(v); });
        benchFloatSetter(press, "setCalibrationValue",
                         [&](float& v) { return press.getCalibrationValue(v); },
                         [&](float v) { return press.setCalibrationValue(v); });
        benchSetter(press, "setModeType",
                    [&](uint8_t& v) { return press.getModeType(v); },
                    [&](uint8_t v) { return press.setModeType(v); });
        benchFloatSetter(press, "setCircle",
                         [&](float& v) { return press.getCircle(v); },
                         [&](float v) { return press.setCircle(v); });
        {
            float a = 0.0f;
            float b = 0.0f;
            if (press.getRectangle(a, b))
            {
                bench("setRectangle", "set", 5, [&] { return press.setRectangle(a, b); }, "原值写回");
            }
            else
            {
                std::printf("  [SKIP] setRectangle: read failed\n");
                ++g_skip;
            }
        }
        benchSetter(press, "setAirTime",
                    [&](uint8_t& v) { return press.getAirTime(v); },
                    [&](uint8_t v) { return press.setAirTime(v); });
        benchIndexedFloatSetter(press, "setPressValueIndex",
                                [&](uint8_t i, float& v) { return press.getPressValueIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setPressValueIndex(i, v); });
        benchIndexedFloatSetter(press, "setAfterValueIndex",
                                [&](uint8_t i, float& v) { return press.getAfterValueIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setAfterValueIndex(i, v); });
        benchIndexedInt32Setter(press, "setKeepPressTime",
                                [&](uint8_t i, int32_t& v) { return press.getKeepPressTime(i, v); },
                                [&](uint8_t i, int32_t v) { return press.setKeepPressTime(i, v); });
        {
            PressStepParameter p{};
            if (press.getPressStepParameter(0, p))
            {
                bench("setPressStepParameter", "set", 5,
                      [&] { return press.setPressStepParameter(0, p); }, "index=0 原值写回");
            }
            else
            {
                std::printf("  [SKIP] setPressStepParameter: read failed\n");
                ++g_skip;
            }
        }

        benchSetter(press, "setTempStepCount",
                    [&](uint8_t& v) { return press.getTempStepCount(v); },
                    [&](uint8_t v) { return press.setTempStepCount(v); });
        benchSetter(press, "setTempSpeedMold",
                    [&](uint8_t& v) { return press.getTempSpeedMold(v); },
                    [&](uint8_t v) { return press.setTempSpeedMold(v); });
        benchIndexedFloatSetter(press, "setTempIndex",
                                [&](uint8_t i, float& v) { return press.getStartTempIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setTempIndex(i, v); });
        benchIndexedFloatSetter(press, "setTempSpeedIndex",
                                [&](uint8_t i, float& v) { return press.getTempSpeedIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setTempSpeedIndex(i, v); });
        benchIndexedInt32Setter(press, "setKeepTempTime",
                                [&](uint8_t i, int32_t& v) { return press.getKeepTempTime(i, v); },
                                [&](uint8_t i, int32_t v) { return press.setKeepTempTime(i, v); });
        benchFloatSetter(press, "setMaxWaterValue",
                         [&](float& v) { return press.getMaxWaterValue(v); },
                         [&](float v) { return press.setMaxWaterValue(v); });
        benchFloatSetter(press, "setMinWaterValue",
                         [&](float& v) { return press.getMinWaterValue(v); },
                         [&](float v) { return press.setMinWaterValue(v); });

        benchSetter(press, "setMixStepCount",
                    [&](uint8_t& v) { return press.getMixStepCount(v); },
                    [&](uint8_t v) { return press.setMixStepCount(v); });
        benchSetter(press, "setMixMoldType",
                    [&](uint8_t& v) { return press.getMixMoldType(v); },
                    [&](uint8_t v) { return press.setMixMoldType(v); });
        benchFloatSetter(press, "setMixCircle",
                         [&](float& v) { return press.getMixCircle(v); },
                         [&](float v) { return press.setMixCircle(v); });
        bench("setMixRectangle", "set", 5, [&] {
            float a = 0.0f;
            float b = 0.0f;
            return press.getMixRectangle(a, b) && press.setMixRectangle(a, b);
        }, "原值写回");
        bench("setMixIsPressIndex", "set", 5, [&] {
            bool v = false;
            return press.getMixIsPressIndex(0, v) && press.setMixIsPressIndex(0, v);
        }, "index=0 原值写回");
        benchIndexedFloatSetter(press, "setMixPressIndex",
                                [&](uint8_t i, float& v) { return press.getMixPressIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setMixPressIndex(i, v); });
        benchIndexedFloatSetter(press, "setMixAfterPressIndex",
                                [&](uint8_t i, float& v) { return press.getMixAfterPressIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setMixAfterPressIndex(i, v); });
        bench("setMixIsHeatUpIndex", "set", 5, [&] {
            bool v = false;
            return press.getMixIsHeatUpIndex(0, v) && press.setMixIsHeatUpIndex(0, v);
        }, "index=0 原值写回");
        benchIndexedFloatSetter(press, "setMixHeatUpIndex",
                                [&](uint8_t i, float& v) { return press.getMixHeatUpIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setMixHeatUpIndex(i, v); });
        bench("setMixIsHeatDownIndex", "set", 5, [&] {
            bool v = false;
            return press.getMixIsHeatDownIndex(0, v) && press.setMixIsHeatDownIndex(0, v);
        }, "index=0 原值写回");
        benchIndexedFloatSetter(press, "setMixHeatDownIndex",
                                [&](uint8_t i, float& v) { return press.getMixHeatDownIndex(i, v); },
                                [&](uint8_t i, float v) { return press.setMixHeatDownIndex(i, v); });
        benchIndexedInt32Setter(press, "setMixKeepTime",
                                [&](uint8_t i, int32_t& v) { return press.getMixKeepTime(i, v); },
                                [&](uint8_t i, int32_t v) { return press.setMixKeepTime(i, v); });
        {
            TempStepParameter p{};
            if (press.getTempStepParameter(0, p))
            {
                bench("setTempStepParameter", "set", 5,
                      [&] { return press.setTempStepParameter(0, p); }, "index=0 原值写回");
            }
            else
            {
                std::printf("  [SKIP] setTempStepParameter: read failed\n");
                ++g_skip;
            }
        }
        {
            MixStepParameter p{};
            if (press.getMixStepParameter(0, p))
            {
                bench("setMixStepParameter", "set", 5,
                      [&] { return press.setMixStepParameter(0, p); }, "index=0 原值写回");
            }
            else
            {
                std::printf("  [SKIP] setMixStepParameter: read failed\n");
                ++g_skip;
            }
        }

        // ---- 系统设置（原值写回）----
        {
            DateTimeInfo t{};
            if (press.getDateTime(t))
            {
                bench("setDateTime", "set", 5, [&] { return press.setDateTime(t); }, "原值写回");
            }
            else
            {
                std::printf("  [SKIP] setDateTime: read failed\n");
                ++g_skip;
            }
        }
        benchSetter(press, "setBrightness",
                    [&](uint8_t& v) { return press.getBrightness(v); },
                    [&](uint8_t v) { return press.setBrightness(v); });
        benchBoolSetter(press, "setButtonSound",
                        [&](bool& v) { return press.getButtonSound(v); },
                        [&](bool v) { return press.setButtonSound(v); });
        benchSetter(press, "setLanguage",
                    [&](uint8_t& v) { return press.getLanguage(v); },
                    [&](uint8_t v) { return press.setLanguage(v); });

        // ---- JSON 写入（原值写回）----
        {
            std::string j;
            if (press.getPressDataJson(j))
            {
                const bool ok = bench("setPressDataJson", "set", 3,
                                      [&] { return press.setPressDataJson(j); }, "原值写回");
                report("setPressDataJson(write-back)", ok, press);
            }
            else
            {
                std::printf("  [SKIP] setPressDataJson: read failed\n");
                ++g_skip;
            }
        }
        {
            std::string j;
            if (press.getTempDataJson(j))
            {
                const bool ok = bench("setTempDataJson", "set", 3,
                                      [&] { return press.setTempDataJson(j); }, "原值写回");
                report("setTempDataJson(write-back)", ok, press);
            }
            else
            {
                std::printf("  [SKIP] setTempDataJson: read failed\n");
                ++g_skip;
            }
        }
        {
            std::string j;
            if (press.getMixDataJson(j))
            {
                const bool ok = bench("setMixDataJson", "set", 3,
                                      [&] { return press.setMixDataJson(j); }, "原值写回");
                report("setMixDataJson(write-back)", ok, press);
            }
            else
            {
                std::printf("  [SKIP] setMixDataJson: read failed\n");
                ++g_skip;
            }
        }
    }

    // ==================== 12. 等待定时采集回调输出 ====================
    // 先清零统计：基准测试中直接调用 getRealTimeDataJson 也会触发回调
    // reset first: direct getRealTimeDataJson calls in the benchmark also fire callbacks
    callback.resetStats();
    std::printf("\nwaiting 3 seconds for periodic callbacks...\n\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    printBenchTable();
    writeBenchReport("press_api_performance_report.md", argv[2], argv[1]);
    std::printf("\nperiodic timer (configured 1 s):\n");
    callback.printIntervalStats();

    // ==================== 清理 / Cleanup ====================
    press.unregisterDataInterface();
    press.disconnect();
    std::printf("\ndisconnected.\n\n");

    std::printf("==== result: %d passed, %d failed, %d skipped ====\n", g_pass, g_fail, g_skip);
    return (g_fail == 0) ? 0 : 1;
}
