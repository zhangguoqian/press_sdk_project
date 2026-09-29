#include "serialport.h"

#include <cctype>

#ifdef _WIN32

#ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0601
#endif

#include <initguid.h>
#include <devguid.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <winioctl.h>

DEFINE_GUID(GUID_DEVINTERFACE_PORTS, 0x86E0D1E8, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73);

namespace {

std::wstring to_wide(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return {};
    std::wstring w(static_cast<std::size_t>(len), L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, &w[0], len);
    w.pop_back();
    return w;
}

std::string to_narrow(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string s(static_cast<std::size_t>(len), '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &s[0], len, nullptr, nullptr);
    s.pop_back();
    return s;
}

BYTE parity_to_win(SerialPort::Parity p) {
    switch (p) {
        case SerialPort::Parity::Even: return EVENPARITY;
        case SerialPort::Parity::Odd:  return ODDPARITY;
        default:                       return NOPARITY;
    }
}

BYTE stopbits_to_win(SerialPort::StopBits s) {
    return s == SerialPort::StopBits::Two ? TWOSTOPBITS : ONESTOPBIT;
}

} // namespace

bool SerialPortInfo::isBusy() const {
    if (name.empty()) return false;
    std::wstring wname = L"\\\\";
    wname += to_wide(name);
    HANDLE h = ::CreateFileW(
        wname.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        DWORD err = ::GetLastError();
        return err == ERROR_ACCESS_DENIED || err == ERROR_SHARING_VIOLATION;
    }
    ::CloseHandle(h);
    return false;
}

std::vector<SerialPortInfo> SerialPort::listAvailablePorts() {
    std::vector<SerialPortInfo> result;

    // Enumerate registered serial port devices via SetupDi, fetch friendly names
    // 使用 SetupDi 枚举已注册的串口设备，获取友好名称
    HDEVINFO hdi = ::SetupDiGetClassDevsW(
        &GUID_DEVINTERFACE_PORTS, nullptr, nullptr,
        DIGCF_PRESENT | DIGCF_PROFILE);
    if (hdi != INVALID_HANDLE_VALUE) {
        SP_DEVINFO_DATA did{};
        did.cbSize = sizeof(did);
        for (DWORD i = 0; ::SetupDiEnumDeviceInfo(hdi, i, &did); ++i) {
            DWORD sz = 0;
            ::SetupDiGetDeviceRegistryPropertyW(hdi, &did, SPDRP_FRIENDLYNAME,
                                                 nullptr, nullptr, 0, &sz);
            if (sz == 0) continue;
            std::wstring fw(sz / sizeof(WCHAR), L'\0');
            if (!::SetupDiGetDeviceRegistryPropertyW(hdi, &did, SPDRP_FRIENDLYNAME,
                                                     nullptr,
                                                     reinterpret_cast<PBYTE>(&fw[0]),
                                                     sz, &sz)) {
                continue;
            }
            std::string friendly = to_narrow(fw);

            // Extract "COMx" from friendly name — format is usually "xxx Serial Device (COM3)"
            // 从友好名称提取 "COMx" — 格式通常是 "xxx Serial Device (COM3)"
            std::string port_name;
            auto open_p = friendly.rfind('(');
            auto close_p = friendly.rfind(')');
            if (open_p != std::string::npos && close_p != std::string::npos &&
                close_p > open_p) {
                auto inside = friendly.substr(open_p + 1, close_p - open_p - 1);
                // Validate format: COM + digits — 验证格式: COM + 数字
                if (inside.size() >= 4 && inside.compare(0, 3, "COM") == 0) {
                    bool all_digit = true;
                    for (std::size_t j = 3; j < inside.size(); ++j) {
                        if (!std::isdigit(static_cast<unsigned char>(inside[j]))) {
                            all_digit = false; break;
                        }
                    }
                    if (all_digit) port_name = inside;
                }
            }

            if (!port_name.empty()) {
                result.emplace_back(port_name, friendly);
            }
        }
        ::SetupDiDestroyDeviceInfoList(hdi);
    }

    // Additionally scan COM1~COM256 and merge & deduplicate (covers virtual serial ports missed by SetupDi)
    // 补扫 COM1~COM256 并合并去重（覆盖 SetupDi 漏掉的虚拟串口）
    auto contains = [&](const std::string &n) {
        for (const auto &p : result) if (p.name == n) return true;
        return false;
    };
    for (int i = 1; i <= 256; ++i) {
        std::string name = "COM" + std::to_string(i);
        if (contains(name)) continue;
        std::wstring wname = to_wide(name);
        wname = L"\\\\.\\" + wname;
        WCHAR target[MAX_PATH];
        if (::QueryDosDeviceW(wname.c_str(), target, MAX_PATH) != 0) {
            std::string desc = to_narrow(target);
            result.emplace_back(name, desc);
        }
    }

    return result;
}

SerialPort::SerialPort() = default;

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open() {
    if (m_is_open) return true;

    std::wstring name = to_wide(m_port_name);
    if (name.rfind(L"\\\\.\\", 0) != 0)
        name = L"\\\\.\\" + name;
    m_handle = ::CreateFileW(
        name.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (m_handle == INVALID_HANDLE_VALUE) return false;

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!::GetCommState(m_handle, &dcb)) {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    dcb.BaudRate     = static_cast<DWORD>(m_baud_rate);
    dcb.ByteSize     = m_byte_size;
    dcb.Parity       = parity_to_win(m_parity);
    dcb.StopBits     = stopbits_to_win(m_stop_bits);
    dcb.fBinary      = TRUE;
    dcb.fParity      = (m_parity != Parity::None) ? TRUE : FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl  = DTR_CONTROL_DISABLE;
    dcb.fRtsControl  = RTS_CONTROL_DISABLE;
    dcb.fOutX        = FALSE;
    dcb.fInX         = FALSE;

    if (!::SetCommState(m_handle, &dcb)) {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    ::PurgeComm(m_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);

    m_is_open = true;
    setReadTimeout(m_read_blocking, m_read_timeout_ms);
    setWriteTimeout(m_write_blocking, m_write_timeout_ms);
    return true;
}

void SerialPort::close() {
    if (m_handle != INVALID_HANDLE_VALUE) {
        ::CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    m_is_open = false;
}

std::size_t SerialPort::send(const void *data, std::size_t len) {
    if (!m_is_open || m_handle == INVALID_HANDLE_VALUE || !data || len == 0) return 0;

    const auto *p = reinterpret_cast<const std::uint8_t *>(data);
    std::size_t total_written = 0;
    while (total_written < len) {
        std::size_t remaining = len - total_written;
        DWORD chunk = (remaining > 0xFFFFFFFFu) ? 0xFFFFFFFFu : static_cast<DWORD>(remaining);
        DWORD written = 0;
        if (!::WriteFile(m_handle, p + total_written, chunk, &written, nullptr))
            break;
        total_written += written;
        if (written == 0) break;
    }
    return total_written;
}

std::size_t SerialPort::receive(void *buf, std::size_t max_size) {
    if (!m_is_open || m_handle == INVALID_HANDLE_VALUE || !buf || max_size == 0) return 0;

    DWORD chunk = (max_size > 0xFFFFFFFFu) ? 0xFFFFFFFFu : static_cast<DWORD>(max_size);
    DWORD read = 0;
    if (!::ReadFile(m_handle, buf, chunk, &read, nullptr))
        return 0;
    return read;
}

void SerialPort::setPortName(const char *portName) {
    if (portName) m_port_name = portName;
}

void SerialPort::setReadTimeout(bool blocking, std::uint32_t ms) {
    m_read_blocking  = blocking;
    m_read_timeout_ms = ms;
    if (!m_is_open || m_handle == INVALID_HANDLE_VALUE) return;

    COMMTIMEOUTS timeouts{};
    if (!::GetCommTimeouts(m_handle, &timeouts)) return;

    if (!blocking) {
        timeouts.ReadIntervalTimeout         = MAXDWORD;
        timeouts.ReadTotalTimeoutConstant    = 0;
        timeouts.ReadTotalTimeoutMultiplier  = 0;
    } else if (ms == 0) {
        timeouts.ReadIntervalTimeout         = 0;
        timeouts.ReadTotalTimeoutConstant    = 0;
        timeouts.ReadTotalTimeoutMultiplier  = 0;
    } else {
        timeouts.ReadIntervalTimeout         = 0;
        timeouts.ReadTotalTimeoutConstant    = static_cast<DWORD>(ms);
        timeouts.ReadTotalTimeoutMultiplier  = 0;
    }
    ::SetCommTimeouts(m_handle, &timeouts);
}

void SerialPort::setWriteTimeout(bool blocking, std::uint32_t ms) {
    m_write_blocking  = blocking;
    m_write_timeout_ms = ms;
    if (!m_is_open || m_handle == INVALID_HANDLE_VALUE) return;

    COMMTIMEOUTS timeouts{};
    if (!::GetCommTimeouts(m_handle, &timeouts)) return;

    if (!blocking) {
        timeouts.WriteTotalTimeoutConstant   = 1;
        timeouts.WriteTotalTimeoutMultiplier = 0;
    } else if (ms == 0) {
        timeouts.WriteTotalTimeoutConstant   = MAXDWORD;
        timeouts.WriteTotalTimeoutMultiplier = 0;
    } else {
        timeouts.WriteTotalTimeoutConstant   = static_cast<DWORD>(ms);
        timeouts.WriteTotalTimeoutMultiplier = 0;
    }
    ::SetCommTimeouts(m_handle, &timeouts);
}

#else // POSIX (Linux / macOS / Android)

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <cerrno>
#include <algorithm>
#include <sys/file.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstring>

#if __has_include(<glob.h>)
#   include <glob.h>
#   define PORT_HAS_GLOB 1
#else
#   define PORT_HAS_GLOB 0
#endif

namespace {

speed_t baudrate_to_posix(SerialPort::BaudRate b) {
    switch (static_cast<int>(b)) {
        case 1200:   return B1200;
        case 2400:   return B2400;
        case 4800:   return B4800;
        case 9600:   return B9600;
        case 19200:  return B19200;
        case 38400:  return B38400;
        case 57600:  return B57600;
        case 115200: return B115200;
        case 230400: return B230400;
        case 460800: return B460800;
        case 921600: return B921600;
#if defined(B250000)
        case 250000: return B250000;
#endif
#if defined(B500000)
        case 500000: return B500000;
#endif
#if defined(B1000000)
        case 1000000: return B1000000;
#endif
#if defined(B1500000)
        case 1500000: return B1500000;
#endif
#if defined(B2000000)
        case 2000000: return B2000000;
#endif
#if defined(B3000000)
        case 3000000: return B3000000;
#endif
        default:     return B115200;
    }
}

} // namespace

bool SerialPortInfo::isBusy() const {
    if (name.empty()) return false;
    int fd = ::open(name.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return errno == EBUSY;
    }
    int rc = ::flock(fd, LOCK_EX | LOCK_NB);
    if (rc != 0) {
        ::close(fd);
        return true;
    }
#ifdef F_SETLK
    struct flock fl{};
    fl.l_type   = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start  = 0;
    fl.l_len    = 0;
    if (::fcntl(fd, F_SETLK, &fl) != 0) {
        ::close(fd);
        return true;
    }
#endif
    ::close(fd);
    return false;
}

std::vector<SerialPortInfo> SerialPort::listAvailablePorts() {
    std::vector<SerialPortInfo> result;
    const char *patterns[] = {
#if defined(__ANDROID__)
        "/dev/ttyHS*",       // Qualcomm QTI UART — 高通 QTI UART
        "/dev/ttyMT*",       // MediaTek UART — 联发科 UART
        "/dev/ttyUF*",       // Android Things / UART — Android Things / UART
        "/dev/ttyMSM*",      // Qualcomm MSM — 高通 MSM
        "/dev/ttyHSIC*",     // Qualcomm HSIC — 高通 HSIC
        "/dev/ttyGS*",       // USB gadget (ADB) — USB gadget 接口 (ADB)
        "/dev/ttyCPM*",      // Qualcomm CPM — 高通 CPM
        "/dev/ttySPI*",      // SPI based UART — SPI 接口 UART
        "/dev/ttyHSL*",      // Qualcomm HSL — 高通 HSL
        "/dev/tts/*",        // Android virtual serial — Android 虚拟串口
        "/dev/ttyS*",        // Standard 8250/16550 — 标准 8250/16550 串口
        "/dev/ttyUSB*",      // USB serial devices — USB 串口设备
        "/dev/ttyACM*",      // USB ACM devices — USB ACM 设备
        "/dev/ttyAMA*",      // Raspberry Pi / ARM AMBA — 树莓派 / ARM AMBA
#else
        "/dev/ttyS*",
        "/dev/ttyUSB*",
        "/dev/ttyACM*",
        "/dev/ttyAMA*",
        "/dev/cu.*",
        "/dev/tty.*",
#endif
    };

#if PORT_HAS_GLOB
    for (const char *pat : patterns) {
        glob_t gl{};
        if (::glob(pat, GLOB_NOSORT, nullptr, &gl) == 0) {
            for (std::size_t i = 0; i < gl.gl_pathc; ++i) {
                struct stat st{};
                if (::stat(gl.gl_pathv[i], &st) == 0 && S_ISCHR(st.st_mode))
                    result.emplace_back(gl.gl_pathv[i]);
            }
            ::globfree(&gl);
        }
    }
#else
    auto scan_prefix = [&](const char *pattern) {
        std::string pat = pattern;
        if (!pat.empty() && pat.back() == '*') pat.pop_back();

        auto last_slash = pat.rfind('/');
        std::string dir_path = (last_slash != std::string::npos) ? pat.substr(0, last_slash) : std::string("/dev");
        std::string prefix = (last_slash != std::string::npos) ? pat.substr(last_slash + 1) : pat;

        DIR *dir = ::opendir(dir_path.c_str());
        if (!dir) return;
        struct dirent *ent;
        std::size_t plen = ::strlen(prefix.c_str());
        while ((ent = ::readdir(dir)) != nullptr) {
            if (plen > 0 && ::strncmp(ent->d_name, prefix.c_str(), plen) != 0) continue;
            std::string path = dir_path + "/" + ent->d_name;
            struct stat st{};
            if (::stat(path.c_str(), &st) == 0 && S_ISCHR(st.st_mode))
                result.emplace_back(path);
        }
        ::closedir(dir);
    };
    for (const char *pat : patterns) {
        scan_prefix(pat);
    }
#endif

    std::sort(result.begin(), result.end(),
              [](const SerialPortInfo &a, const SerialPortInfo &b) {
                  return a.name < b.name;
              });
    result.erase(std::unique(result.begin(), result.end(),
                             [](const SerialPortInfo &a, const SerialPortInfo &b) {
                                 return a.name == b.name;
                             }),
                 result.end());
    return result;
}

SerialPort::SerialPort() = default;

SerialPort::~SerialPort() {
    close();
}

bool SerialPort::open() {
    if (m_is_open) return true;

    m_fd = ::open(m_port_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) return false;

    termios tty{};
    if (::tcgetattr(m_fd, &tty) != 0) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    cfsetospeed(&tty, baudrate_to_posix(m_baud_rate));
    cfsetispeed(&tty, baudrate_to_posix(m_baud_rate));

    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;

    switch (m_byte_size) {
        case 5: tty.c_cflag |= CS5; break;
        case 6: tty.c_cflag |= CS6; break;
        case 7: tty.c_cflag |= CS7; break;
        default: tty.c_cflag |= CS8; break;
    }

    if (m_parity != Parity::None) {
        tty.c_cflag |= PARENB;
        if (m_parity == Parity::Odd)
            tty.c_cflag |= PARODD;
        else
            tty.c_cflag &= ~PARODD;
    }

    if (m_stop_bits == StopBits::Two)
        tty.c_cflag |= CSTOPB;

    tty.c_cflag |= CLOCAL | CREAD;
#ifdef CRTSCTS
    tty.c_cflag &= ~CRTSCTS;
#endif

    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_oflag &= ~(OPOST | ONLCR);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 0;

    if (::tcsetattr(m_fd, TCSANOW, &tty) != 0) {
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    ::tcflush(m_fd, TCIOFLUSH);

    m_is_open = true;
    return true;
}

void SerialPort::close() {
    if (m_fd >= 0) {
        ::close(m_fd);
        m_fd = -1;
    }
    m_is_open = false;
}

std::size_t SerialPort::send(const void *data, std::size_t len) {
    if (!m_is_open || m_fd < 0 || !data || len == 0) return 0;

    const auto *p = reinterpret_cast<const std::uint8_t *>(data);
    std::size_t total_written = 0;
    while (total_written < len) {
        if (m_write_blocking) {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(m_fd, &wfds);

            if (m_write_timeout_ms > 0) {
                timeval tv{};
                tv.tv_sec  = static_cast<time_t>(m_write_timeout_ms / 1000);
                tv.tv_usec = static_cast<suseconds_t>((m_write_timeout_ms % 1000) * 1000);
                int ret = ::select(m_fd + 1, nullptr, &wfds, nullptr, &tv);
                if (ret <= 0) return total_written;
            } else {
                int ret = ::select(m_fd + 1, nullptr, &wfds, nullptr, nullptr);
                if (ret <= 0) return total_written;
            }
        }

        ssize_t n = ::write(m_fd, p + total_written, len - total_written);
        if (n > 0) {
            total_written += static_cast<std::size_t>(n);
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            if (m_write_blocking) continue;
            return total_written;
        } else {
            break;
        }
    }
    return total_written;
}

std::size_t SerialPort::receive(void *buf, std::size_t max_size) {
    if (!m_is_open || m_fd < 0 || !buf || max_size == 0) return 0;

    if (m_read_blocking) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(m_fd, &rfds);

        if (m_read_timeout_ms > 0) {
            timeval tv{};
            tv.tv_sec  = static_cast<time_t>(m_read_timeout_ms / 1000);
            tv.tv_usec = static_cast<suseconds_t>((m_read_timeout_ms % 1000) * 1000);
            int ret = ::select(m_fd + 1, &rfds, nullptr, nullptr, &tv);
            if (ret <= 0) return 0;
        } else {
            int ret = ::select(m_fd + 1, &rfds, nullptr, nullptr, nullptr);
            if (ret <= 0) return 0;
        }
    }

    while (true) {
        ssize_t n = ::read(m_fd, buf, max_size);
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
}

void SerialPort::setPortName(const char *portName) {
    if (portName) m_port_name = portName;
}

void SerialPort::setReadTimeout(bool blocking, std::uint32_t ms) {
    m_read_blocking  = blocking;
    m_read_timeout_ms = ms;
}

void SerialPort::setWriteTimeout(bool blocking, std::uint32_t ms) {
    m_write_blocking  = blocking;
    m_write_timeout_ms = ms;
}

#endif