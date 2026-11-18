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
    public delegate void PressCReadOnlyDataCallback(IntPtr userData, int errorCode, ulong registerNo, IntPtr json);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCRealTimeDataCallback(IntPtr userData, int errorCode, IntPtr json);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCPressDataCallback(IntPtr userData, int errorCode, IntPtr json);

    /// <summary>
    /// Error callback. The native layer reports the raw response pointer and its size using size_t;
    /// UIntPtr matches that ABI better than IntPtr.
    /// </summary>
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCErrorCallback(IntPtr userData, ushort cmdCode, IntPtr response, UIntPtr responseLen);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCTempDataCallback(IntPtr userData, int errorCode, IntPtr json);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void PressCMixDataCallback(IntPtr userData, int errorCode, IntPtr json);

    [StructLayout(LayoutKind.Sequential)]
    public struct PressCDataCallbacks
    {
        public IntPtr userData;
        public IntPtr onReadOnlyData;
        public IntPtr onRealTimeData;
        public IntPtr onPressData;
        public IntPtr onError;
        public IntPtr onTempData;
        public IntPtr onMixData;
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
        private PressCTempDataCallback? _onTempDataCallback;
        private PressCMixDataCallback? _onMixDataCallback;
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

        [DllImport("press", EntryPoint = "cpress_get_read_only_data_json", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern IntPtr NativeGetReadOnlyDataJson(IntPtr handle, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_get_press_data_json", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern IntPtr NativeGetPressDataJson(IntPtr handle, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_set_press_data_json", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetPressDataJson(IntPtr handle, string json, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_get_real_time_data_json", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern IntPtr NativeGetRealTimeDataJson(IntPtr handle, int isCompressed);

        [DllImport("press", EntryPoint = "cpress_set_pressing", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetPressing(IntPtr handle, int isPressing);

        [DllImport("press", EntryPoint = "cpress_set_demolding", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetDemolding(IntPtr handle, int isDemolding);

        // ==================== 压力单值 / 索引命令 ====================

        [DllImport("press", EntryPoint = "cpress_get_press_step_count", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetPressStepCount(IntPtr handle, out byte step);

        [DllImport("press", EntryPoint = "cpress_set_press_step_count", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetPressStepCount(IntPtr handle, byte step);

        [DllImport("press", EntryPoint = "cpress_get_calibration_value", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetCalibrationValue(IntPtr handle, out float value);

        [DllImport("press", EntryPoint = "cpress_set_calibration_value", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetCalibrationValue(IntPtr handle, float value);

        [DllImport("press", EntryPoint = "cpress_get_mode_type", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetModeType(IntPtr handle, out byte type);

        [DllImport("press", EntryPoint = "cpress_set_mode_type", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetModeType(IntPtr handle, byte type);

        [DllImport("press", EntryPoint = "cpress_get_circle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetCircle(IntPtr handle, out float diameter);

        [DllImport("press", EntryPoint = "cpress_set_circle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetCircle(IntPtr handle, float diameter);

        [DllImport("press", EntryPoint = "cpress_get_rectangle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetRectangle(IntPtr handle, out float a, out float b);

        [DllImport("press", EntryPoint = "cpress_set_rectangle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetRectangle(IntPtr handle, float a, float b);

        [DllImport("press", EntryPoint = "cpress_get_air_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetAirTime(IntPtr handle, out byte seconds);

        [DllImport("press", EntryPoint = "cpress_set_air_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetAirTime(IntPtr handle, byte seconds);

        [DllImport("press", EntryPoint = "cpress_get_press_value_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetPressValueIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_press_value_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetPressValueIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_after_value_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetAfterValueIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_after_value_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetAfterValueIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_keep_press_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetKeepPressTime(IntPtr handle, byte index, out int ms);

        [DllImport("press", EntryPoint = "cpress_set_keep_press_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetKeepPressTime(IntPtr handle, byte index, int ms);

        // ==================== 温度单值 / 索引命令 ====================

        [DllImport("press", EntryPoint = "cpress_get_temp_step_count", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetTempStepCount(IntPtr handle, out byte step);

        [DllImport("press", EntryPoint = "cpress_set_temp_step_count", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetTempStepCount(IntPtr handle, byte step);

        [DllImport("press", EntryPoint = "cpress_get_temp_speed_mold", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetTempSpeedMold(IntPtr handle, out byte mold);

        [DllImport("press", EntryPoint = "cpress_set_temp_speed_mold", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetTempSpeedMold(IntPtr handle, byte mold);

        [DllImport("press", EntryPoint = "cpress_get_start_temp_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetStartTempIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_temp_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetTempIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_temp_speed_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetTempSpeedIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_temp_speed_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetTempSpeedIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_keep_temp_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetKeepTempTime(IntPtr handle, byte index, out int ms);

        [DllImport("press", EntryPoint = "cpress_set_keep_temp_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetKeepTempTime(IntPtr handle, byte index, int ms);

        [DllImport("press", EntryPoint = "cpress_get_max_water_value", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMaxWaterValue(IntPtr handle, out float value);

        [DllImport("press", EntryPoint = "cpress_set_max_water_value", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMaxWaterValue(IntPtr handle, float value);

        [DllImport("press", EntryPoint = "cpress_get_min_water_value", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMinWaterValue(IntPtr handle, out float value);

        [DllImport("press", EntryPoint = "cpress_set_min_water_value", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMinWaterValue(IntPtr handle, float value);

        // ==================== 混合模式单值 / 索引命令 ====================

        [DllImport("press", EntryPoint = "cpress_get_mix_step_count", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixStepCount(IntPtr handle, out byte step);

        [DllImport("press", EntryPoint = "cpress_set_mix_step_count", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixStepCount(IntPtr handle, byte step);

        [DllImport("press", EntryPoint = "cpress_get_mix_mold_type", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixMoldType(IntPtr handle, out byte type);

        [DllImport("press", EntryPoint = "cpress_set_mix_mold_type", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixMoldType(IntPtr handle, byte type);

        [DllImport("press", EntryPoint = "cpress_get_mix_rectangle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixRectangle(IntPtr handle, out float a, out float b);

        [DllImport("press", EntryPoint = "cpress_set_mix_rectangle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixRectangle(IntPtr handle, float a, float b);

        [DllImport("press", EntryPoint = "cpress_get_mix_circle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixCircle(IntPtr handle, out float diameter);

        [DllImport("press", EntryPoint = "cpress_set_mix_circle", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixCircle(IntPtr handle, float diameter);

        [DllImport("press", EntryPoint = "cpress_get_mix_is_press_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixIsPressIndex(IntPtr handle, byte index, out int isPress);

        [DllImport("press", EntryPoint = "cpress_set_mix_is_press_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixIsPressIndex(IntPtr handle, byte index, int isPress);

        [DllImport("press", EntryPoint = "cpress_get_mix_press_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixPressIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_mix_press_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixPressIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_mix_after_press_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixAfterPressIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_mix_after_press_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixAfterPressIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_mix_is_heat_up_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixIsHeatUpIndex(IntPtr handle, byte index, out int isHeatUp);

        [DllImport("press", EntryPoint = "cpress_set_mix_is_heat_up_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixIsHeatUpIndex(IntPtr handle, byte index, int isHeatUp);

        [DllImport("press", EntryPoint = "cpress_get_mix_heat_up_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixHeatUpIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_mix_heat_up_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixHeatUpIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_mix_is_heat_down_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixIsHeatDownIndex(IntPtr handle, byte index, out int isHeatDown);

        [DllImport("press", EntryPoint = "cpress_set_mix_is_heat_down_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixIsHeatDownIndex(IntPtr handle, byte index, int isHeatDown);

        [DllImport("press", EntryPoint = "cpress_get_mix_heat_down_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixHeatDownIndex(IntPtr handle, byte index, out float value);

        [DllImport("press", EntryPoint = "cpress_set_mix_heat_down_index", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixHeatDownIndex(IntPtr handle, byte index, float value);

        [DllImport("press", EntryPoint = "cpress_get_mix_keep_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeGetMixKeepTime(IntPtr handle, byte index, out int ms);

        [DllImport("press", EntryPoint = "cpress_set_mix_keep_time", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern int NativeSetMixKeepTime(IntPtr handle, byte index, int ms);

        [DllImport("press", EntryPoint = "cpress_register_data_interface", CallingConvention = CallingConvention.Cdecl, ExactSpelling = true)]
        private static extern void NativeRegisterDataInterface(IntPtr handle, ref PressCDataCallbacks callbacks, int intervalSeconds);

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

        /// <summary>Read ROD JSON; returns null on failure. / 读取 ROD JSON，失败返回 null。</summary>
        public string? GetReadOnlyDataJson(bool isCompressed = false)
        {
            return PtrToUtf8(NativeGetReadOnlyDataJson(_handle, isCompressed ? 1 : 0));
        }

        /// <summary>Read PD JSON; returns null on failure. / 读取 PD JSON，失败返回 null。</summary>
        public string? GetPressDataJson(bool isCompressed = false)
        {
            return PtrToUtf8(NativeGetPressDataJson(_handle, isCompressed ? 1 : 0));
        }

        /// <summary>Write PD JSON. / 写入 PD JSON。</summary>
        public bool SetPressDataJson(string json, bool isCompressed = false)
        {
            return NativeSetPressDataJson(_handle, json, isCompressed ? 1 : 0) != 0;
        }

        /// <summary>Read RT JSON; returns null on failure. / 读取 RT JSON，失败返回 null。</summary>
        public string? GetRealTimeDataJson(bool isCompressed = false)
        {
            return PtrToUtf8(NativeGetRealTimeDataJson(_handle, isCompressed ? 1 : 0));
        }

        private static string? PtrToUtf8(IntPtr ptr)
        {
            if (ptr == IntPtr.Zero)
            {
                return null;
            }

            int length = 0;
            while (Marshal.ReadByte(ptr, length) != 0)
            {
                ++length;
            }

            byte[] buffer = new byte[length];
            Marshal.Copy(ptr, buffer, 0, length);
            return System.Text.Encoding.UTF8.GetString(buffer);
        }

        public bool SetPressing(bool isPressing)
        {
            return NativeSetPressing(_handle, isPressing ? 1 : 0) != 0;
        }

        public bool SetDemolding(bool isDemolding)
        {
            return NativeSetDemolding(_handle, isDemolding ? 1 : 0) != 0;
        }

        // ==================== 压力单值 / 索引命令 ====================

        public bool GetPressStepCount(out byte step)
        {
            return NativeGetPressStepCount(_handle, out step) != 0;
        }

        public bool SetPressStepCount(byte step)
        {
            return NativeSetPressStepCount(_handle, step) != 0;
        }

        public bool GetCalibrationValue(out float value)
        {
            return NativeGetCalibrationValue(_handle, out value) != 0;
        }

        public bool SetCalibrationValue(float value)
        {
            return NativeSetCalibrationValue(_handle, value) != 0;
        }

        public bool GetModeType(out byte type)
        {
            return NativeGetModeType(_handle, out type) != 0;
        }

        public bool SetModeType(byte type)
        {
            return NativeSetModeType(_handle, type) != 0;
        }

        public bool GetCircle(out float diameter)
        {
            return NativeGetCircle(_handle, out diameter) != 0;
        }

        public bool SetCircle(float diameter)
        {
            return NativeSetCircle(_handle, diameter) != 0;
        }

        public bool GetRectangle(out float a, out float b)
        {
            return NativeGetRectangle(_handle, out a, out b) != 0;
        }

        public bool SetRectangle(float a, float b)
        {
            return NativeSetRectangle(_handle, a, b) != 0;
        }

        public bool GetAirTime(out byte seconds)
        {
            return NativeGetAirTime(_handle, out seconds) != 0;
        }

        public bool SetAirTime(byte seconds)
        {
            return NativeSetAirTime(_handle, seconds) != 0;
        }

        public bool GetPressValueIndex(byte index, out float value)
        {
            return NativeGetPressValueIndex(_handle, index, out value) != 0;
        }

        public bool SetPressValueIndex(byte index, float value)
        {
            return NativeSetPressValueIndex(_handle, index, value) != 0;
        }

        public bool GetAfterValueIndex(byte index, out float value)
        {
            return NativeGetAfterValueIndex(_handle, index, out value) != 0;
        }

        public bool SetAfterValueIndex(byte index, float value)
        {
            return NativeSetAfterValueIndex(_handle, index, value) != 0;
        }

        public bool GetKeepPressTime(byte index, out int ms)
        {
            return NativeGetKeepPressTime(_handle, index, out ms) != 0;
        }

        public bool SetKeepPressTime(byte index, int ms)
        {
            return NativeSetKeepPressTime(_handle, index, ms) != 0;
        }

        // ==================== 温度单值 / 索引命令 ====================

        public bool GetTempStepCount(out byte step)
        {
            return NativeGetTempStepCount(_handle, out step) != 0;
        }

        public bool SetTempStepCount(byte step)
        {
            return NativeSetTempStepCount(_handle, step) != 0;
        }

        public bool GetTempSpeedMold(out byte mold)
        {
            return NativeGetTempSpeedMold(_handle, out mold) != 0;
        }

        public bool SetTempSpeedMold(byte mold)
        {
            return NativeSetTempSpeedMold(_handle, mold) != 0;
        }

        /// <summary>Read the start temperature at the given step index. / 读取指定段起始温度。</summary>
        public bool GetStartTempIndex(byte index, out float value)
        {
            return NativeGetStartTempIndex(_handle, index, out value) != 0;
        }

        public bool SetTempIndex(byte index, float value)
        {
            return NativeSetTempIndex(_handle, index, value) != 0;
        }

        public bool GetTempSpeedIndex(byte index, out float value)
        {
            return NativeGetTempSpeedIndex(_handle, index, out value) != 0;
        }

        public bool SetTempSpeedIndex(byte index, float value)
        {
            return NativeSetTempSpeedIndex(_handle, index, value) != 0;
        }

        public bool GetKeepTempTime(byte index, out int ms)
        {
            return NativeGetKeepTempTime(_handle, index, out ms) != 0;
        }

        public bool SetKeepTempTime(byte index, int ms)
        {
            return NativeSetKeepTempTime(_handle, index, ms) != 0;
        }

        public bool GetMaxWaterValue(out float value)
        {
            return NativeGetMaxWaterValue(_handle, out value) != 0;
        }

        public bool SetMaxWaterValue(float value)
        {
            return NativeSetMaxWaterValue(_handle, value) != 0;
        }

        public bool GetMinWaterValue(out float value)
        {
            return NativeGetMinWaterValue(_handle, out value) != 0;
        }

        public bool SetMinWaterValue(float value)
        {
            return NativeSetMinWaterValue(_handle, value) != 0;
        }

        // ==================== 混合模式单值 / 索引命令 ====================

        public bool GetMixStepCount(out byte step)
        {
            return NativeGetMixStepCount(_handle, out step) != 0;
        }

        public bool SetMixStepCount(byte step)
        {
            return NativeSetMixStepCount(_handle, step) != 0;
        }

        public bool GetMixMoldType(out byte type)
        {
            return NativeGetMixMoldType(_handle, out type) != 0;
        }

        public bool SetMixMoldType(byte type)
        {
            return NativeSetMixMoldType(_handle, type) != 0;
        }

        public bool GetMixRectangle(out float a, out float b)
        {
            return NativeGetMixRectangle(_handle, out a, out b) != 0;
        }

        public bool SetMixRectangle(float a, float b)
        {
            return NativeSetMixRectangle(_handle, a, b) != 0;
        }

        public bool GetMixCircle(out float diameter)
        {
            return NativeGetMixCircle(_handle, out diameter) != 0;
        }

        public bool SetMixCircle(float diameter)
        {
            return NativeSetMixCircle(_handle, diameter) != 0;
        }

        public bool GetMixIsPressIndex(byte index, out bool isPress)
        {
            bool succeeded = NativeGetMixIsPressIndex(_handle, index, out int raw) != 0;
            isPress = raw != 0;
            return succeeded;
        }

        public bool SetMixIsPressIndex(byte index, bool isPress)
        {
            return NativeSetMixIsPressIndex(_handle, index, isPress ? 1 : 0) != 0;
        }

        public bool GetMixPressIndex(byte index, out float value)
        {
            return NativeGetMixPressIndex(_handle, index, out value) != 0;
        }

        public bool SetMixPressIndex(byte index, float value)
        {
            return NativeSetMixPressIndex(_handle, index, value) != 0;
        }

        public bool GetMixAfterPressIndex(byte index, out float value)
        {
            return NativeGetMixAfterPressIndex(_handle, index, out value) != 0;
        }

        public bool SetMixAfterPressIndex(byte index, float value)
        {
            return NativeSetMixAfterPressIndex(_handle, index, value) != 0;
        }

        public bool GetMixIsHeatUpIndex(byte index, out bool isHeatUp)
        {
            bool succeeded = NativeGetMixIsHeatUpIndex(_handle, index, out int raw) != 0;
            isHeatUp = raw != 0;
            return succeeded;
        }

        public bool SetMixIsHeatUpIndex(byte index, bool isHeatUp)
        {
            return NativeSetMixIsHeatUpIndex(_handle, index, isHeatUp ? 1 : 0) != 0;
        }

        public bool GetMixHeatUpIndex(byte index, out float value)
        {
            return NativeGetMixHeatUpIndex(_handle, index, out value) != 0;
        }

        public bool SetMixHeatUpIndex(byte index, float value)
        {
            return NativeSetMixHeatUpIndex(_handle, index, value) != 0;
        }

        public bool GetMixIsHeatDownIndex(byte index, out bool isHeatDown)
        {
            bool succeeded = NativeGetMixIsHeatDownIndex(_handle, index, out int raw) != 0;
            isHeatDown = raw != 0;
            return succeeded;
        }

        public bool SetMixIsHeatDownIndex(byte index, bool isHeatDown)
        {
            return NativeSetMixIsHeatDownIndex(_handle, index, isHeatDown ? 1 : 0) != 0;
        }

        public bool GetMixHeatDownIndex(byte index, out float value)
        {
            return NativeGetMixHeatDownIndex(_handle, index, out value) != 0;
        }

        public bool SetMixHeatDownIndex(byte index, float value)
        {
            return NativeSetMixHeatDownIndex(_handle, index, value) != 0;
        }

        public bool GetMixKeepTime(byte index, out int ms)
        {
            return NativeGetMixKeepTime(_handle, index, out ms) != 0;
        }

        public bool SetMixKeepTime(byte index, int ms)
        {
            return NativeSetMixKeepTime(_handle, index, ms) != 0;
        }

        public void RegisterDataInterface(
            PressCReadOnlyDataCallback? onReadOnlyData = null,
            PressCRealTimeDataCallback? onRealTimeData = null,
            PressCPressDataCallback? onPressData = null,
            PressCTempDataCallback? onTempData = null,
            PressCMixDataCallback? onMixData = null,
            PressCErrorCallback? onError = null,
            IntPtr? userData = null,
            int intervalSeconds = 1)
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
                _onTempDataCallback = onTempData;
                _onMixDataCallback = onMixData;
                _onErrorCallback = onError;

                var state = new PressCDataCallbacks
                {
                    userData = userData ?? IntPtr.Zero,
                    onReadOnlyData = onReadOnlyData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onReadOnlyData),
                    onRealTimeData = onRealTimeData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onRealTimeData),
                    onPressData = onPressData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onPressData),
                    onTempData = onTempData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onTempData),
                    onMixData = onMixData == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onMixData),
                    onError = onError == null ? IntPtr.Zero : Marshal.GetFunctionPointerForDelegate(onError),
                };

                _callbackHandle = GCHandle.Alloc(state, GCHandleType.Normal);
                NativeRegisterDataInterface(_handle, ref state, intervalSeconds);
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
