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
    ]


class PressSdk:
    """Thin Python wrapper over the native cpress_* C interface."""

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

    _lib.cpress_get_read_only_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(ReadOnlyData), ctypes.c_int]
    _lib.cpress_get_read_only_data.restype = ctypes.c_int

    _lib.cpress_get_press_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(PressData), ctypes.c_int]
    _lib.cpress_get_press_data.restype = ctypes.c_int

    _lib.cpress_set_press_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(PressData), ctypes.c_int]
    _lib.cpress_set_press_data.restype = ctypes.c_int

    _lib.cpress_get_real_time_data.argtypes = [ctypes.c_void_p, ctypes.POINTER(RealTimeData), ctypes.c_int]
    _lib.cpress_get_real_time_data.restype = ctypes.c_int

    _lib.cpress_set_pressing.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_set_pressing.restype = ctypes.c_int

    _lib.cpress_set_demolding.argtypes = [ctypes.c_void_p, ctypes.c_int]
    _lib.cpress_set_demolding.restype = ctypes.c_int

    _lib.cpress_register_data_interface.argtypes = [ctypes.c_void_p, ctypes.POINTER(PressCDataCallbacks)]
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
        if not port_name:
            raise ValueError("port_name must not be empty")
        return self._lib.cpress_connect(self._handle, port_name.encode("utf-8"), int(port_type)) != 0

    def disconnect(self):
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

    def get_read_only_data(self, is_compressed: bool = False) -> ReadOnlyData:
        data = ReadOnlyData()
        ok = self._lib.cpress_get_read_only_data(self._handle, ctypes.byref(data), 1 if is_compressed else 0)
        if not ok:
            raise RuntimeError(self.get_last_error_info())
        return data

    def get_press_data(self, is_compressed: bool = False) -> PressData:
        data = PressData()
        ok = self._lib.cpress_get_press_data(self._handle, ctypes.byref(data), 1 if is_compressed else 0)
        if not ok:
            raise RuntimeError(self.get_last_error_info())
        return data

    def set_press_data(self, data: PressData, is_compressed: bool = False) -> bool:
        return self._lib.cpress_set_press_data(self._handle, ctypes.byref(data), 1 if is_compressed else 0) != 0

    def get_real_time_data(self, is_compressed: bool = False) -> RealTimeData:
        data = RealTimeData()
        ok = self._lib.cpress_get_real_time_data(self._handle, ctypes.byref(data), 1 if is_compressed else 0)
        if not ok:
            raise RuntimeError(self.get_last_error_info())
        return data

    def set_pressing(self, is_pressing: bool) -> bool:
        return self._lib.cpress_set_pressing(self._handle, 1 if is_pressing else 0) != 0

    def set_demolding(self, is_demolding: bool) -> bool:
        return self._lib.cpress_set_demolding(self._handle, 1 if is_demolding else 0) != 0

    def register_data_interface(self, callbacks: PressCDataCallbacks) -> None:
        """Register low-level native callbacks if the app wants to receive updates."""
        self._lib.cpress_register_data_interface(self._handle, ctypes.byref(callbacks))

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
