# Press SDK 上位机接口测试报告

- **测试日期**：2026-11-15
- **测试对象**：press_sdk_project（C++ 接口，`Press` 类）
- **被测程序**：`build/test/cpp/Debug/press_cpp_test.exe`
- **测试代码**：`test/cpp/main.cpp`
- **设备**：自动热压机，固件源码 `auto_hot_press`（对照审查）
- **连接**：串口 COM14，115200 8N1
- **SDK 版本**：1.0.0
- **结论**：**30 项接口全部通过（0 失败）**；发现 **5 处缺陷/契约错误**（固件 3 处、SDK 2 处）与 **2 处行为/性能问题**。

---

## 1. 测试结果明细

### 1.1 连接

| 接口 | 结果 | 返回值 |
|---|---|---|
| connect | PASS | true，40.3 ms（含版本握手） |

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

### 1.3 JSON 结构体

**ReadOnlyData（ROD）**

```
nameZH=''  nameEN=''
type='TYPE'  sn='ASD123'
Screenshot=0 StartDelay=3 VersionType=2 IsHideLang=0 Remote=1 Network=0 FontZH=0 FontEN=0
MaxPStep=30 MaxP=30.000 MinP=0.500 Max_Min=0.300 Diameter=110.000
PDecimal=1 PressDecimal=2 PModel=0 OutType=0
MaxTStep=30 MaxT=310.000 MinT=25.000 TDecimal=1 IsHasWater=1 IsHasSpeed=0
```

**PressData（PD）**

```
PStep=3 Type=1 A=100.000 B=100.000 D=100.000 OuterD=0.000 InnerD=0.000 CheckValue=0.000
Speed=0 DemoldValue=0.000 AirTime=1
  step[0]: setP=1.000 after=0.700 kpTime=600ms
  step[1]: setP=2.000 after=1.700 kpTime=600ms
  step[2]: setP=3.000 after=2.700 kpTime=600ms
```

**RealTimeData（RT）**

```
mode=2 pressState=0 pdChanged=0 cPStep=0 cTStep=0 cT1Step=34
IsPStart=0 IsTStart=0 IsT1Start=0 IsAir=0
press=6.063 moldPress=15.910 upT=17.3 downT=-16.1
PTime=600ms TTime=500ms T1Time=151661602ms
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
 "IsUpHeat":[1,0,...],"UpTemp":[10.0..300.0],
 "IsDownHeat":[1,0,...],"DownTemp":[10.0..300.0],"KTime":[600 × 30]}
```

### 1.4 类型化助手

| 接口 | 返回值 |
|---|---|
| getByteValue(PRESS_STEP) | 3 |
| getFloatValue(PRESS_CALIBRATE) | 0.000 |
| getFloatPairValue(RECT_AB) | A=100.000 B=100.000 |
| getIndexedFloat(INDEX_PRESS,0) | 1.000 |
| getIndexedInt32(INDEX_KPTIME,0) | 600 |
| getIndexedByte(MIX_INDEX_ISPRESS,0) | 0 |

### 1.5 段参数

| 接口 | 返回值 |
|---|---|
| getPressStepParameter(0) | setP=1.000 after=0.700 kpTime=600ms |
| getTempStepParameter(0) | tempValue=30.000 speedValue=3.000 ktTime=500ms |
| getMixStepParameter(0) | IsPress=0 setP=1.000 after=1.000 IsUpHeat=1 upTemp=10.000 IsDownHeat=1 downTemp=10.000 kTime=600ms |

### 1.6 系统与状态

| 接口 | 返回值 |
|---|---|
| getDateTime | 2026-11-15 20:59:53 |
| getBrightness | 16 |
| setBrightness（读回写） | true，写回 16 |
| getButtonSound | off (0) |
| getLanguage | 1 |
| getMachineState | mode=2 flags=0x00 press=stopped heat=stopped pStep=0 tStep=0 t1Step=0；press=7.522 upT=14.7 downT=-17.0 pRemain=600ms tRemain=500ms t1Remain=0ms flow=34（25 字节） |
| getMachineRegisterNo | 3482765854958183396 |

### 1.7 通用接口

```
genericRead(MACHINE_TYPE)  → 4 字节: 54 59 50 45（"TYPE"）
genericWrite(BRIGHTNESS)   → 请求 10，回显 10
```

### 1.8 性能

| 命令 | 成功 | min | avg | p50 | max |
|---|---|---|---|---|---|
| genericRead(VERSION) | 10/10 | 45.85 | 55.37 | 46.62 | 107.59 |
| getByteValue | 10/10 | 45.57 | 49.45 | 46.64 | 61.83 |
| getFloatValue | 10/10 | 45.84 | 50.97 | 46.70 | 62.55 |
| getFloatPairValue | 10/10 | 45.99 | 54.38 | 47.32 | 92.65 |
| getIndexedFloat | 10/10 | 45.79 | 51.32 | 47.36 | 62.69 |
| getIndexedInt32 | 10/10 | 45.51 | 54.19 | 46.78 | 93.61 |
| getPressParameter | 10/10 | 45.87 | 50.99 | 46.70 | 62.83 |
| getMachineState | 10/10 | 45.70 | 54.45 | 46.80 | 109.44 |
| getReadOnlyData | 5/5 | 47.29 | 68.45 | 61.19 | 93.06 |
| getRealTimeData | 5/5 | 31.14 | 46.51 | 46.34 | 61.93 |
| getTempDataJson | 5/5 | 62.60 | 86.31 | 92.31 | 106.98 |
| getMixDataJson | 5/5 | 141.81 | 161.83 | 153.71 | 205.19 |
| setBrightness（读回） | 5/5 | 61.53 | 93.87 | 95.54 | 110.46 |
| genericRead(MACHINE_TYPE) | 10/10 | 45.59 | 55.34 | 46.50 | 91.64 |

定时器（配置 1 s）：3 次回调，间隔 avg **1051.4 ms**，min 1048.7 / max 1054.1，标准差 **2.67 ms**。

---

## 2. 错误点

### E1【固件 · 高】RT JSON 的 T1 字段为未初始化垃圾值

- **现象**：`RealTimeData.m_CT1Step=34`、`m_T1Time=151661602 ms`，数值每次重启变化（上次为 577661287）。
- **根因**：
  - 设备当前 `mode=2`（`MODEL_STATR_P_TT`，压力+双温），该模式**不使用** `CT1Step`/`T1Time`
  - `RTData::RTData()` 构造函数为空，未初始化任何成员（`auto_hot_press/src/data/rtdata.cpp:15-18`）
  - `toJsonString()` **无条件**序列化 `CT1Step`、`T1Time`（同文件 181、190 行）
- **交叉证据**：同一时刻二进制接口 `getMachineState` 给出 `t1Step=0`、`t1Remain=0ms`（该模式不打包这两个字段），与 JSON 的垃圾值矛盾，证明问题在 JSON 输出侧而非 SDK 解析。
- **影响**：上位机若按 JSON 显示辅助温控，会显示 42 小时倒计时等异常数据。
- **建议**：构造函数零初始化全部成员；`toJsonString()` 按 `ModelState` 分支输出字段（与 `getRtData()` 保持一致）。

### E2【固件 · 中】`m_FlowChange` 从未赋值却随二进制帧下发

- **现象**：`getMachineState` 的 `flow=34`，而同帧其余字段均正常。
- **根因**：`RTData::getRtData()` 末尾无条件 `out.push_back(m_FlowChange)`（`rtdata.cpp:74`），但全工程仅在 `setFlowChange()` 中写入该成员，**没有任何调用点**，且构造函数未初始化 → 栈/堆残留值（34）。
- **影响**：上位机收到的“流量变化”字节无意义；若未来用于控制判断会引入随机行为。
- **建议**：构造函数初始化为 0；或该字段未实现前不写入帧（需同步修改上位机长度解析）。

### E3【固件 · 中】三温模式 flags 位组合使用错误变量

- **位置**：`auto_hot_press/src/data/rtdata.cpp:36`
  ```cpp
  out.push_back((m_IsPStart&0x01)|((m_IsTStart<<1)&0x02)|((m_IsTStart<<2)&0x04));
  ```
- **问题**：bit2 表示 `IsT1Start`，此处却用 `m_IsTStart` 移位，导致 `mode=3`（P_T_T）下辅助加热状态永远与主加热相同。
- **说明**：本机 `mode=2`，该分支未执行，属静态审查发现；上位机 `MachineState.m_Flags` 按 bit2→IsT1Start 解析，语义将错误。
- **建议**：改为 `((m_IsT1Start<<2)&0x04)`。

### E4【SDK · 中】`DateTimeInfo.m_Year` 文档与协议不符，存在误用风险

- **文档**：`press/include/press/presstype.h:254` 注释为“Year (full value, e.g. 2026) / 年（完整数值）”。
- **实际**：协议与固件均使用 `tm_year` 偏移值（`auto_hot_press/src/datetime/datetime.cpp:124` 直接发送 `tm_year`，本机为 126），SDK `getDateTime()` 也原样存储（`pressprivate.cpp:1470`），测试程序需 `+1900` 才显示 2026。
- **风险**：调用方按注释传入 `2026` 调用 `setDateTime()`，会被截断为 234，设备年份变为 2134。
- **建议**：修正注释为 tm_year（1900 偏移），或在 SDK 内统一转换为完整年份并同步 `setDateTime()`。

### E5【SDK · 中】`m_MaxMin` 字段语义注释错误

- **文档**：`presstype.h:119`（ReadOnlyData）与 `:213`（PressParameter）注释为“Max - Min difference / 最大最小压力差”。
- **实际**：固件该值为“自动补压值”，用于 `AfterValue = SetP - m_Max_Min`（`oneflowLogic.cc:633`、`twoflowLogic.cc:766`），界面标签为 `AutomaticPressure`（`setLogic.cc:124`），默认值 0.3。
- **反证**：本次读数 `MaxP=30.0`、`MinP=0.5`，若为差值应为 29.5，实测 0.3。
- **影响**：调用方按“差值”理解会误算保压值。
- **建议**：注释改为“自动补压值 / auto repress offset（AfterValue = SetP - value）”。

### E6【SDK · 设计】直接调用 get 接口会重复触发回调

- **现象**：基准测试中调用 5 次 `getReadOnlyData` 产生 5 条 `onReadOnlyData` 回调；调用 5 次 `getRealTimeData` 产生 5 条 `onRealTimeData`（内容完全相同）。
- **根因**：`PressPrivate::getReadOnlyData()`/`getRealTimeData()` 成功与失败路径都会主动调用 `iface->onReadOnlyData()` / `onRealTimeData()`（`pressprivate.cpp:162`、`:253`）。
- **影响**：同时启用定时器和界面轮询时，UI 会对同一份数据更新两次；错误也会重复上报。
- **建议**：明确二选一语义（定时器模式 or 主动拉取模式），或提供开关抑制主动调用的回调。

### E7【SDK/设备 · 性能与数据质量，非功能缺陷】

1. **固定 46 ms 往返地板**：所有小命令 min≈45.5 ms，其中 SDK 轮询粒度 10 ms、串口传输约 2.6 ms，其余约 33–38 ms 为固件命令处理周期。
2. **1 Hz 定时采集与用户命令争用**：max 列出现 92–109 ms 的翻倍尖峰，与定时器周期吻合。
3. **定时器实际周期 1051 ms 而非 1000 ms**：采用“等满周期再发请求”的固定延迟模型，漂移 = 请求耗时；抖动仅 2.67 ms。
4. **JSON 类受带宽限制**：TD 494 B≈93 ms、MD 1077 B≈154 ms，与 115200 波特率理论传输时间（43/94 ms）吻合。
5. **原始数据未钳位**：`downT≈-16~-19`、压力空闲时可达 `-3.048`，为传感器零漂/未接探头的真实值；`ReadOnlyData` 名称为空是因固件 `init()` 置空、`getName` 另有语言默认值，属设计差异而非缺陷。

---

## 3. 处理优先级建议

| 优先级 | 事项 | 归属 |
|---|---|---|
| P0 | E1 T1 字段未初始化导致 JSON 垃圾值 | 固件 |
| P1 | E4 年份契约错误（误用会写错设备时间） | SDK 文档/实现 |
| P1 | E5 自动补压值语义注释错误 | SDK 文档 |
| P2 | E2 FlowChange 未初始化、E3 三温 flags 位错误 | 固件 |
| P2 | E6 回调重复上报 | SDK 设计 |
| P3 | E7 波特率提升 / 定时器固定速率 / 轮询粒度优化 | SDK + 固件 |

> 本次仅做只读查询与“读出-写回”验证，未改变设备配置。
