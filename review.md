# press_sdk_project 代码审查报告

> **审查日期**：2026-11-18（全量重审）
> **审查范围**：`press/` 全部自研源码（核心引擎、端口层、工具层、C 接口、JNI 封装）与 `press/include` 公共头；`press/src/json/` 为第三方 jsoncpp，`example/`、`test/`、`interface/` 仅用于核对调用面，不逐行审查。
> **审查方式**：静态代码审查 + 修复 + 编译验证（MinGW Debug 构建 `press` 目标通过）；未进行真机回归，运行数据引用既有 `press_sdk_test_report.md` / `press_api_performance_report.md`。
>
> **状态标记规则**：🟢 绿色 = 已修复且无遗留问题；🟡 黄色 = 已修复但仍有遗留问题；🔴 红色 = 未修改 / 新发现且未处理。
>
> **修订记录**
> - 2026-11-15：首次审查，共 15 个问题（S 级 3 个、M 级 8 个、L 级 4 个）。
> - 2026-11-18：复核 + 接口重构审查，15 项历史问题保持修复，新增 E2-E7；E6 固件修复并完成真机验证。
> - 2026-11-18（本次）：全量重审并重新编号为 S1-S2 / M1-M6 / L1-L9；历史 15 项复核见第六节，E 系列遗留见第五节。
> - 2026-11-18（修复）：S1/S2、M1-M5、L1-L3、L5-L9、E2-E4 已修复并完成编译验证；M3 采用 SDK 共享所有权修复，M6 已补齐 C/JNI/Java/C#/Python/JS 的 TD/MD JSON（Dart 仍无 JSON 层，保留黄色）；L4 死代码清理未执行，E7 待固件实现。

---

## 一、总体评价

整体架构仍然清晰：命令队列 / 响应队列 + 序号匹配 + 请求串行锁，配合每响应携带错误码，能有效避免多线程请求交错和错误串扰；`m_JsonDataMutex`、`m_LifecycleMutex`、`_safeGetInterface` 等防护说明作者对并发有明确意识。端口层的帧头重同步、发送前清空接收缓冲、端口关闭清零注册号等修复也已落地。

本轮发现的风险集中在**回调与生命周期的交互**：定时线程会同步执行用户回调，而回调内调用 `stop()` / `disconnect()` / `registerDataInterface()` 会触发定时线程自 join，直接抛出 `std::system_error` 或死锁；JNI 注销路径又把已修复的“先停线程、后释放上下文”顺序写反，存在 use-after-free。上述问题已全部修复：定时器线程增加独立存活令牌与自线程 detach 保护，`connect()` 握手回调移出生命周期锁，JNI 注销改为先停线程再释放上下文并加锁登记；停止流程改为先置停止标志、清空队列并唤醒等待者，避免停止阻塞和命令漏发。其余为协议契约、跨语言接口覆盖和文档一致性问题的修复。

---

## 二、S 级（严重：可崩溃或功能失效）

| # | 位置 | 问题 | 建议 |
|---|------|------|------|
| 🟢 S1 | [pressprivate.cpp:790-837](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L790-L837)、[632-643](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L632-L643)、[58-106](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L58-L106) | **回调内调用生命周期 API 会自 join 或重入生命周期锁。** ① `onRealTimeData` / 定时失败 `onError` 在定时线程内同步执行（811-823）；用户在回调中调用 `stop()` / `disconnect()` / `registerDataInterface()` → `_stopRealTimeTimer()` 对**当前线程自身** `join()`（832-835），抛出 `std::system_error` 并沿线程入口传播为 `std::terminate`；② `connect()` 持有 `m_LifecycleMutex` 期间，版本握手失败会在该锁内触发 `onError`（232-245），回调若调用 `disconnect()` 等接口则对非递归互斥量二次加锁 → 死锁。`press.hpp` 明确宣称“所有公共方法线程安全，可从任意线程调用”，当前实现与该契约冲突 | `_stopRealTimeTimer` / `_stopScheduler` 增加自线程检测：`get_id()==this_thread::get_id()` 时改为置停止标志并延迟回收（不可直接 detach 后让对象析构）；或提供“请求停止”机制由控制线程执行 join。`connect()` 的握手/错误回调应移到释放 `m_LifecycleMutex` 之后，或在文档与运行时同时禁止回调内调用生命周期接口 |
| 🟢 S2 | [press_jni.cpp:1250-1270](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L1250-L1270) | **JNI 注销回调顺序颠倒，存在 use-after-free。** `Java_PressSdk_press_unregister_data_interface` 先 `cleanupCallbackContext`（删除 `JniCallbackContext` 与全局引用），**之后**才 `cpress_unregister_data_interface` 停定时线程。若定时线程回调正在执行（1 Hz 采集随时可能命中），或另一线程的同步 get 正在回调中，回调访问的 `userData` 已被释放 → 崩溃。该问题与 2026-11-15 的 S2 同类，`press_destroy` 已修对，但本函数写反 | 与 `press_destroy` 保持一致：先 `cpress_unregister_data_interface(ctx)`（join 定时线程），再 `cleanupCallbackContext` 并从 map 删除。同时建议明确“销毁/注销期间调用方不得并发调用数据接口”的约束 |

---

## 三、M 级（中等：数据竞争、语义缺陷）

| # | 位置 | 问题 | 建议 |
|---|------|------|------|
| 🟢 M1 | [pressprivate.cpp:115-122](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L115-L122) | `isConnected()` 无锁读取 `mpPort`。`connect()` / `disconnect()` 在 `m_LifecycleMutex` 保护下 `mpPort.reset()`（68-72、646-655），并发调用 `isConnected()` 属于对 `std::unique_ptr` 的数据竞争，可能读到已析构对象（UAF）。该方法为公共接口，且 `press.hpp` 声明所有公共方法线程安全 | 使用 `std::atomic<bool>` 维护连接状态，或在 `isConnected()` 内加 `m_LifecycleMutex`；端口指针的读写统一走同一把锁 |
| 🟢 M2 | [press_jni.cpp:1184-1196](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L1184-L1196)、[1246-1247](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L1246-L1247) | `g_callbackMap[ctx] = jniCtx` 的写入**没有持有** `g_callbackMapMutex`（锁只覆盖了前面的旧上下文清理块）。与 `press_destroy`、`press_unregister_data_interface`、下一次 `press_register_data_interface` 并发时，`std::unordered_map` 的并发插入/查找/删除属于未定义行为，可能破坏桶链导致崩溃 | 将新上下文的登记放入 `g_callbackMapMutex` 保护范围；注册与 map 插入保持原子，避免两个线程同时注册时后写覆盖、前一个 `jniCtx` 泄漏 |
| 🟢 M3 | [cpress.cpp:662-679](file:///E:/code/work/press_project/press_sdk_project/press/src/c/cpress.cpp#L662-L679)、[pressprivate.cpp:737-741](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L737-L741)、[151-154](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L151-L154) | **历史 M4 的残留。** 重复注册时已改为“先 unregister、再 reset adapter”，但 `unregisterDataInterface` 只保证定时线程无在途回调；另一用户线程的同步 get 可能已通过 `_safeGetInterface()` 取得非拥有 `shared_ptr`，随后 `callbackAdapter.reset()` 析构旧 adapter，该线程仍会调用其虚函数 → UAF。C++ 侧重复注册替换回调对象时同理（接口为非拥有指针） | 让 `PressPrivate` 持有 adapter 的 `shared_ptr`（C API 可传拥有语义的包装），或在文档中明确“替换/注销回调时调用方必须保证无并发数据请求”；更稳妥的做法是把回调对象的生命周期托管给 SDK。**修复：C API 的 adapter 改由 `shared_ptr` 托管并通过 `registerDataInterfaceOwned()` 注册，在途回调可安全完成** |
| 🟢 M4 | [pressprivate.cpp:632-643](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L632-L643)、[790-837](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L790-L837)、[451-472](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L451-L472) | **停止/断连可能阻塞最长一个请求超时。** `_stopScheduler()` 先 `_stopRealTimeTimer()`，而定时线程可能正阻塞在 `_requestCommand` 的 `m_ResponseCond.wait_for` 中；该等待谓词只检查 `m_IsRunning`，此时仍为 true，`m_TimerRunning=false` 无法唤醒它。若用户把超时调到 60 s 且设备无响应，`disconnect()` / 析构会卡住最长 60 s | 停止顺序改为先置 `m_IsRunning=false` 并 `notify_all`，让在途请求立即以 `Stopped` 返回，再 join；或在 `_requestCommand` 的等待谓词中加入定时停止/取消标志 |
| 🟢 M5 | [pressprivate.cpp:503-530](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L503-L530)、[632-643](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L632-L643)、[466-472](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L466-L472) | **停止后残留命令仍会被下发。** `_requestCommand` 可能先以 `Stopped` 返回给调用方，但其已入队的命令仍留在 `m_CommandQueue`；调度线程的退出条件只在 `!m_IsRunning && queue.empty()` 时 break，队列非空时仍会 pop 并写端口。结果是调用方收到“已停止/失败”，写命令或控制命令却可能已发送到设备 | `_stopScheduler()` 中清空 `m_CommandQueue`；调度线程 pop 后再次检查 `m_IsRunning`，为 false 时回 `Stopped` 而不是发送。对写/控制命令尤其重要 |
| 🟡 M6 | [press.h:173-217](file:///E:/code/work/press_project/press_sdk_project/press/include/press/press.h#L173-L217)、[88-93](file:///E:/code/work/press_project/press_sdk_project/press/include/press/press.h#L88-L93)、[cpress.cpp:208-266](file:///E:/code/work/press_project/press_sdk_project/press/src/c/cpress.cpp#L208-L266) | **C 接口与各语言绑定未同步 TD/MD JSON 能力。** C API 只暴露 ROD/PD/RT 四个 JSON 接口，没有 `cpress_get/set_temp_data_json`、`cpress_get/set_mix_data_json`；而 `CPressDataCallbacks` 中已提供 `onTempData` / `onMixData` 槽位——C/JNI/Java/C#/Python/JS/Dart 用户永远无法通过这些槽位收到数据。C 接口同时缺少 `getMachineState`、`getName`、`getPressParameter`、`getDateTime` 等公共能力，与 `press.hpp` 的能力面差距较大 | 补齐 C API 与五语言绑定的 TD/MD JSON 接口；或将 C 接口定位为“子集”并在 `press.h` 明确列出未覆盖能力、删除无法触发的回调槽位。**修复：C/JNI/Java/C#/Python/JS 已补齐；Dart 绑定本身无 JSON 层，保留黄色** |

---

## 四、L 级（轻微：语义/一致性/可维护性）

| # | 位置 | 问题 | 建议 |
|---|------|------|------|
| 🟢 L1 | [pressprivate.cpp:796-826](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L796-L826) | 定时采集采用“请求完成后 `wait_for(interval)`”，实际周期 = 设定周期 + 单次请求耗时，实测约 1047 ms（报告已记录）。长期运行会产生累积漂移，无法作为稳定采样时钟 | 改为固定速率调度：维护 `next_deadline += interval`，`wait_until(next_deadline)`；若已错过则跳过错过的节拍并重新对齐 |
| 🟢 L2 | [press.hpp:285-287](file:///E:/code/work/press_project/press_sdk_project/press/include/press/press.hpp#L285-L287)、[579-581](file:///E:/code/work/press_project/press_sdk_project/press/include/press/press.hpp#L579-L581)、[typeprivate.h:83](file:///E:/code/work/press_project/press_sdk_project/press/src/typeprivate.h#L83)、[242](file:///E:/code/work/press_project/press_sdk_project/press/src/typeprivate.h#L242) | 协议长度契约不一致：`getPressParameter` 注释写 20 字节，实际实现与固件均为 21 字节；`getMachineState` 注释写“3-5 bytes”，实际为 20/25/30 字节；`PRESS_PARAMETER_TYPE_BOOL(size == 20)` 与实际不符且已无调用者，`SET_JSON_BOOL` 亦未使用 | 统一注释与实现：压力参数块改为 21 字节；`getMachineState` 注明 20/25/30 及模式对应关系；删除或修正失效宏，避免后续维护者按旧契约改代码 |
| 🟢 L3 | [presstype.h:83](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L83)、[175-178](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L175-L178) | `m_Max_Min` / `m_MaxMin` 注释仍为“最大最小压力差”，实际语义是自动补压值（`AfterValue = SetP - value`，默认 0.3）。调用方按注释理解会误用 | 注释改为“自动补压值 / auto repress offset”，并在参数块注释中给出计算关系 |
| 🔴 L4 | [jsondata.h:25-31](file:///E:/code/work/press_project/press_sdk_project/press/src/tool/jsondata.h#L25-L31)、[pressprivate.h:288](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.h#L288)、[pressprivate.cpp:743-748](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L743-L748) | **内部 JSON 解析层已成为死代码。** `JsonData::jsonToReadOnlyData` / `jsonToRealTimeData` / `jsonToPressData` 全仓库无调用者；`m_JsonData` 仅被离散 get 接口写入，而 `PressPrivate::getMachineData()` 未被 `Press` 暴露、也无调用者。结果是继续携带 jsoncpp 与一套无人使用的解析逻辑，增加二进制体积和维护误导 | 二选一：删除 `jsondata`/`jsontovalue` 及 `m_JsonData` 相关死代码（保留导出的 `toJsonString` 即可），或把 JSON get 接口重新接入解析并对外暴露 `getMachineData()` |
| 🟢 L5 | [pressprivate.cpp:316-352](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L316-L352) | `_buildSendFrame` 仅用 `assert` 校验 payload ≤ 0xFFFF；Release 下 assert 失效，超长 JSON（>65534 字节）会被静默截断长度字段，发出畸形帧，失败原因难以定位 | 在 `_requestCommand` 入口做运行时长度校验并返回明确错误码；或让 `_buildSendFrame` 返回 `bool` + 错误 |
| 🟢 L6 | [pressprivate.cpp:22-23](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L22-L23)、[354-361](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L354-L361)、[565-573](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L565-L573) | 空数据区响应帧实际为 12 头 + 0 数据 + 1 校验 = **13 字节**，但 `CMD_LEN = 14`：扫描循环要求缓冲 ≥14 才开始解析，`_parseResponseFrame` 也要求 ≥14，因此 `len == 0` 的合法响应不会被识别，只能等满超时 | 扫描下限改为 12（可读到长度字段），整帧下限改为 13；`_parseResponseFrame` 的最小长度同步改为 13 |
| 🟢 L7 | [pressprivate.cpp:280-314](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L280-L314)、[258-267](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L258-L267) | 控制类写命令的 1 字节响应 `0x00`(停止成功) / `0x01`(启动成功) 一律视为成功，未校验回执与请求方向是否一致。例如 `setPressing(false)` 收到 `0x01` 时 SDK 仍返回 true，调用方无法感知设备实际已启动 | 按请求方向传入期望状态码并比对；不一致时返回 `StateUnchanged` 或专用错误码 |
| 🟢 L8 | [press/CMakeLists.txt:11](file:///E:/code/work/press_project/press_sdk_project/press/CMakeLists.txt#L11) | `find_package(JNI REQUIRED)` 且 JNI 源文件始终参与编译，意味着纯 C++/C 使用者构建 SDK 也必须安装 JDK；未安装 JDK 的环境直接配置失败 | 增加 `PRESS_BUILD_JNI` 选项（默认 OFF，Android 或显式开启时 ON），JNI 源文件与 `JNI_INCLUDE_DIRS` 仅在开启时加入 |
| 🟢 L9 | [press.hpp:118-121](file:///E:/code/work/press_project/press_sdk_project/press/include/press/press.hpp#L118-L121)、[press_jni.cpp:73-113](file:///E:/code/work/press_project/press_sdk_project/press/src/jni/press_jni.cpp#L73-L113) | 线程契约文档不准确：公共头与 Java 绑定均声称回调在“内部调度线程 / SDK 工作线程”执行，但主动 get 接口触发的 `on*` 回调实际在**调用方线程**同步执行（只有定时采集与 `onError` 可能在定时线程）。调用方按文档做线程判断会失效 | 文档明确区分：定时器回调在工作线程，主动 get 回调在调用线程；JNI/C# 绑定同步修正注释 |

---

## 五、E 系列遗留问题（沿用 2026-11-18 复核编号）

| # | 位置 | 问题 | 建议 |
|---|---|---|---|
| 🟢 E2 | [presstype.h:218](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L218)、[pressprivate.cpp:2228-2272](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L2228-L2272) | `DateTimeInfo::m_Year` 注释为完整年份（如 2026），但协议与固件使用 `tm_year`（1900 偏移）。按注释传 2026 会被 `uint8_t` 截断为 234，设备年份变为 2134；`getDateTime` 也会返回 126 这类偏移值 | 注释改为 `tm_year`，或在 `getDateTime` / `setDateTime` 内统一做 ±1900 转换并同步更新文档 |
| 🟢 E3 | [presstype.h:83](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L83)、[175-178](file:///E:/code/work/press_project/press_sdk_project/press/include/press/presstype.h#L175-L178) | 同 L3：`m_Max_Min` / `m_MaxMin` 的“最大最小压力差”注释与实际“自动补压值”语义不符 | 修正注释 |
| 🟢 E4 | [pressprivate.cpp:129-230](file:///E:/code/work/press_project/press_sdk_project/press/src/pressprivate.cpp#L129-L230) 等 5 个 `get*Json` | 主动调用 get 接口时，成功/失败都会触发对应 `on*` 回调；与 1 Hz 定时采集并用时同一份数据被上报两次（`getTempDataJson` / `getMixDataJson` 失败还会同时触发 `onTempData`/`onMixData` 与 `onError`）。测试报告已记录该重复现象 | 明确“定时器模式 / 主动拉取模式”二选一，或增加抑制主动回调的开关；失败时只保留一条上报路径 |
| 🟢 E5 | 全链路 | 性能符合 115200 串口带宽预期（小命令约 46 ms 地板、TD 494 B≈93 ms、MD 1077 B≈152 ms），非缺陷 | 可结合 L1 做固定速率调度，并评估提高波特率 |
| 🟢 E6 | `auto_hot_press` 固件 PD/TD/MD 的 SET_JSON 分支 | 固件已与 ROD 对齐：读取 payload 首字节压缩标志后剥离，剩余数据交给解析函数；2026-11-18 真机写回 3/3 通过 | 保持 |
| 🔴 E7 | `auto_hot_press` | `CMDID_SET_START_DEMOLD`(0x1523) 未实现，`setDemolding` 两个方向均失败；SDK 侧 C++/C/JNI 均已暴露该接口 | 固件补充处理分支，或在 SDK 文档标注“暂不支持”并考虑暂时下架接口 |

---

## 六、历史 15 项修复复核

| # | 状态 | 复核说明 |
|---|------|----------|
| S1 | 🟢 | JNI `attachCurrentThread` 带 `didAttach` 出参，仅本次附加的线程 Detach（press_jni.cpp:53-66、108-112） |
| S2 | 🟢 | `press_destroy` 与 `press_unregister_data_interface` 均已按“先注销、后清理”执行，见新 S2 修复 |
| S3 | 🟢 | 发送前 `flushInput()`（pressprivate.cpp:534）；接收循环按校验和逐字节重同步（565-588） |
| M1 | 🟢 | `getMachineData()` 改为加锁值返回（743-748），当前无调用者，见 L4 |
| M2 | 🟢 | `intervalSeconds <= 0` 不再启动定时线程（709-719），避免高频空转 |
| M3 | 🟢 | 所有 JNI 回调在 Detach 前 `clearPendingException`（107、146、184、221、258、304） |
| M4 | 🟢 | C API 改为 `shared_ptr` 托管 adapter 并调用 `registerDataInterfaceOwned`，在途同步回调可安全完成；C++ 原始指针路径仍在文档中约定由调用方保证生命周期 |
| M5 | 🟢 | 注册号/命令/长度均显式按大端逐字节写入（pressprivate.cpp:325-346） |
| M6 | 🟢 | 新增 `setRequestTimeout`（C++/C/JNI），0 恢复 300 ms、上限 60 s |
| M7 | 🟢 | 定时采集连续失败达到 3 次时上报 `onError`（816-823） |
| M8 | 🟢 | 旧硬编码长度 30 的 JNI 辅助函数已删除；新数组接口均先 `GetArrayLength` 校验（如 495、519、591） |
| L1 | 🟢 | `_handleSetResponse` 对空回执返回错误，不再读取校验和字节（286-291） |
| L2 | 🟢 | 命令 ID 统一为 `pressprivate.h` 中的 `static constexpr` 常量，别名宏已删除 |
| L3 | 🟢 | `Unity::getCurrentTime` 使用 `localtime_s` / `localtime_r`（unity.cpp:22-28） |
| L4 | 🟢 | `getFloatVector` 参数长度校验已前置（jsontovalue.cpp:151-155） |

---

## 七、做得好的地方（保持）

- 请求串行锁 + 序号匹配 + 错误码随响应携带（pressprivate.cpp:419-493），定时采集与用户请求共用链路时不会互相覆盖错误。
- 响应帧校验和取“头部 + 数据区”之后，并在注释中说明不能取缓冲区末字节；帧头重同步与长度上限 `FRAME_MAX_DATA_LEN` 配合，能从中断/脏数据中恢复。
- `_closePort()` 统一关闭端口并清零 `m_RegisterNo`，避免重连后误用旧注册号。
- `press_jni.cpp` 的 Attach/Detach 配对与异常清理完整，`press_destroy` 的销毁顺序正确。
- `JsonToValue` 对每个字段做存在性 → 类型 → 范围 → 长度校验，字符串写回均显式补 `'\0'`，无越界。
- 端口层 Windows/POSIX 双实现对称，非阻塞 connect + select、部分读写循环、EINTR/EAGAIN 处理完整；TCP 增加 `flushInput` 排空残留。
- `jsondata` 对固件未初始化字段（T1 相关）做宽容解析，避免垃圾值污染整包解析结果。

---

## 八、验证与建议

### 8.1 本轮验证情况

- 已修复代码通过 MinGW Debug 编译：`press` 共享库目标构建成功（含 JNI 源文件）。
- 未执行真机回归；涉及设备行为的结论（定时周期、控制回执、TD/MD 写回）仍需在 COM14 上复测。
- 既有真机回归（COM14）：72 个可基准接口中 **67 通过、0 失败、1 跳过（E7 `setDemolding`）**；性能数据见 `press_api_performance_report.md`，明细见 `press_sdk_test_report.md`。

### 8.2 剩余事项

| # | 状态 | 说明 |
|---|------|------|
| L4 | 🔴 | 内部 JSON 解析层与 `getMachineData()` 仍为死代码；删除会移除内部缓存能力，属于结构性清理，未纳入本次缺陷修复 |
| M6 | 🟡 | C/JNI/Java/C#/Python/JS 已补齐 TD/MD JSON；Dart 绑定本身无 JSON 层（也没有回调层），如需完整能力需单独补齐整套 JSON API |
| E7 | 🔴 | 固件 `CMDID_SET_START_DEMOLD (0x1523)` 未实现；SDK 已在 `setDemolding` 注释中标注“暂不支持”，需固件侧补分支 |

### 8.3 建议复测项

1. 回调内调用 `disconnect()` / `stop()` / `unregisterDataInterface()` / `registerDataInterface()`，确认无崩溃和死锁。
2. 连接失败握手时在 `onError` 中调用 `disconnect()`，确认不阻塞。
3. 定时采集中调用 `disconnect()`，确认 `disconnect()` 不再等待完整请求超时，且停止后无残留控制命令下发。
4. 定时采集开启时主动调用 `getRealTimeDataJson`，确认只收到定时器的一次回调；关闭定时器后确认主动拉取仍会触发回调。
5. 控制命令 `setPressing` / `setStart` / `setStartHeat` 的回执方向校验，确认正常启停不被误判。
6. JNI/C#/Python/JS 的 TD/MD JSON 读写回归，以及 Java 注销回调后重新注册。
