# Press SDK 接口性能报告

- **测试对象**：Press SDK C++ 接口（`Press` 类）
- **设备连接**：serial COM4
- **测量方式**：`std::chrono::steady_clock`，含完整请求/响应往返；
  本地查询 20 次、单值命令 10 次、参数块 5 次、JSON 批量 5 次、
  set 类原值写回 5 次（JSON 写入 3 次）、控制动作 1 次。
- **接口数**：96（全部通过 93，存在失败 3）
- **平均耗时（不含控制类）**：58.44 ms
- **启停分项**：见文末「启停接口分项测试（启动、停止各自单独调用）」，含原始帧 hex
- **全接口逐调用追踪**：见文末「全接口逐调用追踪」章节，100 次调用的参数/返回值与原始帧 hex

> 生命周期接口（`connect` / `disconnect` / `registerDataInterface` / `unregisterDataInterface` /
> `run` / `stop`）已在功能测试中执行，不做重复基准。
> 耗时显示为 0.000 表示本地调用（无串口往返）低于 0.001 ms。

| 接口 | 类别 | 采样 | 成功 | min(ms) | avg(ms) | p50(ms) | max(ms) | 备注 |
|---|---|---|---|---|---|---|---|---|
| `setStartHeat(true/false)` | ctrl | 1 | 1/1 | 3758.306 | 3758.306 | 3758.306 | 3758.306 | 1 次 开→校验→关→校验 |
| `setPressing(true/false)` | ctrl | 1 | 1/1 | 2312.262 | 2312.262 | 2312.262 | 2312.262 | 1 次 启动→动作 0.8 s→停止 |
| `setDemolding(true/false)` | ctrl | 2 | 0/2 | 46.304 | 46.304 | 46.304 | 46.304 | 固件未实现 0x1523 |
| `setStart(true/false)` | ctrl | 1 | 1/1 | 10872.891 | 10872.891 | 10872.891 | 10872.891 | 1 次 启动全部→0.5 s→停止 |
| `selectModelState(1<->2)` | ctrl | 1 | 1/1 | 1442.031 | 1442.031 | 1442.031 | 1442.031 | 切换后恢复原模式 |
| `isConnected` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `getMachineRegisterNo` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `isRunning` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `getLastErrorInfo` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `Press::version` | static | 50 | 50/50 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `Press::getPortList` | static | 5 | 5/5 | 0.324 | 0.924 | 0.367 | 3.184 | - |
| `getName` | get | 10 | 10/10 | 46.138 | 53.839 | 47.174 | 87.715 | - |
| `getMachineType` | get | 10 | 10/10 | 45.659 | 55.373 | 46.611 | 107.182 | - |
| `getSerialNumber` | get | 10 | 10/10 | 45.655 | 50.835 | 46.422 | 61.978 | - |
| `getPressParameter` | get | 10 | 10/10 | 46.030 | 49.432 | 46.437 | 61.962 | - |
| `getTempParameter` | get | 10 | 10/10 | 45.956 | 55.700 | 46.750 | 107.847 | - |
| `getOtherInfo` | get | 10 | 10/10 | 45.783 | 55.685 | 47.101 | 92.200 | - |
| `getReadOnlyDataJson` | get | 5 | 5/5 | 46.193 | 64.556 | 61.384 | 91.843 | - |
| `getPressDataJson` | get | 5 | 5/5 | 61.985 | 83.600 | 92.421 | 108.523 | - |
| `getRealTimeDataJson` | get | 5 | 5/5 | 46.045 | 55.614 | 46.535 | 77.099 | - |
| `getTempDataJson` | get | 5 | 5/5 | 62.063 | 86.592 | 93.022 | 107.611 | - |
| `getMixDataJson` | get | 5 | 5/5 | 153.649 | 164.115 | 154.883 | 201.924 | - |
| `getMachineState` | get | 20 | 20/20 | 45.963 | 52.628 | 46.645 | 109.000 | - |
| `getPressStepCount` | get | 10 | 10/10 | 45.897 | 55.777 | 46.616 | 108.297 | - |
| `getCalibrationValue` | get | 10 | 10/10 | 45.715 | 49.490 | 46.523 | 62.537 | - |
| `getMoldType` | get | 10 | 10/10 | 46.033 | 55.694 | 46.831 | 92.307 | - |
| `getCircle` | get | 10 | 10/10 | 45.490 | 50.717 | 46.642 | 61.743 | - |
| `getRectangle` | get | 10 | 10/10 | 46.112 | 54.379 | 46.641 | 93.801 | - |
| `getAirTime` | get | 10 | 10/10 | 46.539 | 49.722 | 46.675 | 62.079 | - |
| `getPressValueIndex` | get | 10 | 10/10 | 46.164 | 56.011 | 47.082 | 108.830 | - |
| `getAfterValueIndex` | get | 10 | 10/10 | 45.728 | 49.468 | 46.597 | 62.161 | - |
| `getKeepPressTime` | get | 10 | 10/10 | 45.894 | 55.574 | 46.758 | 92.820 | - |
| `getPressStepParameter` | get | 5 | 5/5 | 46.720 | 49.967 | 46.800 | 62.033 | - |
| `getTempStepCount` | get | 10 | 10/10 | 45.765 | 55.620 | 46.883 | 93.723 | - |
| `getTempSpeedMold` | get | 10 | 10/10 | 45.516 | 49.272 | 46.199 | 62.116 | - |
| `getStartTempIndex` | get | 10 | 10/10 | 46.052 | 55.832 | 46.669 | 108.450 | - |
| `getTempSpeedIndex` | get | 10 | 10/10 | 46.129 | 49.611 | 46.612 | 62.253 | - |
| `getKeepTempTime` | get | 10 | 10/10 | 45.722 | 55.669 | 46.941 | 93.263 | - |
| `getMaxWaterValue` | get | 10 | 10/10 | 45.736 | 50.877 | 46.536 | 61.827 | - |
| `getMinWaterValue` | get | 10 | 10/10 | 45.567 | 54.031 | 46.602 | 108.051 | - |
| `getTempStepParameter` | get | 5 | 5/5 | 45.596 | 52.291 | 46.265 | 61.866 | - |
| `getMixStepCount` | get | 10 | 10/10 | 45.935 | 54.012 | 46.582 | 92.352 | - |
| `getMixMoldType` | get | 10 | 10/10 | 46.020 | 50.861 | 46.498 | 61.580 | - |
| `getMixCircle` | get | 10 | 10/10 | 45.936 | 55.381 | 46.779 | 92.180 | - |
| `getMixRectangle` | get | 10 | 0/10 | 46.203 | 49.641 | 46.841 | 61.754 | 固件 0x0303 回调错误 |
| `getMixIsPressIndex` | get | 10 | 10/10 | 46.014 | 55.802 | 47.133 | 92.964 | - |
| `getMixPressIndex` | get | 10 | 10/10 | 45.907 | 49.451 | 46.471 | 62.869 | - |
| `getMixAfterPressIndex` | get | 10 | 10/10 | 46.034 | 55.639 | 46.493 | 93.794 | - |
| `getMixIsHeatUpIndex` | get | 10 | 10/10 | 45.564 | 49.355 | 46.440 | 61.720 | - |
| `getMixHeatUpIndex` | get | 10 | 10/10 | 46.066 | 55.686 | 47.078 | 92.892 | - |
| `getMixIsHeatDownIndex` | get | 10 | 10/10 | 45.823 | 49.570 | 46.500 | 62.468 | - |
| `getMixHeatDownIndex` | get | 10 | 10/10 | 45.963 | 55.673 | 46.444 | 108.857 | - |
| `getMixKeepTime` | get | 10 | 10/10 | 46.011 | 51.057 | 46.532 | 62.357 | - |
| `getMixStepParameter` | get | 5 | 5/5 | 45.569 | 58.374 | 46.244 | 107.523 | - |
| `getDateTime` | get | 10 | 10/10 | 45.877 | 51.088 | 46.630 | 62.702 | - |
| `getBrightness` | get | 10 | 10/10 | 45.916 | 54.130 | 46.715 | 93.250 | - |
| `getButtonSound` | get | 10 | 10/10 | 45.808 | 51.014 | 46.620 | 62.046 | - |
| `getLanguage` | get | 10 | 10/10 | 45.969 | 49.552 | 46.460 | 61.925 | - |
| `setPressStepCount` | set | 5 | 5/5 | 45.981 | 52.608 | 46.757 | 62.179 | 原值写回 |
| `setCalibrationValue` | set | 5 | 5/5 | 46.023 | 52.680 | 47.073 | 62.009 | 原值写回 |
| `setMoldType` | set | 5 | 5/5 | 46.547 | 64.868 | 61.319 | 107.874 | 原值写回 |
| `setCircle` | set | 5 | 5/5 | 45.833 | 52.554 | 46.675 | 62.106 | 原值写回 |
| `setRectangle` | set | 5 | 5/5 | 45.846 | 52.539 | 46.937 | 62.785 | 原值写回 |
| `setAirTime` | set | 5 | 5/5 | 46.164 | 61.738 | 61.748 | 92.223 | 原值写回 |
| `setPressValueIndex` | set | 5 | 5/5 | 45.888 | 52.436 | 46.510 | 61.801 | index=0 原值写回 |
| `setAfterValueIndex` | set | 5 | 5/5 | 45.850 | 52.487 | 46.880 | 61.710 | index=0 原值写回 |
| `setKeepPressTime` | set | 5 | 5/5 | 45.747 | 64.663 | 61.084 | 109.200 | index=0 原值写回 |
| `setPressStepParameter` | set | 5 | 5/5 | 45.755 | 52.662 | 46.282 | 62.781 | index=0 原值写回 |
| `setTempStepCount` | set | 5 | 5/5 | 45.972 | 52.603 | 46.804 | 62.146 | 原值写回 |
| `setTempSpeedMold` | set | 5 | 5/5 | 46.030 | 64.998 | 61.788 | 107.963 | 原值写回 |
| `setTempIndex` | set | 5 | 5/5 | 46.575 | 52.727 | 46.812 | 62.628 | index=0 原值写回 |
| `setTempSpeedIndex` | set | 5 | 5/5 | 45.893 | 52.684 | 46.178 | 63.240 | index=0 原值写回 |
| `setKeepTempTime` | set | 5 | 5/5 | 45.860 | 64.629 | 61.370 | 108.029 | index=0 原值写回 |
| `setMaxWaterValue` | set | 5 | 5/5 | 45.790 | 52.547 | 46.362 | 62.295 | 原值写回 |
| `setMinWaterValue` | set | 5 | 5/5 | 46.245 | 52.554 | 46.306 | 62.815 | 原值写回 |
| `setMixStepCount` | set | 5 | 5/5 | 46.411 | 64.986 | 61.477 | 93.814 | 原值写回 |
| `setMixMoldType` | set | 5 | 5/5 | 46.403 | 52.896 | 46.989 | 62.359 | 原值写回 |
| `setMixCircle` | set | 5 | 5/5 | 46.172 | 52.870 | 47.306 | 62.214 | 原值写回 |
| `setMixRectangle` | set | 5 | 0/5 | 45.913 | 58.639 | 46.557 | 108.172 | 读回失败：响应数据过短 |
| `setMixIsPressIndex` | set | 5 | 5/5 | 93.578 | 105.475 | 108.249 | 108.920 | index=0 原值写回 |
| `setMixPressIndex` | set | 5 | 5/5 | 46.099 | 55.545 | 61.760 | 61.824 | index=0 原值写回 |
| `setMixAfterPressIndex` | set | 5 | 5/5 | 46.055 | 55.539 | 61.357 | 62.103 | index=0 原值写回 |
| `setMixIsHeatUpIndex` | set | 5 | 5/5 | 91.745 | 101.418 | 106.813 | 108.535 | index=0 原值写回 |
| `setMixHeatUpIndex` | set | 5 | 5/5 | 45.941 | 62.176 | 62.508 | 93.271 | index=0 原值写回 |
| `setMixIsHeatDownIndex` | set | 5 | 5/5 | 92.577 | 105.007 | 107.532 | 109.467 | index=0 原值写回 |
| `setMixHeatDownIndex` | set | 5 | 5/5 | 46.189 | 65.156 | 61.540 | 109.056 | index=0 原值写回 |
| `setMixKeepTime` | set | 5 | 5/5 | 45.797 | 92.193 | 61.436 | 245.504 | index=0 原值写回 |
| `setTempStepParameter` | set | 5 | 5/5 | 46.331 | 61.994 | 62.076 | 92.598 | index=0 原值写回 |
| `setMixStepParameter` | set | 5 | 5/5 | 45.755 | 55.928 | 62.129 | 63.101 | index=0 原值写回 |
| `setDateTime` | set | 5 | 5/5 | 45.734 | 52.605 | 46.481 | 62.829 | 原值写回 |
| `setBrightness` | set | 5 | 5/5 | 45.857 | 49.490 | 46.824 | 61.630 | 原值写回 |
| `setButtonSound` | set | 5 | 5/5 | 45.817 | 49.380 | 46.368 | 62.252 | 原值写回 |
| `setLanguage` | set | 5 | 5/5 | 45.798 | 49.396 | 46.169 | 62.322 | 原值写回 |
| `setPressDataJson` | set | 3 | 3/3 | 107.606 | 144.159 | 108.413 | 216.459 | 原值写回 |
| `setTempDataJson` | set | 3 | 3/3 | 107.558 | 108.294 | 108.434 | 108.889 | 原值写回 |
| `setMixDataJson` | set | 3 | 3/3 | 215.938 | 248.031 | 217.519 | 310.637 | 原值写回 |

> 控制类（ctrl）命令包含状态轮询、动作等待与停止确认，单次耗时明显高于普通命令，属预期。
> `setDemolding` 因固件未实现 `CMDID_SET_START_DEMOLD`(0x1523) 而无法通过，仅记录实际往返耗时。
> 混合模式 `getMixRectangle` / `setMixRectangle` 因固件对 0x0303 返回错误（响应数据过短）而失败，耗时按实际往返记录。

## 启停接口分项测试（启动、停止各自单独调用）

- **测试时间**：2026-11-18
- **设备连接**：serial COM4（自动热压机，SN `ASD123`）
- **执行方式**：`press_cpp_test serial COM4 startstop`，每条启停接口的启动与停止各单独调用一次
- **原始帧追踪**：`PRESS_HEX_TRACE=1`（SDK 按需输出完整 TX/RX 帧，默认关闭）
- **工作模式**：测试开始时 mode=2，结束后恢复原模式

| 测试项 | 函数 | 参数 | 命令号 | 返回值 | 错误信息 | 耗时(ms) |
|---|---|---|---|---|---|---|
| 加热启动 | `setStartHeat` | `true` | `0x1522` | `true` | No error | 46.6 |
| 加热停止 | `setStartHeat` | `false` | `0x1522` | `true` | No error | 45.8 |
| 加压启动 | `setPressing` | `true` | `0x1521` | `true` | No error | 46.6 |
| 加压停止 | `setPressing` | `false` | `0x1521` | `true` | No error | 61.4 |
| 脱模启动 | `setDemolding` | `true` | `0x1523` | `false` | Command mismatch | 46.1 |
| 脱模停止 | `setDemolding` | `false` | `0x1523` | `false` | Command mismatch | 46.1 |
| 全流程启动 | `setStart` | `true` | `0x1511` | `true` | No error | 47.0 |
| 全流程停止 | `setStart` | `false` | `0x1511` | `true` | No error | 46.0 |

原始收发帧（注册号 `30 55 45 17 B4 A4 63 E4`，帧格式：注册号 8 + 命令 2 + 长度 2 + 数据 + 校验和 1）：

| 测试项 | TX 帧 | RX 帧 |
|---|---|---|
| 加热启动 | `30 55 45 17 B4 A4 63 E4 15 22 00 01 01 B9` | `30 55 45 17 B4 A4 63 E4 15 22 00 01 01 B9` |
| 加热停止 | `30 55 45 17 B4 A4 63 E4 15 22 00 01 00 B8` | `30 55 45 17 B4 A4 63 E4 15 22 00 01 00 B8` |
| 加压启动 | `30 55 45 17 B4 A4 63 E4 15 21 00 01 01 B8` | `30 55 45 17 B4 A4 63 E4 15 21 00 01 01 B8` |
| 加压停止 | `30 55 45 17 B4 A4 63 E4 15 21 00 01 00 B7` | `30 55 45 17 B4 A4 63 E4 15 21 00 01 00 B7` |
| 脱模启动 | `30 55 45 17 B4 A4 63 E4 15 23 00 01 01 BA` | `30 55 45 17 B4 A4 63 E4 FF FF 00 05 65 72 72 6F 72 AD` |
| 脱模停止 | `30 55 45 17 B4 A4 63 E4 15 23 00 01 00 B9` | `30 55 45 17 B4 A4 63 E4 FF FF 00 05 65 72 72 6F 72 AD` |
| 全流程启动 | `30 55 45 17 B4 A4 63 E4 15 11 00 01 01 A8` | `30 55 45 17 B4 A4 63 E4 15 11 00 01 01 A8` |
| 全流程停止 | `30 55 45 17 B4 A4 63 E4 15 11 00 01 00 A7` | `30 55 45 17 B4 A4 63 E4 15 11 00 01 00 A7` |

> 控制类回执的数据区为 1 字节状态码：`0x01`=启动成功、`0x00`=停止成功；与请求载荷一致。
> 脱模命令固件未实现：回执命令号为 `0xFFFF`，数据区为 ASCII `error`（`65 72 72 6F 72`），SDK 判定 `Command mismatch`。
> 每次调用后的 `0x0500` 状态轮询确认标志位变化：加热 `0x02`→`0x00`，加压 `0x01`→`0x00`。
> `setStart(true)` 返回成功，但 10 s 内未观察到活动状态位（启动延时 3 s 加升温阶段），随后 `setStart(false)` 成功且设备回到空闲。
> 8 次单调用耗时区间 45.8–61.4 ms，与普通命令往返量级一致；分项表只统计单次调用，不包含状态轮询与动作等待。

## 全接口逐调用追踪（参数 / 返回值 / 原始帧 hex）

- **测试时间**：2026-11-19
- **设备连接**：serial COM4（自动热压机，SN `ASD123`）
- **执行方式**：`press_cpp_test serial COM4 traceall`（每个接口调用 1 次，`PRESS_HEX_TRACE=1`）
- **结果**：100 次调用，2 次失败
- **帧统计**：共 246 组 TX/RX；下列帧清单保留 132 组业务命令帧，状态轮询 `0x0500` 已剔除（`getMachineState` 除外）

### 逐调用结果

| # | 接口 | 参数 | 返回值 | 引用返回 | 错误 | 耗时(ms) |
|---|---|---|---|---|---|---|
| 1 | `isConnected` | - | true | value=true | No error | 0.0 |
| 2 | `getMachineRegisterNo` | - | true | value=3482765854958183396 | No error | 0.0 |
| 3 | `isRunning` | - | true | value=true | No error | 0.0 |
| 4 | `getLastErrorInfo` | - | true | value=No error | No error | 0.0 |
| 5 | `Press::version` | - | true | value=1.0.0 | No error | 0.0 |
| 6 | `Press::getPortList` | - | true | count=1,first=COM4 | No error | 4.7 |
| 7 | `getName` | lang=0 | true | value=自动热压机 | No error | 41.9 |
| 8 | `getMachineType` | out=string | true | value=TYPE | No error | 45.9 |
| 9 | `getSerialNumber` | out=string | true | value=ASD123 | No error | 45.8 |
| 10 | `getPressParameter` | out=PressParameter | true | maxP=30.0,minP=0.5,maxStep=30 | No error | 62.2 |
| 11 | `getTempParameter` | out=TempParameter | true | maxT=310.0,minT=25.0,maxStep=30 | No error | 45.9 |
| 12 | `getOtherInfo` | out=OtherInfo | true | startDelay=3,remote=1 | No error | 47.6 |
| 13 | `getReadOnlyDataJson` | compressed=false | true | json=370B | No error | 77.8 |
| 14 | `getPressDataJson` | compressed=false | true | json=520B | No error | 62.0 |
| 15 | `getRealTimeDataJson` | compressed=false | true | json=224B | No error | 30.4 |
| 16 | `getTempDataJson` | compressed=false | true | json=494B | No error | 78.1 |
| 17 | `getMixDataJson` | compressed=false | true | json=1077B | No error | 155.0 |
| 18 | `getMachineState` | out=MachineState | true | flags=0x00,model=2 | No error | 61.6 |
| 19 | `getPressStepCount` | out=uint8 | true | value=3 | No error | 45.9 |
| 20 | `getCalibrationValue` | out=float | true | value=0.0 | No error | 46.3 |
| 21 | `getMoldType` | out=uint8 | true | value=1 | No error | 46.7 |
| 22 | `getCircle` | out=float | true | value=100.0 | No error | 46.0 |
| 23 | `getRectangle` | out=a,b | true | a=100.0,b=100.0 | No error | 61.3 |
| 24 | `getAirTime` | out=uint8 | true | value=1 | No error | 46.2 |
| 25 | `getPressValueIndex` | index=0 | true | value=1.0 | No error | 45.6 |
| 26 | `getAfterValueIndex` | index=0 | true | value=0.7 | No error | 61.3 |
| 27 | `getKeepPressTime` | index=0 | true | value=600 | No error | 46.1 |
| 28 | `getPressStepParameter` | index=0 | true | setP=1.0,afterP=0.7,kp=600 | No error | 46.4 |
| 29 | `getTempStepCount` | out=uint8 | true | value=5 | No error | 46.2 |
| 30 | `getTempSpeedMold` | out=uint8 | true | value=0 | No error | 61.2 |
| 31 | `getStartTempIndex` | index=0 | true | value=30.0 | No error | 46.5 |
| 32 | `getTempSpeedIndex` | index=0 | true | value=3.0 | No error | 46.3 |
| 33 | `getKeepTempTime` | index=0 | true | value=500 | No error | 45.9 |
| 34 | `getMaxWaterValue` | out=float | true | value=25.0 | No error | 61.6 |
| 35 | `getMinWaterValue` | out=float | true | value=25.0 | No error | 46.7 |
| 36 | `getTempStepParameter` | index=0 | true | temp=30.0,speed=3.0,kt=500 | No error | 46.6 |
| 37 | `getMixStepCount` | out=uint8 | true | value=3 | No error | 46.8 |
| 38 | `getMixMoldType` | out=uint8 | true | value=1 | No error | 46.9 |
| 39 | `getMixCircle` | out=float | true | value=100.0 | No error | 62.2 |
| 40 | `getMixRectangle` | out=a,b | true | a=100.0,b=100.0 | No error | 46.2 |
| 41 | `getMixIsPressIndex` | index=0 | true | value=false | No error | 46.5 |
| 42 | `getMixPressIndex` | index=0 | true | value=1.0 | No error | 46.9 |
| 43 | `getMixAfterPressIndex` | index=0 | true | value=1.0 | No error | 61.5 |
| 44 | `getMixIsHeatUpIndex` | index=0 | true | value=true | No error | 46.1 |
| 45 | `getMixHeatUpIndex` | index=0 | true | value=25.0 | No error | 46.1 |
| 46 | `getMixIsHeatDownIndex` | index=0 | true | value=true | No error | 46.1 |
| 47 | `getMixHeatDownIndex` | index=0 | true | value=25.0 | No error | 61.2 |
| 48 | `getMixKeepTime` | index=0 | true | value=600 | No error | 46.2 |
| 49 | `getMixStepParameter` | index=0 | true | isPress=0,setP=1.0,upTemp=25.0 | No error | 46.5 |
| 50 | `getDateTime` | out=DateTimeInfo | true | 2026-11-19 00:07:34 | No error | 61.3 |
| 51 | `getBrightness` | out=uint8 | true | value=79 | No error | 45.7 |
| 52 | `getButtonSound` | out=bool | true | value=false | No error | 45.8 |
| 53 | `getLanguage` | out=uint8 | true | value=1 | No error | 46.4 |
| 54 | `setPressStepCount` | value=3 (原值写回) | true | - | No error | 45.9 |
| 55 | `setCalibrationValue` | value=0.000 (原值写回) | true | - | No error | 61.7 |
| 56 | `setMoldType` | value=1 (原值写回) | true | - | No error | 46.2 |
| 57 | `setCircle` | value=100.000 (原值写回) | true | - | No error | 45.7 |
| 58 | `setRectangle` | a=100.0 b=100.0 (原值写回) | true | - | No error | 61.6 |
| 59 | `setAirTime` | value=1 (原值写回) | true | - | No error | 61.2 |
| 60 | `setPressValueIndex` | index=0 value=1.000 (原值写回) | true | - | No error | 46.4 |
| 61 | `setAfterValueIndex` | index=0 value=0.700 (原值写回) | true | - | No error | 47.0 |
| 62 | `setKeepPressTime` | index=0 value=600 (原值写回) | true | - | No error | 61.1 |
| 63 | `setPressStepParameter` | index=0 (原值写回) | true | - | No error | 45.8 |
| 64 | `setTempStepCount` | value=5 (原值写回) | true | - | No error | 47.3 |
| 65 | `setTempSpeedMold` | value=0 (原值写回) | true | - | No error | 61.9 |
| 66 | `setTempIndex` | index=0 value=30.000 (原值写回) | true | - | No error | 46.2 |
| 67 | `setTempSpeedIndex` | index=0 value=3.000 (原值写回) | true | - | No error | 61.2 |
| 68 | `setKeepTempTime` | index=0 value=500 (原值写回) | true | - | No error | 61.9 |
| 69 | `setMaxWaterValue` | value=25.000 (原值写回) | true | - | No error | 46.3 |
| 70 | `setMinWaterValue` | value=25.000 (原值写回) | true | - | No error | 61.3 |
| 71 | `setMixStepCount` | value=3 (原值写回) | true | - | No error | 61.7 |
| 72 | `setMixMoldType` | value=1 (原值写回) | true | - | No error | 46.1 |
| 73 | `setMixCircle` | value=100.000 (原值写回) | true | - | No error | 46.2 |
| 74 | `setMixRectangle` | a=100.0 b=100.0 (原值写回) | true | - | No error | 62.2 |
| 75 | `setMixIsPressIndex` | index=0 value=false | true | - | No error | 62.2 |
| 76 | `setMixPressIndex` | index=0 value=1.000 (原值写回) | true | - | No error | 46.7 |
| 77 | `setMixAfterPressIndex` | index=0 value=1.000 (原值写回) | true | - | No error | 45.7 |
| 78 | `setMixIsHeatUpIndex` | index=0 value=true | true | - | No error | 61.7 |
| 79 | `setMixHeatUpIndex` | index=0 value=25.000 (原值写回) | true | - | No error | 61.7 |
| 80 | `setMixIsHeatDownIndex` | index=0 value=true | true | - | No error | 62.4 |
| 81 | `setMixHeatDownIndex` | index=0 value=25.000 (原值写回) | true | - | No error | 46.7 |
| 82 | `setMixKeepTime` | index=0 value=600 (原值写回) | true | - | No error | 46.0 |
| 83 | `setTempStepParameter` | index=0 (原值写回) | true | - | No error | 61.9 |
| 84 | `setMixStepParameter` | index=0 (原值写回) | true | - | No error | 46.5 |
| 85 | `setDateTime` | 原值写回 | true | - | No error | 45.9 |
| 86 | `setBrightness` | value=79 (原值写回) | true | - | No error | 61.2 |
| 87 | `setButtonSound` | value=false (原值写回) | true | - | No error | 46.5 |
| 88 | `setLanguage` | value=1 (原值写回) | true | - | No error | 46.6 |
| 89 | `setPressDataJson` | json=520B (原值写回) | true | - | No error | 108.3 |
| 90 | `setTempDataJson` | json=494B (原值写回) | true | - | No error | 154.4 |
| 91 | `setMixDataJson` | json=1077B (原值写回) | true | - | No error | 214.6 |
| 92 | `setStartHeat` | true | true | - | No error | 46.3 |
| 93 | `setStartHeat` | false | true | - | No error | 61.6 |
| 94 | `setPressing` | true | true | - | No error | 46.9 |
| 95 | `setPressing` | false | true | - | No error | 46.6 |
| 96 | `setDemolding` | true | false | - | Command mismatch | 46.6 |
| 97 | `setDemolding` | false | false | - | Command mismatch | 61.5 |
| 98 | `setStart` | true | true | - | No error | 46.1 |
| 99 | `setStart` | false | true | - | No error | 31.3 |
| 100 | `selectModelState` | target=2 (恢复原模式) | true | - | No error | 61.5 |

### 原始收发帧（hex）

```text
[getName]
TX cmd=0x0001: 30 55 45 17 B4 A4 63 E4 00 01 00 01 00 82
RX cmd=0x0001: 30 55 45 17 B4 A4 63 E4 00 01 00 10 01 E8 87 AA E5 8A A8 E7 83 AD E5 8E 8B E6 9C BA 13

[getMachineType]
TX cmd=0x0002: 30 55 45 17 B4 A4 63 E4 00 02 00 01 00 83
RX cmd=0x0002: 30 55 45 17 B4 A4 63 E4 00 02 00 04 54 59 50 45 C8

[getSerialNumber]
TX cmd=0x0003: 30 55 45 17 B4 A4 63 E4 00 03 00 01 00 84
RX cmd=0x0003: 30 55 45 17 B4 A4 63 E4 00 03 00 06 41 53 44 31 32 33 F7

[getPressParameter]
TX cmd=0x0004: 30 55 45 17 B4 A4 63 E4 00 04 00 01 00 85
RX cmd=0x0004: 30 55 45 17 B4 A4 63 E4 00 04 00 15 00 00 F0 41 00 00 00 3F 9A 99 99 3E 00 00 DC 42 1E 01 02 00 00 52

[getTempParameter]
TX cmd=0x0005: 30 55 45 17 B4 A4 63 E4 00 05 00 01 00 86
RX cmd=0x0005: 30 55 45 17 B4 A4 63 E4 00 05 00 0C 00 00 9B 43 00 00 C8 41 1E 01 01 00 98

[getOtherInfo]
TX cmd=0x0006: 30 55 45 17 B4 A4 63 E4 00 06 00 01 00 87
RX cmd=0x0006: 30 55 45 17 B4 A4 63 E4 00 06 00 05 00 03 00 01 02 91

[getReadOnlyDataJson]
TX cmd=0x00FF: 30 55 45 17 B4 A4 63 E4 00 FF 00 01 00 80
RX cmd=0x00FF: 30 55 45 17 B4 A4 63 E4 00 FF 01 72 7B 22 4E 61 6D 65 5A 48 22 3A 22 22 2C 22 4E 61 6D 65 45 4E 22 3A 22 22 2C 22 54 79 70 65 22 3A 22 54 59 50 45 22 2C 22 53 63 72 65 65 6E 73 68 6F 74 22 3A 30 2C 22 53 74 61 72 74 44 65 6C 61 79 22 3A 33 2C 22 56 65 72 73 69 6F 6E 54 79 70 65 22 3A 32 2C 22 53 65 72 69 61 6C 4E 75 6D 62 65 72 22 3A 22 41 53 44 31 32 33 22 2C 22 49 73 48 69 64 65 4C 61 6E 67 22 3A 30 2C 22 52 65 6D 6F 74 65 22 3A 31 2C 22 4E 65 74 77 6F 72 6B 22 3A 30 2C 22 4D 61 78 50 53 74 65 70 22 3A 33 30 2C 22 4D 61 78 50 4C 69 6D 69 74 22 3A 33 30 2E 30 2C 22 4D 69 6E 50 4C 69 6D 69 74 22 3A 30 2E 35 2C 22 4D 61 78 5F 4D 69 6E 22 3A 30 2E 33 2C 22 44 69 61 6D 65 74 65 72 22 3A 31 31 30 2E 30 2C 22 50 44 65 63 69 6D 61 6C 22 3A 31 2C 22 50 72 65 73 73 44 65 63 69 6D 61 6C 22 3A 32 2C 22 50 4D 6F 64 65 6C 22 3A 30 2C 22 4F 75 74 54 79 70 65 22 3A 30 2C 22 4D 61 78 54 53 74 65 70 22 3A 33 30 2C 22 4D 61 78 54 4C 69 6D 69 74 22 3A 33 31 30 2E 30 2C 22 4D 69 6E 54 4C 69 6D 69 74 22 3A 32 35 2E 30 2C 22 54 44 65 63 69 6D 61 6C 22 3A 31 2C 22 49 73 48 61 73 57 61 74 65 72 22 3A 31 2C 22 49 73 48 61 73 53 70 65 65 64 22 3A 30 7D 89

[getPressDataJson]
TX cmd=0x01FF: 30 55 45 17 B4 A4 63 E4 01 FF 00 01 00 81
RX cmd=0x01FF: 30 55 45 17 B4 A4 63 E4 01 FF 02 08 7B 22 50 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 43 68 65 63 6B 56 61 6C 75 65 22 3A 30 2E 30 2C 22 41 69 72 54 69 6D 65 22 3A 31 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 30 2E 37 2C 31 2E 37 2C 32 2E 37 2C 33 2E 37 2C 34 2E 37 2C 35 2E 37 2C 36 2E 37 2C 37 2E 37 2C 38 2E 37 2C 39 2E 37 2C 31 30 2E 37 2C 31 31 2E 37 2C 31 32 2E 37 2C 31 33 2E 37 2C 31 34 2E 37 2C 31 35 2E 37 2C 31 36 2E 37 2C 31 37 2E 37 2C 31 38 2E 37 2C 31 39 2E 37 2C 32 30 2E 37 2C 32 31 2E 37 2C 32 32 2E 37 2C 32 33 2E 37 2C 32 34 2E 37 2C 32 35 2E 37 2C 32 36 2E 37 2C 32 37 2E 37 2C 32 38 2E 37 2C 32 39 2E 37 5D 2C 22 4B 50 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 1E

[getRealTimeDataJson]
TX cmd=0x05FF: 30 55 45 17 B4 A4 63 E4 05 FF 00 01 00 85
RX cmd=0x05FF: 30 55 45 17 B4 A4 63 E4 05 FF 00 E0 7B 22 4D 6F 64 65 6C 53 74 61 74 65 22 3A 32 2C 22 49 73 50 53 74 61 72 74 22 3A 30 2C 22 49 73 54 53 74 61 72 74 22 3A 30 2C 22 49 73 54 31 53 74 61 72 74 22 3A 30 2C 22 43 50 53 74 65 70 22 3A 30 2C 22 43 54 53 74 65 70 22 3A 30 2C 22 43 54 31 53 74 65 70 22 3A 30 2C 22 50 56 61 6C 75 65 22 3A 38 2E 39 30 35 2C 22 4D 6F 6C 64 50 72 65 73 73 75 72 65 22 3A 30 2E 31 32 30 32 2C 22 54 55 70 56 61 6C 75 65 22 3A 32 33 2E 32 39 2C 22 54 44 6F 77 6E 56 61 6C 75 65 22 3A 30 2E 34 35 36 37 2C 22 50 54 69 6D 65 22 3A 36 30 30 2C 22 54 54 69 6D 65 22 3A 35 30 30 2C 22 54 31 54 69 6D 65 22 3A 30 2C 22 49 73 41 69 72 22 3A 30 2C 22 46 6C 6F 77 43 68 61 6E 67 65 22 3A 30 7D 32

[getTempDataJson]
TX cmd=0x02FF: 30 55 45 17 B4 A4 63 E4 02 FF 00 01 00 82
RX cmd=0x02FF: 30 55 45 17 B4 A4 63 E4 02 FF 01 EE 7B 22 54 53 74 65 70 22 3A 35 2C 22 54 79 70 65 22 3A 30 2C 22 4D 69 6E 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 4D 61 78 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 53 74 61 72 74 54 65 6D 70 22 3A 5B 33 30 2E 30 2C 33 35 2E 30 2C 34 30 2E 30 2C 34 35 2E 30 2C 35 30 2E 30 2C 35 35 2E 30 2C 36 30 2E 30 2C 36 35 2E 30 2C 37 30 2E 30 2C 37 35 2E 30 2C 38 30 2E 30 2C 38 35 2E 30 2C 39 30 2E 30 2C 39 35 2E 30 2C 31 30 30 2E 30 2C 31 30 35 2E 30 2C 31 31 30 2E 30 2C 31 31 35 2E 30 2C 31 32 30 2E 30 2C 31 32 35 2E 30 2C 31 33 30 2E 30 2C 31 33 35 2E 30 2C 31 34 30 2E 30 2C 31 34 35 2E 30 2C 31 35 30 2E 30 2C 31 35 35 2E 30 2C 31 36 30 2E 30 2C 31 36 35 2E 30 2C 31 37 30 2E 30 2C 31 37 35 2E 30 5D 2C 22 53 70 65 65 64 22 3A 5B 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 5D 2C 22 4B 54 54 69 6D 65 22 3A 5B 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 5D 7D 0A 3B

[getMixDataJson]
TX cmd=0x03FF: 30 55 45 17 B4 A4 63 E4 03 FF 00 01 00 83
RX cmd=0x03FF: 30 55 45 17 B4 A4 63 E4 03 FF 04 35 7B 22 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 49 73 50 72 65 73 73 22 3A 5B 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 5D 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 49 73 55 70 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 55 70 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 49 73 44 6F 77 6E 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 44 6F 77 6E 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 4B 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 0A 5D

[getMachineState]
TX cmd=0x0500: 30 55 45 17 B4 A4 63 E4 05 00 00 01 00 86
RX cmd=0x0500: 30 55 45 17 B4 A4 63 E4 05 00 00 19 02 00 00 00 25 62 FD 40 1D 48 BA 41 98 D1 E9 3E 58 02 00 00 F4 01 00 00 00 A3

[getPressStepCount]
TX cmd=0x0100: 30 55 45 17 B4 A4 63 E4 01 00 00 01 00 82
RX cmd=0x0100: 30 55 45 17 B4 A4 63 E4 01 00 00 01 03 85

[getCalibrationValue]
TX cmd=0x0101: 30 55 45 17 B4 A4 63 E4 01 01 00 01 00 83
RX cmd=0x0101: 30 55 45 17 B4 A4 63 E4 01 01 00 04 00 00 00 00 86

[getMoldType]
TX cmd=0x0102: 30 55 45 17 B4 A4 63 E4 01 02 00 01 00 84
RX cmd=0x0102: 30 55 45 17 B4 A4 63 E4 01 02 00 01 01 85

[getCircle]
TX cmd=0x0103: 30 55 45 17 B4 A4 63 E4 01 03 00 01 00 85
RX cmd=0x0103: 30 55 45 17 B4 A4 63 E4 01 03 00 04 00 00 C8 42 92

[getRectangle]
TX cmd=0x0104: 30 55 45 17 B4 A4 63 E4 01 04 00 01 00 86
RX cmd=0x0104: 30 55 45 17 B4 A4 63 E4 01 04 00 08 00 00 C8 42 00 00 C8 42 A1

[getAirTime]
TX cmd=0x0106: 30 55 45 17 B4 A4 63 E4 01 06 00 01 00 88
RX cmd=0x0106: 30 55 45 17 B4 A4 63 E4 01 06 00 01 01 89

[getPressValueIndex]
TX cmd=0x0107: 30 55 45 17 B4 A4 63 E4 01 07 00 01 00 89
RX cmd=0x0107: 30 55 45 17 B4 A4 63 E4 01 07 00 05 00 00 00 80 3F 4C

[getAfterValueIndex]
TX cmd=0x0108: 30 55 45 17 B4 A4 63 E4 01 08 00 01 00 8A
RX cmd=0x0108: 30 55 45 17 B4 A4 63 E4 01 08 00 05 00 33 33 33 3F 66

[getKeepPressTime]
TX cmd=0x0109: 30 55 45 17 B4 A4 63 E4 01 09 00 01 00 8B
RX cmd=0x0109: 30 55 45 17 B4 A4 63 E4 01 09 00 05 00 58 02 00 00 E9

[getPressStepParameter]
TX cmd=0x010A: 30 55 45 17 B4 A4 63 E4 01 0A 00 01 00 8C
RX cmd=0x010A: 30 55 45 17 B4 A4 63 E4 01 0A 00 0D 00 00 00 80 3F 33 33 33 3F 58 02 00 00 89

[getTempStepCount]
TX cmd=0x0200: 30 55 45 17 B4 A4 63 E4 02 00 00 01 00 83
RX cmd=0x0200: 30 55 45 17 B4 A4 63 E4 02 00 00 01 05 88

[getTempSpeedMold]
TX cmd=0x0201: 30 55 45 17 B4 A4 63 E4 02 01 00 01 00 84
RX cmd=0x0201: 30 55 45 17 B4 A4 63 E4 02 01 00 01 00 84

[getStartTempIndex]
TX cmd=0x0202: 30 55 45 17 B4 A4 63 E4 02 02 00 01 00 85
RX cmd=0x0202: 30 55 45 17 B4 A4 63 E4 02 02 00 05 00 00 00 F0 41 BA

[getTempSpeedIndex]
TX cmd=0x0203: 30 55 45 17 B4 A4 63 E4 02 03 00 01 00 86
RX cmd=0x0203: 30 55 45 17 B4 A4 63 E4 02 03 00 05 00 00 00 40 40 0A

[getKeepTempTime]
TX cmd=0x0204: 30 55 45 17 B4 A4 63 E4 02 04 00 01 00 87
RX cmd=0x0204: 30 55 45 17 B4 A4 63 E4 02 04 00 05 00 F4 01 00 00 80

[getMaxWaterValue]
TX cmd=0x0206: 30 55 45 17 B4 A4 63 E4 02 06 00 01 00 89
RX cmd=0x0206: 30 55 45 17 B4 A4 63 E4 02 06 00 04 00 00 C8 41 95

[getMinWaterValue]
TX cmd=0x0207: 30 55 45 17 B4 A4 63 E4 02 07 00 01 00 8A
RX cmd=0x0207: 30 55 45 17 B4 A4 63 E4 02 07 00 04 00 00 C8 41 96

[getTempStepParameter]
TX cmd=0x0205: 30 55 45 17 B4 A4 63 E4 02 05 00 01 00 88
RX cmd=0x0205: 30 55 45 17 B4 A4 63 E4 02 05 00 0D 00 00 00 F0 41 00 00 40 40 F4 01 00 00 3A

[getMixStepCount]
TX cmd=0x0300: 30 55 45 17 B4 A4 63 E4 03 00 00 01 00 84
RX cmd=0x0300: 30 55 45 17 B4 A4 63 E4 03 00 00 01 03 87

[getMixMoldType]
TX cmd=0x0301: 30 55 45 17 B4 A4 63 E4 03 01 00 01 00 85
RX cmd=0x0301: 30 55 45 17 B4 A4 63 E4 03 01 00 01 01 86

[getMixCircle]
TX cmd=0x0302: 30 55 45 17 B4 A4 63 E4 03 02 00 01 00 86
RX cmd=0x0302: 30 55 45 17 B4 A4 63 E4 03 02 00 04 00 00 C8 42 93

[getMixRectangle]
TX cmd=0x0303: 30 55 45 17 B4 A4 63 E4 03 03 00 01 00 87
RX cmd=0x0303: 30 55 45 17 B4 A4 63 E4 03 03 00 08 00 00 C8 42 00 00 C8 42 A2

[getMixIsPressIndex]
TX cmd=0x0305: 30 55 45 17 B4 A4 63 E4 03 05 00 01 00 89
RX cmd=0x0305: 30 55 45 17 B4 A4 63 E4 03 05 00 02 00 00 8A

[getMixPressIndex]
TX cmd=0x0306: 30 55 45 17 B4 A4 63 E4 03 06 00 01 00 8A
RX cmd=0x0306: 30 55 45 17 B4 A4 63 E4 03 06 00 05 00 00 00 80 3F 4D

[getMixAfterPressIndex]
TX cmd=0x0307: 30 55 45 17 B4 A4 63 E4 03 07 00 01 00 8B
RX cmd=0x0307: 30 55 45 17 B4 A4 63 E4 03 07 00 05 00 00 00 80 3F 4E

[getMixIsHeatUpIndex]
TX cmd=0x0308: 30 55 45 17 B4 A4 63 E4 03 08 00 01 00 8C
RX cmd=0x0308: 30 55 45 17 B4 A4 63 E4 03 08 00 02 00 01 8E

[getMixHeatUpIndex]
TX cmd=0x0309: 30 55 45 17 B4 A4 63 E4 03 09 00 01 00 8D
RX cmd=0x0309: 30 55 45 17 B4 A4 63 E4 03 09 00 05 00 00 00 C8 41 9A

[getMixIsHeatDownIndex]
TX cmd=0x030A: 30 55 45 17 B4 A4 63 E4 03 0A 00 01 00 8E
RX cmd=0x030A: 30 55 45 17 B4 A4 63 E4 03 0A 00 02 00 01 90

[getMixHeatDownIndex]
TX cmd=0x030B: 30 55 45 17 B4 A4 63 E4 03 0B 00 01 00 8F
RX cmd=0x030B: 30 55 45 17 B4 A4 63 E4 03 0B 00 05 00 00 00 C8 41 9C

[getMixKeepTime]
TX cmd=0x030C: 30 55 45 17 B4 A4 63 E4 03 0C 00 01 00 90
RX cmd=0x030C: 30 55 45 17 B4 A4 63 E4 03 0C 00 05 00 58 02 00 00 EE

[getMixStepParameter]
TX cmd=0x030D: 30 55 45 17 B4 A4 63 E4 03 0D 00 01 00 91
RX cmd=0x030D: 30 55 45 17 B4 A4 63 E4 03 0D 00 18 00 00 00 00 80 3F 00 00 80 3F 01 00 00 C8 41 01 00 00 C8 41 58 02 00 00 94

[getDateTime]
TX cmd=0x0400: 30 55 45 17 B4 A4 63 E4 04 00 00 01 00 85
RX cmd=0x0400: 30 55 45 17 B4 A4 63 E4 04 00 00 06 22 07 00 13 0B 7E 4F

[getBrightness]
TX cmd=0x0401: 30 55 45 17 B4 A4 63 E4 04 01 00 01 00 86
RX cmd=0x0401: 30 55 45 17 B4 A4 63 E4 04 01 00 01 4F D5

[getButtonSound]
TX cmd=0x0402: 30 55 45 17 B4 A4 63 E4 04 02 00 01 00 87
RX cmd=0x0402: 30 55 45 17 B4 A4 63 E4 04 02 00 01 00 87

[getLanguage]
TX cmd=0x0403: 30 55 45 17 B4 A4 63 E4 04 03 00 01 00 88
RX cmd=0x0403: 30 55 45 17 B4 A4 63 E4 04 03 00 01 01 89

[setPressStepCount]
TX cmd=0x0100: 30 55 45 17 B4 A4 63 E4 01 00 00 01 00 82
RX cmd=0x0100: 30 55 45 17 B4 A4 63 E4 01 00 00 01 03 85
TX cmd=0x1100: 30 55 45 17 B4 A4 63 E4 11 00 00 01 03 95
RX cmd=0x1100: 30 55 45 17 B4 A4 63 E4 11 00 00 01 03 95

[setCalibrationValue]
TX cmd=0x0101: 30 55 45 17 B4 A4 63 E4 01 01 00 01 00 83
RX cmd=0x0101: 30 55 45 17 B4 A4 63 E4 01 01 00 04 00 00 00 00 86
TX cmd=0x1101: 30 55 45 17 B4 A4 63 E4 11 01 00 04 00 00 00 00 96
RX cmd=0x1101: 30 55 45 17 B4 A4 63 E4 11 01 00 04 00 00 00 00 96

[setMoldType]
TX cmd=0x0102: 30 55 45 17 B4 A4 63 E4 01 02 00 01 00 84
RX cmd=0x0102: 30 55 45 17 B4 A4 63 E4 01 02 00 01 01 85
TX cmd=0x1102: 30 55 45 17 B4 A4 63 E4 11 02 00 01 01 95
RX cmd=0x1102: 30 55 45 17 B4 A4 63 E4 11 02 00 01 01 95

[setCircle]
TX cmd=0x0103: 30 55 45 17 B4 A4 63 E4 01 03 00 01 00 85
RX cmd=0x0103: 30 55 45 17 B4 A4 63 E4 01 03 00 04 00 00 C8 42 92
TX cmd=0x1103: 30 55 45 17 B4 A4 63 E4 11 03 00 04 00 00 C8 42 A2
RX cmd=0x1103: 30 55 45 17 B4 A4 63 E4 11 03 00 04 00 00 C8 42 A2

[setRectangle]
TX cmd=0x0104: 30 55 45 17 B4 A4 63 E4 01 04 00 01 00 86
RX cmd=0x0104: 30 55 45 17 B4 A4 63 E4 01 04 00 08 00 00 C8 42 00 00 C8 42 A1
TX cmd=0x1104: 30 55 45 17 B4 A4 63 E4 11 04 00 08 00 00 C8 42 00 00 C8 42 B1
RX cmd=0x1104: 30 55 45 17 B4 A4 63 E4 11 04 00 08 00 00 C8 42 00 00 C8 42 B1

[setAirTime]
TX cmd=0x0106: 30 55 45 17 B4 A4 63 E4 01 06 00 01 00 88
RX cmd=0x0106: 30 55 45 17 B4 A4 63 E4 01 06 00 01 01 89
TX cmd=0x1106: 30 55 45 17 B4 A4 63 E4 11 06 00 01 01 99
RX cmd=0x1106: 30 55 45 17 B4 A4 63 E4 11 06 00 01 01 99

[setPressValueIndex]
TX cmd=0x0107: 30 55 45 17 B4 A4 63 E4 01 07 00 01 00 89
RX cmd=0x0107: 30 55 45 17 B4 A4 63 E4 01 07 00 05 00 00 00 80 3F 4C
TX cmd=0x1107: 30 55 45 17 B4 A4 63 E4 11 07 00 05 00 00 00 80 3F 5C
RX cmd=0x1107: 30 55 45 17 B4 A4 63 E4 11 07 00 05 00 00 00 80 3F 5C

[setAfterValueIndex]
TX cmd=0x0108: 30 55 45 17 B4 A4 63 E4 01 08 00 01 00 8A
RX cmd=0x0108: 30 55 45 17 B4 A4 63 E4 01 08 00 05 00 33 33 33 3F 66
TX cmd=0x1108: 30 55 45 17 B4 A4 63 E4 11 08 00 05 00 33 33 33 3F 76
RX cmd=0x1108: 30 55 45 17 B4 A4 63 E4 11 08 00 05 00 33 33 33 3F 76

[setKeepPressTime]
TX cmd=0x0109: 30 55 45 17 B4 A4 63 E4 01 09 00 01 00 8B
RX cmd=0x0109: 30 55 45 17 B4 A4 63 E4 01 09 00 05 00 58 02 00 00 E9
TX cmd=0x1109: 30 55 45 17 B4 A4 63 E4 11 09 00 05 00 58 02 00 00 F9
RX cmd=0x1109: 30 55 45 17 B4 A4 63 E4 11 09 00 05 00 58 02 00 00 F9

[setPressStepParameter]
TX cmd=0x010A: 30 55 45 17 B4 A4 63 E4 01 0A 00 01 00 8C
RX cmd=0x010A: 30 55 45 17 B4 A4 63 E4 01 0A 00 0D 00 00 00 80 3F 33 33 33 3F 58 02 00 00 89
TX cmd=0x110A: 30 55 45 17 B4 A4 63 E4 11 0A 00 0D 00 00 00 80 3F 33 33 33 3F 58 02 00 00 99
RX cmd=0x110A: 30 55 45 17 B4 A4 63 E4 11 0A 00 0D 00 00 00 80 3F 33 33 33 3F 58 02 00 00 99

[setTempStepCount]
TX cmd=0x0200: 30 55 45 17 B4 A4 63 E4 02 00 00 01 00 83
RX cmd=0x0200: 30 55 45 17 B4 A4 63 E4 02 00 00 01 05 88
TX cmd=0x1200: 30 55 45 17 B4 A4 63 E4 12 00 00 01 05 98
RX cmd=0x1200: 30 55 45 17 B4 A4 63 E4 12 00 00 01 05 98

[setTempSpeedMold]
TX cmd=0x0201: 30 55 45 17 B4 A4 63 E4 02 01 00 01 00 84
RX cmd=0x0201: 30 55 45 17 B4 A4 63 E4 02 01 00 01 00 84
TX cmd=0x1201: 30 55 45 17 B4 A4 63 E4 12 01 00 01 00 94
RX cmd=0x1201: 30 55 45 17 B4 A4 63 E4 12 01 00 01 00 94

[setTempIndex]
TX cmd=0x0202: 30 55 45 17 B4 A4 63 E4 02 02 00 01 00 85
RX cmd=0x0202: 30 55 45 17 B4 A4 63 E4 02 02 00 05 00 00 00 F0 41 BA
TX cmd=0x1202: 30 55 45 17 B4 A4 63 E4 12 02 00 05 00 00 00 F0 41 CA
RX cmd=0x1202: 30 55 45 17 B4 A4 63 E4 12 02 00 05 00 00 00 F0 41 CA

[setTempSpeedIndex]
TX cmd=0x0203: 30 55 45 17 B4 A4 63 E4 02 03 00 01 00 86
RX cmd=0x0203: 30 55 45 17 B4 A4 63 E4 02 03 00 05 00 00 00 40 40 0A
TX cmd=0x1203: 30 55 45 17 B4 A4 63 E4 12 03 00 05 00 00 00 40 40 1A
RX cmd=0x1203: 30 55 45 17 B4 A4 63 E4 12 03 00 05 00 00 00 40 40 1A

[setKeepTempTime]
TX cmd=0x0204: 30 55 45 17 B4 A4 63 E4 02 04 00 01 00 87
RX cmd=0x0204: 30 55 45 17 B4 A4 63 E4 02 04 00 05 00 F4 01 00 00 80
TX cmd=0x1204: 30 55 45 17 B4 A4 63 E4 12 04 00 05 00 F4 01 00 00 90
RX cmd=0x1204: 30 55 45 17 B4 A4 63 E4 12 04 00 05 00 F4 01 00 00 90

[setMaxWaterValue]
TX cmd=0x0206: 30 55 45 17 B4 A4 63 E4 02 06 00 01 00 89
RX cmd=0x0206: 30 55 45 17 B4 A4 63 E4 02 06 00 04 00 00 C8 41 95
TX cmd=0x1206: 30 55 45 17 B4 A4 63 E4 12 06 00 04 00 00 C8 41 A5
RX cmd=0x1206: 30 55 45 17 B4 A4 63 E4 12 06 00 04 00 00 C8 41 A5

[setMinWaterValue]
TX cmd=0x0207: 30 55 45 17 B4 A4 63 E4 02 07 00 01 00 8A
RX cmd=0x0207: 30 55 45 17 B4 A4 63 E4 02 07 00 04 00 00 C8 41 96
TX cmd=0x1207: 30 55 45 17 B4 A4 63 E4 12 07 00 04 00 00 C8 41 A6
RX cmd=0x1207: 30 55 45 17 B4 A4 63 E4 12 07 00 04 00 00 C8 41 A6

[setMixStepCount]
TX cmd=0x0300: 30 55 45 17 B4 A4 63 E4 03 00 00 01 00 84
RX cmd=0x0300: 30 55 45 17 B4 A4 63 E4 03 00 00 01 03 87
TX cmd=0x1300: 30 55 45 17 B4 A4 63 E4 13 00 00 01 03 97
RX cmd=0x1300: 30 55 45 17 B4 A4 63 E4 13 00 00 01 03 97

[setMixMoldType]
TX cmd=0x0301: 30 55 45 17 B4 A4 63 E4 03 01 00 01 00 85
RX cmd=0x0301: 30 55 45 17 B4 A4 63 E4 03 01 00 01 01 86
TX cmd=0x1301: 30 55 45 17 B4 A4 63 E4 13 01 00 01 01 96
RX cmd=0x1301: 30 55 45 17 B4 A4 63 E4 13 01 00 01 01 96

[setMixCircle]
TX cmd=0x0302: 30 55 45 17 B4 A4 63 E4 03 02 00 01 00 86
RX cmd=0x0302: 30 55 45 17 B4 A4 63 E4 03 02 00 04 00 00 C8 42 93
TX cmd=0x1302: 30 55 45 17 B4 A4 63 E4 13 02 00 04 00 00 C8 42 A3
RX cmd=0x1302: 30 55 45 17 B4 A4 63 E4 13 02 00 04 00 00 C8 42 A3

[setMixRectangle]
TX cmd=0x0303: 30 55 45 17 B4 A4 63 E4 03 03 00 01 00 87
RX cmd=0x0303: 30 55 45 17 B4 A4 63 E4 03 03 00 08 00 00 C8 42 00 00 C8 42 A2
TX cmd=0x1303: 30 55 45 17 B4 A4 63 E4 13 03 00 08 00 00 C8 42 00 00 C8 42 B2
RX cmd=0x1303: 30 55 45 17 B4 A4 63 E4 13 03 00 08 00 00 C8 42 00 00 C8 42 B2

[setMixIsPressIndex]
TX cmd=0x0305: 30 55 45 17 B4 A4 63 E4 03 05 00 01 00 89
RX cmd=0x0305: 30 55 45 17 B4 A4 63 E4 03 05 00 02 00 00 8A
TX cmd=0x1305: 30 55 45 17 B4 A4 63 E4 13 05 00 02 00 00 9A
RX cmd=0x1305: 30 55 45 17 B4 A4 63 E4 13 05 00 02 00 00 9A

[setMixPressIndex]
TX cmd=0x0306: 30 55 45 17 B4 A4 63 E4 03 06 00 01 00 8A
RX cmd=0x0306: 30 55 45 17 B4 A4 63 E4 03 06 00 05 00 00 00 80 3F 4D
TX cmd=0x1306: 30 55 45 17 B4 A4 63 E4 13 06 00 05 00 00 00 80 3F 5D
RX cmd=0x1306: 30 55 45 17 B4 A4 63 E4 13 06 00 05 00 00 00 80 3F 5D

[setMixAfterPressIndex]
TX cmd=0x0307: 30 55 45 17 B4 A4 63 E4 03 07 00 01 00 8B
RX cmd=0x0307: 30 55 45 17 B4 A4 63 E4 03 07 00 05 00 00 00 80 3F 4E
TX cmd=0x1307: 30 55 45 17 B4 A4 63 E4 13 07 00 05 00 00 00 80 3F 5E
RX cmd=0x1307: 30 55 45 17 B4 A4 63 E4 13 07 00 05 00 00 00 80 3F 5E

[setMixIsHeatUpIndex]
TX cmd=0x0308: 30 55 45 17 B4 A4 63 E4 03 08 00 01 00 8C
RX cmd=0x0308: 30 55 45 17 B4 A4 63 E4 03 08 00 02 00 01 8E
TX cmd=0x1308: 30 55 45 17 B4 A4 63 E4 13 08 00 02 00 01 9E
RX cmd=0x1308: 30 55 45 17 B4 A4 63 E4 13 08 00 02 00 01 9E

[setMixHeatUpIndex]
TX cmd=0x0309: 30 55 45 17 B4 A4 63 E4 03 09 00 01 00 8D
RX cmd=0x0309: 30 55 45 17 B4 A4 63 E4 03 09 00 05 00 00 00 C8 41 9A
TX cmd=0x1309: 30 55 45 17 B4 A4 63 E4 13 09 00 05 00 00 00 C8 41 AA
RX cmd=0x1309: 30 55 45 17 B4 A4 63 E4 13 09 00 05 00 00 00 C8 41 AA

[setMixIsHeatDownIndex]
TX cmd=0x030A: 30 55 45 17 B4 A4 63 E4 03 0A 00 01 00 8E
RX cmd=0x030A: 30 55 45 17 B4 A4 63 E4 03 0A 00 02 00 01 90
TX cmd=0x130A: 30 55 45 17 B4 A4 63 E4 13 0A 00 02 00 01 A0
RX cmd=0x130A: 30 55 45 17 B4 A4 63 E4 13 0A 00 02 00 01 A0

[setMixHeatDownIndex]
TX cmd=0x030B: 30 55 45 17 B4 A4 63 E4 03 0B 00 01 00 8F
RX cmd=0x030B: 30 55 45 17 B4 A4 63 E4 03 0B 00 05 00 00 00 C8 41 9C
TX cmd=0x130B: 30 55 45 17 B4 A4 63 E4 13 0B 00 05 00 00 00 C8 41 AC
RX cmd=0x130B: 30 55 45 17 B4 A4 63 E4 13 0B 00 05 00 00 00 C8 41 AC

[setMixKeepTime]
TX cmd=0x030C: 30 55 45 17 B4 A4 63 E4 03 0C 00 01 00 90
RX cmd=0x030C: 30 55 45 17 B4 A4 63 E4 03 0C 00 05 00 58 02 00 00 EE
TX cmd=0x130C: 30 55 45 17 B4 A4 63 E4 13 0C 00 05 00 58 02 00 00 FE
RX cmd=0x130C: 30 55 45 17 B4 A4 63 E4 13 0C 00 05 00 58 02 00 00 FE

[setTempStepParameter]
TX cmd=0x0205: 30 55 45 17 B4 A4 63 E4 02 05 00 01 00 88
RX cmd=0x0205: 30 55 45 17 B4 A4 63 E4 02 05 00 0D 00 00 00 F0 41 00 00 40 40 F4 01 00 00 3A
TX cmd=0x1205: 30 55 45 17 B4 A4 63 E4 12 05 00 0D 00 00 00 F0 41 00 00 40 40 F4 01 00 00 4A
RX cmd=0x1205: 30 55 45 17 B4 A4 63 E4 12 05 00 0D 00 00 00 F0 41 00 00 40 40 F4 01 00 00 4A

[setMixStepParameter]
TX cmd=0x030D: 30 55 45 17 B4 A4 63 E4 03 0D 00 01 00 91
RX cmd=0x030D: 30 55 45 17 B4 A4 63 E4 03 0D 00 18 00 00 00 00 80 3F 00 00 80 3F 01 00 00 C8 41 01 00 00 C8 41 58 02 00 00 94
TX cmd=0x130D: 30 55 45 17 B4 A4 63 E4 13 0D 00 18 00 00 00 00 80 3F 00 00 80 3F 01 00 00 C8 41 01 00 00 C8 41 58 02 00 00 A4
RX cmd=0x130D: 30 55 45 17 B4 A4 63 E4 13 0D 00 18 00 00 00 00 80 3F 00 00 80 3F 01 00 00 C8 41 01 00 00 C8 41 58 02 00 00 A4

[setDateTime]
TX cmd=0x0400: 30 55 45 17 B4 A4 63 E4 04 00 00 01 00 85
RX cmd=0x0400: 30 55 45 17 B4 A4 63 E4 04 00 00 06 25 07 00 13 0B 7E 52
TX cmd=0x1400: 30 55 45 17 B4 A4 63 E4 14 00 00 06 25 07 00 13 0B 7E 62
RX cmd=0x1400: 30 55 45 17 B4 A4 63 E4 14 00 00 06 25 07 00 13 0B 7E 62

[setBrightness]
TX cmd=0x0401: 30 55 45 17 B4 A4 63 E4 04 01 00 01 00 86
RX cmd=0x0401: 30 55 45 17 B4 A4 63 E4 04 01 00 01 4F D5
TX cmd=0x1401: 30 55 45 17 B4 A4 63 E4 14 01 00 01 4F E5
RX cmd=0x1401: 30 55 45 17 B4 A4 63 E4 14 01 00 01 4F E5

[setButtonSound]
TX cmd=0x0402: 30 55 45 17 B4 A4 63 E4 04 02 00 01 00 87
RX cmd=0x0402: 30 55 45 17 B4 A4 63 E4 04 02 00 01 00 87
TX cmd=0x1402: 30 55 45 17 B4 A4 63 E4 14 02 00 01 00 97
RX cmd=0x1402: 30 55 45 17 B4 A4 63 E4 14 02 00 01 00 97

[setLanguage]
TX cmd=0x0403: 30 55 45 17 B4 A4 63 E4 04 03 00 01 00 88
RX cmd=0x0403: 30 55 45 17 B4 A4 63 E4 04 03 00 01 01 89
TX cmd=0x1403: 30 55 45 17 B4 A4 63 E4 14 03 00 01 01 99
RX cmd=0x1403: 30 55 45 17 B4 A4 63 E4 14 03 00 01 01 99

[setPressDataJson]
TX cmd=0x01FF: 30 55 45 17 B4 A4 63 E4 01 FF 00 01 00 81
RX cmd=0x01FF: 30 55 45 17 B4 A4 63 E4 01 FF 02 08 7B 22 50 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 43 68 65 63 6B 56 61 6C 75 65 22 3A 30 2E 30 2C 22 41 69 72 54 69 6D 65 22 3A 31 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 30 2E 37 2C 31 2E 37 2C 32 2E 37 2C 33 2E 37 2C 34 2E 37 2C 35 2E 37 2C 36 2E 37 2C 37 2E 37 2C 38 2E 37 2C 39 2E 37 2C 31 30 2E 37 2C 31 31 2E 37 2C 31 32 2E 37 2C 31 33 2E 37 2C 31 34 2E 37 2C 31 35 2E 37 2C 31 36 2E 37 2C 31 37 2E 37 2C 31 38 2E 37 2C 31 39 2E 37 2C 32 30 2E 37 2C 32 31 2E 37 2C 32 32 2E 37 2C 32 33 2E 37 2C 32 34 2E 37 2C 32 35 2E 37 2C 32 36 2E 37 2C 32 37 2E 37 2C 32 38 2E 37 2C 32 39 2E 37 5D 2C 22 4B 50 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 1E
TX cmd=0x11FF: 30 55 45 17 B4 A4 63 E4 11 FF 02 09 00 7B 22 50 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 43 68 65 63 6B 56 61 6C 75 65 22 3A 30 2E 30 2C 22 41 69 72 54 69 6D 65 22 3A 31 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 30 2E 37 2C 31 2E 37 2C 32 2E 37 2C 33 2E 37 2C 34 2E 37 2C 35 2E 37 2C 36 2E 37 2C 37 2E 37 2C 38 2E 37 2C 39 2E 37 2C 31 30 2E 37 2C 31 31 2E 37 2C 31 32 2E 37 2C 31 33 2E 37 2C 31 34 2E 37 2C 31 35 2E 37 2C 31 36 2E 37 2C 31 37 2E 37 2C 31 38 2E 37 2C 31 39 2E 37 2C 32 30 2E 37 2C 32 31 2E 37 2C 32 32 2E 37 2C 32 33 2E 37 2C 32 34 2E 37 2C 32 35 2E 37 2C 32 36 2E 37 2C 32 37 2E 37 2C 32 38 2E 37 2C 32 39 2E 37 5D 2C 22 4B 50 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 2F
RX cmd=0x11FF: 30 55 45 17 B4 A4 63 E4 11 FF 02 08 7B 22 50 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 43 68 65 63 6B 56 61 6C 75 65 22 3A 30 2E 30 2C 22 41 69 72 54 69 6D 65 22 3A 31 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 30 2E 37 2C 31 2E 37 2C 32 2E 37 2C 33 2E 37 2C 34 2E 37 2C 35 2E 37 2C 36 2E 37 2C 37 2E 37 2C 38 2E 37 2C 39 2E 37 2C 31 30 2E 37 2C 31 31 2E 37 2C 31 32 2E 37 2C 31 33 2E 37 2C 31 34 2E 37 2C 31 35 2E 37 2C 31 36 2E 37 2C 31 37 2E 37 2C 31 38 2E 37 2C 31 39 2E 37 2C 32 30 2E 37 2C 32 31 2E 37 2C 32 32 2E 37 2C 32 33 2E 37 2C 32 34 2E 37 2C 32 35 2E 37 2C 32 36 2E 37 2C 32 37 2E 37 2C 32 38 2E 37 2C 32 39 2E 37 5D 2C 22 4B 50 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 2E

[setTempDataJson]
TX cmd=0x02FF: 30 55 45 17 B4 A4 63 E4 02 FF 00 01 00 82
RX cmd=0x02FF: 30 55 45 17 B4 A4 63 E4 02 FF 01 EE 7B 22 54 53 74 65 70 22 3A 35 2C 22 54 79 70 65 22 3A 30 2C 22 4D 69 6E 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 4D 61 78 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 53 74 61 72 74 54 65 6D 70 22 3A 5B 33 30 2E 30 2C 33 35 2E 30 2C 34 30 2E 30 2C 34 35 2E 30 2C 35 30 2E 30 2C 35 35 2E 30 2C 36 30 2E 30 2C 36 35 2E 30 2C 37 30 2E 30 2C 37 35 2E 30 2C 38 30 2E 30 2C 38 35 2E 30 2C 39 30 2E 30 2C 39 35 2E 30 2C 31 30 30 2E 30 2C 31 30 35 2E 30 2C 31 31 30 2E 30 2C 31 31 35 2E 30 2C 31 32 30 2E 30 2C 31 32 35 2E 30 2C 31 33 30 2E 30 2C 31 33 35 2E 30 2C 31 34 30 2E 30 2C 31 34 35 2E 30 2C 31 35 30 2E 30 2C 31 35 35 2E 30 2C 31 36 30 2E 30 2C 31 36 35 2E 30 2C 31 37 30 2E 30 2C 31 37 35 2E 30 5D 2C 22 53 70 65 65 64 22 3A 5B 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 5D 2C 22 4B 54 54 69 6D 65 22 3A 5B 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 5D 7D 0A 3B
TX cmd=0x12FF: 30 55 45 17 B4 A4 63 E4 12 FF 01 EF 00 7B 22 54 53 74 65 70 22 3A 35 2C 22 54 79 70 65 22 3A 30 2C 22 4D 69 6E 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 4D 61 78 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 53 74 61 72 74 54 65 6D 70 22 3A 5B 33 30 2E 30 2C 33 35 2E 30 2C 34 30 2E 30 2C 34 35 2E 30 2C 35 30 2E 30 2C 35 35 2E 30 2C 36 30 2E 30 2C 36 35 2E 30 2C 37 30 2E 30 2C 37 35 2E 30 2C 38 30 2E 30 2C 38 35 2E 30 2C 39 30 2E 30 2C 39 35 2E 30 2C 31 30 30 2E 30 2C 31 30 35 2E 30 2C 31 31 30 2E 30 2C 31 31 35 2E 30 2C 31 32 30 2E 30 2C 31 32 35 2E 30 2C 31 33 30 2E 30 2C 31 33 35 2E 30 2C 31 34 30 2E 30 2C 31 34 35 2E 30 2C 31 35 30 2E 30 2C 31 35 35 2E 30 2C 31 36 30 2E 30 2C 31 36 35 2E 30 2C 31 37 30 2E 30 2C 31 37 35 2E 30 5D 2C 22 53 70 65 65 64 22 3A 5B 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 5D 2C 22 4B 54 54 69 6D 65 22 3A 5B 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 5D 7D 0A 4C
RX cmd=0x12FF: 30 55 45 17 B4 A4 63 E4 12 FF 01 EE 7B 22 54 53 74 65 70 22 3A 35 2C 22 54 79 70 65 22 3A 30 2C 22 4D 69 6E 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 4D 61 78 57 61 74 65 72 22 3A 32 35 2E 30 2C 22 53 74 61 72 74 54 65 6D 70 22 3A 5B 33 30 2E 30 2C 33 35 2E 30 2C 34 30 2E 30 2C 34 35 2E 30 2C 35 30 2E 30 2C 35 35 2E 30 2C 36 30 2E 30 2C 36 35 2E 30 2C 37 30 2E 30 2C 37 35 2E 30 2C 38 30 2E 30 2C 38 35 2E 30 2C 39 30 2E 30 2C 39 35 2E 30 2C 31 30 30 2E 30 2C 31 30 35 2E 30 2C 31 31 30 2E 30 2C 31 31 35 2E 30 2C 31 32 30 2E 30 2C 31 32 35 2E 30 2C 31 33 30 2E 30 2C 31 33 35 2E 30 2C 31 34 30 2E 30 2C 31 34 35 2E 30 2C 31 35 30 2E 30 2C 31 35 35 2E 30 2C 31 36 30 2E 30 2C 31 36 35 2E 30 2C 31 37 30 2E 30 2C 31 37 35 2E 30 5D 2C 22 53 70 65 65 64 22 3A 5B 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 2C 33 2E 30 5D 2C 22 4B 54 54 69 6D 65 22 3A 5B 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 2C 35 30 30 5D 7D 0A 4B

[setMixDataJson]
TX cmd=0x03FF: 30 55 45 17 B4 A4 63 E4 03 FF 00 01 00 83
RX cmd=0x03FF: 30 55 45 17 B4 A4 63 E4 03 FF 04 35 7B 22 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 49 73 50 72 65 73 73 22 3A 5B 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 5D 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 49 73 55 70 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 55 70 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 49 73 44 6F 77 6E 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 44 6F 77 6E 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 4B 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 0A 5D
TX cmd=0x13FF: 30 55 45 17 B4 A4 63 E4 13 FF 04 36 00 7B 22 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 49 73 50 72 65 73 73 22 3A 5B 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 5D 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 49 73 55 70 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 55 70 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 49 73 44 6F 77 6E 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 44 6F 77 6E 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 4B 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 0A 6E
RX cmd=0x13FF: 30 55 45 17 B4 A4 63 E4 13 FF 04 35 7B 22 53 74 65 70 22 3A 33 2C 22 54 79 70 65 22 3A 31 2C 22 41 22 3A 31 30 30 2E 30 2C 22 42 22 3A 31 30 30 2E 30 2C 22 44 22 3A 31 30 30 2E 30 2C 22 49 73 50 72 65 73 73 22 3A 5B 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 5D 2C 22 53 65 74 50 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 41 66 74 65 72 56 61 6C 75 65 22 3A 5B 31 2E 30 2C 32 2E 30 2C 33 2E 30 2C 34 2E 30 2C 35 2E 30 2C 36 2E 30 2C 37 2E 30 2C 38 2E 30 2C 39 2E 30 2C 31 30 2E 30 2C 31 31 2E 30 2C 31 32 2E 30 2C 31 33 2E 30 2C 31 34 2E 30 2C 31 35 2E 30 2C 31 36 2E 30 2C 31 37 2E 30 2C 31 38 2E 30 2C 31 39 2E 30 2C 32 30 2E 30 2C 32 31 2E 30 2C 32 32 2E 30 2C 32 33 2E 30 2C 32 34 2E 30 2C 32 35 2E 30 2C 32 36 2E 30 2C 32 37 2E 30 2C 32 38 2E 30 2C 32 39 2E 30 2C 33 30 2E 30 5D 2C 22 49 73 55 70 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 55 70 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 49 73 44 6F 77 6E 48 65 61 74 22 3A 5B 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 2C 31 2C 30 5D 2C 22 44 6F 77 6E 54 65 6D 70 22 3A 5B 32 35 2E 30 2C 32 35 2E 30 2C 33 30 2E 30 2C 34 30 2E 30 2C 35 30 2E 30 2C 36 30 2E 30 2C 37 30 2E 30 2C 38 30 2E 30 2C 39 30 2E 30 2C 31 30 30 2E 30 2C 31 31 30 2E 30 2C 31 32 30 2E 30 2C 31 33 30 2E 30 2C 31 34 30 2E 30 2C 31 35 30 2E 30 2C 31 36 30 2E 30 2C 31 37 30 2E 30 2C 31 38 30 2E 30 2C 31 39 30 2E 30 2C 32 30 30 2E 30 2C 32 31 30 2E 30 2C 32 32 30 2E 30 2C 32 33 30 2E 30 2C 32 34 30 2E 30 2C 32 35 30 2E 30 2C 32 36 30 2E 30 2C 32 37 30 2E 30 2C 32 38 30 2E 30 2C 32 39 30 2E 30 2C 33 30 30 2E 30 5D 2C 22 4B 54 69 6D 65 22 3A 5B 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 2C 36 30 30 5D 7D 0A 6D

[setStartHeat]
TX cmd=0x1522: 30 55 45 17 B4 A4 63 E4 15 22 00 01 01 B9
RX cmd=0x1522: 30 55 45 17 B4 A4 63 E4 15 22 00 01 01 B9

[setStartHeat]
TX cmd=0x1522: 30 55 45 17 B4 A4 63 E4 15 22 00 01 00 B8
RX cmd=0x1522: 30 55 45 17 B4 A4 63 E4 15 22 00 01 00 B8

[setPressing]
TX cmd=0x1521: 30 55 45 17 B4 A4 63 E4 15 21 00 01 01 B8
RX cmd=0x1521: 30 55 45 17 B4 A4 63 E4 15 21 00 01 01 B8

[setPressing]
TX cmd=0x1521: 30 55 45 17 B4 A4 63 E4 15 21 00 01 00 B7
RX cmd=0x1521: 30 55 45 17 B4 A4 63 E4 15 21 00 01 00 B7

[setDemolding]
TX cmd=0x1523: 30 55 45 17 B4 A4 63 E4 15 23 00 01 01 BA
RX cmd=0x1523: 30 55 45 17 B4 A4 63 E4 FF FF 00 05 65 72 72 6F 72 AD

[setDemolding]
TX cmd=0x1523: 30 55 45 17 B4 A4 63 E4 15 23 00 01 00 B9
RX cmd=0x1523: 30 55 45 17 B4 A4 63 E4 FF FF 00 05 65 72 72 6F 72 AD

[setStart]
TX cmd=0x1511: 30 55 45 17 B4 A4 63 E4 15 11 00 01 01 A8
RX cmd=0x1511: 30 55 45 17 B4 A4 63 E4 15 11 00 01 01 A8

[setStart]
TX cmd=0x1511: 30 55 45 17 B4 A4 63 E4 15 11 00 01 00 A7
RX cmd=0x1511: 30 55 45 17 B4 A4 63 E4 15 11 00 01 00 A7

[selectModelState]
TX cmd=0x1507: 30 55 45 17 B4 A4 63 E4 15 07 00 01 02 9F
RX cmd=0x1507: 30 55 45 17 B4 A4 63 E4 15 07 00 01 01 9E

```

> 完整原始日志（含全部 0x0500 状态轮询帧）见 `press_interface_trace.log`。
