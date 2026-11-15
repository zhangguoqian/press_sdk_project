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
 *   - 类型化助手 (单字节/浮点/双浮点/索引)      typed helpers
 *   - 段参数命令 (压力/温度/混合模式)          step-parameter commands
 *   - 系统命令 (日期时间/亮度/提示音/语言)      system commands
 *   - 状态查询与控制命令                        state & control commands
 *   - 通用命令接口                              generic read/write
 *
 * 安全说明: 示例只做只读查询；唯一的写操作是"读出-写回"（不改变设备状态）。
 * Safety: the example only reads; the single write is read-back (state unchanged).
 *****************************************************************************/

#include "press.hpp"
#include "typeprivate.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

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
    std::printf("  mode=%u press=%s heat=%s pStep=%u tStep=%u press=%.3f upT=%.1f downT=%.1f pRemain=%ums (%u bytes)\n",
                s.m_ModelState, press, heat, s.m_PressStep, s.m_TempStep,
                s.m_PressValue, s.m_TempUpValue, s.m_TempDownValue, s.m_PTime, s.m_Count);
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
    report("connect", press.connect(portName.c_str(), portType), press);
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
            std::printf("  MaxP=%.3f MinP=%.3f Diameter=%.3f MaxStep=%u PDecimal=%u OutType=%u\n",
                        pp.m_MaxPLimit, pp.m_MinPLimit, pp.m_Diameter,
                        pp.m_MaxPStep, pp.m_PDecimal, pp.m_OutType);
        }
        else
        {
            ++g_fail;
            std::printf("[FAIL] getPressParameter (last error: %s)\n",
                        press.getLastErrorInfo());
        }

        TempParameter tp{};
        report("getTempParameter", press.getTempParameter(tp), press);

        OtherInfo oi{};
        report("getOtherInfo", press.getOtherInfo(oi), press);
    }

    // ==================== 4. JSON 批量命令 ====================
    {
        ReadOnlyData ro;
        report("getReadOnlyData(json)", press.getReadOnlyData(ro), press);

        PressData pd;
        report("getPressData(json)", press.getPressData(pd), press);

        RealTimeData rt;
        bool rtOk = press.getRealTimeData(rt);
        report("getRealTimeData(json)", rtOk, press);
        if (!rtOk)
        {
            // 解析失败时 dump 固件返回的原始 JSON 辅助诊断
            // dump raw firmware JSON for diagnosis on parse failure
            std::vector<uint8_t> raw;
            if (press.genericRead(CMDID_GET_CURRENT_STATE_JSON, {0x00}, raw))
            {
                std::printf("  raw RT json (%zu bytes): %s\n", raw.size(),
                            std::string(raw.begin(), raw.end()).c_str());
            }
        }

        std::string tempJson;
        report("getTempDataJson", press.getTempDataJson(tempJson), press);
        if (!tempJson.empty())
        {
            std::printf("  temp json (%zu bytes): %.60s...\n", tempJson.size(), tempJson.c_str());
        }

        std::string mixJson;
        report("getMixDataJson", press.getMixDataJson(mixJson), press);
        if (!mixJson.empty())
        {
            std::printf("  mix json (%zu bytes): %.60s...\n", mixJson.size(), mixJson.c_str());
        }
    }

    // ==================== 5. 类型化助手 ====================
    {
        uint8_t step = 0;
        report("getByteValue(PRESS_STEP)", press.getByteValue(CMDID_PRESS_STEP, step), press);
        std::printf("  press step = %u\n", step);

        float calibrate = 0.0f;
        report("getFloatValue(PRESS_CALIBRATE)", press.getFloatValue(CMDID_PRESS_CALIBRATE, calibrate), press);
        std::printf("  calibrate = %.3f\n", calibrate);

        float rectA = 0.0f, rectB = 0.0f;
        report("getFloatPairValue(RECT_AB)", press.getFloatPairValue(CMDID_RECT_AB, rectA, rectB), press);
        std::printf("  rect A=%.3f B=%.3f\n", rectA, rectB);

        float indexPress = 0.0f;
        report("getIndexedFloat(INDEX_PRESS, 0)", press.getIndexedFloat(CMDID_INDEX_PRESS, 0, indexPress), press);
        std::printf("  step0 press = %.3f\n", indexPress);

        int32_t kpTime = 0;
        report("getIndexedInt32(INDEX_KPTIME, 0)", press.getIndexedInt32(CMDID_INDEX_KPTIME, 0, kpTime), press);
        std::printf("  step0 kpTime = %d\n", kpTime);

        uint8_t isPress = 0;
        report("getIndexedByte(MIX_INDEX_ISPRESS, 0)", press.getIndexedByte(CMDID_MIX_INDEX_ISPRESS, 0, isPress), press);
        std::printf("  mix step0 isPress = %u\n", isPress);
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
        report("getTempStepParameter(0)", press.getTempStepParameter(0, tsp), press);

        MixStepParameter msp{};
        report("getMixStepParameter(0)", press.getMixStepParameter(0, msp), press);
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
        report("getBrightness", press.getBrightness(brightness), press);

        // 唯一的写示例：读出-写回，不改变设备状态
        // The single write example: read then write the same value back
        report("setBrightness(read-back)", press.setBrightness(brightness), press);

        bool sound = false;
        report("getButtonSound", press.getButtonSound(sound), press);

        uint8_t language = 0;
        report("getLanguage", press.getLanguage(language), press);
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
        else
        {
            // 失败时 dump 0x0500 原始响应辅助诊断 / dump raw response for diagnosis
            std::vector<uint8_t> raw;
            if (press.genericRead(CMDID_GET_CURRENT_STATE, {0x00}, raw))
            {
                std::printf("  raw 0x0500 response (%zu bytes):", raw.size());
                for (uint8_t b : raw)
                {
                    std::printf(" %02X", b);
                }
                std::printf("\n");
            }
        }

        // 注册号在 connect 时由固件分配 / register number assigned by firmware at connect
        std::printf("  register no = %llu\n",
                    static_cast<unsigned long long>(press.getMachineRegisterNo()));
    }

    // ==================== 9. 通用命令接口 ====================
    {
        // 例：通过 generic 接口读取机型字符串（与 getMachineType 等价）
        // e.g. read the machine type via the generic interface
        std::vector<uint8_t> response;
        bool ok = press.genericRead(CMDID_MACHINE_TYPE, {0x00}, response);
        report("genericRead(MACHINE_TYPE)", ok, press);
        if (ok && !response.empty())
        {
            std::string type(response.begin(), response.end());
            std::printf("  machine type (generic) = %s\n", type.c_str());
        }

        // 例：generic 写——把亮度写回当前值（与 setBrightness 等价，命令需带 SET 位）
        // e.g. generic write; the command must carry the SET bit
        uint8_t brightness = 0;
        if (press.getBrightness(brightness))
        {
            std::vector<uint8_t> echo;
            ok = press.genericWrite(SET_CMD(CMDID_BRIGHTNESS), {brightness}, echo);
            report("genericWrite(BRIGHTNESS)", ok, press);
        }
    }

    // ==================== 10. 等待定时采集回调输出 ====================
    std::printf("\nwaiting 3 seconds for periodic callbacks...\n\n");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // ==================== 清理 / Cleanup ====================
    press.unregisterDataInterface();
    press.disconnect();
    std::printf("disconnected.\n\n");

    std::printf("==== result: %d passed, %d failed ====\n", g_pass, g_fail);
    return (g_fail == 0) ? 0 : 1;
}
