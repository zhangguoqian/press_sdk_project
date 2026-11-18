/**
 * Dart binding for the C API of the Press SDK.
 * Press SDK 的 Dart 语言绑定。
 *
 * This file intentionally wraps the exported cpress_* functions and keeps the
 * native handle opaque. It is designed as a small, safe FFI layer that can be
 * extended to richer typed mappings when a project needs direct struct access.
 *
 * 这个文件有意包装导出的 cpress_* 函数，并保持原生句柄不透明。
 * 它被设计为一个小而安全的 FFI 层；当项目需要直接访问结构体时，
 * 可以在此基础上扩展更详细的类型映射。
 *
 * Key fixes compared to the older version:
 *  - load the actual native library name instead of forcing "press"
 *  - free temporary UTF-8 strings after each native call
 *  - avoid repeated open calls for the same library
 *  - add explicit lifecycle checks for disconnect/destroy
 *  - expose the main port-type constants and public API methods
 *
 * 相比旧版本修正的关键点：
 *  - 不再强制加载 "press"，而是尝试真实的库文件名
 *  - 每次传递 UTF-8 字符串给原生 API 后释放临时内存
 *  - 避免反复打开同一原生库
 *  - 为 disconnect / destroy 增加生命周期检查
 *  - 暴露主要端口类型常量和公共 API 方法
 */
import 'dart:ffi';
import 'dart:io' show Platform;
import 'dart:typed_data' show Uint8List;
import 'package:ffi/ffi.dart';

/// Port type constants for the underlying C API.
/// 对底层 C API 的端口类型常量。
class PortType {
  const PortType._();

  static const int serialPortType = 0;
  static const int tcpSocketPortType = 1;
}

/// A lightweight data container used for the public API.
/// 一个用于公开 API 的轻量数据容器。
class ReadOnlyData {
  String mNameZH = '';
  String mNameEN = '';
  String mType = '';
  String mSerialNumber = '';
  int mScreenshot = 0;
  int mStartDelay = 0;
  int mVersionType = 0;
  int mIsHideLang = 0;
  int mRemote = 0;
  int mNetwork = 0;
  int mFontZH = 0;
  int mFontEN = 0;
  int mMaxPStep = 0;
  double mMaxPLimit = 0.0;
  double mMinPLimit = 0.0;
  double mMaxMin = 0.0;
  double mDiameter = 0.0;
  int mPDecimal = 0;
  int mPressDecimal = 0;
  int mPModel = 0;
  int mOutType = 0;
  int mMaxTStep = 0;
  double mMaxTLimit = 0.0;
  double mMinTLimit = 0.0;
  int mTDecimal = 0;
  int mIsHasWater = 0;
  int mIsHasSpeed = 0;
}

/// A lightweight pressure profile container.
/// 一个轻量的压力曲线参数容器。
class PressData {
  int mPStep = 0;
  int mType = 0;
  double mA = 0.0;
  double mB = 0.0;
  double mD = 0.0;
  double mOuterD = 0.0;
  double mInnerD = 0.0;
  double mCheckValue = 0.0;
  int mSpeed = 0;
  double mDemoldValue = 0.0;
  final List<double> mSetPValue = List<double>.filled(30, 0.0);
  final List<double> mAfterValue = List<double>.filled(30, 0.0);
  final List<int> mKPTime = List<int>.filled(30, 0);
}

/// A lightweight real-time status container.
/// 一个轻量的实时状态容器。
class RealTimeData {
  int mModelState = 0;
  int mPressState = 0;
  int mCPStep = 0;
  double mPressValue = 0.0;
  int mPTime = 0;
  int mPdChanged = 0;
}

/// Dart binding for the native Press SDK.
/// 原生 Press SDK 的 Dart 绑定。
class PressSdk {
  // Keep a single library handle for the whole lifetime of the instance.
  // 整个实例生命周期内保留单个库句柄。
  static final DynamicLibrary _library = _openLibrary();

  final Pointer<Void> _handle;
  bool _disposed = false;

  PressSdk._(this._handle);

  /// Construct a new SDK instance and allocate the native handle.
  /// 创建SDK实例并分配原生句柄。
  factory PressSdk() {
    final create = _library
        .lookupFunction<Pointer<Void> Function(), Pointer<Void> Function()>(
          'cpress_create',
        );

    final handle = create();
    if (handle == nullptr) {
      throw StateError('Failed to create Press SDK handle / 无法创建 Press SDK 句柄');
    }

    return PressSdk._(handle);
  }

  /// Resolve the native library name. Try the real output names before falling
  /// back to a generic name. This is necessary because Windows debug builds often
  /// produce files with a suffix such as "pressd.dll".
  /// 解析原生库文件名。先尝试真实输出名，再回退到通用名；这点对 Windows
  /// 调试构建尤其重要，因为常见输出文件名会带有 "pressd.dll" 后缀。
  static String _resolveLibraryPath() {
    final candidates = <String>[
      if (Platform.isWindows) ...<String>['pressd.dll', 'press.dll'],
      if (Platform.isLinux) ...<String>[
        'libpress.so',
        'libpress.so.1',
        'press',
      ],
      if (Platform.isMacOS) ...<String>['libpress.dylib', 'press'],
      'press',
    ];

    for (final candidate in candidates) {
      try {
        final lib = DynamicLibrary.open(candidate);
        if (lib.handle != nullptr) {
          return candidate;
        }
      } catch (_) {
        // Ignore and continue with the next candidate.
        // 忽略并继续尝试下一个候选库名。
      }
    }

    return 'press';
  }

  /// Open the native library once and reuse the handle for every call.
  /// 只打开一次原生库，并在所有调用中复用该句柄。
  static DynamicLibrary _openLibrary() {
    final libraryPath = _resolveLibraryPath();
    try {
      return DynamicLibrary.open(libraryPath);
    } catch (_) {
      throw StateError(
        'Failed to load Press SDK native library. '
        'Expected one of: press.dll, libpress.so, libpress.dylib, press. '
        'The file may not be in the current working directory or PATH. '
        '/ 无法加载 Press SDK 原生库。期望文件为：press.dll、libpress.so、'
        'libpress.dylib 或 press。该文件可能不在当前工作目录或环境 PATH 中。',
      );
    }
  }

  void _ensureNotDisposed() {
    if (_disposed) {
      throw StateError(
        'Press SDK instance is already disposed / Press SDK 实例已释放',
      );
    }
  }

  /// Connect to a serial or TCP port.
  /// 连接串口或 TCP 端口。
  bool connect(String portName, {int portType = PortType.serialPortType}) {
    _ensureNotDisposed();
    if (portName.trim().isEmpty) {
      throw ArgumentError.value(
        portName,
        'portName',
        'Port name cannot be empty. / 端口名不能为空',
      );
    }

    final connect = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Utf8>, Int32),
          int Function(Pointer<Void>, Pointer<Utf8>, int)
        >('cpress_connect');

    final nativePortName = portName.toNativeUtf8();
    try {
      return connect(_handle, nativePortName.cast<Utf8>(), portType) != 0;
    } finally {
      malloc.free(nativePortName);
    }
  }

  /// Disconnect from the device.
  /// 与设备断开连接。
  void disconnect() {
    if (_disposed) {
      return;
    }

    final disconnect = _library
        .lookupFunction<
          Void Function(Pointer<Void>),
          void Function(Pointer<Void>)
        >('cpress_disconnect');
    disconnect(_handle);
  }

  /// Return whether the SDK is connected to a device.
  /// 返回 SDK 是否已连接到设备。
  bool isConnected() {
    _ensureNotDisposed();
    final isConnected = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>),
          int Function(Pointer<Void>)
        >('cpress_is_connected');
    return isConnected(_handle) != 0;
  }

  /// Return the machine register number.
  /// 返回设备注册号。
  int getMachineRegisterNo() {
    _ensureNotDisposed();
    final getMachineRegisterNo = _library
        .lookupFunction<
          Uint64 Function(Pointer<Void>),
          int Function(Pointer<Void>)
        >('cpress_get_machine_register_no');
    return getMachineRegisterNo(_handle);
  }

  /// Retrieve the most recent native error message.
  /// 获取最近一次原生错误消息。
  String getLastErrorInfo() {
    _ensureNotDisposed();
    final getLastErrorInfo = _library
        .lookupFunction<
          Pointer<Utf8> Function(Pointer<Void>),
          Pointer<Utf8> Function(Pointer<Void>)
        >('cpress_get_last_error_info');

    final ptr = getLastErrorInfo(_handle);
    return ptr == nullptr ? '' : ptr.toDartString();
  }

  /// Start the press run cycle.
  /// 启动加压运行循环。
  void run() {
    _ensureNotDisposed();
    final run = _library
        .lookupFunction<
          Void Function(Pointer<Void>),
          void Function(Pointer<Void>)
        >('cpress_run');
    run(_handle);
  }

  /// Stop the press run cycle.
  /// 停止加压运行循环。
  void stop() {
    _ensureNotDisposed();
    final stop = _library
        .lookupFunction<
          Void Function(Pointer<Void>),
          void Function(Pointer<Void>)
        >('cpress_stop');
    stop(_handle);
  }

  /// Whether the machine is currently running.
  /// 当前设备是否正在运行。
  bool isRunning() {
    _ensureNotDisposed();
    final isRunning = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>),
          int Function(Pointer<Void>)
        >('cpress_is_running');
    return isRunning(_handle) != 0;
  }

  /// Set the press action on or off.
  /// 开启或停止单独的加压动作。
  bool setPressing(bool isPressing) {
    _ensureNotDisposed();
    final setPressing = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Int32),
          int Function(Pointer<Void>, int)
        >('cpress_set_pressing');
    return setPressing(_handle, isPressing ? 1 : 0) != 0;
  }

  /// Set the demolding action on or off.
  /// 开启或停止单独的脱模动作。
  bool setDemolding(bool isDemolding) {
    _ensureNotDisposed();
    final setDemolding = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Int32),
          int Function(Pointer<Void>, int)
        >('cpress_set_demolding');
    return setDemolding(_handle, isDemolding ? 1 : 0) != 0;
  }

  // ==================== Pressure / 压力 ====================

  /// Read the pressure step count.
  /// 读取压力段数。
  int? getPressStepCount() {
    _ensureNotDisposed();
    final getPressStepCount = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_press_step_count');

    final step = malloc<Uint8>();
    try {
      return getPressStepCount(_handle, step) != 0 ? step.value : null;
    } finally {
      malloc.free(step);
    }
  }

  /// Write the pressure step count.
  /// 设置压力段数。
  bool setPressStepCount(int step) {
    _ensureNotDisposed();
    final setPressStepCount = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_press_step_count');
    return setPressStepCount(_handle, step) != 0;
  }

  /// Read the calibration value.
  /// 读取校准值。
  double? getCalibrationValue() {
    _ensureNotDisposed();
    final getCalibrationValue = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>)
        >('cpress_get_calibration_value');

    final value = malloc<Float>();
    try {
      return getCalibrationValue(_handle, value) != 0 ? value.value : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the calibration value.
  /// 设置校准值。
  bool setCalibrationValue(double value) {
    _ensureNotDisposed();
    final setCalibrationValue = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float),
          int Function(Pointer<Void>, double)
        >('cpress_set_calibration_value');
    return setCalibrationValue(_handle, value) != 0;
  }

  /// Read the mode type.
  /// 读取模式类型。
  int? getModeType() {
    _ensureNotDisposed();
    final getModeType = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_mode_type');

    final type = malloc<Uint8>();
    try {
      return getModeType(_handle, type) != 0 ? type.value : null;
    } finally {
      malloc.free(type);
    }
  }

  /// Write the mode type.
  /// 设置模式类型。
  bool setModeType(int type) {
    _ensureNotDisposed();
    final setModeType = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_mode_type');
    return setModeType(_handle, type) != 0;
  }

  /// Read the circle diameter.
  /// 读取圆形直径。
  double? getCircle() {
    _ensureNotDisposed();
    final getCircle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>)
        >('cpress_get_circle');

    final diameter = malloc<Float>();
    try {
      return getCircle(_handle, diameter) != 0 ? diameter.value : null;
    } finally {
      malloc.free(diameter);
    }
  }

  /// Write the circle diameter.
  /// 设置圆形直径。
  bool setCircle(double diameter) {
    _ensureNotDisposed();
    final setCircle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float),
          int Function(Pointer<Void>, double)
        >('cpress_set_circle');
    return setCircle(_handle, diameter) != 0;
  }

  /// Read the rectangle size as [a, b].
  /// 读取矩形尺寸，返回 [a, b]。
  List<double>? getRectangle() {
    _ensureNotDisposed();
    final getRectangle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>, Pointer<Float>)
        >('cpress_get_rectangle');

    final a = malloc<Float>();
    final b = malloc<Float>();
    try {
      if (getRectangle(_handle, a, b) == 0) {
        return null;
      }
      return <double>[a.value, b.value];
    } finally {
      malloc.free(a);
      malloc.free(b);
    }
  }

  /// Write the rectangle size.
  /// 设置矩形尺寸。
  bool setRectangle(double a, double b) {
    _ensureNotDisposed();
    final setRectangle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float, Float),
          int Function(Pointer<Void>, double, double)
        >('cpress_set_rectangle');
    return setRectangle(_handle, a, b) != 0;
  }

  /// Read the air time in seconds.
  /// 读取排气时间（秒）。
  int? getAirTime() {
    _ensureNotDisposed();
    final getAirTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_air_time');

    final seconds = malloc<Uint8>();
    try {
      return getAirTime(_handle, seconds) != 0 ? seconds.value : null;
    } finally {
      malloc.free(seconds);
    }
  }

  /// Write the air time in seconds.
  /// 设置排气时间（秒）。
  bool setAirTime(int seconds) {
    _ensureNotDisposed();
    final setAirTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_air_time');
    return setAirTime(_handle, seconds) != 0;
  }

  /// Read the press value of one step.
  /// 读取指定段的压力值。
  double? getPressValueIndex(int index) {
    _ensureNotDisposed();
    final getPressValueIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_press_value_index');

    final value = malloc<Float>();
    try {
      return getPressValueIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the press value of one step.
  /// 设置指定段的压力值。
  bool setPressValueIndex(int index, double value) {
    _ensureNotDisposed();
    final setPressValueIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_press_value_index');
    return setPressValueIndex(_handle, index, value) != 0;
  }

  /// Read the after-press value of one step.
  /// 读取指定段的保压后值。
  double? getAfterValueIndex(int index) {
    _ensureNotDisposed();
    final getAfterValueIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_after_value_index');

    final value = malloc<Float>();
    try {
      return getAfterValueIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the after-press value of one step.
  /// 设置指定段的保压后值。
  bool setAfterValueIndex(int index, double value) {
    _ensureNotDisposed();
    final setAfterValueIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_after_value_index');
    return setAfterValueIndex(_handle, index, value) != 0;
  }

  /// Read the keep-press time of one step in milliseconds.
  /// 读取指定段的保压时间（毫秒）。
  int? getKeepPressTime(int index) {
    _ensureNotDisposed();
    final getKeepPressTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Int32>),
          int Function(Pointer<Void>, int, Pointer<Int32>)
        >('cpress_get_keep_press_time');

    final ms = malloc<Int32>();
    try {
      return getKeepPressTime(_handle, index, ms) != 0 ? ms.value : null;
    } finally {
      malloc.free(ms);
    }
  }

  /// Write the keep-press time of one step in milliseconds.
  /// 设置指定段的保压时间（毫秒）。
  bool setKeepPressTime(int index, int ms) {
    _ensureNotDisposed();
    final setKeepPressTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Int32),
          int Function(Pointer<Void>, int, int)
        >('cpress_set_keep_press_time');
    return setKeepPressTime(_handle, index, ms) != 0;
  }

  // ==================== Temperature / 温度 ====================

  /// Read the temperature step count.
  /// 读取温度段数。
  int? getTempStepCount() {
    _ensureNotDisposed();
    final getTempStepCount = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_temp_step_count');

    final step = malloc<Uint8>();
    try {
      return getTempStepCount(_handle, step) != 0 ? step.value : null;
    } finally {
      malloc.free(step);
    }
  }

  /// Write the temperature step count.
  /// 设置温度段数。
  bool setTempStepCount(int step) {
    _ensureNotDisposed();
    final setTempStepCount = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_temp_step_count');
    return setTempStepCount(_handle, step) != 0;
  }

  /// Read the temperature speed mold.
  /// 读取温度速率模具。
  int? getTempSpeedMold() {
    _ensureNotDisposed();
    final getTempSpeedMold = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_temp_speed_mold');

    final mold = malloc<Uint8>();
    try {
      return getTempSpeedMold(_handle, mold) != 0 ? mold.value : null;
    } finally {
      malloc.free(mold);
    }
  }

  /// Write the temperature speed mold.
  /// 设置温度速率模具。
  bool setTempSpeedMold(int mold) {
    _ensureNotDisposed();
    final setTempSpeedMold = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_temp_speed_mold');
    return setTempSpeedMold(_handle, mold) != 0;
  }

  /// Read the start temperature of one step.
  /// 读取指定段的起始温度。
  double? getStartTempIndex(int index) {
    _ensureNotDisposed();
    final getStartTempIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_start_temp_index');

    final value = malloc<Float>();
    try {
      return getStartTempIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the start temperature of one step.
  /// 设置指定段的起始温度。
  bool setTempIndex(int index, double value) {
    _ensureNotDisposed();
    final setTempIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_temp_index');
    return setTempIndex(_handle, index, value) != 0;
  }

  /// Read the temperature speed of one step.
  /// 读取指定段的温度速率。
  double? getTempSpeedIndex(int index) {
    _ensureNotDisposed();
    final getTempSpeedIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_temp_speed_index');

    final value = malloc<Float>();
    try {
      return getTempSpeedIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the temperature speed of one step.
  /// 设置指定段的温度速率。
  bool setTempSpeedIndex(int index, double value) {
    _ensureNotDisposed();
    final setTempSpeedIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_temp_speed_index');
    return setTempSpeedIndex(_handle, index, value) != 0;
  }

  /// Read the keep-temperature time of one step in milliseconds.
  /// 读取指定段的保温时间（毫秒）。
  int? getKeepTempTime(int index) {
    _ensureNotDisposed();
    final getKeepTempTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Int32>),
          int Function(Pointer<Void>, int, Pointer<Int32>)
        >('cpress_get_keep_temp_time');

    final ms = malloc<Int32>();
    try {
      return getKeepTempTime(_handle, index, ms) != 0 ? ms.value : null;
    } finally {
      malloc.free(ms);
    }
  }

  /// Write the keep-temperature time of one step in milliseconds.
  /// 设置指定段的保温时间（毫秒）。
  bool setKeepTempTime(int index, int ms) {
    _ensureNotDisposed();
    final setKeepTempTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Int32),
          int Function(Pointer<Void>, int, int)
        >('cpress_set_keep_temp_time');
    return setKeepTempTime(_handle, index, ms) != 0;
  }

  /// Read the maximum water value.
  /// 读取最大水量值。
  double? getMaxWaterValue() {
    _ensureNotDisposed();
    final getMaxWaterValue = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>)
        >('cpress_get_max_water_value');

    final value = malloc<Float>();
    try {
      return getMaxWaterValue(_handle, value) != 0 ? value.value : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the maximum water value.
  /// 设置最大水量值。
  bool setMaxWaterValue(double value) {
    _ensureNotDisposed();
    final setMaxWaterValue = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float),
          int Function(Pointer<Void>, double)
        >('cpress_set_max_water_value');
    return setMaxWaterValue(_handle, value) != 0;
  }

  /// Read the minimum water value.
  /// 读取最小水量值。
  double? getMinWaterValue() {
    _ensureNotDisposed();
    final getMinWaterValue = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>)
        >('cpress_get_min_water_value');

    final value = malloc<Float>();
    try {
      return getMinWaterValue(_handle, value) != 0 ? value.value : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the minimum water value.
  /// 设置最小水量值。
  bool setMinWaterValue(double value) {
    _ensureNotDisposed();
    final setMinWaterValue = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float),
          int Function(Pointer<Void>, double)
        >('cpress_set_min_water_value');
    return setMinWaterValue(_handle, value) != 0;
  }

  // ==================== Mixing / 混合 ====================

  /// Read the mixing step count.
  /// 读取混合段数。
  int? getMixStepCount() {
    _ensureNotDisposed();
    final getMixStepCount = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_mix_step_count');

    final step = malloc<Uint8>();
    try {
      return getMixStepCount(_handle, step) != 0 ? step.value : null;
    } finally {
      malloc.free(step);
    }
  }

  /// Write the mixing step count.
  /// 设置混合段数。
  bool setMixStepCount(int step) {
    _ensureNotDisposed();
    final setMixStepCount = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_mix_step_count');
    return setMixStepCount(_handle, step) != 0;
  }

  /// Read the mixing mold type.
  /// 读取混合模具类型。
  int? getMixMoldType() {
    _ensureNotDisposed();
    final getMixMoldType = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Uint8>),
          int Function(Pointer<Void>, Pointer<Uint8>)
        >('cpress_get_mix_mold_type');

    final type = malloc<Uint8>();
    try {
      return getMixMoldType(_handle, type) != 0 ? type.value : null;
    } finally {
      malloc.free(type);
    }
  }

  /// Write the mixing mold type.
  /// 设置混合模具类型。
  bool setMixMoldType(int type) {
    _ensureNotDisposed();
    final setMixMoldType = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8),
          int Function(Pointer<Void>, int)
        >('cpress_set_mix_mold_type');
    return setMixMoldType(_handle, type) != 0;
  }

  /// Read the mixing rectangle size as [a, b].
  /// 读取混合矩形尺寸，返回 [a, b]。
  List<double>? getMixRectangle() {
    _ensureNotDisposed();
    final getMixRectangle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>, Pointer<Float>)
        >('cpress_get_mix_rectangle');

    final a = malloc<Float>();
    final b = malloc<Float>();
    try {
      if (getMixRectangle(_handle, a, b) == 0) {
        return null;
      }
      return <double>[a.value, b.value];
    } finally {
      malloc.free(a);
      malloc.free(b);
    }
  }

  /// Write the mixing rectangle size.
  /// 设置混合矩形尺寸。
  bool setMixRectangle(double a, double b) {
    _ensureNotDisposed();
    final setMixRectangle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float, Float),
          int Function(Pointer<Void>, double, double)
        >('cpress_set_mix_rectangle');
    return setMixRectangle(_handle, a, b) != 0;
  }

  /// Read the mixing circle diameter.
  /// 读取混合圆形直径。
  double? getMixCircle() {
    _ensureNotDisposed();
    final getMixCircle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Pointer<Float>),
          int Function(Pointer<Void>, Pointer<Float>)
        >('cpress_get_mix_circle');

    final diameter = malloc<Float>();
    try {
      return getMixCircle(_handle, diameter) != 0 ? diameter.value : null;
    } finally {
      malloc.free(diameter);
    }
  }

  /// Write the mixing circle diameter.
  /// 设置混合圆形直径。
  bool setMixCircle(double diameter) {
    _ensureNotDisposed();
    final setMixCircle = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Float),
          int Function(Pointer<Void>, double)
        >('cpress_set_mix_circle');
    return setMixCircle(_handle, diameter) != 0;
  }

  /// Read whether one step presses.
  /// 读取指定段是否加压。
  bool? getMixIsPressIndex(int index) {
    _ensureNotDisposed();
    final getMixIsPressIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Int32>),
          int Function(Pointer<Void>, int, Pointer<Int32>)
        >('cpress_get_mix_is_press_index');

    final isPress = malloc<Int32>();
    try {
      if (getMixIsPressIndex(_handle, index, isPress) == 0) {
        return null;
      }
      return isPress.value != 0;
    } finally {
      malloc.free(isPress);
    }
  }

  /// Write whether one step presses.
  /// 设置指定段是否加压。
  bool setMixIsPressIndex(int index, bool isPress) {
    _ensureNotDisposed();
    final setMixIsPressIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Int32),
          int Function(Pointer<Void>, int, int)
        >('cpress_set_mix_is_press_index');
    return setMixIsPressIndex(_handle, index, isPress ? 1 : 0) != 0;
  }

  /// Read the mixing press value of one step.
  /// 读取指定段的混合压力值。
  double? getMixPressIndex(int index) {
    _ensureNotDisposed();
    final getMixPressIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_mix_press_index');

    final value = malloc<Float>();
    try {
      return getMixPressIndex(_handle, index, value) != 0 ? value.value : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the mixing press value of one step.
  /// 设置指定段的混合压力值。
  bool setMixPressIndex(int index, double value) {
    _ensureNotDisposed();
    final setMixPressIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_mix_press_index');
    return setMixPressIndex(_handle, index, value) != 0;
  }

  /// Read the mixing after-press value of one step.
  /// 读取指定段的混合保压后值。
  double? getMixAfterPressIndex(int index) {
    _ensureNotDisposed();
    final getMixAfterPressIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_mix_after_press_index');

    final value = malloc<Float>();
    try {
      return getMixAfterPressIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the mixing after-press value of one step.
  /// 设置指定段的混合保压后值。
  bool setMixAfterPressIndex(int index, double value) {
    _ensureNotDisposed();
    final setMixAfterPressIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_mix_after_press_index');
    return setMixAfterPressIndex(_handle, index, value) != 0;
  }

  /// Read whether one step heats up.
  /// 读取指定段是否升温。
  bool? getMixIsHeatUpIndex(int index) {
    _ensureNotDisposed();
    final getMixIsHeatUpIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Int32>),
          int Function(Pointer<Void>, int, Pointer<Int32>)
        >('cpress_get_mix_is_heat_up_index');

    final isHeatUp = malloc<Int32>();
    try {
      if (getMixIsHeatUpIndex(_handle, index, isHeatUp) == 0) {
        return null;
      }
      return isHeatUp.value != 0;
    } finally {
      malloc.free(isHeatUp);
    }
  }

  /// Write whether one step heats up.
  /// 设置指定段是否升温。
  bool setMixIsHeatUpIndex(int index, bool isHeatUp) {
    _ensureNotDisposed();
    final setMixIsHeatUpIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Int32),
          int Function(Pointer<Void>, int, int)
        >('cpress_set_mix_is_heat_up_index');
    return setMixIsHeatUpIndex(_handle, index, isHeatUp ? 1 : 0) != 0;
  }

  /// Read the mixing heat-up value of one step.
  /// 读取指定段的升温值。
  double? getMixHeatUpIndex(int index) {
    _ensureNotDisposed();
    final getMixHeatUpIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_mix_heat_up_index');

    final value = malloc<Float>();
    try {
      return getMixHeatUpIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the mixing heat-up value of one step.
  /// 设置指定段的升温值。
  bool setMixHeatUpIndex(int index, double value) {
    _ensureNotDisposed();
    final setMixHeatUpIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_mix_heat_up_index');
    return setMixHeatUpIndex(_handle, index, value) != 0;
  }

  /// Read whether one step cools down.
  /// 读取指定段是否降温。
  bool? getMixIsHeatDownIndex(int index) {
    _ensureNotDisposed();
    final getMixIsHeatDownIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Int32>),
          int Function(Pointer<Void>, int, Pointer<Int32>)
        >('cpress_get_mix_is_heat_down_index');

    final isHeatDown = malloc<Int32>();
    try {
      if (getMixIsHeatDownIndex(_handle, index, isHeatDown) == 0) {
        return null;
      }
      return isHeatDown.value != 0;
    } finally {
      malloc.free(isHeatDown);
    }
  }

  /// Write whether one step cools down.
  /// 设置指定段是否降温。
  bool setMixIsHeatDownIndex(int index, bool isHeatDown) {
    _ensureNotDisposed();
    final setMixIsHeatDownIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Int32),
          int Function(Pointer<Void>, int, int)
        >('cpress_set_mix_is_heat_down_index');
    return setMixIsHeatDownIndex(_handle, index, isHeatDown ? 1 : 0) != 0;
  }

  /// Read the mixing cool-down value of one step.
  /// 读取指定段的降温值。
  double? getMixHeatDownIndex(int index) {
    _ensureNotDisposed();
    final getMixHeatDownIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Float>),
          int Function(Pointer<Void>, int, Pointer<Float>)
        >('cpress_get_mix_heat_down_index');

    final value = malloc<Float>();
    try {
      return getMixHeatDownIndex(_handle, index, value) != 0
          ? value.value
          : null;
    } finally {
      malloc.free(value);
    }
  }

  /// Write the mixing cool-down value of one step.
  /// 设置指定段的降温值。
  bool setMixHeatDownIndex(int index, double value) {
    _ensureNotDisposed();
    final setMixHeatDownIndex = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Float),
          int Function(Pointer<Void>, int, double)
        >('cpress_set_mix_heat_down_index');
    return setMixHeatDownIndex(_handle, index, value) != 0;
  }

  /// Read the mixing keep time of one step in milliseconds.
  /// 读取指定段的混合保持时间（毫秒）。
  int? getMixKeepTime(int index) {
    _ensureNotDisposed();
    final getMixKeepTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Pointer<Int32>),
          int Function(Pointer<Void>, int, Pointer<Int32>)
        >('cpress_get_mix_keep_time');

    final ms = malloc<Int32>();
    try {
      return getMixKeepTime(_handle, index, ms) != 0 ? ms.value : null;
    } finally {
      malloc.free(ms);
    }
  }

  /// Write the mixing keep time of one step in milliseconds.
  /// 设置指定段的混合保持时间（毫秒）。
  bool setMixKeepTime(int index, int ms) {
    _ensureNotDisposed();
    final setMixKeepTime = _library
        .lookupFunction<
          Int32 Function(Pointer<Void>, Uint8, Int32),
          int Function(Pointer<Void>, int, int)
        >('cpress_set_mix_keep_time');
    return setMixKeepTime(_handle, index, ms) != 0;
  }

  /// Fetch the SDK version string from the native library.
  /// 从原生库中获取 SDK 版本字符串。
  static String version() {
    final version = _library
        .lookupFunction<Pointer<Utf8> Function(), Pointer<Utf8> Function()>(
          'cpress_version',
        );

    final ptr = version();
    return ptr == nullptr ? '' : ptr.toDartString();
  }

  /// Release the native handle and free resources. After this call, the object
  /// must not be used again.
  /// 释放原生句柄并回收资源。调用此方法后，该对象不得再次使用。
  void close() {
    if (_disposed) {
      return;
    }

    disconnect();

    final destroy = _library
        .lookupFunction<
          Void Function(Pointer<Void>),
          void Function(Pointer<Void>)
        >('cpress_destroy');
    destroy(_handle);
    _disposed = true;
  }
}
