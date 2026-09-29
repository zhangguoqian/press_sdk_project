#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

/**
 * @brief Abstract base class for communication ports — 通信端口抽象基类
 *
 * Defines a unified communication interface. All concrete implementations
 * (TCP Socket, Serial Port, etc.) should inherit this class and implement
 * its pure virtual functions to achieve polymorphic calls.
 * 定义了统一的通信接口，所有具体的通信实现（TCP Socket、串口等）
 * 都应继承此类并实现其纯虚函数，从而实现多态调用。
 *
 * Send/Receive provide multiple overloads — 发送/接收提供多组重载:
 *   send(const void*, size_t)          — Pure virtual, implemented by derived classes — 底层纯虚，派生类实现
 *   send(const vector<uint8_t>&)       — Convenience overload — 便捷重载
 *   send(const string&)                — For text protocols (AT, HTTP, custom commands) — 方便发送字符串协议
 *   send(const char*)                  — For C strings, auto strlen — 方便发送 C 字符串
 *   receive(void*, size_t)             — Pure virtual, implemented by derived classes — 底层纯虚，派生类实现
 *   receive(size_t max_size = 0)       — Returns vector; default configurable via setReadMaxBufferSize()
 *                                       返回 vector，默认值可通过 setReadMaxBufferSize() 配置 (initially / 初始默认 1024)
 *   write / read are POSIX-style aliases for send / receive with identical signatures
 *   write / read 为 send / receive 的 POSIX 风格别名，签名完全等价
 */
class PortBase {
public:
    /** @brief Virtual destructor, ensures derived classes are properly destroyed — 虚析构函数，保证派生类正确析构 */
    virtual ~PortBase() = default;

    /**
     * @brief Open the port / establish a connection — 打开端口 / 建立连接
     * @return true if opened successfully, false otherwise — true 打开成功，false 打开失败
     */
    virtual bool open() = 0;

    /**
     * @brief Close the port / disconnect — 关闭端口 / 断开连接
     */
    virtual void close() = 0;

    /**
     * @brief Send data (low-level raw memory interface, must be implemented by derived classes)
     *        发送数据（底层原始内存接口，派生类必须实现）
     * @param data Pointer to the data to send; must be valid when non-null — 指向要发送数据的指针
     * @param len  Number of bytes to send — 要发送的字节数
     * @return Number of bytes actually sent; 0 means failure or port not open — 实际发送的字节数，0 表示失败或端口未打开
     */
    virtual std::size_t send(const void *data, std::size_t len) = 0;

    /**
     * @brief Receive data from the peer (low-level raw buffer interface, must be implemented by derived classes)
     *        从对端接收数据（底层原始缓冲区接口，派生类必须实现）
     * @param buf      Pointer to the receive buffer — 接收缓冲区指针
     * @param max_size Buffer capacity in bytes — 缓冲区容量（字节）
     * @return Number of bytes actually received; 0 means timeout, disconnection or failure
     *         实际接收到的字节数，0 表示超时、连接断开或失败
     */
    virtual std::size_t receive(void *buf, std::size_t max_size) = 0;

    /**
     * @brief Send data — std::vector convenience overload — 发送数据 — std::vector 便捷重载
     * @param data Byte data to send — 要发送的字节数据
     * @return Number of bytes actually sent — 实际发送的字节数
     */
    std::size_t send(const std::vector<std::uint8_t>& data) {
        return send(data.data(), data.size());
    }

    /**
     * @brief Send data — std::string convenience overload — 发送数据 — std::string 便捷重载
     *        Commonly used for text protocols (AT commands, HTTP requests, custom CLIs, etc.)
     *        常用于发送文本协议（AT 指令、HTTP 请求、自定义命令行等）
     * @param str String content to send — 要发送的字符串内容
     * @return Number of bytes actually sent — 实际发送的字节数
     */
    std::size_t send(const std::string& str) {
        return send(str.data(), str.size());
    }

    /**
     * @brief Send data — C string convenience overload — 发送数据 — C 字符串便捷重载
     *        Automatically computes length via strlen; returns 0 directly when str is nullptr
     *        自动以 strlen 计算长度；str 为 nullptr 时直接返回 0
     * @param str C string to send (null-terminated) — 要发送的 C 字符串（以 '\0' 结尾）
     * @return Number of bytes actually sent (excluding trailing '\0') — 实际发送的字节数（不含结尾的 '\0'）
     */
    std::size_t send(const char *str) {
        return str ? send(str, std::strlen(str)) : 0;
    }

    /**
     * @brief write — alias of send with identical parameter and return semantics
     *        write — send 的别名，参数与返回值语义完全一致
     *        Convenient for POSIX / Arduino-style code reuse — 方便熟悉 POSIX / Arduino 风格的代码复用
     */
    std::size_t write(const void *data, std::size_t len) {
        return send(data, len);
    }

    /// write — vector overload, same as send(vector) — vector 重载，等同 send(vector)
    std::size_t write(const std::vector<std::uint8_t>& data) {
        return send(data);
    }

    /// write — string overload, same as send(string) — string 重载，等同 send(string)
    std::size_t write(const std::string& str) {
        return send(str);
    }

    /// write — C string overload, same as send(const char*) — C 字符串重载，等同 send(const char*)
    std::size_t write(const char *str) {
        return send(str);
    }

    /**
     * @brief Receive data — convenience overload returning std::vector (blocks until data or timeout)
     *        接收数据 — 返回 std::vector 便捷重载（阻塞直到有数据或超时）
     * @param max_size Maximum bytes to receive at a time; default comes from readMaxBufferSize() (initially 1024)
     *                 单次最多接收的字节数，默认值取自 readMaxBufferSize()（初始默认 1024）
     *                 Passing 0 also uses the default — 传 0 也会使用默认值
     * @return Received byte data; empty vector means timeout, disconnection or failure
     *         接收到的字节数据；空 vector 表示超时、连接断开或失败
     */
    std::vector<std::uint8_t> receive(std::size_t max_size = 0) {
        if (max_size == 0) max_size = m_read_max_buffer_size;
        if (max_size == 0) return {};
        std::vector<std::uint8_t> buf(max_size);
        std::size_t n = receive(buf.data(), max_size);
        buf.resize(n);
        return buf;
    }

    /**
     * @brief read — alias of receive with identical parameter and return semantics
     *        read — receive 的别名，参数与返回值语义完全一致
     *        Convenient for POSIX / Arduino-style code reuse — 方便熟悉 POSIX / Arduino 风格的代码复用
     */
    std::size_t read(void *buf, std::size_t max_size) {
        return receive(buf, max_size);
    }

    /// read — vector-returning overload, same as receive(size_t) — 返回 vector 重载，等同 receive(size_t)
    std::vector<std::uint8_t> read(std::size_t max_size = 0) {
        return receive(max_size);
    }

    /**
     * @brief Set port name / connection target — 设置端口名称 / 连接目标
     *        TCP Socket format: "host:port" (e.g. "192.168.1.1:8080" or "[::1]:8080")
     *        TCP Socket 格式: "host:port" (例 "192.168.1.1:8080" 或 "[::1]:8080")
     *        Serial Port format: serial port name string (e.g. "COM3" or "/dev/ttyUSB0")
     *        串口    格式: 串口名称字符串  (例 "COM3" 或 "/dev/ttyUSB0")
     * @param portName Port name / target address string; internally copied — 端口名称 / 目标地址字符串，内部会被拷贝保存
     */
    virtual void setPortName(const char *portName) = 0;

    /**
     * @brief Set blocking / timeout behavior for read operations — 设置接收操作的阻塞 / 超时行为
     * @param blocking true = blocking mode, false = non-blocking (returns immediately)
     *                 true = 阻塞模式，false = 非阻塞（立即返回）
     * @param ms        Timeout in milliseconds for blocking mode — 阻塞模式下的超时毫秒数：
     *                  - 0   = block forever (wait until data available) — 0  表示永久阻塞（一直等到有数据）
     *                  - >0  = block up to ms milliseconds, return 0 on timeout
     *                          >0 表示阻塞等待最多 ms 毫秒，超时返回 0
     *                  Ignored in non-blocking mode — 非阻塞模式下此参数被忽略
     */
    virtual void setReadTimeout(bool blocking, std::uint32_t ms) = 0;

    /**
     * @brief Set blocking / timeout behavior for write operations — 设置发送操作的阻塞 / 超时行为
     * @param blocking true = blocking mode, false = non-blocking (returns immediately)
     *                 true = 阻塞模式，false = 非阻塞（立即返回）
     * @param ms        Timeout in milliseconds for blocking mode — 阻塞模式下的超时毫秒数：
     *                  - 0   = block forever (wait until writable) — 0  表示永久阻塞（一直等到可写）
     *                  - >0  = block up to ms milliseconds, return bytes written so far or 0 on timeout
     *                          >0 表示阻塞等待最多 ms 毫秒，超时返回已写量或 0
     *                  Ignored in non-blocking mode — 非阻塞模式下此参数被忽略
     */
    virtual void setWriteTimeout(bool blocking, std::uint32_t ms) = 0;

    /**
     * @brief Query whether the port is currently open — 查询端口当前是否已打开
     * @return true if open, false otherwise — true 已打开，false 未打开
     */
    bool isOpen() const noexcept { return m_is_open; }

    /**
     * @brief Set the default buffer size for no-arg convenience overloads receive() / read()
     *        设置便捷重载 receive() / read() 无参调用时的默认缓冲区大小
     * @param size Default buffer size in bytes; setting to 0 is ignored (keeps previous value)
     *             默认缓冲区字节数，设为 0 会被忽略（保持原值）
     */
    void setReadMaxBufferSize(std::size_t size) noexcept {
        if (size > 0) m_read_max_buffer_size = size;
    }

    /**
     * @brief Query the default buffer size for no-arg convenience overloads receive() / read()
     *        查询便捷重载 receive() / read() 无参调用时的默认缓冲区大小
     * @return Current default size in bytes, initial value is 1024 — 当前默认字节数，初始值 1024
     */
    std::size_t readMaxBufferSize() const noexcept { return m_read_max_buffer_size; }

protected:
    bool          m_is_open              = false;    ///< Port open state — 端口打开状态
    bool          m_read_blocking       = false;     ///< Whether read is blocking; default: non-blocking (returns immediately) — 读操作是否阻塞，默认非阻塞（立即返回）
    std::uint32_t m_read_timeout_ms      = 0;        ///< Read timeout (ms); 0 = block forever in blocking mode — 读超时 (毫秒)，阻塞模式下 0=永久阻塞
    bool          m_write_blocking      = true;      ///< Whether write is blocking; default: block until fully sent — 写操作是否阻塞，默认阻塞直到发送完成
    std::uint32_t m_write_timeout_ms     = 0;        ///< Write timeout (ms); 0 = block forever (until done) in blocking mode — 写超时 (毫秒)，阻塞模式下 0=永久阻塞（直到写完）
    std::size_t   m_read_max_buffer_size = 1024;     ///< Default buffer size for receive() / read() — receive() / read() 默认缓冲区大小
};