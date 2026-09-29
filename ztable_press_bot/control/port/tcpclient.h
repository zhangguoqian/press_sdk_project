//
// Created by 11518 on 2026/9/27.
//

#ifndef ZTABLE_PRESS_BOT_TCPCLIENT_H
#define ZTABLE_PRESS_BOT_TCPCLIENT_H

#include "portbase.h"
#include <string>
#include <cstdint>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class TcpClient : public PortBase
{
public:
    TcpClient();
    ~TcpClient() override;

    void setPortName(const char* portName) override;
    bool open() override;
    void close() override;
    int write(const uint8_t* data, int len) override;
    int read(uint8_t* data, int len) override;
    bool isOpen() override;

private:
    std::string m_Ip;
    uint16_t m_Port{0};
    int m_Socket{-1};

    static bool s_WsaInitialized;
    static bool _ensureWsa();
    void _parseAddress(const char* address);
};


#endif //ZTABLE_PRESS_BOT_TCPCLIENT_H