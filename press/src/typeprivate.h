/******************************************************************************
 * typeprivate.h — low-level command IDs and protocol helper macros
 * typeprivate.h — 协议层命令 ID 与辅助宏定义
 *
 *  This header is intentionally C-compatible. It defines the machine protocol's
 *  command identifiers and helper macros used during frame packing, validation,
 *  and response parsing.
 *  该头文件刻意保持 C 兼容，定义了机器协议中的命令标识符和辅助宏，这些
 *  常量用于帧打包、校验和响应解析。
 *
 *  Command groups / 命令分组:
 *      ROD_CMDID    (0x00xx) — read-only device identity and metadata / 只读设备身份与基本信息
 *      PRESS_CMDID  (0x01xx) — pressure control parameters / 压力控制参数
 *      TEMP_CMDID   (0x02xx) — temperature control parameters / 温度控制参数
 *      MIX_CMDID    (0x03xx) — mixed-mode parameters / 混合模式参数
 *      SYSTEM_CMDID (0x04xx) — system settings / 系统设置
 *      STATE_CMDID  (0x05xx / 0x15xx) — state queries and write control commands / 状态查询与写控制命令
 *
 *  Bit layout / 位布局:
 *      bit12 = SET flag / SET 标志  (1 = write command, 0 = read command)
 *      bit11..8 = category field / 类别字段 (0..5)
 *      bit7..0 = command index / 命令索引
 *
 *  External dependencies / 外部依赖:
 *    - <stdint.h> provides fixed-width integer types / 提供 uint8_t / uint16_t 等类型
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_MTYPE_H
#define PRESS_SDK_PROJECT_MTYPE_H

#include <stdint.h>


/******************************************************************************
 * CmdID Utilities / 命令 ID 工具宏
 *****************************************************************************/

//! Add the SET bit (0x1000) to a command ID to mark it as a write command
//! 将命令 ID 加上 SET 位 (0x1000) 以标记为写命令
#define SET_CMD(CMDID)    (CMDID | 0x1000)

//! Check whether a command ID has the SET bit (write command flag)
//! 判断命令 ID 是否带有 SET 位 (写命令标志)
#define IS_SET_CMD(CMDID) ((CMDID & 0x1000) >> 12)

//! Generic 1-byte response validator (convenience alias for size == 1)
//! 通用 1 字节响应校验 (size == 1 的便捷别名)
#define GET_BOOL(size)    (size == 1)


/******************************************************************************
 * ROD_CMDID — Read-Only Device Commands / 只读设备命令 (类别 0x0)
 *
 *  全部为读命令（无 SET 位），用于获取设备的静态身份标识：
 *  版本号、设备名、型号、序列号、压力/温度参数块等。
 *  CMDID_ROD_JSON (0x00FF) 一次性返回全部只读数据（JSON 格式）。
 *****************************************************************************/

enum ROD_CMDID {
    CMDID_VERSION        = 0x0000,     // Get firmware version           // 获取固件版本
    CMDID_NAME           = 0x0001,     // Get device name                // 获取设备名称
    CMDID_MACHINE_TYPE   = 0x0002,     // Get machine model              // 获取机器型号
    CMDID_SERIAL_NUMBER  = 0x0003,     // Get serial number              // 获取序列号
    CMDID_PRESS_PARAMETER = 0x0004,    // Get pressure parameter block   // 获取压力参数块
    CMDID_TEMP_PARAMETER = 0x0005,     // Get temperature parameter block// 获取温度参数块
    CMDID_OTHER_INFO     = 0x0006,     // Get other miscellaneous info   // 获取其他杂项信息

    CMDID_ROD_JSON       = 0x00FF,     // Get all read-only data as JSON // 获取全部只读数据(JSON格式)
};


/******************************************************************************
 * ROD size validators / 只读数据长度校验宏
 *
 *  以下宏用于通信层预校验设备响应字节数是否符合协议预期。
 *  返回 true 表示 size 匹配，false 表示异常。
 *****************************************************************************/

#define VERSION_BOOL(size)                  (size == 4)
#define NAME_BOOL(size)                     (size > 1 && size < 32)
#define MACHINE_TYPE_BOOL(size)             (size <= 32)
#define SERIAL_NUMBER_TYPE_BOOL(size)       (size <= 32)
#define PRESS_PARAMETER_TYPE_BOOL(size)     (size == 20)
#define TEMP_PARAMETER_TYPE_BOOL(size)      (size == 12)
#define OTHER_INFO_TYPE_BOOL(size)          (size == 5)


/******************************************************************************
 * PRESS_CMDID — Pressure Control Commands / 压力控制命令 (类别 0x1)
 *
 *  读命令 (0x01xx) 查询压力参数，加 SET 位 (0x11xx) 即可写回。
 *  CMDID_PD_JSON (0x01FF / 0x11FF) 批量 JSON 读写。
 *****************************************************************************/

enum PRESS_CMDID {
    CMDID_PRESS_STEP           = 0x0100,  // Current pressure step          // 当前压力步骤
    CMDID_PRESS_CALIBRATE      = 0x0101,  // Pressure calibration value     // 压力校准值
    CMDID_MOLD_TYPE            = 0x0102,  // Mold type                      // 模具类型
    CMDID_CIRCLE_D             = 0x0103,  // Circle diameter                // 圆形容器直径
    CMDID_RECT_AB              = 0x0104,  // Rectangular A/B dimensions     // 矩形容器A/B尺寸
    CMDID_RING_OUT_IN          = 0x0105,  // Ring outer/inner diameters     // 环形外/内径
    CMDID_AIR_TIME             = 0x0106,  // Air time                       // 放气时间
    CMDID_INDEX_PRESS          = 0x0107,  // Index pressure value           // 索引压力值
    CMDID_INDEX_AFTER          = 0x0108,  // Index after-pressure value     // 索引保压值
    CMDID_INDEX_KPTIME         = 0x0109,  // Index keep-pressure time       // 索引保压时间
    CMDID_INDEX_PSTEP_PARAMETER = 0x010A, // Indexed pressure step param    // 索引压力步骤参数
    CMDID_PRESS_DECIMAL        = 0x010B,  // Pressure display decimal       // 压力显示小数位
    CMDID_PRESSURE_DECIMAL     = 0x010C,  // Pressure unit decimal          // 压强单位小数位
    CMDID_PMODEL               = 0x010D,  // Pressure model switch          // 压力模式切换
    CMDID_DEMOLD_VALUE         = 0x010E,  // Demold pressure value          // 脱模压力值

    CMDID_PD_JSON              = 0x01FF,  // Get/set all press data (JSON)  // 获取/设置全部压力数据(JSON格式)
};

#define PRESS_STEP_BOOL(size)                   (size == 1)
#define PRESS_CALIBRATE_BOOL(size)              (size == 4)
#define MOLD_TYPE_BOOL(size)                    (size == 1)
#define CIRCLE_D_BOOL(size)                     (size == 4)
#define RECT_AB_BOOL(size)                      (size == 8)
#define RING_OUT_IN_BOOL(size)                  (size == 8)
#define AIR_TIME_BOOL(size)                     (size == 1)
#define INDEX_PRESS_BOOL(size)                  (size == 5)
#define INDEX_AFTER_BOOL(size)                  (size == 5)
#define INDEX_KPTIME_BOOL(size)                 (size == 5)
#define INDEX_PSTEP_PARAMETER_BOOL(size)        (size == 13)

#define PRESS_DECIMAL_BOOL(size)                (size == 1)
#define PRESSURE_DECIMAL_BOOL(size)             (size == 1)
#define PMODEL_BOOL(size)                       (size == 1)
#define DEMOLD_VALUE_BOOL(size)                 (size == 4)


/******************************************************************************
 * TEMP_CMDID — Temperature Control Commands / 温度控制命令 (类别 0x2)
 *****************************************************************************/

enum TEMP_CMDID {
    CMDID_TEMP_STEP            = 0x0200,  // Current temperature step       // 当前温度步骤
    CMDID_SPEED_MOLD           = 0x0201,  // Speed mold flag                // 速度模式标志
    CMDID_INDEX_TEMP           = 0x0202,  // Index temperature value        // 索引温度值
    CMDID_INDEX_SPEED          = 0x0203,  // Index speed value              // 索引速度值
    CMDID_INDEX_KTTIME         = 0x0204,  // Index keep-temperature time    // 索引保温时间
    CMDID_INDEX_TSTEP_PARAMETER = 0x0205, // Indexed temp step parameter    // 索引温度步骤参数
    CMDID_WATER_UP             = 0x0206,  // Water up threshold             // 水温上限
    CMDID_WATER_DOWN           = 0x0207,  // Water down threshold           // 水温下限

    CMDID_TD_JSON              = 0x02FF,  // Get all temperature data (JSON)// 获取全部温度数据(JSON格式)
};

#define TEMP_STEP_BOOL(size)                    (size == 1)
#define SPEED_MOLD_BOOL(size)                   (size == 1)
#define INDEX_TEMP_BOOL(size)                   (size == 5)
#define INDEX_SPEED_BOOL(size)                  (size == 5)
#define INDEX_KTTIME_BOOL(size)                 (size == 5)
#define INDEX_TSTEP_PARAMETER_BOOL(size)        (size == 13)
#define WATER_UP_BOOL(size)                     (size == 4)
#define WATER_DOWN_BOOL(size)                   (size == 4)


/******************************************************************************
 * MIX_CMDID — Mixed Mode Commands / 混合模式命令 (类别 0x3)
 *****************************************************************************/

enum MIX_CMDID {
    CMDID_MIX_STEP                 = 0x0300,  // Current mixed mode step     // 当前混合模式步骤
    CMDID_MIX_MOLD_TYPE            = 0x0301,  // Mixed mold type             // 混合模具类型
    CMDID_MIX_CIRCLE_D             = 0x0302,  // Mixed circle diameter       // 混合圆形容器直径
    CMDID_MIX_RECT_AB              = 0x0303,  // Mixed rectangular A/B       // 混合矩形容器A/B尺寸
    CMDID_MIX_RING_OUT_IN          = 0x0304,  // Mixed ring outer/inner      // 混合环形外/内径
    CMDID_MIX_INDEX_ISPRESS        = 0x0305,  // Mixed index is-press flag   // 混合索引是否加压
    CMDID_MIX_INDEX_PRESS          = 0x0306,  // Mixed index pressure        // 混合索引压力值
    CMDID_MIX_INDEX_AFTER          = 0x0307,  // Mixed index after-pressure  // 混合索引保压值
    CMDID_MIX_INDEX_ISHEAT_UP      = 0x0308,  // Mixed index is-heat-up      // 混合索引是否加热
    CMDID_MIX_INDEX_TEMP_UP        = 0x0309,  // Mixed index temp-up         // 混合索引温度上限
    CMDID_MIX_INDEX_ISHEAT_DOWN    = 0x030A,  // Mixed index is-heat-down    // 混合索引是否冷却
    CMDID_MIX_INDEX_TEMP_DOWN      = 0x030B,  // Mixed index temp-down       // 混合索引温度下限
    CMDID_MIX_INDEX_KTIME          = 0x030C,  // Mixed index keep-time       // 混合索引保持时间
    CMDID_MIX_INDEX_STEP_PARAMETER = 0x030D,  // Mixed index step parameter  // 混合索引步骤参数

    CMDID_MD_JSON                  = 0x03FF,  // Get all mixed data (JSON)   // 获取全部混合模式数据(JSON格式)
};

#define MIX_STEP_BOOL(size)                     (size == 1)
#define MIX_MOLD_TYPE_BOOL(size)                (size == 1)
#define MIX_CIRCLE_D_BOOL(size)                 (size == 4)
#define MIX_RECT_AB_BOOL(size)                  (size == 8)
#define MIX_RING_OUT_IN_BOOL(size)              (size == 8)
#define MIX_INDEX_ISPRESS_BOOL(size)            (size == 2)
#define MIX_INDEX_PRESS_BOOL(size)              (size == 5)
#define MIX_INDEX_AFTER_BOOL(size)              (size == 5)
#define MIX_INDEX_ISHEAT_UP_BOOL(size)          (size == 2)
#define MIX_INDEX_TEMP_UP_BOOL(size)            (size == 5)
#define MIX_INDEX_ISHEAT_DOWN_BOOL(size)        (size == 2)
#define MIX_INDEX_TEMP_DOWN_BOOL(size)          (size == 5)
#define MIX_INDEX_KTIME_BOOL(size)              (size == 5)
#define MIX_INDEX_STEP_PARAMETER_BOOL(size)     (size == 23)


/******************************************************************************
 * SYSTEM_CMDID — System Control Commands / 系统控制命令 (类别 0x4)
 *****************************************************************************/

enum SYSTEM_CMDID {
    CMDID_DATE_TIME      = 0x0400,   // Date & time                    // 日期时间
    CMDID_BRIGHTNESS     = 0x0401,   // Screen brightness              // 屏幕亮度
    CMDID_BUTTON_SOUND   = 0x0402,   // Button sound on/off            // 按键声音开关
    CMDID_LANGUAGE       = 0x0403,   // UI language                    // 界面语言
};

#define DATE_TIME_BOOL(size)                    (size == 6)
#define BRIGHTNESS_BOOL(size)                   (size == 1)
#define BUTTON_SOUND_BOOL(size)                 (size == 1)
#define LANGUAGE_BOOL(size)                     (size == 1)


/******************************************************************************
 * STATE_CMDID — State & Control Commands / 状态与控制命令 (类别 0x5)
 *
 *  0x05xx 为只读状态查询；0x15xx 为写控制命令（启动/停止/切换模式）。
 *****************************************************************************/

enum STATE_CMDID {
    CMDID_GET_CURRENT_STATE     = 0x0500,   // Get current machine state      // 获取机器当前状态
    CMDID_SET_START_PRESS       = 0x1521,   // Start / stop pressing          // 启动/停止加压
    CMDID_SET_START_HEAT        = 0x1522,   // Start / stop heating           // 启动/停止加热
    CMDID_SET_START_DEMOLD      = 0x1523,   // Start / stop demolding         // 启动/停止脱模

    CMDID_SET_START             = 0x1511,   // Start / stop all motors        // 启动/停止全部电机

    CMDID_SET_START_P           = 0x1531,   // Start pressure module only     // 仅启动压力模块
    CMDID_SET_START_TU          = 0x1532,   // Start heating (up) module      // 启动升温模块
    CMDID_SET_START_TD          = 0x1533,   // Start cooling (down) module    // 启动降温模块

    CMDID_SELECT_MODEL_STATE     = 0x1507,   // Select work mode / model       // 选择工作模式

    CMDID_GET_GPIO4_STATE       = 0x05A4,   // Get GPIO line 4 state          // 获取GPIO4引脚状态

    CMDID_GET_CURRENT_STATE_JSON = 0x05FF,  // Get full real-time state (JSON)// 获取完整实时状态(JSON格式)
};

#define SET_START_BOOL(size)                    (size == 1)
#define SET_JSON_BOOL(size)                     (size > 3)

//! Extract category field (bit11..8) from a command ID
//! 从命令 ID 中提取类别字段 (bit11..8)
#define CMID_TYPE(CMDID)  ((CMDID & 0x0F00) >> 8)

#endif /* PRESS_SDK_PROJECT_MTYPE_H */