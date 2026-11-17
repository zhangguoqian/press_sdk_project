# press_sdk_project 代码审查报告

> **审查日期**：2026-11-18
> **审查范围**：`press/` 全部自研源码（核心引擎、端口层、工具层、C 接口、JNI 封装）。
> `press/src/json/` 为第三方 jsoncpp 库，`example/`、`test/`、`build/` 不在本次范围。
>
> **状态标记规则**：🟢 绿色 = 已修复且无遗留问题；🟡 黄色 = 已修复但仍有遗留问题；🔴 红色 = 未修改。
>
> **修订记录**
> - 2026-11-15：首次审查，共 15 个问题（S 级 3 个、M 级 8 个、L 级 4 个）。
> - 2026-11-15：**全部 15 个问题修复完成**（S1-S3、M1-M8、L1-L4），并实现两个遗留功能扩展（`getSerialNumber` 查询、采集周期参数化）。已用 CMake + MinGW 13.1 完成编译验证，`libpress.dll`（含核心引擎、C 接口、JNI）构建通过；Qt 示例链接失败为验证工具链与 llvm-mingw 版 Qt 库 ABI 不匹配，与本次改动无关。C#/Dart/JS/Python 四个语言绑定待同步新增的 `get_serial_number` / `set_request_timeout` / 注册周期参数。
> - 2026-11-18：**复核 + 接口面重构审查**。15 个历史问题保持修复状态；期间完成公共接口收敛（删除通用命令接口与类型化命令助手、数据接口全面 JSON 字符串化、新增 32 个压力/温度命令接口、命令 ID 统一为 `static constexpr` 常量、公共头不再依赖私有 `typeprivate.h`）。本轮新增 6 项发现（E2 年份契约、E3 自动补压值注释、E4 回调重复、E5 性能、E6 JSON 写命令压缩标志、E7 脱模命令未实现），其中 E6 已于同日修复并真机验证，详见第七节。

---

## 一、总体评价

整体架构清晰：命令队列 / 响应队列 + 序号匹配 + 请求串行锁的设计正确规避了多线程请求交错问题；`m_JsonDataMutex`、`m_LifecycleMutex`、`_safeGetInterface` 等防护说明作者对并发有意识，比一般嵌入式 SDK 规范。内存安全方面 jsondata/jsontovalue 的越界与类型校验做得很到位。

主要风险集中在两处：**① JNI 层回调线程的 Attach/Detach 不配对与回调上下文先释放后注销的竞态（S1/S2），可在 Android 上直接崩溃；② 响应流无帧头重同步与残留清理（S3），设备半帧/脏数据后会持续失联直到重连**。其余为生命周期、健壮性与可维护性问题。

---

## 二、S 级（严重：可崩溃或功能失效）

| # | 位置 | 问题 | 建议 |
|---|------|------|------|
| 🟢 S1 | [press_jni.cpp:296-312](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L296-L312)、[350](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L350)、[387](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L387)、[424](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L424)、[465](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L465) | JNI 回调 Attach/Detach 不配对：`attachCurrentThread()` 在线程**已附加**时（`GetEnv` 返回 `JNI_OK`）直接返回 env，但 4 个回调结尾均**无条件**调用 `detachCurrentThread()`。同步接口（`press_get_read_only_data` 等）在 Java 调用线程内同步触发回调 → 会把调用者自身（如 UI 主线程）的 JNI 环境分离，该线程后续任何 JNI 调用直接崩溃（ART abort） | `attachCurrentThread` 增加 `bool* didAttach` 出参（或返回结构体），仅本次真正 Attach 的线程才在回调结束时 Detach |
| 🟢 S2 | [press_jni.cpp:507-518](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L507-L518) | `press_destroy` 先 `cleanupCallbackContext`（`delete jniCtx`、删全局引用），**之后**才 `cpress_unregister_data_interface`（停定时线程）。若定时线程回调正在执行，回调持有的 `ctx`（userData）已被释放 → use-after-free | 顺序倒置：先 unregister 停线程（join 完成后无在途回调），再清理全局引用与 jniCtx |
| 🟢 S3 | [pressprivate.cpp:566-601](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L566-L601) | 响应流无帧头重同步、无残留清理：① 超时后 `rawResponse` 丢弃，但串口缓冲中残留的半帧/迟到字节仍在，下一请求从错位字节开始拼接；② 帧长检测固定取缓冲 `[10],[11]` 当长度字段，不扫描 8 字节注册号帧头，一旦错位永远无法恢复，表现为持续超时直到重连 | ① 每次发送请求前清空端口接收缓冲（`PurgeComm`/`tcflush`）；② `_parseResponseFrame` 失败时从缓冲中丢弃首字节重新扫描帧头，而非整包丢弃 |

## 三、M 级（中等：数据竞争、健壮性缺陷）

| # | 位置 | 问题 | 建议 |
|---|------|------|------|
| 🟢 M1 | [pressprivate.cpp:724-727](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L724-L727) | `getMachineData()` 无锁返回 `m_JsonData` 的 const 引用；定时线程正持 `m_JsonDataMutex` 对其解析写入，调用方并发读取 → 数据竞争（`getReadOnlyData` 内部专门加锁说明该成员需要保护） | 改为值返回：加锁拷贝一份 `JsonData` 返回 |
| 🟢 M2 | [pressprivate.cpp:683](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L683)、[735-744](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L735-L744) | `registerDataInterface` 仅用 `assert(intervalSeconds > 0)` 校验周期，Release 构建下 assert 失效；传入 0 或负数时 `wait_for` 时长 ≤0 立即超时 → 定时线程高频空转并连续轰炸设备 `getRealTimeData` | 运行时校验：`intervalSeconds <= 0` 时钳制为默认值 1 或拒绝注册并上报错误 |
| 🟢 M3 | [press_jni.cpp:339-343](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L339-L343) 等 4 处回调、[46-51](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L46-L51) | `CallVoidMethod` 调用 Java 回调后无 `ExceptionCheck/Clear`：Java 侧回调抛异常时，带着挂起异常执行 `DetachCurrentThread` → ART abort；`GetFieldID` 失败同样留下挂起异常未清理 | 每次 JNI 可能抛异常的调用后检查 `ExceptionCheck()`，必要时 `ExceptionDescribe()+ExceptionClear()` |
| 🟢 M4 | [cpress.cpp:257-259](file:///E:/code/work/press_project/press_sdk_project/press/src/c/cpress.cpp#L257-L259) | 重复调用 `cpress_register_data_interface` 时，`handle->callbackAdapter.reset(new ...)` 立即析构旧 adapter，而 impl 内 `mpMachineDataInterface` 是**非拥有** shared_ptr（空删除器）；若旧 adapter 尚有回调在途（如刚被 `_safeGetInterface` 取出），访问已析构对象 | 注册路径先走一遍 unregister（停定时线程、清引用）再换 adapter，保证无在途回调 |
| 🟢 M5 | [pressprivate.cpp:354-360](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L354-L360) | `_buildSendFrame` 用 `reinterpret_cast<const uint8_t*>(&cmd)` 写命令字段：正确性依赖"m_Command 已存 htons 值 + 主机为小端"两个隐含条件，大端平台会双重交换发错字节序；长度字段同法写 `htons` 结果，同样隐晦 | 显式按字节写大端：`p[8]=cmd>>8; p[9]=cmd&0xFF`（cmd 用原始值），长度同理，消除隐含假设 |
| 🟢 M6 | [pressprivate.cpp:470-512](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L470-L512) | 请求超时固定 300ms 且无重试；对串口 115200 波特率下较大 JSON 响应（PressData 约 1KB ≈ 90ms 传输）余量尚可，但 TCP 弱网时偏紧；超时后不做端口级恢复 | 超时时间做成可配置；连续 N 次超时后自动 flush 端口缓冲或触发断线回调 |
| 🟢 M7 | [pressprivate.cpp:750-752](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L750-L752) | 定时线程忽略 `getRealTimeData` 返回值：设备拔线后每秒一次超时循环永久持续，无退避、无断线事件上报，上层只能靠错误码回调自行发现 | 统计连续失败次数，超过阈值时通过 `onError` 上报专用错误码（如 `ConnectFailed`），并考虑暂停采集 |
| 🟢 M8 | [press_jni.cpp:167-195](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L167-L195)、[243-278](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L243-L278) | `fillPressData`/`readPressData` 硬编码数组长度 30 并直接 `Set/GetFloatArrayRegion`，未校验 Java 侧数组长度是否为 30（Java 端改字段长度即越界） | 取 `env->GetArrayLength` 校验后再拷贝，不匹配时报错并清异常 |

## 四、L 级（轻微：语义/一致性问题）

| # | 位置 | 问题 | 建议 |
|---|------|------|------|
| 🟢 L1 | [pressprivate.cpp:425-429](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L425-L429) | SET 命令响应若 `readDataLen == 0`，`push_back(response[12])` 取到的是校验和字节（边界检查虽通过，但语义错误） | `readDataLen >= 1` 时才取 `response[12]`，否则视为 `ResponseTooShort` |
| 🟢 L2 | [pressprivate.cpp:261](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L261)、[268](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L268) | 版本查询失败上报用 `CMDID_VERSION`，其余命令用 `*_V` 别名常量（值相同），命名不一致易误导 | 统一使用 `GET_VERSION_V` |
| 🟢 L3 | [unity.cpp:22](file:///E:/code/work/press_project/press_sdk_project/press/src/common/unity.cpp#L22) | `std::localtime` 非线程安全（POSIX 下并发调用可能返回 nullptr 并被解引用；MSVC 报弃用警告） | 改用 `localtime_r` / `localtime_s` |
| 🟢 L4 | [jsontovalue.cpp:148-155](file:///E:/code/work/press_project/press_sdk_project/press/src/tool/jsontovalue.cpp#L148-L155) | `getFloatVector`（vector 上限重载）在解析成功**之后**才校验 `maxLimitValue.size() != maxSize`；参数错误时出参 `value` 已被填充，产生副作用 | 参数长度校验移到解析之前 |

## 五、做得好的地方（保持）

- 响应队列按序号匹配、丢弃迟到旧响应（[pressprivate.cpp:493-510](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L493-L510)），错误码随响应携带而非读全局 —— 规避了定时器与用户请求互相覆盖错误的问题。
- 校验和取自 `帧头+数据` 之后（[pressprivate.cpp:390-396](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L390-L396)），并注释了"不能取缓冲区最后一字节"的原因，边界检查完整。
- `jsontovalue.cpp` 对每个字段做 存在性→类型→范围→长度 四重校验，jsondata 全部 `strncpy` 显式补 `'\0'`，无越界。
- 端口层 Windows/POSIX 双平台实现对称，非阻塞 connect + select 超时、部分读写循环、EINTR/EAGAIN 处理均正确。
- `Press` 析构经 `PressPrivate::stop()` join 全部线程后再关端口，无裸线程泄漏。

## 六、遗留事项

- ~~`ReadOnlyData::m_SerialNumber` 只序列化不解析~~ **已实现**：新增 `Press::getSerialNumber()`（CMDID_SERIAL_NUMBER 0x0003，C/JNI/Java 三层同步暴露），`jsonToReadOnlyData` 现将 JSON 中的 `SerialNumber` 字段作为可选项解析填充（[jsondata.cpp](file:///E:/code/work/press_project/press_sdk_project/press/src/tool/jsondata.cpp#L195-L201)）。
- ~~JNI 采集周期固定 1 秒~~ **已实现**：`cpress_register_data_interface` 与 `press_register_data_interface` 增加 `intervalSeconds` 参数，≤0 时不启动定时采集（与 press.hpp 文档一致）；Java 层保留原签名重载（默认 1 秒）。
- **待同步**：`interface/` 下 C#（PressSdk.cs）、Dart、JavaScript、Python 四个语言绑定需同步新增 `get_serial_number` / `set_request_timeout` 接口与注册周期参数。
- `build_review/` 为本次编译验证的临时目录（已加入 .gitignore），文件被 Qt Creator 进程锁定暂时无法删除，关闭 Qt Creator 后可手动删除。

## 七、2026-11-18 复核

### 7.1 复核结论

S1-S3、M1-M8、L1-L4 共 15 项**仍保持修复状态**，未发现回归。受接口重构影响的三处表述需更新：

| 条目 | 变化 |
|---|---|
| M8 | `fillPressData` / `readPressData` 等硬编码长度 30 的 JNI 辅助函数已随 JSON 字符串化删除，问题不复存在 |
| L2 | `GET_VERSION` 等 12 个命令别名宏已删除，全部命令统一为 `pressprivate.h` 中的 `static constexpr` 常量（读取 `GET_*_V`、写入/控制 `SET_*_V`） |
| S1/S2/M3 | JNI 回调改为 `const char* json` + `NewStringUTF`，Attach/Detach 配对、注册前先注销、异常清理逻辑保持不变 |

### 7.2 本轮接口面变更

- 删除通用命令接口 `genericRead` / `genericWrite` 及其 C API、语言绑定。
- 删除 12 个类型化命令助手（`getByteValue` / `setFloatValue` / `getIndexedInt32` 等）。
- 8 个 JSON 数据接口统一命名并改为 `std::string` 直传：`getReadOnlyDataJson` / `getPressDataJson` / `setPressDataJson` / `getRealTimeDataJson` / `getTempDataJson` / `setTempDataJson` / `getMixDataJson` / `setMixDataJson`。
- 新增 32 个命令接口：压力单值/索引 18 个（段数量、校准值、模具类型、圆形/矩形尺寸、排气时间、按索引的压力/保压/保压时间）、温度单值/索引 14 个（段数量、速度模式、按索引的温度/速度/保温时间、最大/最小水量）。
- `PressDataInterface` 五个数据回调全部改为 `const std::string& json`，并新增 `onTempData` / `onMixData`。
- C API（`cpress_*`）、JNI/Java、C#、Python、JavaScript 同步；Dart 未暴露 JSON 接口，无需改动。
- 公共头 `presstype.h` 不再包含私有 `typeprivate.h`，命令常量宏移入 `press/src/typeprivate.h`，安装包不再发布私有头。
- `press.hpp` 按连接管理、机器身份、运行控制、JSON 数据接口、设备参数、系统命令、状态控制、静态工具八类重排。

### 7.3 新增发现

| # | 位置 | 问题 | 建议 |
|---|---|---|---|
| 🟡 E2 | [presstype.h:218](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L218) | `DateTimeInfo::m_Year` 注释为完整年份（如 2026），但协议与实现均为 `tm_year`（1900 偏移）；按注释传入 2026 会被截断为 234，设备年份变为 2134 | 注释改为 tm_year，或 SDK 内统一转换并同步 `getDateTime()` / `setDateTime()` |
| 🟡 E3 | [presstype.h:83](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L83)、[177](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L177) | `m_Max_Min` / `m_MaxMin` 注释为“最大最小压力差”，实际是自动补压值（`AfterValue = SetP - value`，默认 0.3） | 注释改为“自动补压值 / auto repress offset” |
| 🟡 E4 | [pressprivate.cpp](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L129-L156) 等 5 个 `get*Json()` | 主动调用 get 接口时，成功/失败路径都会触发对应 `on*` 回调；与定时采集同时使用时同一份数据会被上报两次 | 明确“定时器模式 / 主动拉取模式”二选一，或提供抑制主动回调的开关 |
| 🟢 E5 | 全链路 | 性能符合 115200 串口带宽预期（小命令 46 ms 地板、TD 494 B≈93 ms、MD 1077 B≈152 ms、定时器 1047 ms±6.22 ms），非缺陷 | 后续可考虑提高波特率、固定速率定时器、缩小轮询粒度 |
| 🟢 E6 | [receive.cpp](file:///E:/code/work/press_project/auto_hot_press/src/interface/receive.cpp) PD/TD/MD 的 SET_JSON 分支 | 固件原未剥离 SDK 写入的首字节压缩标志，导致三个 JSON 写接口失败；**已修复**：与 ROD 对齐，读 `data[0]` 为标志，`data+1`/`size-1` 交给解析函数 | 已通过 2026-11-18 真机写回验证（3/3） |
| 🔴 E7 | `auto_hot_press` | `CMDID_SET_START_DEMOLD`(0x1523) 未实现，`setDemolding` 两个方向均失败 | 补充处理分支，或在协议文档标注暂不支持 |

> 固件侧 `RTData` 成员未初始化（T1 垃圾值）、`FlowChange` 未初始化、混合段参数 23/24 字节 off-by-one 三项已由固件修复并在 2026-11-18 真机回归通过，不属本仓库范围。

### 7.4 验证方式

- Release 构建：`press.dll`、`press_bot_qtgui.exe`、`press_cpp_test.exe` 全部通过，无警告。
- 真机回归：串口 COM14，72 个可基准接口中 **67 项通过、0 项失败、1 项跳过（E7）**；含 22 项参数设置“读出-写回-校验”、4 项真实运行控制命令（加热、加压、全部启动、模式切换）及 3 项 JSON 批量写回，结束后设备恢复空闲与原模式。
- 性能报告：`press_api_performance_report.md`（全部接口 min/avg/p50/max），明细见 `press_sdk_test_report.md`。
