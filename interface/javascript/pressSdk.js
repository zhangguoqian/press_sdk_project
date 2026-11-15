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
  cpress_get_read_only_data: ['int', ['void*', 'pointer', 'int']],
  cpress_get_press_data: ['int', ['void*', 'pointer', 'int']],
  cpress_set_press_data: ['int', ['void*', 'pointer', 'int']],
  cpress_get_real_time_data: ['int', ['void*', 'pointer', 'int']],
  cpress_set_pressing: ['int', ['void*', 'int']],
  cpress_set_demolding: ['int', ['void*', 'int']],

  // Generic read/write commands / 通用读写命令
  // size_t is used for lengths so they round-trip correctly on 64-bit hosts.
  // 长度使用 size_t，保证在 64 位平台上能正确来回传递。
  cpress_generic_read: ['int', ['void*', 'ushort', 'pointer', 'size_t', 'pointer', 'size_t', 'pointer']],
  cpress_generic_write: ['int', ['void*', 'ushort', 'pointer', 'size_t', 'pointer', 'size_t', 'pointer']],

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

  // Read read-only data / 读取只读设备信息
  getReadOnlyData(isCompressed = false) {
    this._assertHandle();
    const data = new ReadOnlyData();
    const result = pressLib.cpress_get_read_only_data(this.handle, data.ref(), isCompressed ? 1 : 0);
    return result !== 0 ? data : null;
  }

  // Read pressure data / 读取压力参数
  getPressData(isCompressed = false) {
    this._assertHandle();
    const data = new PressData();
    const result = pressLib.cpress_get_press_data(this.handle, data.ref(), isCompressed ? 1 : 0);
    return result !== 0 ? data : null;
  }

  // Write pressure data / 写入压力参数
  setPressData(data, isCompressed = false) {
    this._assertHandle();
    if (!data || typeof data.ref !== 'function') {
      throw new TypeError('PressData object is required / 需要传入 PressData 对象');
    }
    return pressLib.cpress_set_press_data(this.handle, data.ref(), isCompressed ? 1 : 0) !== 0;
  }

  // Read real-time data / 读取实时状态
  getRealTimeData(isCompressed = false) {
    this._assertHandle();
    const data = new RealTimeData();
    const result = pressLib.cpress_get_real_time_data(this.handle, data.ref(), isCompressed ? 1 : 0);
    return result !== 0 ? data : null;
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

  // Generic read command, covers all CMDID_* commands / 通用读命令，覆盖全部 CMDID_* 命令
  // command must NOT carry the SET bit; read requests carry exactly 1 payload
  // byte. Returns the response bytes on success, null on failure.
  // command 不带 SET 位；读命令请求 payload 固定 1 字节。
  // 成功返回响应字节，失败返回 null。
  genericRead(command, requestData, responseCapacity) {
    this._assertHandle();
    return this._genericCommand(
      pressLib.cpress_generic_read,
      command,
      requestData,
      responseCapacity
    );
  }

  // Generic write command, covers all SET_CMD() commands / 通用写命令，覆盖全部 SET_CMD() 命令
  // command must carry the SET bit (= read command ID | 0x1000); requestData is
  // the full write payload (indexed commands carry their index prefix). The
  // response is the firmware echo (parameter commands) or a 1-byte status code
  // (control commands: 0/1 accepted, 3 rejected). Returns the response bytes on
  // success, null on failure.
  // command 必须带 SET 位（= 读命令 ID | 0x1000）；requestData 为完整写入
  // payload（索引型命令自带索引前缀）。响应为固件回显数据（参数类）或 1 字节
  // 状态码（控制类 0/1 成功，3 拒绝）。成功返回响应字节，失败返回 null。
  genericWrite(command, requestData, responseCapacity) {
    this._assertHandle();
    return this._genericCommand(
      pressLib.cpress_generic_write,
      command,
      requestData,
      responseCapacity
    );
  }

  // Shared helper for the generic read/write commands / 通用读写命令的公共辅助方法
  _genericCommand(fn, command, requestData, responseCapacity) {
    let request = null;
    let requestLen = 0;
    if (requestData !== undefined && requestData !== null) {
      request = Buffer.isBuffer(requestData) ? requestData : Buffer.from(requestData);
      requestLen = request.length;
    }

    const capacity = Math.max(0, responseCapacity | 0);
    const responseBuffer = Buffer.alloc(capacity);
    const responseLen = ref.alloc('size_t', 0);

    const result = fn(
      this.handle,
      command & 0xffff,
      request === null ? ref.NULL : request,
      requestLen,
      capacity > 0 ? responseBuffer : ref.NULL,
      capacity,
      responseLen
    );

    if (result === 0) {
      return null;
    }

    const actualLen = Math.min(Number(ref.get(responseLen)), capacity);
    return responseBuffer.slice(0, actualLen);
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

    assignCallback('onReadOnlyData', callbacks.onReadOnlyData, ['pointer', 'int', 'uint64', 'pointer'], (userData, errorCode, registerNo, readOnlyData) => {
      callbacks.onReadOnlyData(userData, errorCode, registerNo, readOnlyData);
    });

    assignCallback('onRealTimeData', callbacks.onRealTimeData, ['pointer', 'int', 'pointer'], (userData, errorCode, realTimeData) => {
      callbacks.onRealTimeData(userData, errorCode, realTimeData);
    });

    assignCallback('onPressData', callbacks.onPressData, ['pointer', 'int', 'pointer'], (userData, errorCode, pressData) => {
      callbacks.onPressData(userData, errorCode, pressData);
    });

    assignCallback('onError', callbacks.onError, ['pointer', 'ushort', 'pointer', 'size_t'], (userData, cmdCode, response, responseLen) => {
      callbacks.onError(userData, cmdCode, response, responseLen);
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
