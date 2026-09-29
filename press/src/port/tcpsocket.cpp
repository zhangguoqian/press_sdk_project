#include "tcpsocket.h"

#include <climits>
#include <stdexcept>

// #ifdef _WIN32
// #   pragma comment(lib, "ws2_32.lib")
// #endif

TcpSocket::TcpSocket() = default;

TcpSocket::~TcpSocket() {
    close();
#ifdef _WIN32
    if (m_wsa_initialized) {
        ::WSACleanup();
    }
#endif
}

bool TcpSocket::open() {
    if (m_is_open) return true;

#ifdef _WIN32
    WSADATA wsa{};
    if (!m_wsa_initialized) {
        if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0 ||
            LOBYTE(wsa.wVersion) < 2 ||
            (LOBYTE(wsa.wVersion) == 2 && HIBYTE(wsa.wVersion) < 2))
            return false;
        m_wsa_initialized = true;
    }

    int family = m_host_v6 ? AF_INET6 : AF_INET;
    m_socket = ::socket(family, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) return false;

    unsigned long mode = 1;
    ::ioctlsocket(m_socket, FIONBIO, &mode);

    auto do_connect = [this]() -> bool {
        if (m_host_v6) {
            sockaddr_in6 addr{};
            addr.sin6_family = AF_INET6;
            addr.sin6_port   = ::htons(m_port);
            if (inet_pton(AF_INET6, m_host.c_str(), &addr.sin6_addr) != 1)
                return false;
            return ::connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR;
        } else {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port   = ::htons(m_port);
            if (inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) != 1)
                return false;
            return ::connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != SOCKET_ERROR;
        }
    };

    if (do_connect()) {
        m_is_open = true;
        return true;
    }

    int err = ::WSAGetLastError();
    if (err != WSAEWOULDBLOCK) {
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(m_socket, &wfds);

    int sel_ret;
    if (m_connect_timeout_ms > 0) {
        timeval tv{};
        tv.tv_sec  = static_cast<long>(m_connect_timeout_ms / 1000);
        tv.tv_usec = static_cast<long>((m_connect_timeout_ms % 1000) * 1000);
        sel_ret = ::select(0, nullptr, &wfds, nullptr, &tv);
    } else {
        sel_ret = ::select(0, nullptr, &wfds, nullptr, nullptr);
    }

    if (sel_ret <= 0) {
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    int so_err = 0;
    int so_len = sizeof(so_err);
    ::getsockopt(m_socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_err), &so_len);
    if (so_err != 0) {
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_is_open = true;
    return true;
#else
    int family = m_host_v6 ? AF_INET6 : AF_INET;
    m_socket = ::socket(family, SOCK_STREAM, 0);
    if (m_socket < 0) return false;

    int flags = ::fcntl(m_socket, F_GETFL, 0);
    ::fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);

    auto do_connect = [this, family]() -> bool {
        if (m_host_v6) {
            sockaddr_in6 addr{};
            addr.sin6_family = AF_INET6;
            addr.sin6_port   = ::htons(m_port);
            if (::inet_pton(AF_INET6, m_host.c_str(), &addr.sin6_addr) != 1)
                return false;
            return ::connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
        } else {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port   = ::htons(m_port);
            if (::inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr) != 1)
                return false;
            return ::connect(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0;
        }
    };

    if (do_connect()) {
        m_is_open = true;
        return true;
    }

    if (errno != EINPROGRESS) {
        ::close(m_socket);
        m_socket = -1;
        return false;
    }

    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(m_socket, &wfds);

    int sel_ret;
    if (m_connect_timeout_ms > 0) {
        timeval tv{};
        tv.tv_sec  = static_cast<time_t>(m_connect_timeout_ms / 1000);
        tv.tv_usec = static_cast<suseconds_t>((m_connect_timeout_ms % 1000) * 1000);
        sel_ret = ::select(m_socket + 1, nullptr, &wfds, nullptr, &tv);
    } else {
        sel_ret = ::select(m_socket + 1, nullptr, &wfds, nullptr, nullptr);
    }

    if (sel_ret <= 0) {
        ::close(m_socket);
        m_socket = -1;
        return false;
    }

    int so_err = 0;
    socklen_t so_len = sizeof(so_err);
    ::getsockopt(m_socket, SOL_SOCKET, SO_ERROR, &so_err, &so_len);
    if (so_err != 0) {
        ::close(m_socket);
        m_socket = -1;
        return false;
    }

    m_is_open = true;
    return true;
#endif
}

void TcpSocket::close() {
#ifdef _WIN32
    if (m_socket != INVALID_SOCKET) {
        ::closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
#else
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
#endif
    m_is_open = false;
}

std::size_t TcpSocket::send(const void *data, std::size_t len) {
    if (!m_is_open || !data || len == 0) return 0;

#ifdef _WIN32
    if (m_socket == INVALID_SOCKET) return 0;

    const auto *p = reinterpret_cast<const std::uint8_t *>(data);
    std::size_t total = 0;
    while (total < len) {
        if (m_write_blocking) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(m_socket, &wfds);

            if (m_write_timeout_ms > 0) {
                timeval tv{};
                tv.tv_sec  = static_cast<long>(m_write_timeout_ms / 1000);
                tv.tv_usec = static_cast<long>((m_write_timeout_ms % 1000) * 1000);
                int ret = ::select(0, nullptr, &wfds, nullptr, &tv);
                if (ret <= 0) return total;
            } else {
                int ret = ::select(0, nullptr, &wfds, nullptr, nullptr);
                if (ret <= 0) return total;
            }
        }

        std::size_t remaining = len - total;
        std::size_t chunk = (remaining > static_cast<std::size_t>(INT_MAX)) ? static_cast<std::size_t>(INT_MAX) : remaining;
        int sent = ::send(m_socket, reinterpret_cast<const char*>(p + total), static_cast<int>(chunk), 0);
        if (sent > 0) {
            total += static_cast<std::size_t>(sent);
        } else if (sent == SOCKET_ERROR) {
            int err = ::WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                if (m_write_blocking) continue;
                return total;
            }
            return total;
        } else {
            break;
        }
    }
    return total;
#else
    if (m_socket < 0) return 0;

    const auto *p = reinterpret_cast<const std::uint8_t *>(data);
    std::size_t total = 0;
    while (total < len) {
        if (m_write_blocking) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(m_socket, &wfds);

            if (m_write_timeout_ms > 0) {
                timeval tv{};
                tv.tv_sec  = static_cast<time_t>(m_write_timeout_ms / 1000);
                tv.tv_usec = static_cast<suseconds_t>((m_write_timeout_ms % 1000) * 1000);
                int ret = ::select(m_socket + 1, nullptr, &wfds, nullptr, &tv);
                if (ret <= 0) return total;
            } else {
                int ret = ::select(m_socket + 1, nullptr, &wfds, nullptr, nullptr);
                if (ret <= 0) return total;
            }
        }

        ssize_t n = ::send(m_socket, p + total, len - total, 0);
        if (n > 0) {
            total += static_cast<std::size_t>(n);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (m_write_blocking) continue;
            return total;
        } else {
            break;
        }
    }
    return total;
#endif
}

std::size_t TcpSocket::receive(void *buf, std::size_t max_size) {
    if (!m_is_open || !buf || max_size == 0) return 0;

#ifdef _WIN32
    if (m_socket == INVALID_SOCKET) return 0;

    while (true) {
        if (m_read_blocking) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(m_socket, &rfds);

            if (m_read_timeout_ms > 0) {
                timeval tv{};
                tv.tv_sec  = static_cast<long>(m_read_timeout_ms / 1000);
                tv.tv_usec = static_cast<long>((m_read_timeout_ms % 1000) * 1000);
                int ret = ::select(0, &rfds, nullptr, nullptr, &tv);
                if (ret <= 0) return 0;
            } else {
                int ret = ::select(0, &rfds, nullptr, nullptr, nullptr);
                if (ret <= 0) return 0;
            }
        }

        std::size_t chunk = (max_size > static_cast<std::size_t>(INT_MAX)) ? static_cast<std::size_t>(INT_MAX) : max_size;
        int received = ::recv(m_socket, reinterpret_cast<char*>(buf), static_cast<int>(chunk), 0);
        if (received > 0) {
            return static_cast<std::size_t>(received);
        } else if (received == SOCKET_ERROR) {
            int err = ::WSAGetLastError();
            if (err == WSAEWOULDBLOCK) {
                if (m_read_blocking) continue;
                return 0;
            }
            return 0;
        } else {
            return 0;
        }
    }
#else
    if (m_socket < 0) return 0;

    while (true) {
        if (m_read_blocking) {
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(m_socket, &rfds);

            if (m_read_timeout_ms > 0) {
                timeval tv{};
                tv.tv_sec  = static_cast<time_t>(m_read_timeout_ms / 1000);
                tv.tv_usec = static_cast<suseconds_t>((m_read_timeout_ms % 1000) * 1000);
                int ret = ::select(m_socket + 1, &rfds, nullptr, nullptr, &tv);
                if (ret <= 0) return 0;
            } else {
                int ret = ::select(m_socket + 1, &rfds, nullptr, nullptr, nullptr);
                if (ret <= 0) return 0;
            }
        }

        ssize_t n = ::recv(m_socket, buf, max_size, 0);
        if (n > 0) {
            return static_cast<std::size_t>(n);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (m_read_blocking) continue;
            return 0;
        }
        return 0;
    }
#endif
}

void TcpSocket::setPortName(const char *portName) {
    if (!portName) return;

    std::string s = portName;
    auto pos = s.rfind(':');
    if (pos == std::string::npos) return;

    std::string host = s.substr(0, pos);
    std::string port = s.substr(pos + 1);

    if (!host.empty() && host.front() == '[' && host.back() == ']')
        host = host.substr(1, host.size() - 2);

    std::uint16_t new_port = 0;
    try {
        int p = std::stoi(port);
        if (p < 0 || p > 65535) return;
        new_port = static_cast<std::uint16_t>(p);
    } catch (const std::invalid_argument&) {
        return;
    } catch (const std::out_of_range&) {
        return;
    }

    m_host    = host;
    m_host_v6 = (host.find(':') != std::string::npos);
    m_port    = new_port;
}

void TcpSocket::setReadTimeout(bool blocking, std::uint32_t ms) {
    m_read_blocking  = blocking;
    m_read_timeout_ms = ms;
}

void TcpSocket::setWriteTimeout(bool blocking, std::uint32_t ms) {
    m_write_blocking  = blocking;
    m_write_timeout_ms = ms;
}

void TcpSocket::setConnectTimeout(std::uint32_t ms) {
    m_connect_timeout_ms = ms;
}

bool TcpSocket::connect(const std::string &ip, std::uint16_t port) {
    if (ip.empty()) return false;
    close();
    std::string target;
    if (ip.front() == '[') {
        target = ip + ":" + std::to_string(port);
    } else if (ip.find(':') != std::string::npos) {
        target = "[" + ip + "]:" + std::to_string(port);
    } else {
        target = ip + ":" + std::to_string(port);
    }
    setPortName(target.c_str());
    return open();
}

bool TcpSocket::connect(const char *ip, std::uint16_t port) {
    if (ip == nullptr) return false;
    return connect(std::string(ip), port);
}