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
