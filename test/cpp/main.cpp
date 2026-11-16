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
 *   - 段参数命令 (压力/温度/混合模式)          step-parameter commands
 *   - 系统命令 (日期时间/亮度/提示音/语言)      system commands
 *   - 状态查询与控制命令                        state & control commands
 *
 * 安全说明: 示例只做只读查询；唯一的写操作是"读出-写回"（不改变设备状态）。
 * Safety: the example only reads; the single write is read-back (state unchanged).
 *****************************************************************************/

#include "press.hpp"
#include "typeprivate.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
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
void bench(const char* name, int repeats, Fn&& fn)
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
    const BenchResult r{name, okCount, repeats,
                        samples.front(), sum / samples.size(),
                        samples[samples.size() / 2], samples.back()};
    g_bench.push_back(r);
    std::printf("  [bench] %-34s ok=%2d/%2d  min=%7.2f  avg=%7.2f  p50=%7.2f  max=%7.2f ms\n",
                name, r.ok, r.n, r.minMs, r.avgMs, r.p50Ms, r.maxMs);
}

//! 打印性能汇总表 / Print the latency summary table
void printBenchTable()
{
    if (g_bench.empty())
    {
        return;
    }
    std::printf("\n================ latency summary / 耗时汇总 ================\n");
    std::printf("%-34s %5s %9s %9s %9s %9s\n",
                "command", "ok", "min(ms)", "avg(ms)", "p50(ms)", "max(ms)");
    std::printf("-------------------------------------------------------------------------------\n");
    for (const auto& r : g_bench)
    {
        std::printf("%-34s %2d/%-2d %9.2f %9.2f %9.2f %9.2f\n",
                    r.name, r.ok, r.n, r.minMs, r.avgMs, r.p50Ms, r.maxMs);
    }
    std::printf("===============================================================================\n");
}

//! 回调实现：打印异步上报的数据 / Callback impl printing async reports
class TestCallback : public PressDataInterface
{
public:
    void onReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& data) override
    {
        std::printf("  [callback] onReadOnlyData: err=%d regNo=%llu type=%s\n",
                    errorCode, static_cast<unsigned long long>(registerNo), data.m_Type);
    }

    void onPressData(int errorCode, const PressData& data) override
    {
        std::printf("  [callback] onPressData: err=%d step=%u\n", errorCode, data.m_PStep);
    }

    void onRealTimeData(int errorCode, const RealTimeData& data) override
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

        std::printf("  [callback] onRealTimeData: mode=%u P:%s T:%s pStep=%u tStep=%u press=%.3f upT=%.1f downT=%.1f remain=%ums\n",
                    static_cast<unsigned>(data.m_ModelState),
                    data.m_IsPStart ? "on" : "off",
                    data.m_IsTStart ? "on" : "off",
                    data.m_CPStep, data.m_CTStep,
                    data.m_PressValue, data.m_TUpValue, data.m_TDownValue, data.m_PTime);
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

void printReadOnlyData(const ReadOnlyData& d)
{
    std::printf("  nameZH='%.64s'\n", d.m_NameZH);
    std::printf("  nameEN='%.64s'\n", d.m_NameEN);
    std::printf("  type='%.64s'  sn='%.64s'\n", d.m_Type, d.m_SerialNumber);
    std::printf("  Screenshot=%u StartDelay=%u VersionType=%u IsHideLang=%u Remote=%u Network=%u FontZH=%u FontEN=%u\n",
                d.m_Screenshot, d.m_StartDelay, d.m_VersionType, d.m_IsHideLang,
                d.m_Remote, d.m_Network, d.m_FontZH, d.m_FontEN);
    std::printf("  MaxPStep=%u MaxP=%.3f MinP=%.3f Max_Min=%.3f Diameter=%.3f PDecimal=%u PressDecimal=%u PModel=%u OutType=%u\n",
                d.m_MaxPStep, d.m_MaxPLimit, d.m_MinPLimit, d.m_Max_Min, d.m_Diameter,
                d.m_PDecimal, d.m_PressDecimal, d.m_PModel, d.m_OutType);
    std::printf("  MaxTStep=%u MaxT=%.3f MinT=%.3f TDecimal=%u IsHasWater=%u IsHasSpeed=%u\n",
                d.m_MaxTStep, d.m_MaxTLimit, d.m_MinTLimit,
                d.m_TDecimal, d.m_IsHasWater, d.m_IsHasSpeed);
}

void printPressData(const PressData& d)
{
    std::printf("  PStep=%u Type=%u A=%.3f B=%.3f D=%.3f OuterD=%.3f InnerD=%.3f CheckValue=%.3f\n",
                d.m_PStep, d.m_Type, d.m_A, d.m_B, d.m_D,
                d.m_OuterD, d.m_InnerD, d.m_CheckValue);
    std::printf("  Speed=%u DemoldValue=%.3f AirTime=%u\n",
                d.m_Speed, d.m_DemoldValue, d.m_AirTime);
    const unsigned n = (d.m_PStep < MAX_PStep) ? d.m_PStep : MAX_PStep;
    for (unsigned i = 0; i < n; ++i)
    {
        std::printf("    step[%u]: setP=%.3f after=%.3f kpTime=%ums\n",
                    i, d.m_SetPValue[i], d.m_AfterValue[i], d.m_KPTime[i]);
    }
    if (n == 0)
    {
        std::printf("    (no active step / 无有效步骤)\n");
    }
}

void printRealTimeData(const RealTimeData& d)
{
    std::printf("  mode=%u pressState=%u pdChanged=%u cPStep=%u cTStep=%u cT1Step=%u\n",
                d.m_ModelState, d.m_PressState, d.m_PdChanged,
                d.m_CPStep, d.m_CTStep, d.m_CT1Step);
    std::printf("  IsPStart=%u IsTStart=%u IsT1Start=%u IsAir=%u\n",
                d.m_IsPStart, d.m_IsTStart, d.m_IsT1Start, d.m_IsAir);
    std::printf("  press=%.3f moldPress=%.3f upT=%.1f downT=%.1f\n",
                d.m_PressValue, d.m_MoldPressure, d.m_TUpValue, d.m_TDownValue);
    std::printf("  PTime=%ums TTime=%ums T1Time=%ums\n", d.m_PTime, d.m_TTime, d.m_T1Time);
}

void usage()
{
    std::printf("Press SDK C++ interface test / C++ 接口测试\n\n");
    std::printf("Usage:\n");
    std::printf("  press_cpp_test serial <port>        e.g. serial COM3\n");
    std::printf("  press_cpp_test tcp <host:port>      e.g. tcp 192.168.1.100:9000\n\n");

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
        ReadOnlyData ro;
        bool roOk = press.getReadOnlyDataJson(ro);
        report("getReadOnlyDataJson(json)", roOk, press);
        if (roOk)
        {
            printReadOnlyData(ro);
        }

        PressData pd;
        bool pdOk = press.getPressDataJson(pd);
        report("getPressDataJson(json)", pdOk, press);
        if (pdOk)
        {
            printPressData(pd);
        }

        RealTimeData rt;
        bool rtOk = press.getRealTimeDataJson(rt);
        report("getRealTimeDataJson(json)", rtOk, press);
        if (rtOk)
        {
            printRealTimeData(rt);
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

    // ==================== 7. 系统命令 ====================
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

    // ==================== 8. 状态查询 ====================
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

    // ==================== 10. 性能基准 / Latency benchmark ====================
    // 每个命令重复执行并统计 min/avg/p50/max，用于评估各层函数性能。
    // Every command runs repeatedly to collect latency distribution.
    std::printf("\n================ benchmark / 性能基准 ================\n");
    {
        bench("getPressParameter", 10, [&] {
            PressParameter p{};
            return press.getPressParameter(p);
        });
        bench("getMachineState", 10, [&] {
            MachineState s{};
            return press.getMachineState(s);
        });
        bench("getReadOnlyDataJson(JSON)", 5, [&] {
            ReadOnlyData d{};
            return press.getReadOnlyDataJson(d);
        });
        bench("getRealTimeDataJson(JSON)", 5, [&] {
            RealTimeData d{};
            return press.getRealTimeDataJson(d);
        });
        bench("getTempDataJson", 5, [&] {
            std::string j;
            return press.getTempDataJson(j);
        });
        bench("getMixDataJson", 5, [&] {
            std::string j;
            return press.getMixDataJson(j);
        });
        bench("setBrightness(read-back)", 5, [&] {
            uint8_t b = 0;
            if (!press.getBrightness(b))
            {
                return false;
            }
            return press.setBrightness(b);
        });
    }

    // ==================== 11. 等待定时采集回调输出 ====================
    // 先清零统计：基准测试中直接调用 getRealTimeDataJson 也会触发回调
    // reset first: direct getRealTimeDataJson calls in the benchmark also fire callbacks
    callback.resetStats();
    std::printf("\nwaiting 3 seconds for periodic callbacks...\n\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    printBenchTable();
    std::printf("\nperiodic timer (configured 1 s):\n");
    callback.printIntervalStats();

    // ==================== 清理 / Cleanup ====================
    press.unregisterDataInterface();
    press.disconnect();
    std::printf("\ndisconnected.\n\n");

    std::printf("==== result: %d passed, %d failed ====\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
