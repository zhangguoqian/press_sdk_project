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
         * Called when read-only device information (ROD JSON) is fetched asynchronously.
         * 只读设备信息（ROD JSON）异步获取完成时触发。
         *
         * @param errorCode   0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param registerNo  Device hardware register number. / 设备硬件注册号
         * @param json        Raw ROD JSON payload; empty on failure. / 原始 ROD JSON 数据，失败时为空字符串
         */
        default void onReadOnlyData(int errorCode, long registerNo, String json) {}

        /**
         * Called when the native SDK pushes a new real-time machine state (RT JSON).
         * 底层 SDK 推送新的实时机器状态（RT JSON）时触发。
         *
         * @param errorCode 0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param json      Raw RT JSON payload; empty on failure. / 原始 RT JSON 数据，失败时为空字符串
         */
        default void onRealTimeData(int errorCode, String json) {}

        /**
         * Called when pressure configuration data (PD JSON) is fetched asynchronously.
         * 压力配置数据（PD JSON）异步获取完成时触发。
         *
         * @param errorCode 0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param json      Raw PD JSON payload; empty on failure. / 原始 PD JSON 数据，失败时为空字符串
         */
        default void onPressData(int errorCode, String json) {}

        /**
         * Called when temperature data (TD JSON) is fetched asynchronously.
         * 温度数据（TD JSON）异步获取完成时触发。
         *
         * @param errorCode 0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param json      Raw TD JSON payload; empty on failure. / 原始 TD JSON 数据，失败时为空字符串
         */
        default void onTempData(int errorCode, String json) {}

        /**
         * Called when mix data (MD JSON) is fetched asynchronously.
         * 混合数据（MD JSON）异步获取完成时触发。
         *
         * @param errorCode 0 means success; non-zero means an error occurred. / 0 成功，非 0 表示出错
         * @param json      Raw MD JSON payload; empty on failure. / 原始 MD JSON 数据，失败时为空字符串
         */
        default void onMixData(int errorCode, String json) {}

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

    /** Read read-only data as ROD JSON; null on failure. / 以 ROD JSON 读取只读数据，失败返回 null。 */
    public String getReadOnlyDataJson(boolean isCompressed) {
        ensureOpen();
        return press_get_read_only_data_json(this.handle, isCompressed ? 1 : 0);
    }

    /** Read pressure parameters as PD JSON; null on failure. / 以 PD JSON 读取压力参数，失败返回 null。 */
    public String getPressDataJson(boolean isCompressed) {
        ensureOpen();
        return press_get_press_data_json(this.handle, isCompressed ? 1 : 0);
    }

    /** Write pressure parameters from PD JSON. / 以 PD JSON 写入压力参数。 */
    public boolean setPressDataJson(String json, boolean isCompressed) {
        ensureOpen();
        if (json == null) {
            throw new IllegalArgumentException("json must not be null.");
        }
        return press_set_press_data_json(this.handle, json, isCompressed ? 1 : 0) != 0;
    }

    /** Read live machine state as RT JSON; null on failure. / 以 RT JSON 读取实时状态，失败返回 null。 */
    public String getRealTimeDataJson(boolean isCompressed) {
        ensureOpen();
        return press_get_real_time_data_json(this.handle, isCompressed ? 1 : 0);
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
     * Read the device serial number (CMDID_SERIAL_NUMBER 0x0003).
     * On success the value is also cached into the native ReadOnlyData.m_SerialNumber.
     * 读取设备序列号；成功后该值同时缓存到原生 ReadOnlyData.m_SerialNumber。
     *
     * @return serial number string, empty on failure / 序列号字符串，失败返回空串
     */
    public String getSerialNumber() {
        ensureOpen();
        return press_get_serial_number(this.handle);
    }

    /**
     * Set the per-request timeout in milliseconds (default 300ms, max 60000ms).
     * 设置单次请求超时（毫秒），默认 300ms，上限 60000ms。
     *
     * @param ms timeout in ms; 0 restores the default / 超时毫秒数，0 恢复默认
     */
    public void setRequestTimeout(int ms) {
        ensureOpen();
        press_set_request_timeout(this.handle, ms);
    }

    // ==================== 压力单值 / 索引命令 ====================

    /** Read pressure step count; null on failure. / 读取压力段数量，失败返回 null。 */
    public Integer getPressStepCount() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_press_step_count(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write pressure step count. / 写入压力段数量。 */
    public boolean setPressStepCount(int step) {
        ensureOpen();
        return press_set_press_step_count(this.handle, step) != 0;
    }

    /** Read pressure calibration value; null on failure. / 读取压力校准值，失败返回 null。 */
    public Float getCalibrationValue() {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_calibration_value(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write pressure calibration value. / 写入压力校准值。 */
    public boolean setCalibrationValue(float value) {
        ensureOpen();
        return press_set_calibration_value(this.handle, value) != 0;
    }

    /** Read mold type; null on failure. / 读取模具类型，失败返回 null。 */
    public Integer getModeType() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mode_type(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write mold type. / 写入模具类型。 */
    public boolean setModeType(int type) {
        ensureOpen();
        return press_set_mode_type(this.handle, type) != 0;
    }

    /** Read circular mold diameter; null on failure. / 读取圆形容器直径，失败返回 null。 */
    public Float getCircle() {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_circle(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write circular mold diameter. / 写入圆形容器直径。 */
    public boolean setCircle(float diameter) {
        ensureOpen();
        return press_set_circle(this.handle, diameter) != 0;
    }

    /** Read rectangular mold A/B dimensions as [a, b]; null on failure. / 读取矩形容器 A/B 尺寸，失败返回 null。 */
    public float[] getRectangle() {
        ensureOpen();
        float[] out = new float[2];
        if (press_get_rectangle(this.handle, out) == 0) {
            return null;
        }
        return out;
    }

    /** Write rectangular mold A/B dimensions. / 写入矩形容器 A/B 尺寸。 */
    public boolean setRectangle(float a, float b) {
        ensureOpen();
        return press_set_rectangle(this.handle, a, b) != 0;
    }

    /** Read air (exhaust) time in seconds; null on failure. / 读取排气时间（秒），失败返回 null。 */
    public Integer getAirTime() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_air_time(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write air (exhaust) time in seconds. / 写入排气时间（秒）。 */
    public boolean setAirTime(int seconds) {
        ensureOpen();
        return press_set_air_time(this.handle, seconds) != 0;
    }

    /** Read the target pressure of one step; null on failure. / 读取指定压力段的设定压力，失败返回 null。 */
    public Float getPressValueIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_press_value_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the target pressure of one step. / 写入指定压力段的设定压力。 */
    public boolean setPressValueIndex(int index, float value) {
        ensureOpen();
        return press_set_press_value_index(this.handle, index, value) != 0;
    }

    /** Read the hold pressure of one step; null on failure. / 读取指定压力段的保压值，失败返回 null。 */
    public Float getAfterValueIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_after_value_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the hold pressure of one step. / 写入指定压力段的保压值。 */
    public boolean setAfterValueIndex(int index, float value) {
        ensureOpen();
        return press_set_after_value_index(this.handle, index, value) != 0;
    }

    /** Read the keep-pressure time of one step in milliseconds; null on failure. / 读取指定压力段的保压时间（毫秒），失败返回 null。 */
    public Integer getKeepPressTime(int index) {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_keep_press_time(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the keep-pressure time of one step in milliseconds. / 写入指定压力段的保压时间（毫秒）。 */
    public boolean setKeepPressTime(int index, int ms) {
        ensureOpen();
        return press_set_keep_press_time(this.handle, index, ms) != 0;
    }

    // ==================== 温度单值 / 索引命令 ====================

    /** Read temperature step count; null on failure. / 读取温度段数量，失败返回 null。 */
    public Integer getTempStepCount() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_temp_step_count(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write temperature step count. / 写入温度段数量。 */
    public boolean setTempStepCount(int step) {
        ensureOpen();
        return press_set_temp_step_count(this.handle, step) != 0;
    }

    /** Read temperature speed mold; null on failure. / 读取温度速度模式，失败返回 null。 */
    public Integer getTempSpeedMold() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_temp_speed_mold(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write temperature speed mold. / 写入温度速度模式。 */
    public boolean setTempSpeedMold(int mold) {
        ensureOpen();
        return press_set_temp_speed_mold(this.handle, mold) != 0;
    }

    /** Read the temperature value of one step; null on failure. / 读取指定温度段的温度值，失败返回 null。 */
    public Float getStartTempIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_start_temp_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the temperature value of one step. / 写入指定温度段的温度值。 */
    public boolean setTempIndex(int index, float value) {
        ensureOpen();
        return press_set_temp_index(this.handle, index, value) != 0;
    }

    /** Read the speed value of one step; null on failure. / 读取指定温度段的速度值，失败返回 null。 */
    public Float getTempSpeedIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_temp_speed_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the speed value of one step. / 写入指定温度段的速度值。 */
    public boolean setTempSpeedIndex(int index, float value) {
        ensureOpen();
        return press_set_temp_speed_index(this.handle, index, value) != 0;
    }

    /** Read the keep-temperature time of one step in milliseconds; null on failure. / 读取指定温度段的保温时间（毫秒），失败返回 null。 */
    public Integer getKeepTempTime(int index) {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_keep_temp_time(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the keep-temperature time of one step in milliseconds. / 写入指定温度段的保温时间（毫秒）。 */
    public boolean setKeepTempTime(int index, int ms) {
        ensureOpen();
        return press_set_keep_temp_time(this.handle, index, ms) != 0;
    }

    /** Read max water value; null on failure. / 读取最大水冷，失败返回 null。 */
    public Float getMaxWaterValue() {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_max_water_value(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write max water value. / 写入最大水冷。 */
    public boolean setMaxWaterValue(float value) {
        ensureOpen();
        return press_set_max_water_value(this.handle, value) != 0;
    }

    /** Read min water value; null on failure. / 读取最小水冷，失败返回 null。 */
    public Float getMinWaterValue() {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_min_water_value(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write min water value. / 写入最小水冷。 */
    public boolean setMinWaterValue(float value) {
        ensureOpen();
        return press_set_min_water_value(this.handle, value) != 0;
    }

    // ==================== 混合模式单值 / 索引命令 ====================

    /** Read mixed-mode step count; null on failure. / 读取混合模式段数量，失败返回 null。 */
    public Integer getMixStepCount() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mix_step_count(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write mixed-mode step count. / 写入混合模式段数量。 */
    public boolean setMixStepCount(int step) {
        ensureOpen();
        return press_set_mix_step_count(this.handle, step) != 0;
    }

    /** Read mixed-mode mold type; null on failure. / 读取混合模式模具类型，失败返回 null。 */
    public Integer getMixMoldType() {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mix_mold_type(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write mixed-mode mold type. / 写入混合模式模具类型。 */
    public boolean setMixMoldType(int type) {
        ensureOpen();
        return press_set_mix_mold_type(this.handle, type) != 0;
    }

    /** Read mixed-mode rectangle A/B dimensions as [a, b]; null on failure. / 读取混合模式矩形尺寸 A/B，失败返回 null。 */
    public float[] getMixRectangle() {
        ensureOpen();
        float[] out = new float[2];
        if (press_get_mix_rectangle(this.handle, out) == 0) {
            return null;
        }
        return out;
    }

    /** Write mixed-mode rectangle A/B dimensions. / 写入混合模式矩形尺寸 A/B。 */
    public boolean setMixRectangle(float a, float b) {
        ensureOpen();
        return press_set_mix_rectangle(this.handle, a, b) != 0;
    }

    /** Read mixed-mode circle diameter; null on failure. / 读取混合模式圆形容器直径，失败返回 null。 */
    public Float getMixCircle() {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_mix_circle(this.handle, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write mixed-mode circle diameter. / 写入混合模式圆形容器直径。 */
    public boolean setMixCircle(float diameter) {
        ensureOpen();
        return press_set_mix_circle(this.handle, diameter) != 0;
    }

    /** Read per-step is-press flag; null on failure. / 读取指定段是否加压，失败返回 null。 */
    public Boolean getMixIsPressIndex(int index) {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mix_is_press_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0] != 0;
    }

    /** Write per-step is-press flag. / 写入指定段是否加压。 */
    public boolean setMixIsPressIndex(int index, boolean isPress) {
        ensureOpen();
        return press_set_mix_is_press_index(this.handle, index, isPress ? 1 : 0) != 0;
    }

    /** Read per-step pressure value; null on failure. / 读取指定段压力值，失败返回 null。 */
    public Float getMixPressIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_mix_press_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write per-step pressure value. / 写入指定段压力值。 */
    public boolean setMixPressIndex(int index, float value) {
        ensureOpen();
        return press_set_mix_press_index(this.handle, index, value) != 0;
    }

    /** Read per-step hold-pressure value; null on failure. / 读取指定段保压值，失败返回 null。 */
    public Float getMixAfterPressIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_mix_after_press_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write per-step hold-pressure value. / 写入指定段保压值。 */
    public boolean setMixAfterPressIndex(int index, float value) {
        ensureOpen();
        return press_set_mix_after_press_index(this.handle, index, value) != 0;
    }

    /** Read per-step heat-up flag; null on failure. / 读取指定段是否加热，失败返回 null。 */
    public Boolean getMixIsHeatUpIndex(int index) {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mix_is_heat_up_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0] != 0;
    }

    /** Write per-step heat-up flag. / 写入指定段是否加热。 */
    public boolean setMixIsHeatUpIndex(int index, boolean isHeatUp) {
        ensureOpen();
        return press_set_mix_is_heat_up_index(this.handle, index, isHeatUp ? 1 : 0) != 0;
    }

    /** Read per-step heat-up temperature; null on failure. / 读取指定段加热温度，失败返回 null。 */
    public Float getMixHeatUpIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_mix_heat_up_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write per-step heat-up temperature. / 写入指定段加热温度。 */
    public boolean setMixHeatUpIndex(int index, float value) {
        ensureOpen();
        return press_set_mix_heat_up_index(this.handle, index, value) != 0;
    }

    /** Read per-step heat-down flag; null on failure. / 读取指定段是否冷却，失败返回 null。 */
    public Boolean getMixIsHeatDownIndex(int index) {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mix_is_heat_down_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0] != 0;
    }

    /** Write per-step heat-down flag. / 写入指定段是否冷却。 */
    public boolean setMixIsHeatDownIndex(int index, boolean isHeatDown) {
        ensureOpen();
        return press_set_mix_is_heat_down_index(this.handle, index, isHeatDown ? 1 : 0) != 0;
    }

    /** Read per-step heat-down temperature; null on failure. / 读取指定段冷却温度，失败返回 null。 */
    public Float getMixHeatDownIndex(int index) {
        ensureOpen();
        float[] out = new float[1];
        if (press_get_mix_heat_down_index(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write per-step heat-down temperature. / 写入指定段冷却温度。 */
    public boolean setMixHeatDownIndex(int index, float value) {
        ensureOpen();
        return press_set_mix_heat_down_index(this.handle, index, value) != 0;
    }

    /** Read the keep time of one mixed-mode step in milliseconds; null on failure. / 读取指定混合段的保持时间（毫秒），失败返回 null。 */
    public Integer getMixKeepTime(int index) {
        ensureOpen();
        int[] out = new int[1];
        if (press_get_mix_keep_time(this.handle, index, out) == 0) {
            return null;
        }
        return out[0];
    }

    /** Write the keep time of one mixed-mode step in milliseconds. / 写入指定混合段的保持时间（毫秒）。 */
    public boolean setMixKeepTime(int index, int ms) {
        ensureOpen();
        return press_set_mix_keep_time(this.handle, index, ms) != 0;
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
     * @param callbacks       callback handler or null to unregister / 回调处理器，null 表示注销
     * @param intervalSeconds periodic RealTimeData polling interval in seconds;
     *                        &lt;= 0 disables the timer / 实时数据定时采集周期（秒），
     *                        小于等于 0 时不启动定时采集
     * <p><b>Threading note / 线程说明</b>: Callbacks are invoked on a native SDK worker thread.</p>
     */
    public void registerDataCallbacks(DataCallbacks callbacks, int intervalSeconds) {
        ensureOpen();
        press_register_data_interface(this.handle, this, callbacks, intervalSeconds);
    }

    /**
     * Register a callback handler with the default 1-second polling interval.
     * 以默认 1 秒采集周期注册回调处理器。
     */
    public void registerDataCallbacks(DataCallbacks callbacks) {
        registerDataCallbacks(callbacks, 1);
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
    public static native String press_get_read_only_data_json(long handle, int isCompressed);
    public static native String press_get_press_data_json(long handle, int isCompressed);
    public static native int press_set_press_data_json(long handle, String json, int isCompressed);
    public static native String press_get_real_time_data_json(long handle, int isCompressed);
    public static native int press_set_pressing(long handle, int isPressing);
    public static native int press_set_demolding(long handle, int isDemolding);
    public static native void press_register_data_interface(long handle, PressSdk self, DataCallbacks callbacks, int intervalSeconds);
    public static native void press_unregister_data_interface(long handle);
    public static native String press_version();
    public static native String press_get_serial_number(long handle);
    public static native void press_set_request_timeout(long handle, int ms);

    // 压力单值 / 索引命令 / Pressure single-value / indexed commands
    public static native int press_get_press_step_count(long handle, int[] out);
    public static native int press_set_press_step_count(long handle, int step);
    public static native int press_get_calibration_value(long handle, float[] out);
    public static native int press_set_calibration_value(long handle, float value);
    public static native int press_get_mode_type(long handle, int[] out);
    public static native int press_set_mode_type(long handle, int type);
    public static native int press_get_circle(long handle, float[] out);
    public static native int press_set_circle(long handle, float diameter);
    public static native int press_get_rectangle(long handle, float[] out);
    public static native int press_set_rectangle(long handle, float a, float b);
    public static native int press_get_air_time(long handle, int[] out);
    public static native int press_set_air_time(long handle, int seconds);
    public static native int press_get_press_value_index(long handle, int index, float[] out);
    public static native int press_set_press_value_index(long handle, int index, float value);
    public static native int press_get_after_value_index(long handle, int index, float[] out);
    public static native int press_set_after_value_index(long handle, int index, float value);
    public static native int press_get_keep_press_time(long handle, int index, int[] out);
    public static native int press_set_keep_press_time(long handle, int index, int ms);

    // 温度单值 / 索引命令 / Temperature single-value / indexed commands
    public static native int press_get_temp_step_count(long handle, int[] out);
    public static native int press_set_temp_step_count(long handle, int step);
    public static native int press_get_temp_speed_mold(long handle, int[] out);
    public static native int press_set_temp_speed_mold(long handle, int mold);
    public static native int press_get_start_temp_index(long handle, int index, float[] out);
    public static native int press_set_temp_index(long handle, int index, float value);
    public static native int press_get_temp_speed_index(long handle, int index, float[] out);
    public static native int press_set_temp_speed_index(long handle, int index, float value);
    public static native int press_get_keep_temp_time(long handle, int index, int[] out);
    public static native int press_set_keep_temp_time(long handle, int index, int ms);
    public static native int press_get_max_water_value(long handle, float[] out);
    public static native int press_set_max_water_value(long handle, float value);
    public static native int press_get_min_water_value(long handle, float[] out);
    public static native int press_set_min_water_value(long handle, float value);

    // 混合模式单值 / 索引命令 / Mixed-mode single-value / indexed commands
    public static native int press_get_mix_step_count(long handle, int[] out);
    public static native int press_set_mix_step_count(long handle, int step);
    public static native int press_get_mix_mold_type(long handle, int[] out);
    public static native int press_set_mix_mold_type(long handle, int type);
    public static native int press_get_mix_rectangle(long handle, float[] out);
    public static native int press_set_mix_rectangle(long handle, float a, float b);
    public static native int press_get_mix_circle(long handle, float[] out);
    public static native int press_set_mix_circle(long handle, float diameter);
    public static native int press_get_mix_is_press_index(long handle, int index, int[] out);
    public static native int press_set_mix_is_press_index(long handle, int index, int isPress);
    public static native int press_get_mix_press_index(long handle, int index, float[] out);
    public static native int press_set_mix_press_index(long handle, int index, float value);
    public static native int press_get_mix_after_press_index(long handle, int index, float[] out);
    public static native int press_set_mix_after_press_index(long handle, int index, float value);
    public static native int press_get_mix_is_heat_up_index(long handle, int index, int[] out);
    public static native int press_set_mix_is_heat_up_index(long handle, int index, int isHeatUp);
    public static native int press_get_mix_heat_up_index(long handle, int index, float[] out);
    public static native int press_set_mix_heat_up_index(long handle, int index, float value);
    public static native int press_get_mix_is_heat_down_index(long handle, int index, int[] out);
    public static native int press_set_mix_is_heat_down_index(long handle, int index, int isHeatDown);
    public static native int press_get_mix_heat_down_index(long handle, int index, float[] out);
    public static native int press_set_mix_heat_down_index(long handle, int index, float value);
    public static native int press_get_mix_keep_time(long handle, int index, int[] out);
    public static native int press_set_mix_keep_time(long handle, int index, int ms);

    public static void main(String[] args) {
        try (PressSdk sdk = new PressSdk()) {
            System.out.println("Press SDK Java binding initialized. Native version: " + version());
        }
    }
}