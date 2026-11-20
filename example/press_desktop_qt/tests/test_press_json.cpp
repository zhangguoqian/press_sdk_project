#include "core/press_json.h"

#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

#include <cmath>
#include <string>

namespace {

int g_failures = 0;
int g_checks = 0;
QTextStream g_out(stdout);

std::string readFixture(const QString &name)
{
    QFile file(QStringLiteral(FIXTURE_DIR "/%1.json").arg(name));
    if (!file.open(QIODevice::ReadOnly)) {
        ++g_failures;
        g_out << "FAIL 无法读取样例报文: " << file.fileName() << "\n";
        return std::string();
    }
    return file.readAll().toStdString();
}

void check(bool ok, const QString &what)
{
    ++g_checks;
    if (!ok) {
        ++g_failures;
        g_out << "FAIL " << what << "\n";
    }
}

void checkInt(int actual, int expected, const QString &what)
{
    check(actual == expected,
          QStringLiteral("%1：期望 %2，实际 %3").arg(what).arg(expected).arg(actual));
}

void checkNumber(double actual, double expected, const QString &what)
{
    check(std::fabs(actual - expected) < 1e-6,
          QStringLiteral("%1：期望 %2，实际 %3").arg(what).arg(expected).arg(actual));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    // ---- ReadOnlyData ----
    const PressReadOnlyData ro = PressJson::parseReadOnly(readFixture("readonly"));
    check(ro.nameZh == QStringLiteral("自动热压机"), QStringLiteral("NameZH"));
    check(ro.nameEn == QStringLiteral("AUTO HOT PRESS"), QStringLiteral("NameEN"));
    check(ro.type == QStringLiteral("TYPE345"), QStringLiteral("Type"));
    check(ro.serialNumber == QStringLiteral("SN-2026-0001"), QStringLiteral("SerialNumber"));
    checkInt(ro.maxPStep, 30, QStringLiteral("MaxPStep"));
    checkInt(ro.maxTStep, 30, QStringLiteral("MaxTStep"));
    checkNumber(ro.maxPLimit, 30.0, QStringLiteral("MaxPLimit"));
    checkNumber(ro.minPLimit, 1.0, QStringLiteral("MinPLimit"));
    checkNumber(ro.autoBoost, 0.3, QStringLiteral("Max_Min"));
    checkNumber(ro.diameter, 100.0, QStringLiteral("Diameter"));
    checkInt(ro.pDecimal, 1, QStringLiteral("PDecimal"));
    checkInt(ro.pressDecimal, 2, QStringLiteral("PressDecimal"));
    checkInt(ro.tDecimal, 1, QStringLiteral("TDecimal"));
    check(ro.hasWater, QStringLiteral("IsHasWater"));
    check(ro.hasSpeed, QStringLiteral("IsHasSpeed"));
    check(ro.network, QStringLiteral("Network"));
    checkInt(ro.startDelay, 3, QStringLiteral("StartDelay"));
    checkInt(ro.versionType, 2, QStringLiteral("VersionType"));

    // ---- PressData ----
    const PressProgramData press = PressJson::parsePressProgram(readFixture("press"));
    checkInt(press.stepCount, 6, QStringLiteral("PStep"));
    checkInt(press.type, 1, QStringLiteral("Press.Type"));
    checkNumber(press.a, 100.0, QStringLiteral("Press.A"));
    checkNumber(press.d, 80.0, QStringLiteral("Press.D"));
    checkNumber(press.checkValue, 0.5, QStringLiteral("CheckValue"));
    checkInt(press.airTime, 2, QStringLiteral("AirTime"));
    checkInt(press.setP.size(), 6, QStringLiteral("SetPValue 数量"));
    checkInt(press.keepSec.size(), 6, QStringLiteral("KPTime 数量"));
    checkNumber(press.setP.value(0), 5.0, QStringLiteral("SetPValue[0]"));
    checkNumber(press.afterP.value(5), 29.7, QStringLiteral("AfterValue[5]"));
    checkInt(press.keepSec.value(0), 60, QStringLiteral("KPTime[0]"));

    // ---- TempData ----
    const PressTempProgramData temp = PressJson::parseTempProgram(readFixture("temp"));
    checkInt(temp.stepCount, 6, QStringLiteral("TStep"));
    checkNumber(temp.minWater, 15.0, QStringLiteral("MinWater"));
    checkNumber(temp.maxWater, 45.0, QStringLiteral("MaxWater"));
    checkInt(temp.startTemp.size(), 6, QStringLiteral("StartTemp 数量"));
    checkNumber(temp.startTemp.value(0), 30.0, QStringLiteral("StartTemp[0]"));
    checkNumber(temp.speed.value(2), 2.5, QStringLiteral("Speed[2]"));
    checkInt(temp.keepSec.value(1), 300, QStringLiteral("KTTime[1]"));

    // ---- MixData ----
    const PressMixProgramData mix = PressJson::parseMixProgram(readFixture("mix"));
    checkInt(mix.stepCount, 6, QStringLiteral("Step"));
    checkInt(mix.type, 2, QStringLiteral("Mix.Type"));
    checkNumber(mix.a, 120.0, QStringLiteral("Mix.A"));
    checkNumber(mix.b, 80.0, QStringLiteral("Mix.B"));
    checkInt(mix.isPress.value(0), 1, QStringLiteral("IsPress[0]"));
    checkInt(mix.isPress.value(3), 0, QStringLiteral("IsPress[3]"));
    checkNumber(mix.setP.value(2), 15.0, QStringLiteral("Mix.SetPValue[2]"));
    checkNumber(mix.afterP.value(2), 14.7, QStringLiteral("Mix.AfterValue[2]"));
    checkInt(mix.isUpHeat.value(1), 1, QStringLiteral("IsUpHeat[1]"));
    checkNumber(mix.upTemp.value(1), 75.0, QStringLiteral("UpTemp[1]"));
    checkInt(mix.isDownHeat.value(0), 0, QStringLiteral("IsDownHeat[0]"));
    checkNumber(mix.downTemp.value(2), 60.0, QStringLiteral("DownTemp[2]"));
    checkInt(mix.keepSec.value(5), 900, QStringLiteral("KTime[5]"));

    // ---- RealTimeData ----
    const PressRtData rt = PressJson::parseRealTime(readFixture("realtime"));
    checkInt(rt.modelState, 2, QStringLiteral("ModelState"));
    check(rt.pressOn, QStringLiteral("IsPStart"));
    check(rt.heatOn, QStringLiteral("IsTStart/IsT1Start"));
    check(rt.airAllowed, QStringLiteral("IsAir"));
    checkInt(rt.pressStep, 2, QStringLiteral("CPStep"));
    checkInt(rt.tempStep, 3, QStringLiteral("CTStep"));
    checkNumber(rt.pressure, 12.5, QStringLiteral("PValue"));
    checkNumber(rt.tempUp, 88.5, QStringLiteral("TUpValue"));
    checkNumber(rt.tempDown, 90.2, QStringLiteral("TDownValue"));
    checkInt(rt.pressRemainSec, 123, QStringLiteral("PTime"));
    checkInt(rt.tempRemainSec, 456, QStringLiteral("TTime"));
    checkInt(rt.flowChange, 3, QStringLiteral("FlowChange"));

    // 空/非法报文应安全返回默认值，不崩溃
    const PressRtData empty = PressJson::parseRealTime("{}");
    checkInt(empty.flowChange, 0, QStringLiteral("空报文 FlowChange"));
    const PressReadOnlyData broken = PressJson::parseReadOnly("not-json");
    check(broken.nameZh.isEmpty(), QStringLiteral("非法报文返回空名称"));

    if (g_failures == 0) {
        g_out << "全部通过：" << g_checks << " 项检查\n";
        return 0;
    }
    g_out << "失败 " << g_failures << " / " << g_checks << " 项\n";
    return 1;
}
