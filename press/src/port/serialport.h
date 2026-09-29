#pragma once

#include "portbase.h"

#include <cstdint>
#include <string>
#include <vector>

#ifdef _WIN32
#   include <windows.h>
#endif

/**
 * @brief Serial port device information structure — 串口设备信息结构体
 *
 * Describes basic information about an available serial port device.
 * Returned by SerialPort::listAvailablePorts().
 * 用于描述单个可用串口设备的基本信息，由 SerialPort::listAvailablePorts() 返回。
 */
struct SerialPortInfo {
    std::string name;        ///< Serial port name / device path, e.g. "COM3" or "/dev/ttyUSB0" — 串口名称 / 设备路径，如 "COM3" 或 "/dev/ttyUSB0"
    std::string description; ///< Device description / friendly name; may be empty across platforms — 设备描述 / 友好名称，跨平台可能为空

    SerialPortInfo() = default;
    SerialPortInfo(std::string n, std::string d = {})
        : name(std::move(n)), description(std::move(d)) {}

    /**
     * @brief Check whether this serial port is currently occupied by another process
     *        检查该串口当前是否被其他进程占用
     *        Windows: attempts exclusive open; failure means occupied
     *        Windows: 尝试以独占方式打开，失败即为被占用
     *        POSIX:    attempts to acquire an exclusive file lock (flock); failure means occupied
     *        POSIX:    尝试获取文件独占锁（flock），失败即为被占用
     * @return true   Port is occupied by another process — 串口被其他进程占用
     * @return false  Port is idle or device does not exist — 串口空闲或设备不存在
     */
    bool isBusy() const;
};

/**
 * @brief Serial port communication wrapper — 串口通信封装类
 *
 * Provides cross-platform (Windows / Linux / macOS / Android) serial port communication
 * through the unified PortBase interface.
 * 提供跨平台（Windows / Linux / macOS / Android）的串口通信能力，
 * 通过继承 PortBase 统一通信接口。
 *
 * Usage example:
 * @code
 *   SerialPort sp;
 *   sp.setPortName("COM3");
 *   sp.setReadTimeout(true, 500);
 *   sp.setWriteTimeout(true, 0);
 *   if (sp.open()) {
 *       sp.send({0xAA, 0xBB});
 *       auto data = sp.receive(64);
 *       sp.close();
 *   }
 * @endcode
 */
class SerialPort final : public PortBase {
public:
    using PortBase::send;
    using PortBase::receive;
    using PortBase::write;
    using PortBase::read;

    /** @brief Serial port baud rate — 串口波特率 */
    enum class BaudRate {
        B1200    = 1200,
        B2400    = 2400,
        B4800    = 4800,
        B9600    = 9600,
        B19200   = 19200,
        B38400   = 38400,
        B57600   = 57600,
        B115200  = 115200,
        B230400  = 230400,
        B460800  = 460800,
        B921600  = 921600,
        B250000  = 250000,
        B500000  = 500000,
        B1000000 = 1000000,
        B1500000 = 1500000,
        B2000000 = 2000000,
        B3000000 = 3000000,
    };

    /** @brief Parity type — 校验位类型 */
    enum class Parity  { None, Even, Odd };

    /** @brief Stop bits — 停止位数量 */
    enum class StopBits { One, Two };

    /**
     * @brief Get list of available serial ports on the current system (static function, no instance needed)
     *        获取当前系统中可用的串口列表（静态函数，无需实例化）
     *        Windows: auto-scans COM1 ~ COM256, attempts to read friendly names
     *        Windows: 自动扫描 COM1 ~ COM256，尝试读取友好名称
     *        Linux:   scans /dev/ttyS*, /dev/ttyUSB*, /dev/ttyACM*, /dev/ttyAMA*
     *        macOS:   scans /dev/cu.*, /dev/tty.*
     *        Android: scans /dev/ttyHS*, /dev/ttyMT*, /dev/ttyGS* etc. (device-specific paths)
     * @return Array of SerialPortInfo; empty array when no ports available
     *         SerialPortInfo 数组，无可用串口时返回空数组
     */
    static std::vector<SerialPortInfo> listAvailablePorts();

    /** @brief Default constructor; port is COM1 / /dev/ttyUSB0, default 115200 8N1, read non-blocking, write permanently blocking
     *         默认构造，串口为 COM1 / /dev/ttyUSB0，默认 115200 8N1，读非阻塞，写永久阻塞 */
    SerialPort();

    /** @brief Destructor automatically closes the serial port — 析构时自动关闭串口 */
    ~SerialPort() override;

    SerialPort(const SerialPort&)            = delete;   ///< Copy construction forbidden (serial port resource not shareable) — 禁止拷贝构造（串口资源不可共享）
    SerialPort& operator=(const SerialPort&) = delete;   ///< Copy assignment forbidden — 禁止拷贝赋值

    /**
     * @brief Open the serial port and apply current configuration (baud rate, parity, stop bits, etc.)
     *        打开串口并应用当前配置（波特率、校验位、停止位等）
     *        Uses CreateFileW on Windows, open + termios on Linux/macOS/Android
     *        Windows 上使用 CreateFileW，Linux/macOS/Android 使用 open + termios
     * @return true if opened successfully, false on failure or already open
     *         true 打开成功，false 打开失败或已处于打开状态
     */
    bool open() override;

    /**
     * @brief Close the serial port — 关闭串口
     *        Safe to call multiple times; internal handle / file descriptor is reset when already closed
     *        安全多次调用；已关闭时内部句柄 / 文件描述符会被重置
     */
    void close() override;

    /**
     * @brief Send data to the serial port (low-level implementation) — 向串口发送数据（底层实现）
     *        On Linux/macOS/Android, loops writes and uses select when write timeout is configured
     *        Linux/macOS/Android 上会循环写入并在配置了写超时时用 select 等待
     * @param data Pointer to the data to send — 要发送数据的指针
     * @param len  Number of bytes to send — 要发送的字节数
     * @return Number of bytes actually sent; 0 means port not open or send failure
     *         实际发送的字节数，0 表示端口未打开或发送失败
     */
    std::size_t send(const void *data, std::size_t len) override;

    /**
     * @brief Receive data from the serial port (low-level implementation, blocks until data or timeout)
     *        从串口接收数据（底层实现，阻塞直到有数据或超时）
     * @param buf      Receive buffer pointer — 接收缓冲区指针
     * @param max_size Buffer capacity in bytes — 缓冲区容量（字节）
     * @return Number of bytes actually received; 0 means timeout or failure
     *         实际接收到的字节数，0 表示超时或失败
     */
    std::size_t receive(void *buf, std::size_t max_size) override;

    /**
     * @brief Set the serial port name — 设置串口名称
     *        Windows: "COM1" ~ "COM256"
     *        Linux:   "/dev/ttyS0" "/dev/ttyUSB0" "/dev/ttyAMA0", etc.
     *        macOS:   "/dev/cu.usbmodemXXXX" "/dev/tty.usbserial-XXXX", etc.
     *        Android: "/dev/ttyGS0" "/dev/ttyHS*" "/dev/ttyMT*", etc.
     * @param portName Serial port name string — 串口名称字符串
     */
    void setPortName(const char *portName) override;

    /**
     * @brief Set read timeout / blocking behavior — 设置接收超时 / 阻塞行为
     *        Windows uses COMMTIMEOUTS; Linux/macOS/Android uses select inside receive()
     *        Windows 使用 COMMTIMEOUTS，Linux/macOS/Android 由 receive() 内部 select 控制
     * @param blocking true = blocking mode, false = non-blocking (returns immediately)
     *                 true = 阻塞模式，false = 非阻塞（立即返回）
     * @param ms        Timeout in milliseconds for blocking mode; 0 = block forever
     *                  阻塞模式下的超时毫秒数，0=永久阻塞
     */
    void setReadTimeout(bool blocking, std::uint32_t ms) override;

    /**
     * @brief Set write timeout / blocking behavior — 设置发送超时 / 阻塞行为
     *        Windows uses COMMTIMEOUTS; Linux/macOS/Android uses select inside send()
     *        Windows 使用 COMMTIMEOUTS，Linux/macOS/Android 由 send() 内部 select 控制
     * @param blocking true = blocking mode, false = non-blocking (returns immediately)
     *                 true = 阻塞模式，false = 非阻塞（立即返回）
     * @param ms        Timeout in milliseconds for blocking mode; 0 = block forever
     *                  阻塞模式下的超时毫秒数，0=永久阻塞
     */
    void setWriteTimeout(bool blocking, std::uint32_t ms) override;

private:
#ifdef _WIN32
    std::string   m_port_name        = "COM1";     ///< Serial port name (Windows: "COMx") — 串口名称 (Windows: "COMx")
#elif defined(__ANDROID__)
    std::string   m_port_name        = "/dev/ttyGS0";  ///< Serial port name (Android: "/dev/ttyGS0"(ADB), /dev/ttyHS*, /dev/ttyMT*, etc.) — 串口名称 (Android: "/dev/ttyGS0"(ADB)、/dev/ttyHS*、/dev/ttyMT* 等)
#else
    std::string   m_port_name        = "/dev/ttyUSB0";  ///< Serial port name (Linux: "/dev/ttyUSB0", macOS: "/dev/cu.*") — 串口名称 (Linux: "/dev/ttyUSB0"，macOS: "/dev/cu.*")
#endif
    BaudRate      m_baud_rate        = BaudRate::B115200;  ///< Baud rate — 波特率
    std::uint8_t  m_byte_size        = 8;         ///< Data bits (5~8) — 数据位 (5~8)
    Parity        m_parity           = Parity::None;      ///< Parity — 校验位
    StopBits      m_stop_bits        = StopBits::One;     ///< Stop bits — 停止位

#ifdef _WIN32
    HANDLE m_handle = INVALID_HANDLE_VALUE;       ///< Windows serial port device handle — Windows 串口设备句柄
#else
    int m_fd = -1;                                ///< POSIX file descriptor — POSIX 文件描述符
#endif
};