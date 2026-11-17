# Press SDK 接口性能报告

- **测试对象**：Press SDK C++ 接口（`Press` 类）
- **设备连接**：serial COM14
- **测量方式**：`std::chrono::steady_clock`，含完整请求/响应往返；
  本地查询 20 次、单值命令 10 次、参数块 5 次、JSON 批量 5 次、
  set 类原值写回 5 次（JSON 写入 3 次）、控制动作 1 次。
- **接口数**：72（全部通过 71，存在失败 1）
- **平均耗时（不含控制类）**：56.62 ms

> 生命周期接口（`connect` / `disconnect` / `registerDataInterface` / `unregisterDataInterface` /
> `run` / `stop`）已在功能测试中执行，不做重复基准。
> 耗时显示为 0.000 表示本地调用（无串口往返）低于 0.001 ms。

| 接口 | 类别 | 采样 | 成功 | min(ms) | avg(ms) | p50(ms) | max(ms) | 备注 |
|---|---|---|---|---|---|---|---|---|
| `setStartHeat(true/false)` | ctrl | 1 | 1/1 | 4017.310 | 4017.310 | 4017.310 | 4017.310 | 1 次 开→校验→关→校验 |
| `setPressing(true/false)` | ctrl | 1 | 1/1 | 2155.431 | 2155.431 | 2155.431 | 2155.431 | 1 次 启动→动作 0.8 s→停止 |
| `setDemolding(true/false)` | ctrl | 2 | 0/2 | 54.663 | 54.663 | 54.663 | 54.663 | 固件未实现 0x1523 |
| `setStart(true/false)` | ctrl | 1 | 1/1 | 10832.444 | 10832.444 | 10832.444 | 10832.444 | 1 次 启动全部→0.5 s→停止 |
| `selectModelState(1<->2)` | ctrl | 1 | 1/1 | 1454.339 | 1454.339 | 1454.339 | 1454.339 | 切换后恢复原模式 |
| `isConnected` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `getMachineRegisterNo` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `isRunning` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `getLastErrorInfo` | get | 20 | 20/20 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `Press::version` | static | 50 | 50/50 | 0.000 | 0.000 | 0.000 | 0.000 | - |
| `Press::getPortList` | static | 5 | 5/5 | 0.998 | 2.401 | 1.006 | 7.984 | - |
| `getName` | get | 10 | 10/10 | 44.986 | 54.556 | 47.544 | 95.446 | - |
| `getMachineType` | get | 10 | 10/10 | 46.101 | 49.739 | 46.611 | 62.789 | - |
| `getSerialNumber` | get | 10 | 10/10 | 46.201 | 55.987 | 47.085 | 109.414 | - |
| `getPressParameter` | get | 10 | 10/10 | 45.845 | 49.596 | 47.076 | 61.725 | - |
| `getTempParameter` | get | 10 | 10/10 | 45.918 | 55.607 | 47.327 | 91.863 | - |
| `getOtherInfo` | get | 10 | 10/10 | 45.953 | 49.616 | 46.923 | 61.367 | - |
| `getReadOnlyDataJson` | get | 5 | 5/5 | 46.104 | 76.953 | 91.599 | 108.211 | - |
| `getPressDataJson` | get | 5 | 5/5 | 62.409 | 93.275 | 94.258 | 108.668 | - |
| `getRealTimeDataJson` | get | 5 | 5/5 | 31.052 | 46.628 | 46.915 | 62.074 | - |
| `getTempDataJson` | get | 5 | 5/5 | 62.459 | 96.006 | 93.011 | 139.361 | - |
| `getMixDataJson` | get | 5 | 5/5 | 140.151 | 151.916 | 154.566 | 156.036 | - |
| `getMachineState` | get | 20 | 20/20 | 45.364 | 50.436 | 46.830 | 63.148 | - |
| `getPressStepCount` | get | 10 | 10/10 | 45.517 | 55.781 | 47.019 | 107.809 | - |
| `getCalibrationValue` | get | 10 | 10/10 | 45.763 | 50.952 | 47.065 | 61.965 | - |
| `getModeType` | get | 10 | 10/10 | 46.190 | 54.302 | 46.952 | 107.751 | - |
| `getCircle` | get | 10 | 10/10 | 45.828 | 49.823 | 46.869 | 63.172 | - |
| `getRectangle` | get | 10 | 10/10 | 45.712 | 51.280 | 46.919 | 62.459 | - |
| `getAirTime` | get | 10 | 10/10 | 45.855 | 54.193 | 47.123 | 91.748 | - |
| `getPressValueIndex` | get | 10 | 10/10 | 45.936 | 49.653 | 46.823 | 63.328 | - |
| `getAfterValueIndex` | get | 10 | 10/10 | 45.090 | 55.961 | 46.970 | 94.547 | - |
| `getKeepPressTime` | get | 10 | 10/10 | 45.863 | 49.549 | 46.798 | 62.268 | - |
| `getPressStepParameter` | get | 5 | 5/5 | 45.902 | 52.690 | 46.843 | 62.575 | - |
| `getTempStepCount` | get | 10 | 10/10 | 44.788 | 49.390 | 46.670 | 61.883 | - |
| `getTempSpeedMold` | get | 10 | 10/10 | 46.778 | 54.576 | 47.195 | 93.039 | - |
| `getStartTempIndex` | get | 10 | 10/10 | 45.314 | 50.727 | 46.521 | 62.002 | - |
| `getTempSpeedIndex` | get | 10 | 10/10 | 45.913 | 55.786 | 46.789 | 108.103 | - |
| `getKeepTempTime` | get | 10 | 10/10 | 45.921 | 49.573 | 47.053 | 61.775 | - |
| `getMaxWaterValue` | get | 10 | 10/10 | 45.480 | 49.636 | 46.794 | 62.531 | - |
| `getMinWaterValue` | get | 10 | 10/10 | 45.238 | 55.925 | 47.390 | 108.984 | - |
| `getTempStepParameter` | get | 5 | 5/5 | 46.116 | 49.674 | 46.639 | 62.254 | - |
| `getMixStepParameter` | get | 5 | 5/5 | 45.949 | 49.525 | 46.508 | 61.709 | - |
| `getDateTime` | get | 10 | 10/10 | 45.820 | 50.999 | 46.901 | 61.379 | - |
| `getBrightness` | get | 10 | 10/10 | 46.140 | 54.385 | 47.089 | 92.203 | - |
| `getButtonSound` | get | 10 | 10/10 | 45.849 | 49.849 | 47.229 | 62.220 | - |
| `getLanguage` | get | 10 | 10/10 | 45.871 | 55.918 | 46.946 | 109.322 | - |
| `setPressStepCount` | set | 5 | 5/5 | 45.835 | 52.305 | 46.481 | 61.875 | 原值写回 |
| `setCalibrationValue` | set | 5 | 5/5 | 46.361 | 53.014 | 47.574 | 62.252 | 原值写回 |
| `setModeType` | set | 5 | 5/5 | 45.971 | 52.787 | 46.976 | 62.803 | 原值写回 |
| `setCircle` | set | 5 | 5/5 | 45.922 | 53.044 | 47.005 | 63.173 | 原值写回 |
| `setRectangle` | set | 5 | 5/5 | 46.177 | 53.225 | 47.310 | 62.737 | 原值写回 |
| `setAirTime` | set | 5 | 5/5 | 46.364 | 52.867 | 47.450 | 62.417 | 原值写回 |
| `setPressValueIndex` | set | 5 | 5/5 | 45.635 | 52.051 | 45.985 | 61.882 | index=0 原值写回 |
| `setAfterValueIndex` | set | 5 | 5/5 | 46.053 | 52.451 | 46.710 | 61.636 | index=0 原值写回 |
| `setKeepPressTime` | set | 5 | 5/5 | 46.210 | 52.662 | 46.926 | 61.824 | index=0 原值写回 |
| `setPressStepParameter` | set | 5 | 5/5 | 46.647 | 53.381 | 47.519 | 63.389 | index=0 原值写回 |
| `setTempStepCount` | set | 5 | 5/5 | 45.471 | 61.613 | 62.125 | 91.299 | 原值写回 |
| `setTempSpeedMold` | set | 5 | 5/5 | 45.642 | 52.727 | 46.898 | 63.095 | 原值写回 |
| `setTempIndex` | set | 5 | 5/5 | 45.796 | 52.517 | 47.234 | 62.263 | index=0 原值写回 |
| `setTempSpeedIndex` | set | 5 | 5/5 | 45.845 | 52.286 | 46.474 | 61.878 | index=0 原值写回 |
| `setKeepTempTime` | set | 5 | 5/5 | 45.844 | 52.802 | 47.072 | 62.317 | index=0 原值写回 |
| `setMaxWaterValue` | set | 5 | 5/5 | 45.827 | 52.572 | 46.747 | 62.745 | 原值写回 |
| `setMinWaterValue` | set | 5 | 5/5 | 46.168 | 52.789 | 47.365 | 62.355 | 原值写回 |
| `setTempStepParameter` | set | 5 | 5/5 | 46.044 | 52.897 | 47.542 | 61.877 | index=0 原值写回 |
| `setMixStepParameter` | set | 5 | 5/5 | 45.921 | 55.787 | 61.385 | 63.216 | index=0 原值写回 |
| `setDateTime` | set | 5 | 5/5 | 46.205 | 49.930 | 46.798 | 62.733 | 原值写回 |
| `setBrightness` | set | 5 | 5/5 | 46.312 | 51.003 | 47.261 | 62.929 | 原值写回 |
| `setButtonSound` | set | 5 | 5/5 | 45.669 | 62.210 | 47.853 | 108.960 | 原值写回 |
| `setLanguage` | set | 5 | 5/5 | 45.930 | 49.497 | 46.722 | 61.042 | 原值写回 |
| `setPressDataJson` | set | 3 | 3/3 | 109.138 | 140.034 | 154.123 | 156.843 | 原值写回 |
| `setTempDataJson` | set | 3 | 3/3 | 122.348 | 144.941 | 154.762 | 157.714 | 原值写回 |
| `setMixDataJson` | set | 3 | 3/3 | 217.103 | 249.045 | 264.357 | 265.675 | 原值写回 |

> 控制类（ctrl）命令包含状态轮询、动作等待与停止确认，单次耗时明显高于普通命令，属预期。
> `setDemolding` 因固件未实现 `CMDID_SET_START_DEMOLD`(0x1523) 而无法通过，仅记录实际往返耗时。
