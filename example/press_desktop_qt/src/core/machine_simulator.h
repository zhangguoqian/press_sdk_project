#pragma once

class MachineState;

/**
 * 本地模拟运行推进（未连接设备时使用）。
 * 与 MachineState 分离，状态仍由 MachineState 统一持有。
 */
namespace Simulation {

// 推进一个节拍（500 ms）
void step(MachineState &state);

} // namespace Simulation
