# 设备只读参数记录（ReadOnlyData）

连接设备后读取的 ReadOnlyData JSON 已解析到 `PressReadOnlyData`（`src/core/press_service.h`），
“关于本机”卡片展示主要参数；以下字段已解析但暂未在界面展示，后续按需使用。

| JSON 字段 | 含义 | 设备当前值 | 备注 |
|---|---|---|---|
| `Screenshot` | 截屏使能 | 0 | 0=关闭，1=开启 |
| `StartDelay` | 启动延迟（秒） | 3 | 启动流程前的等待时间 |
| `VersionType` | 固件版本类型 | 2 | 不同版本功能集合不同 |
| `IsHideLang` | 隐藏语言切换菜单 | 0 | 0=显示，1=隐藏 |
| `Remote` | 远程控制使能 | 2 | 设备上报原始值，SDK 定义为 0~2 |
| `Network` | 网络模块使能 | 0 | 0=无网络模块，1=有 |
| `PModel` | 压力模式切换 | 0 | 设备压力控制模式 |
| `OutType` | 输出信号类型 | 0 | 设备输出接口类型 |

已用于界面显隐的字段：

- `IsHasSpeed`：0 时隐藏“升温速率”区域，并强制普通模式（不显示温度阶段“速率”列）。
- `IsHasWater`：0 时隐藏“水冷回路”区域。

## 压力工艺参数（PressData）

已接入“压力阶段”表格、阶段数与模具类型：

- `PStep` → 阶段数（1~30）
- `SetPValue[]` → 压力列
- `AfterValue[]` → 补压设定列
- `KPTime[]` → 计时列（毫秒换算为秒）
- `Type` → 模具类型（0 异形 / 1 圆形 / 2 矩形；3 环形暂未支持）
- `D` → 圆形模具直径（mm）
- `A` / `B` → 矩形模具长 / 宽（mm）

已解析但暂未展示：

| JSON 字段 | 含义 | 设备当前值 | 备注 |
|---|---|---|---|
| `CheckValue` | 校准值 | 0.0 | 0~5 |
| `AirTime` | 排气时间（秒） | 1 | 0~60 |

## 温度工艺参数（TempData）

已接入“温度阶段”表格、温度阶段数、速率模式与水温：

- `TStep` → 阶段数（1~30）
- `StartTemp[]` → 温度列
- `Speed[]` → 速率列（不支持速率模式时该列隐藏）
- `KTTime[]` → 计时列（毫秒换算为秒）
- `Type` → 升温速率模式（0 普通，非 0 速率；设备不支持速率时强制普通）
- `MinWater` → 降温水温（°C）
- `MaxWater` → 升温水温（°C）

无暂未展示字段。

## 合作工艺参数（MixData）

已接入“合作流程明细”表格与合作阶段数：

- `Step` → 阶段数（1~30）
- `IsPress[]` → 是否加压
- `SetPValue[]` → 压力列
- `AfterValue[]` → 补压设定列
- `IsUpHeat[]` → 是否加热上板
- `UpTemp[]` → 上板温度列
- `IsDownHeat[]` → 是否加热下板
- `DownTemp[]` → 下板温度列
- `KTime[]` → 计时列（毫秒换算为秒）
- `Type` → 合作模式模具类型（0 异形 / 1 圆形 / 2 矩形；3 环形暂未支持）
- `D` → 合作模式圆形模具直径（mm）
- `A` / `B` → 合作模式矩形模具长 / 宽（mm）

说明：合作模式模具与单独模式模具互相独立，分别对应 MixData 与 PressData 的
`Type/A/B/D`；合作模式下的修改通过 `setMixMoldType / setMixCircle / setMixRectangle`
下发。

无暂未展示字段。

