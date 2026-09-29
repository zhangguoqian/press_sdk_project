#pragma once

#include "portbase.h"

#include <cstdint>
#include <string>

#ifdef _WIN32
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#   endif
#   ifndef _WIN32_WINNT
#       define _WIN32_WINNT 0x0601
#   endif
#   include <winsock2.h>
#   include <ws2tcpip.h>
#else
#   include <sys/socket.h>
#   include <sys/time.h>
#   include <sys/select.h>
#   include <netinet/in.h>
#   include <arpa/inet.h>
#   include <fcntl.h>
#   include <unistd.h>
#   include <cerrno>
#endif

/**
 * @brief TCP client Socket wrapper — TCP 客户端 Socket 封装类
 *
 * Provides cross-platform (Windows / Linux / macOS / Android) TCP client connectivity
 * through the unified PortBase interface.
 * 提供跨平台（Windows / Linux / macOS / Android）的 TCP 客户端连接能力，
 * 通过继承 PortBase 统一通信接口。
 *
 * Usage example:
 * @code
 *   TcpSocket sock;
 *   sock.setPortName("192.168.1.100:9000");
 *   sock.setReadTimeout(true, 1000);
 *   sock.setWriteTimeout(true, 1000);
 *   if (sock.open()) {
 *       sock.send({0x01, 0x02, 0x03});
 *       auto data = sock.receive(256);
 *       sock.close();
 *   }
 * @endcode
 */
class TcpSocket final : public PortBase {
public:
    using PortBase::send;
    using PortBase::receive;
    using PortBase::write;
    using PortBase::read;

    /** @brief Default constructor; target address initialized to 127.0.0.1:8080
     *         默认构造，目标地址初始化为 127.0.0.1:8080 */
    TcpSocket();

    /** @brief Destructor automatically closes the connection and (on Windows) cleans up Winsock
     *         析构时自动关闭连接并（Windows 上）清理 Winsock */
    ~TcpSocket() override;

    TcpSocket(const TcpSocket&)            = delete;   ///< Copy construction forbidden (Socket resource not shareable) — 禁止拷贝构造（Socket 资源不可共享）
    TcpSocket& operator=(const TcpSocket&) = delete;   ///< Copy assignment forbidden — 禁止拷贝赋值

    /**
     * @brief Establish a TCP connection — 建立 TCP 连接
     *        On Windows, automatically initializes Winsock (WSAStartup 2.2)
     *        Windows 平台会自动初始化 Winsock (WSAStartup 2.2)
     * @return true if connected successfully, false on failure or already open
     *         true 连接成功，false 连接失败或已处于打开状态
     */
    bool open() override;

    /**
     * @brief Close the TCP connection — 关闭 TCP 连接
     *        Safe to call multiple times; internal resource handles are reset when already closed
     *        安全多次调用；已关闭时内部资源句柄会被重置
     */
    void close() override;

    /**
     * @brief Send data to the peer (low-level implementation) — 向对端发送数据（底层实现）
     *        On Linux/macOS/Android, loops until all data is written or an error occurs
     *        Linux/macOS/Android 上会循环发送直到数据全部写完或出错
     * @param data Pointer to the data to send — 要发送数据的指针
     * @param len  Number of bytes to send — 要发送的字节数
     * @return Number of bytes actually sent; 0 means port not open or send failure
     *         实际发送的字节数，0 表示端口未打开或发送失败
     */
    std::size_t send(const void *data, std::size_t len) override;

    /**
     * @brief Receive data from the peer (low-level implementation, blocks until data or timeout)
     *        从对端接收数据（底层实现，阻塞直到有数据或超时）
     * @param buf      Receive buffer pointer — 接收缓冲区指针
     * @param max_size Buffer capacity in bytes — 缓冲区容量（字节）
     * @return Number of bytes actually received; 0 means timeout, disconnection or failure
     *         实际接收到的字节数，0 表示超时、连接断开或失败
     */
    std::size_t receive(void *buf, std::size_t max_size) override;

    /**
     * @brief Parse and set the target host address and port — 解析并设置目标主机地址与端口
     *        Format: "host:port" — 格式: "host:port"
     *        - IPv4: "192.168.1.1:8080"
     *        - IPv6: "[::1]:8080"  (brackets are auto-stripped — 自动剥离方括号)
     * @param portName Target address string; the part after the last colon must be a numeric port
     *                 目标地址字符串，末尾冒号后的部分必须是数字端口
     */
    void setPortName(const char *portName) override;

    /**
     * @brief Set read timeout / blocking behavior — 设置接收超时 / 阻塞行为
     *        Takes effect only after connection is established; values are saved as member variables
     *        before connection and applied automatically after open() succeeds
     *        仅连接建立后生效；未连接时会先保存到成员变量，open() 成功后自动应用
     * @param blocking true = blocking mode, false = non-blocking (returns immediately)
     *                 true = 阻塞模式，false = 非阻塞（立即返回）
     * @param ms        Timeout in milliseconds for blocking mode; 0 = block forever
     *                  阻塞模式下的超时毫秒数，0=永久阻塞
     */
    void setReadTimeout(bool blocking, std::uint32_t ms) override;

    /**
     * @brief Set write timeout / blocking behavior — 设置发送超时 / 阻塞行为
     *        Takes effect only after connection is established; values are saved as member variables
     *        before connection and applied automatically after open() succeeds
     *        仅连接建立后生效；未连接时会先保存到成员变量，open() 成功后自动应用
     * @param blocking true = blocking mode, false = non-blocking (returns immediately)
     *                 true = 阻塞模式，false = 非阻塞（立即返回）
     * @param ms        Timeout in milliseconds for blocking mode; 0 = block forever
     *                  阻塞模式下的超时毫秒数，0=永久阻塞
     */
    void setWriteTimeout(bool blocking, std::uint32_t ms) override;

    /**
     * @brief Set connection timeout — 设置连接超时时间
     *        Only takes effect when open() establishes the connection; open() returns false on timeout
     *        仅在 open() 建立连接时生效；超时时 open() 返回 false
     * @param ms Timeout in milliseconds; 0 = block forever (wait for system TCP timeout, ~75s typically), default 3000ms
     *           超时毫秒数，0=永久阻塞（等待系统 TCP 超时，通常 ~75s），默认 3000ms
     */
    void setConnectTimeout(std::uint32_t ms);

    /**
     * @brief Convenience function: set target address and establish connection in one step
     *        便捷函数：一步设置目标地址并建立连接
     *        Equivalent to setPortName(ip + ":" + port) + open() — 等价于 setPortName(ip + ":" + port) + open()
     * @param ip   Target IP address (IPv4: "192.168.1.1"; IPv6: "::1" or bracketed "[::1]")
     *             目标 IP 地址 (IPv4: "192.168.1.1"，IPv6: "::1" 或带方括号 "[::1]")
     * @param port Target port number — 目标端口号
     * @return true if connected successfully, false on failure — true 连接成功，false 连接失败
     */
    bool connect(const std::string &ip, std::uint16_t port);

    /** @brief C string overload of connect(const std::string&, std::uint16_t) — connect 的 C 字符串重载 */
    bool connect(const char *ip, std::uint16_t port);

private:
    std::string   m_host              = "127.0.0.1";  ///< Target host address (supports IPv4 / IPv6) — 目标主机地址 (支持 IPv4 / IPv6)
    std::uint16_t m_port              = 8080;         ///< Target port — 目标端口号
    bool          m_host_v6           = false;        ///< Whether m_host is an IPv6 address — m_host 是否为 IPv6 地址
    std::uint32_t m_connect_timeout_ms = 3000;        ///< Connection timeout (ms), default 3s — 连接超时 (毫秒)，默认 3s

#ifdef _WIN32
    SOCKET m_socket = INVALID_SOCKET;   ///< Windows Socket handle — Windows Socket 句柄
    bool   m_wsa_initialized = false;   ///< Whether Winsock has been initialized — 是否已初始化 Winsock
#else
    int m_socket = -1;                 ///< POSIX Socket file descriptor — POSIX Socket 文件描述符
#endif
};