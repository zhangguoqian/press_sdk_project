#include "core/machine_state.h"

#include "core/app_settings.h"
#include "core/format.h"
#include "core/machine_simulator.h"

#include <QTimer>
#include <QtMath>

namespace {
constexpr double kPi = 3.14159265358979323846;
} // namespace

MachineState::MachineState(QObject *parent)
    : QObject(parent)
{
    // 压力流程：30 段，1–30 Ton，补压比设定低 0.3 Ton
    pStages.reserve(30);
    for (int i = 0; i < 30; ++i) {
        PressureStage s;
        s.p = double(i + 1);
        s.boost = double(i + 1) - 0.3;
        s.hold = 600;
        pStages.append(s);
    }

    // 温度流程：30 段，30–175 °C，每段升温 5 °C
    tStages.reserve(30);
    for (int i = 0; i < 30; ++i) {
        TemperatureStage s;
        s.t = 30.0 + i * 5.0;
        s.rate = 3.0;
        s.hold = 500;
        tStages.append(s);
    }

    // 合作流程：独立可编辑数据
    coopRows.reserve(30);
    for (int i = 0; i < 30; ++i) {
        CoopStage s;
        s.p = double(i + 1);
        s.boost = double(i + 1) - 0.3;
        s.tUp = 30.0 + i * 5.0;
        s.tDown = 30.0 + i * 5.0;
        s.hold = 600;
        coopRows.append(s);
    }

    m_timer = new QTimer(this);
    m_timer->setInterval(AppSettings::instance().tickIntervalMs());
    connect(m_timer, &QTimer::timeout, this, &MachineState::tick);
    m_timer->start();
}

int MachineState::pressureIndex() const
{
    if (pCount <= 0)
        return 0;
    const int index = (pStage > 0 ? pStage - 1 : 0);
    return qBound(0, index, pCount - 1);
}

int MachineState::temperatureIndex() const
{
    if (tCount <= 0)
        return 0;
    const int index = (tStage > 0 ? tStage - 1 : 0);
    return qBound(0, index, tCount - 1);
}

int MachineState::coopIndex() const
{
    if (coopCount <= 0)
        return 0;
    const int index = (coopStage > 0 ? coopStage - 1 : 0);
    return qBound(0, index, coopCount - 1);
}

double MachineState::moldArea() const
{
    switch (moldShape) {
    case MoldShape::Circle:
        return kPi * qPow(qMax(1.0, moldDia) / 2.0, 2);
    case MoldShape::Rect:
        return qMax(1.0, moldLen) * qMax(1.0, moldWid);
    case MoldShape::Special:
    default:
        return 0.0;
    }
}

double MachineState::moldPressure(double ton) const
{
    const double area = moldArea();
    if (area <= 0.0)
        return -1.0;
    // P = F / A = (Ton × 1000 kgf × 9.80665 N/kgf) / 面积(mm²) → MPa
    return ton * 9806.65 / area;
}

void MachineState::attachService(PressService *service)
{
    if (!service || m_service == service)
        return;
    m_service = service;

    // 只读参数 → 设备能力（控制升温速率 / 水冷回路是否可见）
    connect(service, &PressService::readOnlyDataUpdated, this,
            [this](const PressReadOnlyData &data) {
                hasSpeed = data.hasSpeed;
                hasWater = data.hasWater;
                if (!hasSpeed)
                    rateMode = RateMode::Normal;

                // 版本类型决定可用模式：0 仅单独，1 仅合作，2 双模式
                versionType = data.versionType;
                singleAllowed = versionType != 1;
                coopAllowed = versionType != 0;

                pDecimals = qBound(0, data.pDecimal, 4);
                moldDecimals = qBound(0, data.pressDecimal, 4);
                autoBoost = qBound(0.0, data.autoBoost, 10.0);

                emit settingsChanged();
            });

    // 压力工艺参数 → 压力阶段表
    connect(service, &PressService::pressProgramUpdated,
            this, &MachineState::applyPressProgram);
    // 温度 / 合作工艺参数 → 温度阶段表、合作流程表
    connect(service, &PressService::tempProgramUpdated,
            this, &MachineState::applyTempProgram);
    connect(service, &PressService::mixProgramUpdated,
            this, &MachineState::applyMixProgram);

    // 定时回调 → 界面线程排队更新
    connect(service, &PressService::realTimeUpdated, this, &MachineState::applyRealTime);
    connect(service, &PressService::sdkError, this, &MachineState::sdkError);
    connect(service, &PressService::connectionChanged, this,
            [this](bool connected, const QString &endpoint) {
                if (!connected) {
                    // 断开后回到本地模拟，清空设备侧运行态
                    press = false;
                    heat = false;
                    coopRunning = false;
                    m_coopRequested = false;
                    m_coopSeenRunning = false;
                    m_coopIdleFrames = 0;
                    pStage = tStage = coopStage = 0;
                    pRemain = tRemain = coopRemain = 0;
                    liveP = 0.0;
                    airAllowed = true;
                    // 重连后从新样本重新建立滑动平均
                    m_pressSampleCount = 0;
                    m_pressSampleIndex = 0;
                }
                emit linkChanged(connected, endpoint);
                emit changed();
            });
}

void MachineState::applyRealTime(const PressRtData &data)
{
    if (!sdkMode())
        return;

    const bool wasPress = press;
    const bool wasHeat = heat;

    press = data.pressOn;
    heat = data.heatOn;
    // CPStep / CTStep 与下位机阶段编号一致（从 1 开始），直接使用
    pStage = press ? qBound(1, data.pressStep, qMax(1, pCount)) : 0;
    tStage = heat ? qBound(1, data.tempStep, qMax(1, tCount)) : 0;
    pRemain = press ? qMax(0, data.pressRemainSec) : 0;
    tRemain = heat ? qMax(0, data.tempRemainSec) : 0;

    // 压力：3 点滑动平均，减少末位跳动
    m_pressSamples[m_pressSampleIndex] = qMax(0.0, data.pressure);
    m_pressSampleIndex = (m_pressSampleIndex + 1) % 3;
    if (m_pressSampleCount < 3)
        ++m_pressSampleCount;
    double sum = 0.0;
    for (int i = 0; i < m_pressSampleCount; ++i)
        sum += m_pressSamples[i];
    liveP = sum / m_pressSampleCount;

    // 温度：负值表示传感器无效/断线，保留 -1 供界面显示“—”
    upT = data.tempUp < 0.0 ? -1.0 : data.tempUp;
    downT = data.tempDown < 0.0 ? -1.0 : data.tempDown;
    airAllowed = data.airAllowed;

    // 非上位机发起、状态由停止变为运行：提示设备侧已启动
    // 上位机指令下发后 5 s 内的状态变化视为自身操作，避免误报
    if (!m_coopRequested) {
        const bool externalPress = !wasPress && press
            && (!m_pressCmdAge.isValid() || m_pressCmdAge.elapsed() > 5000);
        const bool externalHeat = !wasHeat && heat
            && (!m_heatCmdAge.isValid() || m_heatCmdAge.elapsed() > 5000);
        if (externalPress)
            emit externalRunDetected(QStringLiteral("检测到下位机已启动加压"));
        if (externalHeat)
            emit externalRunDetected(QStringLiteral("检测到下位机已启动加热"));
    }

    if (modelState != data.modelState) {
        modelState = data.modelState;
        emit modelStateChanged(modelState);
    }

    if (m_coopRequested) {
        if (press || heat) {
            coopRunning = true;
            coopStage = press ? pStage : tStage;
            m_coopSeenRunning = true;
            m_coopIdleFrames = 0;
        } else if (m_coopSeenRunning && ++m_coopIdleFrames >= 3) {
            // 设备启动存在 1~2 帧延迟，且需先观测到运行再判定结束
            coopRunning = false;
            coopStage = 0;
            m_coopRequested = false;
            m_coopSeenRunning = false;
            m_coopIdleFrames = 0;
        }
    }

    emit changed();
}

void MachineState::commitPressureStage(int index)
{
    if (!sdkMode() || index < 0 || index >= pStages.size())
        return;
    const PressureStage &stage = pStages.at(index);
    if (!m_service->pushPressureStage(index, stage.p, stage.boost, stage.hold))
        emit sdkError(QStringLiteral("压力阶段参数下发失败"));
}

void MachineState::commitTemperatureStage(int index)
{
    if (!sdkMode() || index < 0 || index >= tStages.size())
        return;
    const TemperatureStage &stage = tStages.at(index);
    if (!m_service->pushTemperatureStage(index, stage.t, stage.rate, stage.hold))
        emit sdkError(QStringLiteral("温度阶段参数下发失败"));
}

void MachineState::commitCoopStage(int index)
{
    if (!sdkMode() || index < 0 || index >= coopRows.size())
        return;
    const CoopStage &stage = coopRows.at(index);
    if (!m_service->pushCoopStage(index, stage.press, stage.p, stage.boost,
                                  stage.heatUp, stage.tUp, stage.heatDown,
                                  stage.tDown, stage.hold))
        emit sdkError(QStringLiteral("合作流程参数下发失败"));
}

void MachineState::commitMold(bool includeType)
{
    if (!sdkMode())
        return;
    if (includeType) {
        const int type = moldShape == MoldShape::Special ? 0
                       : (moldShape == MoldShape::Circle ? 1 : 2);
        if (!m_service->pushMoldType(type))
            emit sdkError(QStringLiteral("模具类型下发失败"));
    }
    if (moldShape == MoldShape::Circle) {
        if (!m_service->pushMoldCircle(moldDia))
            emit sdkError(QStringLiteral("圆形模具参数下发失败"));
    } else if (moldShape == MoldShape::Rect) {
        if (!m_service->pushMoldRect(moldLen, moldWid))
            emit sdkError(QStringLiteral("矩形模具参数下发失败"));
    }
}

void MachineState::commitCoopMold()
{
    if (!sdkMode())
        return;
    const int type = coopMoldShape == MoldShape::Special ? 0
                   : (coopMoldShape == MoldShape::Circle ? 1 : 2);
    if (!m_service->pushCoopMoldType(type))
        emit sdkError(QStringLiteral("合作模式模具类型下发失败"));
    if (coopMoldShape == MoldShape::Circle) {
        if (!m_service->pushCoopMoldCircle(coopMoldDia))
            emit sdkError(QStringLiteral("合作模式圆形模具参数下发失败"));
    } else if (coopMoldShape == MoldShape::Rect) {
        if (!m_service->pushCoopMoldRect(coopMoldLen, coopMoldWid))
            emit sdkError(QStringLiteral("合作模式矩形模具参数下发失败"));
    }
}


QString MachineState::moldAreaText() const
{
    const double area = moldArea();
    if (area <= 0.0)
        return QStringLiteral("—");
    return fmtNumber(area / 100.0, 1);
}

double MachineState::coopMoldArea() const
{
    switch (coopMoldShape) {
    case MoldShape::Circle:
        return kPi * qPow(qMax(1.0, coopMoldDia) / 2.0, 2);
    case MoldShape::Rect:
        return qMax(1.0, coopMoldLen) * qMax(1.0, coopMoldWid);
    case MoldShape::Special:
    default:
        return 0.0;
    }
}

double MachineState::coopMoldPressure(double ton) const
{
    const double area = coopMoldArea();
    if (area <= 0.0)
        return -1.0;
    return ton * 9806.65 / area;
}

QString MachineState::coopMoldAreaText() const
{
    const double area = coopMoldArea();
    if (area <= 0.0)
        return QStringLiteral("—");
    return fmtNumber(area / 100.0, 1);
}

void MachineState::applyPressProgram(const PressProgramData &data)
{
    const int count = qBound(1, data.stepCount, pStages.size());
    for (int i = 0; i < pStages.size(); ++i) {
        if (i < data.setP.size())
            pStages[i].p = data.setP.at(i);
        if (i < data.afterP.size())
            pStages[i].boost = data.afterP.at(i);
        if (i < data.keepSec.size())
            pStages[i].hold = qBound(0, data.keepSec.at(i), 35999);
    }

    pCount = count;
    if (press)
        pStage = qMin(pStage, pCount);

    calibration = qBound(-3.0, data.checkValue, 3.0);
    airTime = qBound(1, data.airTime, 5);

    // 模具：Type 0 异形 / 1 圆形 / 2 矩形（3 环形暂未支持，保持当前类型）
    if (data.type >= 0 && data.type <= 2) {
        moldShape = data.type == 0 ? MoldShape::Special
                  : data.type == 1 ? MoldShape::Circle
                                   : MoldShape::Rect;
        if (data.a > 0.0)
            moldLen = data.a;
        if (data.b > 0.0)
            moldWid = data.b;
        if (data.d > 0.0)
            moldDia = data.d;
    }

    emit pressProgramChanged();
    emit settingsChanged();
    emit changed();
}

void MachineState::applyTempProgram(const PressTempProgramData &data)
{
    const int count = qBound(1, data.stepCount, tStages.size());
    for (int i = 0; i < tStages.size(); ++i) {
        if (i < data.startTemp.size())
            tStages[i].t = data.startTemp.at(i);
        if (i < data.speed.size())
            tStages[i].rate = data.speed.at(i);
        if (i < data.keepSec.size())
            tStages[i].hold = qBound(0, data.keepSec.at(i), 35999);
    }

    tCount = count;
    if (heat)
        tStage = qMin(tStage, tCount);

    // 水温：MinWater 为降温水温，MaxWater 为升温水温
    coolT = data.minWater;
    warmT = data.maxWater;
    // Type 表示普通 / 速率模式（0 普通，非 0 速率）；设备不支持速率时强制普通
    rateMode = (data.type != 0 && hasSpeed) ? RateMode::Fast : RateMode::Normal;

    emit tempProgramChanged();
    emit settingsChanged();
    emit changed();
}

void MachineState::applyMixProgram(const PressMixProgramData &data)
{
    const int count = qBound(1, data.stepCount, coopRows.size());
    for (int i = 0; i < coopRows.size(); ++i) {
        CoopStage &row = coopRows[i];
        if (i < data.isPress.size())
            row.press = data.isPress.at(i) != 0;
        if (i < data.setP.size())
            row.p = data.setP.at(i);
        if (i < data.afterP.size())
            row.boost = data.afterP.at(i);
        if (i < data.isUpHeat.size())
            row.heatUp = data.isUpHeat.at(i) != 0;
        if (i < data.upTemp.size())
            row.tUp = data.upTemp.at(i);
        if (i < data.isDownHeat.size())
            row.heatDown = data.isDownHeat.at(i) != 0;
        if (i < data.downTemp.size())
            row.tDown = data.downTemp.at(i);
        if (i < data.keepSec.size())
            row.hold = qBound(0, data.keepSec.at(i), 35999);
    }

    coopCount = count;
    if (coopRunning)
        coopStage = qMin(coopStage, coopCount);

    // 合作模式模具：MixData 的 Type/A/B/D 独立于 PressData
    if (data.type >= 0 && data.type <= 2) {
        coopMoldShape = data.type == 0 ? MoldShape::Special
                      : data.type == 1 ? MoldShape::Circle
                                       : MoldShape::Rect;
        if (data.a > 0.0)
            coopMoldLen = data.a;
        if (data.b > 0.0)
            coopMoldWid = data.b;
        if (data.d > 0.0)
            coopMoldDia = data.d;
    }

    emit mixProgramChanged();
    emit settingsChanged();
    emit changed();
}

void MachineState::setPCount(int value)
{
    pCount = qBound(1, value, 30);
    if (press) {
        pStage = qMin(pStage, pCount);
        pRemain = pStages[pStage - 1].hold;
    }
    if (sdkMode())
        m_service->pushStepCounts(pCount, tCount);
    emit changed();
}

void MachineState::setTCount(int value)
{
    tCount = qBound(1, value, 30);
    if (heat) {
        tStage = qMin(tStage, tCount);
        tRemain = tStages[tStage - 1].hold;
    }
    if (sdkMode())
        m_service->pushStepCounts(pCount, tCount);
    emit changed();
}

void MachineState::setCoopCount(int value)
{
    coopCount = qBound(1, value, 30);
    if (coopRunning) {
        coopStage = qMin(coopStage, coopCount);
        coopRemain = coopRows[coopStage - 1].hold;
    }
    if (sdkMode())
        m_service->pushCoopCount(coopCount);
    emit changed();
}

void MachineState::startPress()
{
    if (press)
        return;
    if (sdkMode()) {
        QString error;
        if (!m_service->setPressing(true, &error)) {
            emit sdkError(error);
            return;
        }
        m_pressCmdAge.restart();
    }
    press = true;
    pStage = 1;
    pRemain = pStages[0].hold;
    runtime = runtime > 0 ? runtime : 0.0;
    emit changed();
}

void MachineState::stopPress()
{
    if (sdkMode()) {
        QString error;
        if (!m_service->setPressing(false, &error)) {
            emit sdkError(error);
            return;
        }
    }
    press = false;
    pStage = 0;
    pRemain = 0;
    emit changed();
}

void MachineState::startHeat()
{
    if (heat)
        return;
    if (sdkMode()) {
        QString error;
        if (!m_service->setStartHeat(true, &error)) {
            emit sdkError(error);
            return;
        }
        m_heatCmdAge.restart();
    }
    heat = true;
    tStage = 1;
    tRemain = tStages[0].hold;
    if (upT <= 0.0)
        upT = 24.5;
    if (downT <= 0.0)
        downT = 23.8;
    emit changed();
}

void MachineState::stopHeat()
{
    if (sdkMode()) {
        QString error;
        if (!m_service->setStartHeat(false, &error)) {
            emit sdkError(error);
            return;
        }
    }
    heat = false;
    tStage = 0;
    emit changed();
}

void MachineState::toggleCoop()
{
    const bool next = !coopRunning;
    if (sdkMode()) {
        QString error;
        if (!m_service->setStart(next, &error)) {
            emit sdkError(error);
            return;
        }
    }
    coopRunning = next;
    m_coopRequested = next;
    m_coopSeenRunning = false;
    m_coopIdleFrames = 0;
    coopStage = next ? 1 : 0;
    coopRemain = next ? coopRows[0].hold : 0;
    emit changed();
}

bool MachineState::vent()
{
    if (sdkMode())
        return m_service->startAir();
    liveP = 0.0;
    emit changed();
    return true;
}

void MachineState::setCalibration(double value)
{
    const double next = qBound(-3.0, value, 3.0);
    if (qFuzzyCompare(calibration + 1.0, next + 1.0))
        return;
    if (sdkMode() && !m_service->pushCalibrationValue(next)) {
        emit sdkError(QStringLiteral("校准值下发失败"));
        return;
    }
    calibration = next;
    emit settingsChanged();
}

void MachineState::setAirTime(int seconds)
{
    const int next = qBound(1, seconds, 5);
    if (airTime == next)
        return;
    if (sdkMode() && !m_service->pushAirTime(next)) {
        emit sdkError(QStringLiteral("排气时间下发失败"));
        return;
    }
    airTime = next;
    emit settingsChanged();
}

void MachineState::setMoldShape(MoldShape shape)
{
    moldShape = shape;
    commitMold(true);
    emit settingsChanged();
    emit changed();
}

void MachineState::setMoldDiameter(double mm)
{
    moldDia = qMax(1.0, qRound(mm * 10.0) / 10.0);
    commitMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setMoldLength(double mm)
{
    moldLen = qMax(1.0, qRound(mm * 10.0) / 10.0);
    commitMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setMoldWidth(double mm)
{
    moldWid = qMax(1.0, qRound(mm * 10.0) / 10.0);
    commitMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setCoopMoldShape(MoldShape shape)
{
    coopMoldShape = shape;
    commitCoopMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setCoopMoldDiameter(double mm)
{
    coopMoldDia = qMax(1.0, qRound(mm * 10.0) / 10.0);
    commitCoopMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setCoopMoldLength(double mm)
{
    coopMoldLen = qMax(1.0, qRound(mm * 10.0) / 10.0);
    commitCoopMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setCoopMoldWidth(double mm)
{
    coopMoldWid = qMax(1.0, qRound(mm * 10.0) / 10.0);
    commitCoopMold();
    emit settingsChanged();
    emit changed();
}

void MachineState::setRateMode(RateMode mode)
{
    rateMode = mode;
    emit settingsChanged();
    emit changed();
}

void MachineState::notifySettingsChanged()
{
    emit settingsChanged();
    emit changed();
}

void MachineState::tick()
{
    // 已连接设备：运行数据由 SDK 1s 回调驱动，
    // 这里只保留界面节拍供曲线刷新。
    if (sdkMode()) {
        if (running())
            runtime += m_timer->interval() / 1000.0;
        emit ticked();
        return;
    }

    Simulation::step(*this);

    emit ticked();
    emit changed();
}
