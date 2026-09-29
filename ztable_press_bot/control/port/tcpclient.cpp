//
// Created by 11518 on 2026/9/27.
//

#include "tcpclient.h"
#include <cstring>
#include <cstdlib>

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")
#endif

bool TcpClient::s_WsaInitialized = false;

TcpClient::TcpClient() : PortBase()
{
}

TcpClient::~TcpClient()
{
    close();
}

bool TcpClient::_ensureWsa()
{
#ifdef _WIN32
    if (s_WsaInitialized)
    {
        return true;
    }
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return false;
    }
    s_WsaInitialized = true;
#endif
    return true;
}

void TcpClient::_parseAddress(const char* address)
{
    if (address == nullptr)
    {
        m_Ip.clear();
        m_Port = 0;
        return;
    }

    std::string s(address);
    auto colonPos = s.rfind(':');
    if (colonPos == std::string::npos)
    {
        m_Ip = s;
        m_Port = 0;
        return;
    }

    m_Ip = s.substr(0, colonPos);
    m_Port = static_cast<uint16_t>(std::atoi(s.substr(colonPos + 1).c_str()));
}

void TcpClient::setPortName(const char* portName)
{
    _parseAddress(portName);
}

bool TcpClient::open()
{
    if (!_ensureWsa())
    {
        return false;
    }

    if (m_Socket != -1)
    {
        close();
    }

#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        return false;
    }
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return false;
    }
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(m_Port);

#ifdef _WIN32
    unsigned long addr = inet_addr(m_Ip.c_str());
    if (addr == INADDR_NONE)
    {
        closesocket(sock);
        return false;
    }
    serverAddr.sin_addr.s_addr = addr;
#else
    if (inet_pton(AF_INET, m_Ip.c_str(), &serverAddr.sin_addr) <= 0)
    {
        ::close(sock);
        return false;
    }
#endif

    if (::connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0)
    {
#ifdef _WIN32
        closesocket(sock);
#else
        ::close(sock);
#endif
        return false;
    }

    m_Socket = static_cast<int>(sock);
    return true;
}

void TcpClient::close()
{
    if (m_Socket == -1)
    {
        return;
    }

#ifdef _WIN32
    closesocket(static_cast<SOCKET>(m_Socket));
#else
    ::close(m_Socket);
#endif
    m_Socket = -1;
}

int TcpClient::write(const uint8_t* data, int len)
{
    if (m_Socket == -1 || data == nullptr || len <= 0)
    {
        return 0;
    }

#ifdef _WIN32
    return static_cast<int>(send(static_cast<SOCKET>(m_Socket), reinterpret_cast<const char*>(data), len, 0));
#else
    return static_cast<int>(send(m_Socket, data, len, 0));
#endif
}

int TcpClient::read(uint8_t* data, int len)
{
    if (m_Socket == -1 || data == nullptr || len <= 0)
    {
        return 0;
    }

#ifdef _WIN32
    return static_cast<int>(recv(static_cast<SOCKET>(m_Socket), reinterpret_cast<char*>(data), len, 0));
#else
    return static_cast<int>(recv(m_Socket, data, len, 0));
#endif
}

bool TcpClient::isOpen()
{
    return m_Socket != -1;
}