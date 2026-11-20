"""Python bindings for the Press SDK.

The module loads the native ``press`` shared library (Windows ``press.dll`` /
``pressd.dll``, Linux ``libpress.so``, macOS ``libpress.dylib``) and exposes a
thin, Pythonic wrapper around the ``cpress_*`` C API.

- **Lifecycle**: create :class:`PressSdk`, call :meth:`PressSdk.connect`, then
  use the get/set methods; use the object as a context manager to release the
  native handle.
- **Threading**: timer callbacks run on the SDK timer thread; callbacks caused
  by a synchronous ``get_*_json`` call run on the calling thread.
- **Data**: JSON methods exchange UTF-8 strings (ROD / PD / RT / TD / MD);
  array helpers operate with 0-based indices.
- **Errors**: getters raise ``RuntimeError`` with the SDK error text on failure;
  setters return ``False``.

Press SDK 的 Python 绑定：加载原生动态库并以 Python 风格封装 ``cpress_*``
C 接口，覆盖生命周期、JSON 数据与常用单值命令。
"""

import ctypes
import os
from enum import IntEnum


MAX_P_STEP = 30


def _iter_candidate_dirs():
    """Yield likely native-library locations around the project tree.

    The generated SDK can appear under several layouts depending on the build
    system: project root, build/, build_jni_check/, install/bin/, and nested
    Debug/Release output directories. The loader should search all of these,
    not just the current working directory.
    """
    project_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    roots = [
        os.getcwd(),
        project_root,
        os.path.join(project_root, "build"),
        os.path.join(project_root, "build_jni_check"),
        os.path.join(project_root, "install", "bin"),
        os.path.join(project_root, "install", "lib"),
    ]

    seen = set()
    for root in roots:
        normalized = os.path.abspath(root)
        if normalized not in seen:
            seen.add(normalized)
            yield normalized

    for root in (os.path.join(project_root, "build"), os.path.join(project_root, "build_jni_check")):
        for subdir in ("press", "x64"):
            for config in ("Debug", "Release"):
                candidate = os.path.join(root, subdir, config)
                if candidate not in seen:
                    seen.add(candidate)
                    yield candidate


def _resolve_library_path() -> str:
    """Resolve the native SDK library path across common build layouts.

    On Windows the debug build often produces "pressd.dll" instead of
    "press.dll", and the library may live under build_jni_check/press/Debug/.
    We therefore search several candidates and respect an explicit override via
    the PRESS_SDK_LIB environment variable when provided.
    """
    env_override = os.environ.get("PRESS_SDK_LIB")
    if env_override:
        return env_override

    library_names = [
        "pressd.dll",
        "press.dll",
        "libpress.so",
        "libpress.dylib",
        "pressd",
        "press",
    ]

    for directory in _iter_candidate_dirs():
        for library_name in library_names:
            candidate = os.path.join(directory, library_name)
            if os.path.isfile(candidate):
                return candidate

    # Final fallback: ask the system for a discoverable name if the runtime
    # exposes one on the host machine. This is cheaper than a hard-coded guess.
    try:
        import ctypes.util

        for name in ("pressd", "press"):
            resolved = ctypes.util.find_library(name)
            if resolved:
                return resolved
    except Exception:
        pass

    # The native library is still optional here; callers get a clearer error at
    # the point of loading the DLL rather than a silent mis-resolution.
    return "press"


class PortType(IntEnum):
    """Enum values match the native C ABI: 0 = serial, 1 = TCP socket."""

    SerialPortType = 0
    TcpSocketPortType = 1


class ReadOnlyData(ctypes.Structure):
    """Static device metadata and hardware limits.

    The field order is intentionally strict because this structure is passed
    directly to the native C library via ctypes; reordering it would break the
    ABI contract.
    """

    _fields_ = [
        ("m_NameZH", ctypes.c_char * 64),
        ("m_NameEN", ctypes.c_char * 64),
        ("m_Type", ctypes.c_char * 64),
        ("m_SerialNumber", ctypes.c_char * 64),
        ("m_Screenshot", ctypes.c_ubyte),
        ("m_StartDelay", ctypes.c_ubyte),
        ("m_VersionType", ctypes.c_ubyte),
        ("m_IsHideLang", ctypes.c_ubyte),
        ("m_Remote", ctypes.c_ubyte),
        ("m_Network", ctypes.c_ubyte),
        ("m_FontZH", ctypes.c_ubyte),
        ("m_FontEN", ctypes.c_ubyte),
        ("m_MaxPStep", ctypes.c_ubyte),
        ("m_MaxPLimit", ctypes.c_float),
        ("m_MinPLimit", ctypes.c_float),
        ("m_Max_Min", ctypes.c_float),
        ("m_Diameter", ctypes.c_float),
        ("m_PDecimal", ctypes.c_ubyte),
        ("m_PressDecimal", ctypes.c_ubyte),
        ("m_PModel", ctypes.c_ubyte),
        ("m_OutType", ctypes.c_ubyte),
        ("m_MaxTStep", ctypes.c_ubyte),
        ("m_MaxTLimit", ctypes.c_float),
        ("m_MinTLimit", ctypes.c_float),
        ("m_TDecimal", ctypes.c_ubyte),
        ("m_IsHasWater", ctypes.c_ubyte),
        ("m_IsHasSpeed", ctypes.c_ubyte),
    ]


class DeviceVersionInfo(ctypes.Structure):
    """Device firmware version cached during connect (m_JsonData.m_VersionInfo).

    Reading it does not issue a new device request.
    设备固件版本，连接握手时缓存（m_JsonData.m_VersionInfo）；读取不发起新请求。
    """

    _fields_ = [
        ("appNo", ctypes.c_ubyte),
        ("major", ctypes.c_ubyte),
        ("minor", ctypes.c_ubyte),
        ("patch", ctypes.c_ubyte),
    ]


class PressData(ctypes.Structure):
    """Multi-step pressure profile passed to the native SDK."""

    _fields_ = [
        ("m_PStep", ctypes.c_ubyte),
        ("m_Type", ctypes.c_ubyte),
        ("m_A", ctypes.c_float),
        ("m_B", ctypes.c_float),
        ("m_D", ctypes.c_float),
        ("m_OuterD", ctypes.c_float),
        ("m_InnerD", ctypes.c_float),
        ("m_CheckValue", ctypes.c_float),
        ("m_Speed", ctypes.c_ubyte),
        ("m_DemoldValue", ctypes.c_float),
        ("m_SetPValue", ctypes.c_float * MAX_P_STEP),
        ("m_AfterValue", ctypes.c_float * MAX_P_STEP),
        ("m_KPTime", ctypes.c_uint32 * MAX_P_STEP),
    ]


class RealTimeData(ctypes.Structure):
    """Live status information returned by the device."""

    _fields_ = [
        ("m_ModelState", ctypes.c_ubyte),
        ("m_PressState", ctypes.c_ubyte),
        ("m_CPStep", ctypes.c_ubyte),
        ("m_PressValue", ctypes.c_float),
        ("m_PTime", ctypes.c_uint32),
        ("m_PdChanged", ctypes.c_ubyte),
    ]


class PressCDataCallbacks(ctypes.Structure):
    """Low-level callback registration structure mirrored from the C API."""

    _fields_ = [
        ("userData", ctypes.c_void_p),
        ("onReadOnlyData", ctypes.c_void_p),
        ("onRealTimeData", ctypes.c_void_p),
        ("onPressData", ctypes.c_void_p),
        ("onError", ctypes.c_void_p),
        ("onTempData", ctypes.c_void_p),
        ("onMixData", ctypes.c_void_p),
    ]


class PressSdk:
    """High-level Python wrapper over the native ``cpress_*`` C API.

    Use it as a context manager (``with PressSdk() as sdk:``) or call
    :meth:`close` explicitly. All indexed helpers use 0-based indices and
    return ``False`` when the device rejects the operation.

    Press SDK 的高层 Python 封装。建议使用 ``with`` 语句或显式调用
    ``close()`` 释放原生句柄；所有索引接口均从 0 开始，写入失败返回
    ``False``。
    """

    try:
        _lib = ctypes.CDLL(_resolve_library_path())
    except OSError as exc:  # pragma: no cover - only triggered when the DLL is absent.
        raise RuntimeError(
            "Unable to load the Press SDK native library. "
            "Set PRESS_SDK_LIB to a valid DLL/SO path or place the library in a known build directory."
        ) from exc

    # Expose the full C ABI so the Python wrapper matches the native function
    # signatures exactly instead of relying on implicit defaults.
    _lib.cpress_create.restype = ctypes.c_void_p
    _lib.cpress_destroy.argtypes = [ctypes.c_void_p]

    _lib.cpress_connect.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    _lib.cpress_connect.restype = ctypes.c_int
    _lib.cpress_disconnect.argtypes = [ctypes.c_void_p]

    _lib.cpress_is_connected.argtypes = [ctypes.c_void_p]
    _lib.cpress_is_connected.restype = ctypes.c_int

    _lib.cpress_get_machine_register_no.argtypes = [ctypes.c_void_p]
    _lib.cpress_get_machine_register_no.restype = ctypes.c_uint64

    _lib.cpress_get_last_error_info.argtypes = [ctypes.c_void_p]
    _lib.cpress_get_last_error_info.restype = ctypes.c_char_p

    _lib.cpress_run.argtypes = [ctypes.c_void_p]
    _lib.cpress_stop.argtypes = [ctypes.c_void_p]

    _lib.cpress_is_running.argtypes = [ctypes.c_void_p]
    _lib.cpress_is_running.restype = ctypes.c_int

    _lib.cpress_get_read_only_data_json.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_get_read_only_data_json.restype = ctypes.c_char_p

    _lib.cpress_get_press_data_json.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_get_press_data_json.restype = ctypes.c_char_p

    _lib.cpress_set_press_data_json.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    _lib.cpress_set_press_data_json.restype = ctypes.c_int

    _lib.cpress_get_real_time_data_json.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_get_real_time_data_json.restype = ctypes.c_char_p

    _lib.cpress_get_temp_data_json.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_get_temp_data_json.restype = ctypes.c_char_p
    _lib.cpress_set_temp_data_json.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    _lib.cpress_set_temp_data_json.restype = ctypes.c_int

    _lib.cpress_get_mix_data_json.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_get_mix_data_json.restype = ctypes.c_char_p
    _lib.cpress_set_mix_data_json.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
    _lib.cpress_set_mix_data_json.restype = ctypes.c_int

    _lib.cpress_set_pressing.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_set_pressing.restype = ctypes.c_int

    _lib.cpress_set_demolding.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_set_demolding.restype = ctypes.c_int

    _lib.cpress_start_air.argtypes = [ctypes.c_void_p]
    _lib.cpress_start_air.restype = ctypes.c_int

    _lib.cpress_get_device_version_info.argtypes = [ctypes.c_void_p, ctypes.POINTER(DeviceVersionInfo)]
    _lib.cpress_get_device_version_info.restype = ctypes.c_int

    # ==================== 压力单值 / 索引命令 ====================
    _lib.cpress_get_press_step_count.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_press_step_count.restype = ctypes.c_int
    _lib.cpress_set_press_step_count.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_press_step_count.restype = ctypes.c_int

    _lib.cpress_get_calibration_value.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_calibration_value.restype = ctypes.c_int
    _lib.cpress_set_calibration_value.argtypes = [ctypes.c_void_p, ctypes.c_float]
    _lib.cpress_set_calibration_value.restype = ctypes.c_int

    _lib.cpress_get_mold_type.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_mold_type.restype = ctypes.c_int
    _lib.cpress_set_mold_type.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_mold_type.restype = ctypes.c_int

    _lib.cpress_get_circle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_circle.restype = ctypes.c_int
    _lib.cpress_set_circle.argtypes = [ctypes.c_void_p, ctypes.c_float]
    _lib.cpress_set_circle.restype = ctypes.c_int

    _lib.cpress_get_rectangle.argtypes = [
        ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_rectangle.restype = ctypes.c_int
    _lib.cpress_set_rectangle.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float]
    _lib.cpress_set_rectangle.restype = ctypes.c_int

    _lib.cpress_get_air_time.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_air_time.restype = ctypes.c_int
    _lib.cpress_set_air_time.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_air_time.restype = ctypes.c_int

    _lib.cpress_get_press_value_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_press_value_index.restype = ctypes.c_int
    _lib.cpress_set_press_value_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_press_value_index.restype = ctypes.c_int

    _lib.cpress_get_after_value_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_after_value_index.restype = ctypes.c_int
    _lib.cpress_set_after_value_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_after_value_index.restype = ctypes.c_int

    _lib.cpress_get_keep_press_time.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_int32)]
    _lib.cpress_get_keep_press_time.restype = ctypes.c_int
    _lib.cpress_set_keep_press_time.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int32]
    _lib.cpress_set_keep_press_time.restype = ctypes.c_int

    # ==================== 温度单值 / 索引命令 ====================
    _lib.cpress_get_temp_step_count.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_temp_step_count.restype = ctypes.c_int
    _lib.cpress_set_temp_step_count.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_temp_step_count.restype = ctypes.c_int

    _lib.cpress_get_temp_speed_mold.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_temp_speed_mold.restype = ctypes.c_int
    _lib.cpress_set_temp_speed_mold.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_temp_speed_mold.restype = ctypes.c_int

    _lib.cpress_get_start_temp_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_start_temp_index.restype = ctypes.c_int
    _lib.cpress_set_temp_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_temp_index.restype = ctypes.c_int

    _lib.cpress_get_temp_speed_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_temp_speed_index.restype = ctypes.c_int
    _lib.cpress_set_temp_speed_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_temp_speed_index.restype = ctypes.c_int

    _lib.cpress_get_keep_temp_time.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_int32)]
    _lib.cpress_get_keep_temp_time.restype = ctypes.c_int
    _lib.cpress_set_keep_temp_time.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int32]
    _lib.cpress_set_keep_temp_time.restype = ctypes.c_int

    _lib.cpress_get_max_water_value.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_max_water_value.restype = ctypes.c_int
    _lib.cpress_set_max_water_value.argtypes = [ctypes.c_void_p, ctypes.c_float]
    _lib.cpress_set_max_water_value.restype = ctypes.c_int

    _lib.cpress_get_min_water_value.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_min_water_value.restype = ctypes.c_int
    _lib.cpress_set_min_water_value.argtypes = [ctypes.c_void_p, ctypes.c_float]
    _lib.cpress_set_min_water_value.restype = ctypes.c_int

    # ==================== 混合模式单值 / 索引命令 ====================
    _lib.cpress_get_mix_step_count.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_mix_step_count.restype = ctypes.c_int
    _lib.cpress_set_mix_step_count.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_mix_step_count.restype = ctypes.c_int

    _lib.cpress_get_mix_mold_type.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ubyte)]
    _lib.cpress_get_mix_mold_type.restype = ctypes.c_int
    _lib.cpress_set_mix_mold_type.argtypes = [ctypes.c_void_p, ctypes.c_ubyte]
    _lib.cpress_set_mix_mold_type.restype = ctypes.c_int

    _lib.cpress_get_mix_rectangle.argtypes = [
        ctypes.c_void_p, ctypes.POINTER(ctypes.c_float), ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_mix_rectangle.restype = ctypes.c_int
    _lib.cpress_set_mix_rectangle.argtypes = [ctypes.c_void_p, ctypes.c_float, ctypes.c_float]
    _lib.cpress_set_mix_rectangle.restype = ctypes.c_int

    _lib.cpress_get_mix_circle.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_mix_circle.restype = ctypes.c_int
    _lib.cpress_set_mix_circle.argtypes = [ctypes.c_void_p, ctypes.c_float]
    _lib.cpress_set_mix_circle.restype = ctypes.c_int

    _lib.cpress_get_mix_is_press_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_int)]
    _lib.cpress_get_mix_is_press_index.restype = ctypes.c_int
    _lib.cpress_set_mix_is_press_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int]
    _lib.cpress_set_mix_is_press_index.restype = ctypes.c_int

    _lib.cpress_get_mix_press_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_mix_press_index.restype = ctypes.c_int
    _lib.cpress_set_mix_press_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_mix_press_index.restype = ctypes.c_int

    _lib.cpress_get_mix_after_press_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_mix_after_press_index.restype = ctypes.c_int
    _lib.cpress_set_mix_after_press_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_mix_after_press_index.restype = ctypes.c_int

    _lib.cpress_get_mix_is_heat_up_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_int)]
    _lib.cpress_get_mix_is_heat_up_index.restype = ctypes.c_int
    _lib.cpress_set_mix_is_heat_up_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int]
    _lib.cpress_set_mix_is_heat_up_index.restype = ctypes.c_int

    _lib.cpress_get_mix_heat_up_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_mix_heat_up_index.restype = ctypes.c_int
    _lib.cpress_set_mix_heat_up_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_mix_heat_up_index.restype = ctypes.c_int

    _lib.cpress_get_mix_is_heat_down_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_int)]
    _lib.cpress_get_mix_is_heat_down_index.restype = ctypes.c_int
    _lib.cpress_set_mix_is_heat_down_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int]
    _lib.cpress_set_mix_is_heat_down_index.restype = ctypes.c_int

    _lib.cpress_get_mix_heat_down_index.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_float)]
    _lib.cpress_get_mix_heat_down_index.restype = ctypes.c_int
    _lib.cpress_set_mix_heat_down_index.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_float]
    _lib.cpress_set_mix_heat_down_index.restype = ctypes.c_int

    _lib.cpress_get_mix_keep_time.argtypes = [
        ctypes.c_void_p, ctypes.c_ubyte, ctypes.POINTER(ctypes.c_int32)]
    _lib.cpress_get_mix_keep_time.restype = ctypes.c_int
    _lib.cpress_set_mix_keep_time.argtypes = [ctypes.c_void_p, ctypes.c_ubyte, ctypes.c_int32]
    _lib.cpress_set_mix_keep_time.restype = ctypes.c_int

    _lib.cpress_register_data_interface.argtypes = [ctypes.c_void_p, ctypes.POINTER(PressCDataCallbacks), ctypes.c_int]
    _lib.cpress_unregister_data_interface.argtypes = [ctypes.c_void_p]
    _lib.cpress_version.restype = ctypes.c_char_p

    def __init__(self):
        self._handle = self._lib.cpress_create()
        if not self._handle:
            raise RuntimeError("Failed to create Press SDK handle")

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()
        return False

    def connect(self, port_name: str, port_type: int | PortType = PortType.SerialPortType) -> bool:
        """Open a serial/TCP connection and perform the version handshake.

        :param port_name: ``"COM3"`` style serial port or ``"host:port"``.
        :param port_type: :class:`PortType` value.
        :return: ``True`` on success; check :meth:`get_last_error_info` on failure.
        / 打开串口或 TCP 连接并执行版本握手，失败时可通过
        ``get_last_error_info()`` 查询原因。
        """
        if not port_name:
            raise ValueError("port_name must not be empty")
        return self._lib.cpress_connect(self._handle, port_name.encode("utf-8"), int(port_type)) != 0

    def disconnect(self):
        """Disconnect and stop the SDK worker/timer threads.
        / 断开连接并停止 SDK 工作线程与定时线程。"""
        if getattr(self, "_handle", None):
            self._lib.cpress_disconnect(self._handle)

    def is_connected(self) -> bool:
        return self._lib.cpress_is_connected(self._handle) != 0

    def get_machine_register_no(self) -> int:
        return self._lib.cpress_get_machine_register_no(self._handle)

    def get_last_error_info(self) -> str:
        result = self._lib.cpress_get_last_error_info(self._handle)
        if not result:
            return ""
        return result.decode("utf-8", errors="ignore")

    def run(self):
        self._lib.cpress_run(self._handle)

    def stop(self):
        self._lib.cpress_stop(self._handle)

    def is_running(self) -> bool:
        return self._lib.cpress_is_running(self._handle) != 0

    def get_read_only_data_json(self, is_compressed: bool = False) -> str:
        """Read static device information as ROD JSON.
        / 以 ROD JSON 读取设备静态信息。"""
        result = self._lib.cpress_get_read_only_data_json(self._handle, 1 if is_compressed else 0)
        if result is None:
            raise RuntimeError(self.get_last_error_info())
        return result.decode("utf-8")

    def get_press_data_json(self, is_compressed: bool = False) -> str:
        result = self._lib.cpress_get_press_data_json(self._handle, 1 if is_compressed else 0)
        if result is None:
            raise RuntimeError(self.get_last_error_info())
        return result.decode("utf-8")

    def set_press_data_json(self, json: str, is_compressed: bool = False) -> bool:
        return self._lib.cpress_set_press_data_json(
            self._handle, json.encode("utf-8"), 1 if is_compressed else 0) != 0

    def get_real_time_data_json(self, is_compressed: bool = False) -> str:
        result = self._lib.cpress_get_real_time_data_json(self._handle, 1 if is_compressed else 0)
        if result is None:
            raise RuntimeError(self.get_last_error_info())
        return result.decode("utf-8")

    def get_temp_data_json(self, is_compressed: bool = False) -> str:
        """Read the complete temperature profile as TD JSON.
        / 以 TD JSON 读取完整温度参数。"""
        result = self._lib.cpress_get_temp_data_json(self._handle, 1 if is_compressed else 0)
        if result is None:
            raise RuntimeError(self.get_last_error_info())
        return result.decode("utf-8")

    def set_temp_data_json(self, json: str, is_compressed: bool = False) -> bool:
        """Write the complete temperature profile from TD JSON.
        / 以 TD JSON 写入完整温度参数。"""
        return self._lib.cpress_set_temp_data_json(
            self._handle, json.encode("utf-8"), 1 if is_compressed else 0) != 0

    def get_mix_data_json(self, is_compressed: bool = False) -> str:
        """Read the complete mixed-mode profile as MD JSON.
        / 以 MD JSON 读取完整混合模式参数。"""
        result = self._lib.cpress_get_mix_data_json(self._handle, 1 if is_compressed else 0)
        if result is None:
            raise RuntimeError(self.get_last_error_info())
        return result.decode("utf-8")

    def set_mix_data_json(self, json: str, is_compressed: bool = False) -> bool:
        return self._lib.cpress_set_mix_data_json(
            self._handle, json.encode("utf-8"), 1 if is_compressed else 0) != 0

    def set_pressing(self, is_pressing: bool) -> bool:
        return self._lib.cpress_set_pressing(self._handle, 1 if is_pressing else 0) != 0

    def set_demolding(self, is_demolding: bool) -> bool:
        return self._lib.cpress_set_demolding(self._handle, 1 if is_demolding else 0) != 0

    def start_air(self) -> bool:
        """Start air venting (CMDID_SET_START_AIR 0x1524). / 启动排气，释放管路内压力。"""
        return self._lib.cpress_start_air(self._handle) != 0

    def get_device_version_info(self) -> DeviceVersionInfo:
        """Read cached device firmware version (m_JsonData.m_VersionInfo).
        读取连接握手时缓存的设备固件版本，不发起新请求。"""
        info = DeviceVersionInfo()
        if self._lib.cpress_get_device_version_info(self._handle, ctypes.byref(info)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return info

    # ==================== 压力单值 / 索引命令 ====================

    def get_press_step_count(self) -> int:
        value = ctypes.c_ubyte()
        if self._lib.cpress_get_press_step_count(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_press_step_count(self, step: int) -> bool:
        return self._lib.cpress_set_press_step_count(self._handle, step) != 0

    def get_calibration_value(self) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_calibration_value(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_calibration_value(self, value: float) -> bool:
        return self._lib.cpress_set_calibration_value(self._handle, value) != 0

    def get_mold_type(self) -> int:
        value = ctypes.c_ubyte()
        if self._lib.cpress_get_mold_type(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mold_type(self, mold_type: int) -> bool:
        return self._lib.cpress_set_mold_type(self._handle, mold_type) != 0

    def get_circle(self) -> float:
        diameter = ctypes.c_float()
        if self._lib.cpress_get_circle(self._handle, ctypes.byref(diameter)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return diameter.value

    def set_circle(self, diameter: float) -> bool:
        return self._lib.cpress_set_circle(self._handle, diameter) != 0

    def get_rectangle(self) -> tuple[float, float]:
        a = ctypes.c_float()
        b = ctypes.c_float()
        if self._lib.cpress_get_rectangle(self._handle, ctypes.byref(a), ctypes.byref(b)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return a.value, b.value

    def set_rectangle(self, a: float, b: float) -> bool:
        return self._lib.cpress_set_rectangle(self._handle, a, b) != 0

    def get_air_time(self) -> int:
        seconds = ctypes.c_ubyte()
        if self._lib.cpress_get_air_time(self._handle, ctypes.byref(seconds)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return seconds.value

    def set_air_time(self, seconds: int) -> bool:
        return self._lib.cpress_set_air_time(self._handle, seconds) != 0

    def get_press_value_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_press_value_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_press_value_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_press_value_index(self._handle, index, value) != 0

    def get_after_value_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_after_value_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_after_value_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_after_value_index(self._handle, index, value) != 0

    def get_keep_press_time(self, index: int) -> int:
        ms = ctypes.c_int32()
        if self._lib.cpress_get_keep_press_time(self._handle, index, ctypes.byref(ms)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return ms.value

    def set_keep_press_time(self, index: int, ms: int) -> bool:
        return self._lib.cpress_set_keep_press_time(self._handle, index, ms) != 0

    # ==================== 温度单值 / 索引命令 ====================

    def get_temp_step_count(self) -> int:
        value = ctypes.c_ubyte()
        if self._lib.cpress_get_temp_step_count(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_temp_step_count(self, step: int) -> bool:
        return self._lib.cpress_set_temp_step_count(self._handle, step) != 0

    def get_temp_speed_mold(self) -> int:
        value = ctypes.c_ubyte()
        if self._lib.cpress_get_temp_speed_mold(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_temp_speed_mold(self, mold: int) -> bool:
        return self._lib.cpress_set_temp_speed_mold(self._handle, mold) != 0

    def get_start_temp_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_start_temp_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_temp_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_temp_index(self._handle, index, value) != 0

    def get_temp_speed_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_temp_speed_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_temp_speed_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_temp_speed_index(self._handle, index, value) != 0

    def get_keep_temp_time(self, index: int) -> int:
        ms = ctypes.c_int32()
        if self._lib.cpress_get_keep_temp_time(self._handle, index, ctypes.byref(ms)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return ms.value

    def set_keep_temp_time(self, index: int, ms: int) -> bool:
        return self._lib.cpress_set_keep_temp_time(self._handle, index, ms) != 0

    def get_max_water_value(self) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_max_water_value(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_max_water_value(self, value: float) -> bool:
        return self._lib.cpress_set_max_water_value(self._handle, value) != 0

    def get_min_water_value(self) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_min_water_value(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_min_water_value(self, value: float) -> bool:
        return self._lib.cpress_set_min_water_value(self._handle, value) != 0

    # ==================== 混合模式单值 / 索引命令 ====================

    def get_mix_step_count(self) -> int:
        value = ctypes.c_ubyte()
        if self._lib.cpress_get_mix_step_count(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mix_step_count(self, step: int) -> bool:
        return self._lib.cpress_set_mix_step_count(self._handle, step) != 0

    def get_mix_mold_type(self) -> int:
        value = ctypes.c_ubyte()
        if self._lib.cpress_get_mix_mold_type(self._handle, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mix_mold_type(self, mold_type: int) -> bool:
        return self._lib.cpress_set_mix_mold_type(self._handle, mold_type) != 0

    def get_mix_rectangle(self) -> tuple[float, float]:
        a = ctypes.c_float()
        b = ctypes.c_float()
        if self._lib.cpress_get_mix_rectangle(self._handle, ctypes.byref(a), ctypes.byref(b)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return a.value, b.value

    def set_mix_rectangle(self, a: float, b: float) -> bool:
        return self._lib.cpress_set_mix_rectangle(self._handle, a, b) != 0

    def get_mix_circle(self) -> float:
        diameter = ctypes.c_float()
        if self._lib.cpress_get_mix_circle(self._handle, ctypes.byref(diameter)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return diameter.value

    def set_mix_circle(self, diameter: float) -> bool:
        return self._lib.cpress_set_mix_circle(self._handle, diameter) != 0

    def get_mix_is_press_index(self, index: int) -> bool:
        value = ctypes.c_int()
        if self._lib.cpress_get_mix_is_press_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value != 0

    def set_mix_is_press_index(self, index: int, is_press: bool) -> bool:
        return self._lib.cpress_set_mix_is_press_index(self._handle, index, 1 if is_press else 0) != 0

    def get_mix_press_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_mix_press_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mix_press_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_mix_press_index(self._handle, index, value) != 0

    def get_mix_after_press_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_mix_after_press_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mix_after_press_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_mix_after_press_index(self._handle, index, value) != 0

    def get_mix_is_heat_up_index(self, index: int) -> bool:
        value = ctypes.c_int()
        if self._lib.cpress_get_mix_is_heat_up_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value != 0

    def set_mix_is_heat_up_index(self, index: int, is_heat_up: bool) -> bool:
        return self._lib.cpress_set_mix_is_heat_up_index(self._handle, index, 1 if is_heat_up else 0) != 0

    def get_mix_heat_up_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_mix_heat_up_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mix_heat_up_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_mix_heat_up_index(self._handle, index, value) != 0

    def get_mix_is_heat_down_index(self, index: int) -> bool:
        value = ctypes.c_int()
        if self._lib.cpress_get_mix_is_heat_down_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value != 0

    def set_mix_is_heat_down_index(self, index: int, is_heat_down: bool) -> bool:
        return self._lib.cpress_set_mix_is_heat_down_index(self._handle, index, 1 if is_heat_down else 0) != 0

    def get_mix_heat_down_index(self, index: int) -> float:
        value = ctypes.c_float()
        if self._lib.cpress_get_mix_heat_down_index(self._handle, index, ctypes.byref(value)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return value.value

    def set_mix_heat_down_index(self, index: int, value: float) -> bool:
        return self._lib.cpress_set_mix_heat_down_index(self._handle, index, value) != 0

    def get_mix_keep_time(self, index: int) -> int:
        ms = ctypes.c_int32()
        if self._lib.cpress_get_mix_keep_time(self._handle, index, ctypes.byref(ms)) == 0:
            raise RuntimeError(self.get_last_error_info())
        return ms.value

    def set_mix_keep_time(self, index: int, ms: int) -> bool:
        return self._lib.cpress_set_mix_keep_time(self._handle, index, ms) != 0

    def register_data_interface(self, callbacks: PressCDataCallbacks, interval_seconds: int = 1) -> None:
        """Register low-level native callbacks.

        :param callbacks: Callback table; keep the object alive until
            :meth:`unregister_data_interface` returns.
        :param interval_seconds: RealTimeData polling period; ``<= 0`` disables
            the timer.
        / 注册底层原生回调。回调表对象需保持存活直到注销返回；
        ``interval_seconds <= 0`` 时不启动定时采集。
        """
        self._lib.cpress_register_data_interface(self._handle, ctypes.byref(callbacks), interval_seconds)

    def unregister_data_interface(self) -> None:
        self._lib.cpress_unregister_data_interface(self._handle)

    def version(self) -> str:
        result = self._lib.cpress_version()
        if not result:
            return ""
        return result.decode("utf-8", errors="ignore")

    def close(self):
        if getattr(self, "_handle", None):
            try:
                self.disconnect()
            finally:
                self._lib.cpress_destroy(self._handle)
                self._handle = None

    def __del__(self):
        try:
            if getattr(self, "_handle", None) is not None:
                self.close()
        except Exception:
            pass


__all__ = ["MAX_P_STEP", "PortType", "ReadOnlyData", "PressData", "RealTimeData", "PressSdk", "PressCDataCallbacks"]


if __name__ == "__main__":
    sdk = PressSdk()
    print("Python Press SDK binding initialized")
    print("SDK version:", sdk.version())
    sdk.close()
