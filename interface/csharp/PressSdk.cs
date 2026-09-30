using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;

namespace PressSdk.Interop
{
    public enum PortType
    {
        SerialPortType = 0,
        TcpSocketPortType = 1,
    }

    public static class PressConstants
    {
        public const int MaxPStep = 30;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct ReadOnlyData
    {
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)] public string m_NameZH;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)] public string m_NameEN;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)] public string m_Type;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)] public string m_SerialNumber;

        public byte m_Screenshot;
        public byte m_StartDelay;
        public byte m_VersionType;
        public byte m_IsHideLang;
        public byte m_Remote;
        public byte m_Network;
        public byte m_FontZH;
        public byte m_FontEN;

        public byte m_MaxPStep;
        public float m_MaxPLimit;
        public float m_MinPLimit;
        public float m_Max_Min;
        public float m_Diameter;
        public byte m_PDecimal;
        public byte m_PressDecimal;
        public byte m_PModel;
        public byte m_OutType;

        public byte m_MaxTStep;
        public float m_MaxTLimit;
        public float m_MinTLimit;
        public byte m_TDecimal;
        public byte m_IsHasWater;
        public byte m_IsHasSpeed;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PressData
    {
        public byte m_PStep;
        public byte m_Type;
        public float m_A;
        public float m_B;
        public float m_D;
        public float m_OuterD;
        public float m_InnerD;
        public float m_CheckValue;
        public byte m_Speed;
        public float m_DemoldValue;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = PressConstants.MaxPStep)]
        public float[] m_SetPValue;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = PressConstants.MaxPStep)]
        public float[] m_AfterValue;

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = PressConstants.MaxPStep)]
        public uint[] m_KPTime;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct RealTimeData
    {
        public byte m_ModelState;
        public byte m_PressState;
        public byte m_CPStep;
        public float m_PressValue;
        public uint m_PTime;
        public byte m_PdChanged;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCReadOnlyDataCallback(IntPtr userData, int errorCode, ulong registerNo, ref ReadOnlyData readOnlyData);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCRealTimeDataCallback(IntPtr userData, int errorCode, ref RealTimeData realTimeData);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCPressDataCallback(IntPtr userData, int errorCode, ref PressData pressData);

    /// <summary>
    /// Error callback. The native layer reports the raw response pointer and its size using size_t;
    /// UIntPtr matches that ABI better than IntPtr.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCErrorCallback(IntPtr userData, ushort cmdCode, IntPtr response, UIntPtr responseLen);

    [StructLayout(LayoutKind.Sequential)]
    public struct PressCDataCallbacks
    {
        public IntPtr userData;
        public IntPtr onReadOnlyData;
        public IntPtr onRealTimeData;
        public IntPtr onPressData;
        public IntPtr onError;
    }

    /// <summary>
    /// High-level managed wrapper around the native Press SDK.
    /// 对底层图像工具链的托管包装，负责管理句柄和数据回调。
    ///
    /// 注意：此类名与命名空间 <c>PressSdk.Interop</c> 共享同名，C# 在直接 <c>using PressSdk.Interop;</c> 后可能出现“namespace used as type”的歧义。
    /// 建议使用：
    /// <code>
    /// using PressSdkApi = PressSdk.Interop.PressSdk;
    /// using var sdk = new PressSdkApi();
    /// </code>
    /// </summary>
    public sealed class PressSdk : IDisposable
    {
        private const string NativeLibraryBaseName = "press";
        private static readonly string NativeLibraryName = ResolveNativeLibraryName();

        private readonly IntPtr _handle;
        private readonly object _syncRoot = new object();
        private GCHandle _callbackHandle;
        private PressCReadOnlyDataCallback? _onReadOnlyDataCallback;
        private PressCRealTimeDataCallback? _onRealTimeDataCallback;
        private PressCPressDataCallback? _onPressDataCallback;
        private PressCErrorCallback? _onErrorCallback;

        static PressSdk()
        {
            // 运行时需要在未显式加入 PATH 的情况下也能找到实际 DLL。
            NativeLibrary.SetDllImportResolver(typeof(PressSdk).Assembly, (libraryName, _, _) =>
            {
                if (!IsManagedLibraryName(libraryName))
                {
                    return IntPtr.Zero;
                }

                foreach (var candidate in GetNativeLibraryCandidates())
                {
                    if (NativeLibrary.TryLoad(candidate, out var handle))
                    {
                        return handle;
                    }
                }

                return IntPtr.Zero;
            });
        }

        private static bool IsManagedLibraryName(string libraryName)
        {
            return string.Equals(libraryName, NativeLibraryBaseName, StringComparison.OrdinalIgnoreCase)
                || string.Equals(libraryName, "press.dll", StringComparison.OrdinalIgnoreCase)
                || string.Equals(libraryName, "libpress.so", StringComparison.OrdinalIgnoreCase)
                || string.Equals(libraryName, "libpress.dylib", StringComparison.OrdinalIgnoreCase);
        }

        private static IEnumerable<string> GetNativeLibraryCandidates()
        {
            var searchRoots = new HashSet<string>(StringComparer.OrdinalIgnoreCase)
            {
                AppContext.BaseDirectory,
                Directory.GetCurrentDirectory(),
                Path.Combine(AppContext.BaseDirectory, "install", "bin"),
                Path.Combine(AppContext.BaseDirectory, "..", "..", "install", "bin"),
                Path.Combine(AppContext.BaseDirectory, "build", "press", "Debug"),
                Path.Combine(AppContext.BaseDirectory, "build_jni_check", "press", "Debug"),
                Path.Combine(AppContext.BaseDirectory, "..", "..", "build", "press", "Debug"),
                Path.Combine(AppContext.BaseDirectory, "..", "..", "build_jni_check", "press", "Debug"),
            };

            foreach (var dir in searchRoots)
            {
                foreach (var fileName in new[] { "press.dll", "pressd.dll", "libpress.so", "libpress.dylib", "press" })
                {
                    if (!string.IsNullOrWhiteSpace(dir))
                    {
                        yield return Path.Combine(dir, fileName);
                    }
                }
            }

            foreach (var fileName in new[] { "press.dll", "pressd.dll", "libpress.so", "libpress.dylib", "press" })
            {
                yield return fileName;
            }

            var configuredPath = Environment.GetEnvironmentVariable("PRESS_SDK_LIB_PATH");
            if (!string.IsNullOrWhiteSpace(configuredPath))
            {
                foreach (var fileName in new[] { "press.dll", "pressd.dll", "libpress.so", "libpress.dylib", "press" })
                {
                    yield return Path.Combine(configuredPath, fileName);
                }
            }
        }

        private static string ResolveNativeLibraryName()
        {
            foreach (var candidate in GetNativeLibraryCandidates())
            {
                try
                {
                    if (!string.IsNullOrWhiteSpace(candidate) && NativeLibrary.TryLoad(candidate, out _))
                    {
                        return Path.GetFileName(candidate);
                    }
                }
                catch
                {
                    // Ignore candidate-specific failures and try the next location.
                }
            }

            return NativeLibraryBaseName;
        }

        [DllImport("press", EntryPoint = "cpress_create", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern IntPtr NativeCreate();

        [DllImport("press", EntryPoint = "cpress_destroy", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeDestroy(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_connect", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeConnect(IntPtr handle, string portName, int portType);

        [DllImport("press", EntryPoint = "cpress_disconnect", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeDisconnect(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_is_connected", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeIsConnected(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_get_machine_register_no", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern ulong NativeGetMachineRegisterNo(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_get_last_error_info", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern IntPtr NativeGetLastErrorInfo(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_run", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeRun(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_stop", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeStop(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_is_running", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeIsRunning(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_get_read_only_data", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetReadOnlyData(IntPtr handle, out ReadOnlyData data, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_get_press_data", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetPressData(IntPtr handle, out PressData data, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_set_press_data", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetPressData(IntPtr handle, ref PressData data, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_get_real_time_data", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetRealTimeData(IntPtr handle, out RealTimeData data, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_set_pressing", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetPressing(IntPtr handle, int isPressing);

        [DllImport("press", EntryPoint = "cpress_set_demolding", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetDemolding(IntPtr handle, int isDemolding);

        [DllImport("press", EntryPoint = "cpress_register_data_interface", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeRegisterDataInterface(IntPtr handle, ref PressCDataCallbacks callbacks);

        [DllImport("press", EntryPoint = "cpress_unregister_data_interface", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeUnregisterDataInterface(IntPtr handle);

        [DllImport("press", EntryPoint = "cpress_version", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern IntPtr NativeVersion();

        public PressSdk()
        {
            _handle = NativeCreate();
            if (_handle == IntPtr.Zero)
            {
                throw new InvalidOperationException("Failed to create Press SDK handle.");
            }
        }

        public bool Connect(string portName, PortType portType)
        {
            if (string.IsNullOrEmpty(portName))
            {
                throw new ArgumentException("portName cannot be null or empty.", nameof(portName));
            }

            return NativeConnect(_handle, portName, (int)portType) != 0;
        }

        public void Disconnect()
        {
            NativeDisconnect(_handle);
        }

        public bool IsConnected()
        {
            return NativeIsConnected(_handle) != 0;
        }

        public ulong GetMachineRegisterNo()
        {
            return NativeGetMachineRegisterNo(_handle);
        }

        public string GetLastErrorInfo()
        {
            var ptr = NativeGetLastErrorInfo(_handle);
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(ptr) ?? string.Empty;
        }

        public void Run()
        {
            NativeRun(_handle);
        }

        public void Stop()
        {
            NativeStop(_handle);
        }

        public bool IsRunning()
        {
            return NativeIsRunning(_handle) != 0;
        }

        public bool GetReadOnlyData(out ReadOnlyData data, bool isCompressed = false)
        {
            return NativeGetReadOnlyData(_handle, out data, isCompressed ? 1 : 0) != 0;
        }

        public bool GetPressData(out PressData data, bool isCompressed = false)
        {
            return NativeGetPressData(_handle, out data, isCompressed ? 1 : 0) != 0;
        }

        public bool SetPressData(ref PressData data, bool isCompressed = false)
        {
            return NativeSetPressData(_handle, ref data, isCompressed ? 1 : 0) != 0;
        }

        public bool GetRealTimeData(out RealTimeData data, bool isCompressed = false)
        {
            return NativeGetRealTimeData(_handle, out data, isCompressed ? 1 : 0) != 0;
        }

        public bool SetPressing(bool isPressing)
        {
            return NativeSetPressing(_handle, isPressing ? 1 : 0) != 0;
        }

        public bool SetDemolding(bool isDemolding)
        {
            return NativeSetDemolding(_handle, isDemolding ? 1 : 0) != 0;
        }

        public void RegisterDataInterface(
            PressCReadOnlyDataCallback? onReadOnlyData = null,
            PressCRealTimeDataCallback? onRealTimeData = null,
            PressCPressDataCallback? onPressData = null,
            PressCErrorCallback? onError = null,
            IntPtr? userData = null)
        {
            lock (_syncRoot)
            {
                if (_callbackHandle.IsAllocated)
                {
                    _callbackHandle.Free();
                }

                _onReadOnlyDataCallback = onReadOnlyData;
                _onRealTimeDataCallback = onRealTimeData;
                _onPressDataCallback = onPressData;
                _onErrorCallback = onError;

                var state = new PressCDataCallbacks
                {
                    userData = userData ?? IntPtr.Zero,
                    onReadOnlyData = onReadOnlyData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onReadOnlyData),
                    onRealTimeData = onRealTimeData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onRealTimeData),
                    onPressData = onPressData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onPressData),
                    onError = onError == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onError),
                };

                _callbackHandle = GCHandle.Alloc(state, GCHandleType.Normal);
                NativeRegisterDataInterface(_handle, ref state);
            }
        }

        public void UnregisterDataInterface()
        {
            lock (_syncRoot)
            {
                NativeUnregisterDataInterface(_handle);

                if (_callbackHandle.IsAllocated)
                {
                    _callbackHandle.Free();
                    _callbackHandle = default;
                }
            }
        }

        public static string Version()
        {
            var ptr = NativeVersion();
            return ptr == IntPtr.Zero ? string.Empty : Marshal.PtrToStringAnsi(ptr) ?? string.Empty;
        }

        public void Dispose()
        {
            try
            {
                UnregisterDataInterface();
            }
            finally
            {
                if (_handle != IntPtr.Zero)
                {
                    NativeDisconnect(_handle);
                    NativeDestroy(_handle);
                }
            }

            GC.SuppressFinalize(this);
        }
    }
}
