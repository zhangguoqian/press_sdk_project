#include "core/machine_simulator.h"

#include "core/machine_state.h"

#include <QRandomGenerator>
#include <QtMath>

namespace {

double noise(double amplitude)
{
    return (QRandomGenerator::global()->generateDouble() - 0.5) * amplitude;
}

} // namespace

namespace Simulation {

void step(MachineState &state)
{
    bool anyRunning = state.press || state.heat || state.coopRunning;

    if (state.press) {
        const double target = state.pStages[state.pressureIndex()].p;
        state.liveP += (target - state.liveP) * 0.18 + noise(0.03);
        state.liveP = qBound(0.0, state.liveP, target * 1.02);
        state.pRemain -= 30;
        if (state.pRemain <= 0) {
            if (state.pStage < state.pCount) {
                ++state.pStage;
                state.pRemain = state.pStages[state.pStage - 1].hold;
            } else {
                state.press = false;
                state.pStage = 0;
                state.liveP = 0.0;
                state.pRemain = 0;
                ++state.output;
            }
        }
    } else if (state.liveP > 0.0) {
        state.liveP = qMax(0.0, state.liveP - state.liveP * 0.3);
        if (state.liveP < 0.02)
            state.liveP = 0.0;
    }

    if (state.heat) {
        const TemperatureStage &ts = state.tStages[state.temperatureIndex()];
        const double diff = ts.t - state.upT;
        state.upT += (diff >= 0 ? 1.0 : -1.0)
            * qMin(qAbs(diff), qMax(0.08, qAbs(diff) * 0.08))
            + noise(0.05);
        state.downT += ((ts.t - 0.6) - state.downT) * 0.1;

        state.tRemain -= 12;
        if (state.tRemain <= 0) {
            if (state.tStage < state.tCount) {
                ++state.tStage;
                state.tRemain = state.tStages[state.tStage - 1].hold;
            } else {
                state.heat = false;
                state.tStage = 0;
            }
        }
    }

    if (state.coopRunning) {
        state.coopRemain -= 30;
        if (state.coopRemain <= 0) {
            if (state.coopStage < state.coopCount) {
                ++state.coopStage;
                state.coopRemain = state.coopRows[state.coopStage - 1].hold;
            } else {
                state.coopRunning = false;
                state.coopStage = 0;
                state.coopRemain = 0;
            }
        }
    }

    if (anyRunning)
        state.runtime += 0.5;
}

} // namespace Simulation
