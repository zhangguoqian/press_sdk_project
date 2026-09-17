//
// Created by 11518 on 2026/9/13.
//
#ifndef PRESS_BOT_PROJECT_MACHINEPRIVATE_H
#define PRESS_BOT_PROJECT_MACHINEPRIVATE_H

#include "port/portbase.h"
#include "machinedata.h"
#include <memory>
#include <vector>
#include <cstdint>
#include <queue>
#include <thread>
#include <mutex>
#include <unordered_map>


class MachinePrivate
{
public:
    MachinePrivate();
    ~MachinePrivate();

    bool connect(const char* portName);
    void disconnect() const;
    bool isConnected() const;
    uint64_t getMachineRegisterNo() const;
    void setCommand(uint16_t command, const std::vector<uint8_t>& data);
    std::vector<uint8_t> getResponse() const;
    int runCommand();
    int runCommand(uint16_t command, const std::vector<uint8_t>& data);
    int runCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData);

    void run();
    void stop();
    bool isRunning() const;
    void addCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData);

    static std::vector<std::string> getPortList();
    const char *getLastErrorInfo();

    void registerDataInterface(MachineDataInterface *dataInterface);
    void unregisterDataInterface();

private:
    std::vector<uint8_t> _combined_send_data();
    int _is_valid_response_data(std::vector<uint8_t>& response);
    char m_ErrorInfo[128] = {0};

    std::unique_ptr<PortBase> mpPort = nullptr;
    uint64_t m_RegisterNo = 0;
    uint16_t m_Command = 0;
    std::vector<uint8_t> m_CommandData = {};
    std::vector<uint8_t> m_ResponseData = {};

    std::unique_ptr<std::thread> mpRunThread = nullptr;
    std::mutex m_Mutex;


    std::atomic<bool> m_IsRunning = false;
    std::queue<std::pair<uint16_t, std::vector<uint8_t>>> m_CommandQueue;

    MachineData m_MachineData;

    MachineDataInterface* mpMachineDataInterface = nullptr;
    std::mutex m_MachineDataMutex;

    std::pair<uint16_t,std::vector<uint8_t>> m_ResponsePair; // 命令 -> 响应数据
    std::mutex m_ResponseMapMutex;
};

#endif