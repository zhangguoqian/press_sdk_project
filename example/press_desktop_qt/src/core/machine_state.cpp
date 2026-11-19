#include "core/machine_state.h"
#include "core/format.h"

#include <QRandomGenerator>
#include <QTimer>
#include <QtMath>

namespace {
constexpr double kPi = 3.14159265358979323846;

double noise(double amplitude)
{
    return (QRandomGenerator::global()->generateDouble() - 0.5) * amplitude;
}
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
    m_timer->setInterval(500);
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

QString MachineState::moldAreaText() const
{
    const double area = moldArea();
    if (area <= 0.0)
        return QStringLiteral("—");
    return fmtNumber(area / 100.0, 1);
}

void MachineState::setPCount(int value)
{
    pCount = qBound(1, value, 30);
    if (press) {
        pStage = qMin(pStage, pCount);
        pRemain = pStages[pStage - 1].hold;
    }
    emit changed();
}

void MachineState::setTCount(int value)
{
    tCount = qBound(1, value, 30);
    if (heat) {
        tStage = qMin(tStage, tCount);
        tRemain = tStages[tStage - 1].hold;
    }
    emit changed();
}

void MachineState::setCoopCount(int value)
{
    coopCount = qBound(1, value, 30);
    if (coopRunning) {
        coopStage = qMin(coopStage, coopCount);
        coopRemain = coopRows[coopStage - 1].hold;
    }
    emit changed();
}

void MachineState::startPress()
{
    if (press)
        return;
    press = true;
    pStage = 1;
    pRemain = pStages[0].hold;
    runtime = runtime > 0 ? runtime : 0.0;
    emit changed();
}

void MachineState::stopPress()
{
    press = false;
    pStage = 0;
    pRemain = 0;
    emit changed();
}

void MachineState::startHeat()
{
    if (heat)
        return;
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
    heat = false;
    tStage = 0;
    emit changed();
}

void MachineState::toggleCoop()
{
    coopRunning = !coopRunning;
    coopStage = coopRunning ? 1 : 0;
    coopRemain = coopRunning ? coopRows[0].hold : 0;
    emit changed();
}

void MachineState::vent()
{
    liveP = 0.0;
    emit changed();
}

void MachineState::setMoldShape(MoldShape shape)
{
    moldShape = shape;
    emit settingsChanged();
    emit changed();
}

void MachineState::setMoldDiameter(double mm)
{
    moldDia = qMax(1.0, qRound(mm * 10.0) / 10.0);
    emit settingsChanged();
    emit changed();
}

void MachineState::setMoldLength(double mm)
{
    moldLen = qMax(1.0, qRound(mm * 10.0) / 10.0);
    emit settingsChanged();
    emit changed();
}

void MachineState::setMoldWidth(double mm)
{
    moldWid = qMax(1.0, qRound(mm * 10.0) / 10.0);
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
    bool anyRunning = press || heat || coopRunning;

    if (press) {
        const double target = pStages[pressureIndex()].p;
        liveP += (target - liveP) * 0.18 + noise(0.03);
        liveP = qBound(0.0, liveP, target * 1.02);
        pRemain -= 30;
        if (pRemain <= 0) {
            if (pStage < pCount) {
                ++pStage;
                pRemain = pStages[pStage - 1].hold;
            } else {
                press = false;
                pStage = 0;
                liveP = 0.0;
                pRemain = 0;
                ++output;
            }
        }
    } else if (liveP > 0.0) {
        liveP = qMax(0.0, liveP - liveP * 0.3);
        if (liveP < 0.02)
            liveP = 0.0;
    }

    if (heat) {
        const TemperatureStage &ts = tStages[temperatureIndex()];
        const double diff = ts.t - upT;
        upT += (diff >= 0 ? 1.0 : -1.0)
            * qMin(qAbs(diff), qMax(0.08, qAbs(diff) * 0.08))
            + noise(0.05);
        downT += ((ts.t - 0.6) - downT) * 0.1;

        tRemain -= 12;
        if (tRemain <= 0) {
            if (tStage < tCount) {
                ++tStage;
                tRemain = tStages[tStage - 1].hold;
            } else {
                heat = false;
                tStage = 0;
            }
        }
    }

    if (coopRunning) {
        coopRemain -= 30;
        if (coopRemain <= 0) {
            if (coopStage < coopCount) {
                ++coopStage;
                coopRemain = coopRows[coopStage - 1].hold;
            } else {
                coopRunning = false;
                coopStage = 0;
                coopRemain = 0;
            }
        }
    }

    if (anyRunning)
        runtime += 0.5;

    emit ticked();
    emit changed();
}
