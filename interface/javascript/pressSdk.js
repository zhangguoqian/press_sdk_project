/**
 * JavaScript binding for the C API of the Press SDK.
 * Press SDK 的 JavaScript 语言绑定。
 *
 * This wrapper intentionally mirrors the native cpress_* API and keeps the
 * call surface simple and predictable for Node.js callers. The library itself
 * is loaded from the project build output or the local install folder, and the
 * data structures are mapped to native memory using ref/ffi.
 *
 * 这个封装有意保持接近原生 cpress_* API，并让 Node.js 调用方使用起来
 * 简单直接。库文件会优先从工程构建输出或本地安装目录解析，数据结构
 * 则通过 ref/ffi 映射到原生内存。
 */
const fs = require('fs');
const path = require('path');

let ffi;
let ref;
let StructType;
let ArrayType;

try {
  ffi = require('ffi-napi');
  ref = require('ref-napi');
  StructType = require('ref-struct-di')(ref);
  ArrayType = require('ref-array-di')(ref);
} catch (error) {
  // Provide a clear, actionable error instead of a raw ModuleNotFound error.
  // 在依赖未安装时给出更明确的提示，而不是原始 ModuleNotFound 错误。
  throw new Error(
    'Missing Node.js native dependencies for Press SDK. Please run: npm install ffi-napi ref-napi ref-struct-di ref-array-di\n' +
      'Press SDK 缺少 Node.js 原生依赖，请执行：npm install ffi-napi ref-napi ref-struct-di ref-array-di\n' +
      error.message
  );
}

const MAX_P_STEP = 30;

function resolveLibraryPath() {
  // Prefer a local, repository-relative lookup instead of only relying on the
  // current working directory. This makes the wrapper work when launched from a
  // different folder or from an Electron/Node host process.
  // 优先使用相对仓库位置解析库文件，而不是只依赖当前工作目录，这样在从
  // 不同目录启动进程时也能够找到构建产物。
  const searchRoots = [
    __dirname,
    path.resolve(__dirname, '..'),
    path.resolve(__dirname, '..', '..'),
    path.resolve(__dirname, '..', '..', 'build'),
    path.resolve(__dirname, '..', '..', 'build', 'press'),
    path.resolve(__dirname, '..', '..', 'build', 'x64', 'Debug'),
    path.resolve(__dirname, '..', '..', 'install', 'bin'),
    process.cwd(),
  ];

  const libraryNames = ['press.dll', 'libpress.so', 'libpress.dylib', 'press'];

  for (const root of searchRoots) {
    for (const libraryName of libraryNames) {
      const candidate = path.join(root, libraryName);
      try {
        if (fs.existsSync(candidate)) {
          return candidate;
        }
      } catch (err) {
        // Ignore unreadable paths and continue with the next candidate.
      }
    }
  }

  return 'press';
}

const PortType = {
  // Communication port type / 通信端口类型
  SerialPortType: 0,
  TcpSocketPortType: 1,
};

const pressLib = ffi.Library(resolveLibraryPath(), {
  // Handle / 句柄
  cpress_create: ['void*', []],
  cpress_destroy: ['void', ['void*']],

  // Connection / 连接
  cpress_connect: ['int', ['void*', 'string', 'int']],
  cpress_disconnect: ['void', ['void*']],
  cpress_is_connected: ['int', ['void*']],

  // Status / 状态
  // uint64_t on the native side must be mapped as uint64 here instead of ulong,
  // because Windows x64 uses 64-bit integer for this register value.
  // 原生侧是 uint64_t，因此这里不能写成 ulong；Windows x64 下该值是 64 位整数。
  cpress_get_machine_register_no: ['uint64', ['void*']],
  cpress_get_last_error_info: ['string', ['void*']],
  cpress_run: ['void', ['void*']],
  cpress_stop: ['void', ['void*']],
  cpress_is_running: ['int', ['void*']],

  // Data read/write / 数据读写
  cpress_get_read_only_data_json: ['string', ['void*', 'int']],
  cpress_get_press_data_json: ['string', ['void*', 'int']],
  cpress_set_press_data_json: ['int', ['void*', 'string', 'int']],
  cpress_get_real_time_data_json: ['string', ['void*', 'int']],
  cpress_set_pressing: ['int', ['void*', 'int']],
  cpress_set_demolding: ['int', ['void*', 'int']],

  // Callback / 回调
  cpress_register_data_interface: ['void', ['void*', 'pointer']],
  cpress_unregister_data_interface: ['void', ['void*']],
  cpress_version: ['string', []],
});

const FloatArray30 = ArrayType('float', MAX_P_STEP);
const UInt32Array30 = ArrayType('uint32', MAX_P_STEP);

const ReadOnlyData = StructType({
  // Read-only machine info / 只读设备信息
  m_NameZH: 'char[64]',
  m_NameEN: 'char[64]',
  m_Type: 'char[64]',
  m_SerialNumber: 'char[64]',
  m_Screenshot: 'uchar',
  m_StartDelay: 'uchar',
  m_VersionType: 'uchar',
  m_IsHideLang: 'uchar',
  m_Remote: 'uchar',
  m_Network: 'uchar',
  m_FontZH: 'uchar',
  m_FontEN: 'uchar',
  m_MaxPStep: 'uchar',
  m_MaxPLimit: 'float',
  m_MinPLimit: 'float',
  m_Max_Min: 'float',
  m_Diameter: 'float',
  m_PDecimal: 'uchar',
  m_PressDecimal: 'uchar',
  m_PModel: 'uchar',
  m_OutType: 'uchar',
  m_MaxTStep: 'uchar',
  m_MaxTLimit: 'float',
  m_MinTLimit: 'float',
  m_TDecimal: 'uchar',
  m_IsHasWater: 'uchar',
  m_IsHasSpeed: 'uchar',
});

const PressData = StructType({
  // Pressure parameter profile / 压力参数配置
  m_PStep: 'uchar',
  m_Type: 'uchar',
  m_A: 'float',
  m_B: 'float',
  m_D: 'float',
  m_OuterD: 'float',
  m_InnerD: 'float',
  m_CheckValue: 'float',
  m_Speed: 'uchar',
  m_DemoldValue: 'float',
  m_SetPValue: FloatArray30,
  m_AfterValue: FloatArray30,
  m_KPTime: UInt32Array30,
});

const RealTimeData = StructType({
  // Real-time machine state / 实时机器状态
  m_ModelState: 'uchar',
  m_PressState: 'uchar',
  m_CPStep: 'uchar',
  m_PressValue: 'float',
  m_PTime: 'uint32',
  m_PdChanged: 'uchar',
});

const PressCDataCallbacks = StructType({
  // Keep structure layout aligned with PressCDataCallbacks in press.h.
  // Keep the same field order and pointer types as the native API.
  userData: 'pointer',
  onReadOnlyData: 'pointer',
  onRealTimeData: 'pointer',
  onPressData: 'pointer',
  onError: 'pointer',
  onTempData: 'pointer',
  onMixData: 'pointer',
});

class PressSdk {
  // High-level SDK wrapper / 高层 SDK 封装
  constructor() {
    this.handle = pressLib.cpress_create();
    if (!this.handle) {
      throw new Error('Failed to create Press SDK handle / 无法创建 Press SDK 句柄');
    }
    this._callbackRefs = [];
  }

  // Ensure the handle is valid before any native call.
  // 在调用任何原生 API 前，确保句柄有效。
  _assertHandle() {
    if (!this.handle) {
      throw new Error('Press SDK handle is invalid / Press SDK 句柄无效');
    }
  }

  // Connect to the machine / 连接设备
  connect(portName, portType = PortType.SerialPortType) {
    this._assertHandle();
    if (portName === undefined || portName === null || portName === '') {
      throw new TypeError('Port name is required / 端口名不能为空');
    }

    return pressLib.cpress_connect(this.handle, String(portName), Number(portType)) !== 0;
  }

  // Disconnect / 断开连接
  disconnect() {
    if (this.handle) {
      pressLib.cpress_disconnect(this.handle);
    }
  }

  // Query connection status / 查询连接状态
  isConnected() {
    this._assertHandle();
    return pressLib.cpress_is_connected(this.handle) !== 0;
  }

  // Get machine register ID / 获取设备注册码
  getMachineRegisterNo() {
    this._assertHandle();
    return pressLib.cpress_get_machine_register_no(this.handle);
  }

  // Get last error info / 获取最后一条错误信息
  getLastErrorInfo() {
    this._assertHandle();
    return pressLib.cpress_get_last_error_info(this.handle) || '';
  }

  // Start the machine / 启动设备
  run() {
    this._assertHandle();
    pressLib.cpress_run(this.handle);
  }

  // Stop the machine / 停止设备
  stop() {
    this._assertHandle();
    pressLib.cpress_stop(this.handle);
  }

  // Check whether it is running / 查询是否正在运行
  isRunning() {
    this._assertHandle();
    return pressLib.cpress_is_running(this.handle) !== 0;
  }

  // Read read-only data / 读取只读设备信息（返回 JSON 字符串）
  getReadOnlyDataJson(isCompressed = false) {
    this._assertHandle();
    return pressLib.cpress_get_read_only_data_json(this.handle, isCompressed ? 1 : 0) || null;
  }

  // Read pressure data / 读取压力参数（返回 JSON 字符串）
  getPressDataJson(isCompressed = false) {
    this._assertHandle();
    return pressLib.cpress_get_press_data_json(this.handle, isCompressed ? 1 : 0) || null;
  }

  // Write pressure data from JSON / 以 JSON 字符串写入压力参数
  setPressDataJson(json, isCompressed = false) {
    this._assertHandle();
    if (typeof json !== 'string') {
      throw new TypeError('PD JSON string is required / 需要传入 PD JSON 字符串');
    }
    return pressLib.cpress_set_press_data_json(this.handle, json, isCompressed ? 1 : 0) !== 0;
  }

  // Read real-time data / 读取实时状态（返回 JSON 字符串）
  getRealTimeDataJson(isCompressed = false) {
    this._assertHandle();
    return pressLib.cpress_get_real_time_data_json(this.handle, isCompressed ? 1 : 0) || null;
  }

  // Start or stop pressing / 启动或停止加压
  setPressing(isPressing) {
    this._assertHandle();
    return pressLib.cpress_set_pressing(this.handle, isPressing ? 1 : 0) !== 0;
  }

  // Start or stop demolding / 启动或停止脱模
  setDemolding(isDemolding) {
    this._assertHandle();
    return pressLib.cpress_set_demolding(this.handle, isDemolding ? 1 : 0) !== 0;
  }

  // Register callback interface / 注册回调接口
  registerDataInterface(callbacks) {
    this._assertHandle();

    if (!callbacks) {
      this.unregisterDataInterface();
      this._callbackRefs = [];
      return;
    }

    const callbackRefs = [];
    const table = new PressCDataCallbacks();
    table.userData = callbacks.userData ?? ref.NULL;

    const assignCallback = (fieldName, callback, argTypes, handler) => {
      if (!callback) {
        table[fieldName] = ref.NULL;
        return;
      }

      const wrapped = ffi.Callback('void', argTypes, (...args) => {
        try {
          handler(...args);
        } catch (err) {
          // Keep the callback exception visible to the caller without crashing the SDK worker thread.
          // 将回调异常暴露给调用方，但不让它直接导致 SDK 工作线程崩溃。
          console.error('Press SDK callback error:', err);
        }
      });

      callbackRefs.push(wrapped);
      table[fieldName] = wrapped;
    };

    assignCallback('onReadOnlyData', callbacks.onReadOnlyData, ['pointer', 'int', 'uint64', 'string'], (userData, errorCode, registerNo, json) => {
      callbacks.onReadOnlyData(userData, errorCode, registerNo, json);
    });

    assignCallback('onRealTimeData', callbacks.onRealTimeData, ['pointer', 'int', 'string'], (userData, errorCode, json) => {
      callbacks.onRealTimeData(userData, errorCode, json);
    });

    assignCallback('onPressData', callbacks.onPressData, ['pointer', 'int', 'string'], (userData, errorCode, json) => {
      callbacks.onPressData(userData, errorCode, json);
    });

    assignCallback('onError', callbacks.onError, ['pointer', 'ushort', 'pointer', 'size_t'], (userData, cmdCode, response, responseLen) => {
      callbacks.onError(userData, cmdCode, response, responseLen);
    });

    assignCallback('onTempData', callbacks.onTempData, ['pointer', 'int', 'string'], (userData, errorCode, json) => {
      callbacks.onTempData(userData, errorCode, json);
    });

    assignCallback('onMixData', callbacks.onMixData, ['pointer', 'int', 'string'], (userData, errorCode, json) => {
      callbacks.onMixData(userData, errorCode, json);
    });

    this._callbackRefs = callbackRefs;
    pressLib.cpress_register_data_interface(this.handle, table.ref());
  }

  // Unregister callback interface / 注销回调接口
  unregisterDataInterface() {
    if (this.handle) {
      pressLib.cpress_unregister_data_interface(this.handle);
    }
    this._callbackRefs = [];
  }

  // Get version string / 获取版本字符串
  static version() {
    return pressLib.cpress_version();
  }

  // Release handle / 释放句柄
  close() {
    if (!this.handle) {
      return;
    }

    try {
      this.unregisterDataInterface();
      this.disconnect();
    } finally {
      pressLib.cpress_destroy(this.handle);
      this.handle = null;
      this._callbackRefs = [];
    }
  }
}

module.exports = {
  MAX_P_STEP,
  PortType,
  ReadOnlyData,
  PressData,
  RealTimeData,
  PressSdk,
};
