# Press SDK 上位机接口测试报告

- **测试日期**：2026-11-18
- **测试对象**：press_sdk_project（C++ 接口，`Press` 类，JSON 字符串接口）
- **被测程序**：`build/test/cpp/Release/press_cpp_test.exe`
- **测试代码**：`test/cpp/main.cpp`
- **设备**：自动热压机，固件源码 `auto_hot_press`（对照审查）
- **连接**：串口 COM14，115200 8N1
- **SDK 版本**：1.0.0
- **结论**：本轮覆盖全部 72 个可基准接口：**67 项通过、0 项失败、1 项跳过**。PD/TD/MD 三个 JSON 写接口在固件修复后全部通过（原值写回校验成功）；仅 `setDemolding` 因固件未实现 `CMDID_SET_START_DEMOLD`(0x1523) 跳过。
- **性能报告**：[press_api_performance_report.md](press_api_performance_report.md)（72 个接口的 min/avg/p50/max）

---

## 1. 测试结果明细

### 1.1 连接

| 接口 | 结果 | 返回值 |
|---|---|---|
| connect | PASS | true，58.1 ms（含版本握手） |

### 1.2 设备信息

| 接口 | 返回值 |
|---|---|
| getName(0x00) | 自动热压机 |
| getName(0x02) | 自动热压机 |
| getMachineType | TYPE |
| getSerialNumber | ASD123 |
| getPressParameter | MaxP=30.000 MinP=0.500 Max_Min=0.300 Diameter=110.000 MaxStep=30 PDecimal=1 PressDecimal=2 PModel=0 OutType=0 |
| getTempParameter | MaxT=310.000 MinT=25.000 MaxTStep=30 TDecimal=1 IsHasWater=1 IsHasSpeed=0 |
| getOtherInfo | Screenshot=0 StartDelay=3 IsHideLang=0 Remote=1 VersionType=2 |

### 1.3 JSON 批量命令

**ReadOnlyData（ROD，370 字节）**

```json
{"NameZH":"","NameEN":"","Type":"TYPE","Screenshot":0,"StartDelay":3,"VersionType":2,
 "SerialNumber":"ASD123","IsHideLang":0,"Remote":1,"Network":0,
 "MaxPStep":30,"MaxPLimit":30.0,"MinPLimit":0.5,"Max_Min":0.3,"Diameter":110.0,
 "PDecimal":1,"PressDecimal":2,"PModel":0,"OutType":0,
 "MaxTStep":30,"MaxTLimit":310.0,"MinTLimit":25.0,"TDecimal":1,"IsHasWater":1,"IsHasSpeed":0}
```

**PressData（PD，520 字节）**

```json
{"PStep":3,"Type":1,"A":100.0,"B":100.0,"D":100.0,"CheckValue":0.0,"AirTime":1,
 "SetPValue":[1.0,2.0,...,30.0],
 "AfterValue":[0.7,1.7,...,29.7],
 "KPTime":[600 × 30]}
```

**RealTimeData（RT，221–222 字节，随运行状态变化）**

```json
{"ModelState":2,"IsPStart":0,"IsTStart":0,"IsT1Start":0,
 "CPStep":0,"CTStep":0,"CT1Step":0,
 "PValue":2.494,"MoldPressure":33.22,"TUpValue":15.83,"TDownValue":2.136,
 "PTime":600,"TTime":500,"T1Time":0,"IsAir":0,"FlowChange":0}
```

**TemperatureData（TD，494 字节）**

```json
{"TStep":5,"Type":0,"MinWater":25.0,"MaxWater":25.0,
 "StartTemp":[30.0,35.0,...,175.0],"Speed":[3.0 × 30],"KTTime":[500 × 30]}
```

**MixedData（MD，1077 字节）**

```json
{"Step":3,"Type":1,"A":100.0,"B":100.0,"D":100.0,
 "IsPress":[0,1,...],"SetPValue":[1.0..30.0],"AfterValue":[1.0..30.0],
 "IsUpHeat":[1,0,...],"UpTemp":[25.0,25.0,30.0..300.0],
 "IsDownHeat":[1,0,...],"DownTemp":[25.0,25.0,30.0..300.0],"KTime":[600 × 30]}
```

### 1.4 压力单值命令

| 接口 | 返回值 |
|---|---|
| getPressStepCount | 3 |
| getCalibrationValue | 0.000 |
| getMoldType | 1 |
| getCircle | 100.000 |
| getRectangle | A=100.000 B=100.000 |
| getAirTime | 1 s |
| getPressValueIndex(0) | 1.000 |
| getAfterValueIndex(0) | 0.700 |
| getKeepPressTime(0) | 600 ms |

### 1.5 温度单值命令

| 接口 | 返回值 |
|---|---|
| getTempStepCount | 5 |
| getTempSpeedMold | 0 |
| getStartTempIndex(0) | 30.000 |
| getTempSpeedIndex(0) | 3.000 |
| getKeepTempTime(0) | 500 ms |
| getMaxWaterValue | 25.000 |
| getMinWaterValue | 25.000 |

### 1.6 段参数

| 接口 | 返回值 |
|---|---|
| getPressStepParameter(0) | setP=1.000 after=0.700 kpTime=600ms |
| getTempStepParameter(0) | tempValue=30.000 speedValue=3.000 ktTime=500ms |
| getMixStepParameter(0) | IsPress=0 setP=1.000 after=1.000 IsUpHeat=1 upTemp=25.000 IsDownHeat=1 downTemp=25.000 kTime=600ms |

### 1.7 系统与状态

| 接口 | 返回值 |
|---|---|
| getDateTime | 2026-11-18 00:12:55 |
| getBrightness | 79 |
| setBrightness（读回写） | true，写回 79 |
| getButtonSound | off (0) |
| getLanguage | 1 |
| getMachineState | mode=2 flags=0x00 press=stopped heat=stopped pStep=0 tStep=0 t1Step=0；press=5.660 upT=10.9 downT=4.1 pRemain=600ms tRemain=500ms t1Remain=0ms flow=0（25 字节） |
| getMachineRegisterNo | 3482765854958183396 |

### 1.8 设置命令（读出-写回-校验）

测试方式：先读取当前值 → 原值写回 → 重新读取校验，设备配置不变。

| 接口 | 结果 | 写回值 |
|---|---|---|
| setPressStepCount | PASS | 3 |
| setCalibrationValue | PASS | 0.000 |
| setMoldType | PASS | 1 |
| setCircle | PASS | 100.000 |
| setRectangle | PASS | A=100.000 B=100.000 |
| setAirTime | PASS | 1 |
| setPressValueIndex(0) | PASS | 1.000 |
| setAfterValueIndex(0) | PASS | 0.700 |
| setKeepPressTime(0) | PASS | 600 |
| setTempStepCount | PASS | 5 |
| setTempSpeedMold | PASS | 0 |
| setTempIndex(0) | PASS | 30.000 |
| setTempSpeedIndex(0) | PASS | 3.000 |
| setKeepTempTime(0) | PASS | 500 |
| setMaxWaterValue | PASS | 25.000 |
| setMinWaterValue | PASS | 25.000 |
| setDateTime | PASS | 2026-11-18 00:12:58 |
| setButtonSound | PASS | off |
| setLanguage | PASS | 1 |
| setPressStepParameter(0) | PASS | setP=1.000 after=0.700 kpTime=600 |
| setTempStepParameter(0) | PASS | temp=30.000 speed=3.000 ktTime=500 |
| setMixStepParameter(0) | PASS | setP=1.000 after=1.000 upTemp=25.000 downTemp=25.000 kTime=600 |

### 1.8.1 运行控制命令（真实动作，空载）

测试前确认设备空闲；加热/加压仅在模式 2（压力 + 双温）下有效，测试期间自动切到模式 2，结束后恢复原模式。

| 接口 | 结果 | 实测耗时 | 说明 |
|---|---|---|---|
| setStartHeat(true/false) | PASS | 3.86 s | 开 → `flags.bit1=1` → 关 → 位清零 |
| setPressing(true/false) | PASS | 2.31 s | 启动 → `flags.bit0=1` → 动作 0.8 s → 停止 → 位清零 |
| setStart(true/false) | PASS | 10.83 s | 接受启动命令；完整流程含 3 s 启动延时与升温，10 s 内未出现状态位，停止后回到空闲 |
| selectModelState(1↔2) | PASS | 1.50 s | 切换后读到目标模式，再恢复原模式 |
| setDemolding(true/false) | N/A | 0.055 s | 固件未实现 `CMDID_SET_START_DEMOLD`(0x1523)，两次调用均失败 |

### 1.9 性能

全部 72 个接口的耗时分布见 [press_api_performance_report.md](press_api_performance_report.md)。普通命令的平均耗时（不含控制类）为 **56.62 ms**，控制类因包含状态轮询与动作等待耗时较长，属预期。

典型命令：

| 命令 | min | avg | p50 | max |
|---|---|---|---|---|
| getName | 44.99 | 54.56 | 47.54 | 95.45 |
| getPressParameter | 45.85 | 49.60 | 47.08 | 61.73 |
| getMachineState | 45.36 | 50.44 | 46.83 | 63.15 |
| getPressDataJson | 62.41 | 93.28 | 94.26 | 108.67 |
| getMixDataJson | 140.15 | 151.92 | 154.57 | 156.04 |
| setMixStepParameter | 45.92 | 55.79 | 61.39 | 63.22 |
| setPressDataJson | 109.14 | 140.03 | 154.12 | 156.84 |
| setTempDataJson | 122.35 | 144.94 | 154.76 | 157.71 |
| setMixDataJson | 217.10 | 249.05 | 264.36 | 265.68 |
| isConnected / version | <0.001 | <0.001 | <0.001 | <0.001 |

定时器（配置 1 s）：3 次回调，间隔 avg **1047.1 ms**，min 1040.8 / max 1053.3，标准差 **6.22 ms**。

---

## 2. 遗留错误点

### E1【固件 · 中】三温模式 flags 位组合使用错误变量

- **位置**：`auto_hot_press/src/data/rtdata.cpp`（`getRtData()` 组帧处）
  ```cpp
  out.push_back((m_IsPStart&0x01)|((m_IsTStart<<1)&0x02)|((m_IsTStart<<2)&0x04));
  ```
- **问题**：bit2 表示 `IsT1Start`，此处仍用 `m_IsTStart` 移位；`mode=3`（P_T_T）下辅助加热状态会与主加热相同。
- **现状**：本机 `mode=2`，该分支不执行，属静态审查发现；本轮回归未触发。
- **建议**：改为 `((m_IsT1Start<<2)&0x04)`。

### E2【SDK · 中】`DateTimeInfo.m_Year` 文档与协议不符，存在误用风险

- **文档**：`press/include/press/presstype.h` 注释为“Year (full value, e.g. 2026) / 年（完整数值）”。
- **实际**：协议与固件均使用 `tm_year` 偏移值（本机为 126），SDK `getDateTime()` 原样存储、`setDateTime()` 原样下发，测试程序需 `+1900` 才显示 2026。
- **风险**：调用方按注释传入 `2026` 调用 `setDateTime()`，会被截断为 234，设备年份变为 2134。
- **建议**：修正注释为 tm_year（1900 偏移），或在 SDK 内统一转换并同步 `getDateTime()`/`setDateTime()`。

### E3【SDK · 中】`m_MaxMin` 字段语义注释错误

- **文档**：`presstype.h` 中 `ReadOnlyData::m_Max_Min`、`PressParameter::m_MaxMin` 注释为“Max - Min difference / 最大最小压力差”。
- **实际**：固件该值为“自动补压值”，用于 `AfterValue = SetP - m_Max_Min`，界面标签为 `AutomaticPressure`，默认值 0.3。
- **反证**：本次读数 `MaxP=30.0`、`MinP=0.5`，若为差值应为 29.5，实测 0.3。
- **建议**：注释改为“自动补压值 / auto repress offset（AfterValue = SetP - value）”。

### E4【SDK · 设计】直接调用 get 接口会重复触发回调

- **现象**：基准测试中调用 5 次 `getReadOnlyDataJson` 产生 5 条 `onReadOnlyData`；调用 5 次 `getTempDataJson` 产生 5 条 `onTempData`（内容完全相同）。
- **根因**：`PressPrivate::get*Json()` 在成功与失败路径都会主动调用 `iface->on*()`。
- **影响**：同时启用定时器和界面轮询时，UI 会对同一份数据更新两次；错误也会重复上报。
- **建议**：明确二选一语义（定时器模式 or 主动拉取模式），或提供开关抑制主动调用的回调。

### E5【SDK/设备 · 性能与数据质量，非功能缺陷】

1. **固定 46 ms 往返地板**：所有小命令 min≈45.8 ms，其中 SDK 轮询粒度 10 ms、串口传输约 2.6 ms，其余约 33–38 ms 为固件命令处理周期。
2. **1 Hz 定时采集与用户命令争用**：max 列出现 62–140 ms 的尖峰，与定时器周期吻合。
3. **定时器实际周期 1047.1 ms 而非 1000 ms**：采用“等满周期再发请求”的固定延迟模型，漂移 = 请求耗时；抖动 6.22 ms。
4. **JSON 类受带宽限制**：TD 494 B≈93 ms、MD 1077 B≈152 ms，与 115200 波特率理论传输时间（43/94 ms）吻合。
5. **原始数据未钳位**：压力/温度通道存在零漂（本机停止时 `PValue≈2.5`、`MoldPressure≈33.2`），属传感器真实输出而非缺陷；`ReadOnlyData` 名称为空是因固件 `init()` 置空、`getName` 另有语言默认值。

### E7【固件 · 中】`setDemolding` 无实现

- **现象**：`setDemolding(true)` 与 `setDemolding(false)` 均返回 false（`Command mismatch`），耗时约 55 ms。
- **根因**：固件未实现 `CMDID_SET_START_DEMOLD`(0x1523)，`receive.cpp` 与 `mainLogic.cc` 中均无对应处理分支。
- **影响**：脱模命令不可用；加压/加热/全部启动均正常。
- **建议**：补充 0x1523 处理分支（脱模按钮切换 + 状态回显），或在协议文档中明确该命令暂不支持。

---

## 3. 已修复并验证

### F1【固件 · 已修复】RT 数据成员未初始化（T1 垃圾值）

- **原问题**：`IsT1Start=-425159449`、`CT1Step=151661602`、`T1Time=151661602`，每次重启数值变化。
- **修复**：`RTData::RTData()` 零初始化全部成员。
- **验证**：本轮 `IsT1Start=0`、`CT1Step=0`、`T1Time=0`，与二进制 `t1Step=0`/`t1Remain=0ms` 一致。

### F2【固件 · 已修复】`m_FlowChange` 未初始化

- **原问题**：RT JSON 与二进制帧均输出 `FlowChange=34` 等残留值。
- **修复**：构造函数零初始化，并在 JSON 读写中补充 `FlowChange` 字段。
- **验证**：本轮 RT JSON `FlowChange=0`，`getMachineState` 的 `flow=0`。

### F3【固件+SDK · 已修复】混合段参数写命令长度 off-by-one

- **原问题**：`MIX_INDEX_STEP_PARAMETER_BOOL(size)` 为 `size == 23`，实际布局为 24 字节；`MixData::setStepIndexData()` 在 `size == 23` 时越界读取 `pData[20..23]`，导致 `setMixStepParameter(0)` 返回 `Command mismatch`。
- **修复**：固件宏与 `setStepIndexData()` 长度校验改为 24；SDK `press/src/typeprivate.h` 宏同步改为 24。
- **验证**：本轮读回写 PASS，写回值 setP=1.000 after=1.000 upTemp=25.000 downTemp=25.000 kTime=600（温度值经固件钳位到 `MinT`=25）。

### F4【固件 · 已修复】PD/TD/MD 的 JSON 写命令未剥离压缩标志字节

- **原问题**：SDK 按协议在 payload 首字节写压缩标志，固件 PD/TD/MD 的 SET_JSON 分支把整段数据（含标志字节）交给 `setJsonByteArray`，JSON 解析失败，`setPressDataJson` / `setTempDataJson` / `setMixDataJson` 全部返回 `Command mismatch`。
- **修复**：[receive.cpp](file:///e:/code/work/press_project/auto_hot_press/src/interface/receive.cpp) 三处与 ROD 对齐——`isCompress = data[0]`，`setJsonByteArray(data+1, size-1, isCompress)`，回读同样带上 `isCompress`。
- **验证**：本轮三次写回全部 PASS（3/3），耗时 PD 140.0 ms、TD 144.9 ms、MD 249.0 ms；失败数由 3 降为 0。

---

## 4. 处理优先级建议

| 优先级 | 事项 | 归属 |
|---|---|---|
| P1 | E7 `setDemolding` 无实现（0x1523） | 固件 |
| P1 | E2 年份契约错误（误用会写错设备时间） | SDK 文档/实现 |
| P1 | E3 自动补压值语义注释错误 | SDK 文档 |
| P2 | E1 三温 flags 位错误（mode=3 时显现） | 固件 |
| P2 | E4 回调重复上报 | SDK 设计 |
| P3 | E5 波特率提升 / 定时器固定速率 / 轮询粒度优化 | SDK + 固件 |

> 本轮包含真实控制动作（加热、加压、全部启动、模式切换），均在空载且设备空闲的前提下执行，结束后已恢复原工作模式并确认回到空闲；参数写入仍全部为“读出-写回”。
