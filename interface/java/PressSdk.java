/**
 * Java binding for the C API of the Press SDK.
 * Press SDK 的 Java 语言绑定。
 *
 * 设计目标：
 * 1. 保持和底层 cpress_* API 一致，避免对原生实现做不必要的封装；
 * 2. 通过 JNI 访问本地库，适合 Java / JVM 应用接入；
 * 3. 对句柄生命周期和参数输入做基本防护，减少使用端常见错误；
 * 4. 兼容 Windows / Linux / macOS 下的不同动态库命名规则。
 *
 * This class intentionally mirrors the native cpress_* API and provides a
 * small Java-friendly wrapper around the native handle. The wrapper keeps the
 * actual implementation opaque while validating basic input and cleanup.
 */
import java.util.HashSet;
import java.util.Set;

public final class PressSdk implements AutoCloseable {
    /**
     * Maximum number of pressure steps used by the SDK.
     * SDK 使用的最大压力步数。
     */
    public static final int MAX_P_STEP = 30;

    /**
     * Port type constants defined by the underlying native API.
     * 底层原生 API 定义的端口类型常量。
     */
    public static final class PortType {
        public static final int SerialPortType = 0;
        public static final int TcpSocketPortType = 1;

        private PortType() {
            // Prevent accidental instantiation.
            // 防止被误实例化。
        }
    }

    /**
     * Read-only data structure returned by the native SDK.
     * 设备只读信息结构体，映射自底层原生 ReadOnlyData。
     */
    public static final class ReadOnlyData {
        public String m_NameZH = "";
        public String m_NameEN = "";
        public String m_Type = "";
        public String m_SerialNumber = "";
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

    /**
     * Pressure configuration structure used for write/read operations.
     * 压力配置结构体，用于读写压力参数。
     */
    public static final class PressData {
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
        public float[] m_SetPValue = new float[MAX_P_STEP];
        public float[] m_AfterValue = new float[MAX_P_STEP];
        public int[] m_KPTime = new int[MAX_P_STEP];
    }

    /**
     * Runtime machine state returned by the device.
     * 设备实时状态结构体。
     */
    public static final class RealTimeData {
        public byte m_ModelState;
        public byte m_PressState;
        public byte m_CPStep;
        public float m_PressValue;
        public long m_PTime;
        public byte m_PdChanged;
    }

    /**
     * Callback interface for receiving asynchronous data push from the native SDK.
     * 用于接收底层 SDK 异步数据推送的回调接口。
     *
     * <p>All four methods are default implementations that do nothing, so callers only
     * need to override the callbacks they care about.</p>
     * <p>四个方法都提供了空的 default 实现，调用方只需覆写自己关心的回调即可。</p>
     *
     * <p><b>Threading note / 线程说明</b>: All callbacks are invoked on a native SDK worker
     * thread. If you need to update the UI, you must dispatch the call to your application's
     * UI thread yourself.</p>
     * <p>所有回调都在 SDK 工作线程中触发，如需更新 UI，请自行切换线程。</p>
     */
    public interface DataCallbacks {
        /**
         * Called when read-only device information is decoded asynchronously.
         * 只读设备信息异步解码完成时触发。
         *
         * @param errorCode   0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param registerNo  Device hardware register number. / 设备硬件注册号
         * @param readOnlyData Decoded read-only data snapshot. / 解码后的只读数据快照
         */
        default void onReadOnlyData(int errorCode, long registerNo, ReadOnlyData readOnlyData) {}

        /**
         * Called when the native SDK pushes a new real-time machine state.
         * 底层 SDK 推送新的实时机器状态时触发。
         *
         * @param errorCode    0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param realTimeData Decoded real-time state snapshot. / 解码后的实时状态快照
         */
        default void onRealTimeData(int errorCode, RealTimeData realTimeData) {}

        /**
         * Called when pressure configuration data is decoded asynchronously.
         * 压力配置数据异步解码完成时触发。
         *
         * @param errorCode 0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param pressData Decoded pressure configuration snapshot. / 解码后的压力配置快照
         */
        default void onPressData(int errorCode, PressData pressData) {}

        /**
         * Called when the native SDK receives an error response from the device.
         * 底层 SDK 收到设备返回的错误响应时触发。
         *
         * @param cmdCode  Command code that produced the error. / 触发错误的命令码
         * @param response Raw response bytes from the device. / 设备返回的原始响应字节
         */
        default void onError(int cmdCode, byte[] response) {}
    }

    /**
     * 1) 问题：旧实现只在当前工作目录尝试加载动态库，很多情况下库并不在这里。
     *    解决：优先扫描 java.library.path、常见 install/bin、build 产物目录，
     *    同时保留 System.loadLibrary("press") 的回退。
     * 2) 问题：旧实现没有对资源释放和空端口参数做保护。
     *    解决：增加实例状态、close() 幂等化和参数校验。
     */
    private static final String NATIVE_LIBRARY_NAME = resolveNativeLibraryName();

    static {
        loadNativeLibrary();
    }

    private static void loadNativeLibrary() {
        if (NATIVE_LIBRARY_NAME == null || NATIVE_LIBRARY_NAME.isEmpty()) {
            throw new UnsatisfiedLinkError("No native Press SDK library was found in the current environment.");
        }

        try {
            System.load(NATIVE_LIBRARY_NAME);
            return;
        } catch (UnsatisfiedLinkError directLoadError) {
            try {
                System.loadLibrary("press");
                return;
            } catch (UnsatisfiedLinkError fallbackError) {
                String message = "Failed to load Press SDK native library. Tried: "
                        + NATIVE_LIBRARY_NAME + " and System.loadLibrary(\"press\"). "
                        + directLoadError.getMessage();
                System.err.println(message);
                throw new UnsatisfiedLinkError(fallbackError.getMessage() + " | " + message);
            }
        }
    }

    private static String resolveNativeLibraryName() {
        String osName = System.getProperty("os.name", "").toLowerCase();
        Set<String> seen = new HashSet<>();

        String[] candidates = {
            osName.contains("windows") ? "press.dll" : "libpress.so",
            osName.contains("mac") ? "libpress.dylib" : "libpress.so",
            osName.contains("windows") ? "pressd.dll" : "libpress.dylib",
            "press",
            "libpress"
        };

        String[] roots = {
            System.getProperty("user.dir"),
            System.getProperty("java.library.path"),
            new java.io.File("install", "bin").getAbsolutePath(),
            new java.io.File(new java.io.File("build", "press"), "Release").getAbsolutePath(),
            new java.io.File(new java.io.File("build", "press"), "Debug").getAbsolutePath(),
            new java.io.File(new java.io.File("build_jni_check", "press"), "Release").getAbsolutePath(),
            new java.io.File(new java.io.File("build_jni_check", "press"), "Debug").getAbsolutePath(),
            new java.io.File(".").getAbsolutePath()
        };

        for (String root : roots) {
            if (root == null || root.trim().isEmpty()) {
                continue;
            }

            for (String entry : root.split(java.io.File.pathSeparator)) {
                if (entry == null || entry.trim().isEmpty()) {
                    continue;
                }

                java.io.File directory = new java.io.File(entry);
                if (!directory.exists() || !directory.isDirectory()) {
                    continue;
                }

                for (String candidate : candidates) {
                    if (candidate == null || candidate.trim().isEmpty()) {
                        continue;
                    }

                    java.io.File libraryFile = new java.io.File(directory, candidate);
                    if (libraryFile.exists() && libraryFile.isFile()) {
                        String absolute = libraryFile.getAbsolutePath();
                        if (seen.add(absolute)) {
                            return absolute;
                        }
                    }
                }
            }
        }

        return "press";
    }

    /**
     * Native handle to the underlying SDK context.
     * 底层 SDK 上下文的原生句柄。
     */
    private final long handle;
    private volatile boolean closed;

    public PressSdk() {
        this.handle = press_create();
        if (this.handle == 0L) {
            throw new IllegalStateException("Failed to create Press SDK handle.");
        }
    }

    /**
     * Ensure the instance is still open before invoking native calls.
     * 在调用本地方法前，确保实例尚未关闭。
     */
    private void ensureOpen() {
        if (closed) {
            throw new IllegalStateException("Press SDK instance is already closed.");
        }
    }

    /**
     * Validate common port name input.
     * 校验常见端口名输入，避免传入 null 或空字符串。
     */
    private static void validatePortName(String portName) {
        if (portName == null || portName.trim().isEmpty()) {
            throw new IllegalArgumentException("portName must not be null or empty.");
        }
    }

    public boolean connect(String portName, int portType) {
        ensureOpen();
        validatePortName(portName);
        return press_connect(this.handle, portName, portType) != 0;
    }

    public void disconnect() {
        if (closed) {
            return;
        }
        press_disconnect(this.handle);
    }

    /**
     * Release the native handle.
     * 释放底层原生句柄，保证资源正确回收。
     */
    @Override
    public void close() {
        if (closed || this.handle == 0L) {
            return;
        }

        try {
            disconnect();
        } finally {
            press_destroy(this.handle);
            closed = true;
        }
    }

    public boolean isConnected() {
        ensureOpen();
        return press_is_connected(this.handle) != 0;
    }

    public long getMachineRegisterNo() {
        ensureOpen();
        return press_get_machine_register_no(this.handle);
    }

    public String getLastErrorInfo() {
        ensureOpen();
        return press_get_last_error_info(this.handle);
    }

    public void run() {
        ensureOpen();
        press_run(this.handle);
    }

    public void stop() {
        ensureOpen();
        press_stop(this.handle);
    }

    public boolean isRunning() {
        ensureOpen();
        return press_is_running(this.handle) != 0;
    }

    public boolean getReadOnlyData(ReadOnlyData data, boolean isCompressed) {
        ensureOpen();
        if (data == null) {
            throw new IllegalArgumentException("data must not be null.");
        }
        return press_get_read_only_data(this.handle, data, isCompressed ? 1 : 0) != 0;
    }

    public boolean getPressData(PressData data, boolean isCompressed) {
        ensureOpen();
        if (data == null) {
            throw new IllegalArgumentException("data must not be null.");
        }
        return press_get_press_data(this.handle, data, isCompressed ? 1 : 0) != 0;
    }

    public boolean setPressData(PressData data, boolean isCompressed) {
        ensureOpen();
        if (data == null) {
            throw new IllegalArgumentException("data must not be null.");
        }
        return press_set_press_data(this.handle, data, isCompressed ? 1 : 0) != 0;
    }

    public boolean getRealTimeData(RealTimeData data, boolean isCompressed) {
        ensureOpen();
        if (data == null) {
            throw new IllegalArgumentException("data must not be null.");
        }
        return press_get_real_time_data(this.handle, data, isCompressed ? 1 : 0) != 0;
    }

    public boolean setPressing(boolean isPressing) {
        ensureOpen();
        return press_set_pressing(this.handle, isPressing ? 1 : 0) != 0;
    }

    public boolean setDemolding(boolean isDemolding) {
        ensureOpen();
        return press_set_demolding(this.handle, isDemolding ? 1 : 0) != 0;
    }

    /**
     * Register a callback handler to receive asynchronous data push from the native SDK.
     * 注册回调处理器，用于接收底层 SDK 的异步数据推送。
     *
     * <p>Passing the same callbacks instance again will replace the previous registration.
     * Pass {@code null} to effectively unregister (same as calling {@link #unregisterDataCallbacks()}).</p>
     * <p>再次传入同一个 callbacks 实例会替换之前的注册；传入 {@code null} 等效于
     * 调用 {@link #unregisterDataCallbacks()} 注销回调。</p>
     *
     * <p><b>Threading note / 线程说明</b>: Callbacks are invoked on a native SDK worker thread.</p>
     */
    public void registerDataCallbacks(DataCallbacks callbacks) {
        ensureOpen();
        press_register_data_interface(this.handle, this, callbacks);
    }

    /**
     * Unregister any previously registered callback handler.
     * 注销之前注册的回调处理器。
     */
    public void unregisterDataCallbacks() {
        if (closed || this.handle == 0L) {
            return;
        }
        press_unregister_data_interface(this.handle);
    }

    public static String version() {
        return press_version();
    }

    /**
     * Native JNI entry points.
     * JNIEnv 绑定方法：这些方法在原生库中由 JNI 实现。
     */
    public static native long press_create();
    public static native void press_destroy(long handle);
    public static native int press_connect(long handle, String portName, int portType);
    public static native void press_disconnect(long handle);
    public static native int press_is_connected(long handle);
    public static native long press_get_machine_register_no(long handle);
    public static native String press_get_last_error_info(long handle);
    public static native void press_run(long handle);
    public static native void press_stop(long handle);
    public static native int press_is_running(long handle);
    public static native int press_get_read_only_data(long handle, ReadOnlyData data, int isCompressed);
    public static native int press_get_press_data(long handle, PressData data, int isCompressed);
    public static native int press_set_press_data(long handle, PressData data, int isCompressed);
    public static native int press_get_real_time_data(long handle, RealTimeData data, int isCompressed);
    public static native int press_set_pressing(long handle, int isPressing);
    public static native int press_set_demolding(long handle, int isDemolding);
    public static native void press_register_data_interface(long handle, PressSdk self, DataCallbacks callbacks);
    public static native void press_unregister_data_interface(long handle);
    public static native String press_version();

    public static void main(String[] args) {
        try (PressSdk sdk = new PressSdk()) {
            System.out.println("Press SDK Java binding initialized. Native version: " + version());
        }
    }
}