//
// Created by 11518 on 2026/9/13.
//
#include "machineprivate.h"
#include "machine.h"
#include "port/serialport.h"
#include "common/unity.h"
#include <winsock2.h>
#include <iostream>
#include <thread>

constexpr uint64_t ONE_MILLION_SECOND    = 1000;
constexpr uint64_t HALF_MILLION_SECONDS  = 500;

MachinePrivate::MachinePrivate()
{
}

MachinePrivate::~MachinePrivate()
{
}

bool MachinePrivate::connect(const char* portName)
{
    if (nullptr == mpPort)
    {
        mpPort = std::make_unique<Serialport>();
    }
    mpPort->setPortName(portName);
    return mpPort->open();
}

void MachinePrivate::disconnect() const
{
    if (nullptr != mpPort)
    {
        mpPort->close();
    }
}

bool MachinePrivate::isConnected() const
{
    if (nullptr == mpPort)
    {
        return false;
    }
    return mpPort->isOpen();
}


uint64_t MachinePrivate::getMachineRegisterNo() const
{
    return m_RegisterNo;
}

void MachinePrivate::setCommand(uint16_t command, const std::vector<uint8_t>& data)
{
    m_Command = command;
    m_CommandData = data;
}

std::vector<uint8_t> MachinePrivate::getResponse() const
{
    return m_ResponseData;
}

std::vector<uint8_t> MachinePrivate::_combined_send_data()
{
    std::vector<uint8_t> command;
    std::vector<uint8_t> data;
    uint16_t commandNet = htons(m_Command);
    uint16_t commandDataLen = m_CommandData.size();
    commandDataLen = htons(commandDataLen);

    command.insert(command.begin(), reinterpret_cast<uint8_t*>(&m_RegisterNo),
                   reinterpret_cast<uint8_t*>(&m_RegisterNo) + sizeof(m_RegisterNo));
    command.insert(command.end(), reinterpret_cast<uint8_t*>(&commandNet),
                   reinterpret_cast<uint8_t*>(&commandNet) + sizeof(commandNet));
    command.insert(command.end(), reinterpret_cast<uint8_t*>(&commandDataLen),
                   reinterpret_cast<uint8_t*>(&commandDataLen) + sizeof(commandDataLen));
    command.insert(command.end(), m_CommandData.begin(), m_CommandData.end());
    command.push_back(Unity::getChecksum(command));
    return command;
}

int MachinePrivate::_is_valid_response_data(std::vector<uint8_t>& response)
{
    int len = sizeof(m_RegisterNo) + sizeof(m_Command) + sizeof(uint16_t) + 1 + sizeof(uint8_t);
    if (response.size() < len)
    {
        sprintf(m_ErrorInfo, "Invalid response length: %llu < %d.", response.size(), len);
        return -3;
    }
    if (response.back() != Unity::getChecksum({response.begin(), response.end() - 1}))
    {
        sprintf(m_ErrorInfo, "Invalid checksum: %02x != %02x.", response.back(),
                Unity::getChecksum({response.begin(), response.end() - 1}));
        return -4;
    }
    uint16_t responseDataLen = (response[10] << 8) + response[11];
    response = std::vector<uint8_t>(response.begin() + 12, response.begin() + 12 + responseDataLen);
    return 0;
}

int MachinePrivate::runCommand()
{
    m_ResponseData.clear();
    if (nullptr == mpPort)
    {
        strcpy(m_ErrorInfo, "Port not connected.");
        return -1;
    }
    std::vector<uint8_t> command = _combined_send_data();
    int ret = mpPort->write(command.data(), command.size());
    if (ret != command.size())
    {
        strcpy(m_ErrorInfo, "Failed to send command.");
        return -2;
    }
    uint8_t pData[1024];
    int cycle = 6;
    int retLen = 0;
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        ret = mpPort->read(pData + retLen, 1024);
        if (ret <= 0 && cycle != 6)
        {
            break;
        }
        retLen += ret;
    }
    while (cycle-- > 0);
    std::vector<uint8_t> data = std::vector<uint8_t>(pData, pData + retLen);
    int info = _is_valid_response_data(data);
    if (0 != info)
    {
        return info;
    }
    memcpy(&m_RegisterNo, pData, sizeof(m_RegisterNo));
    m_ResponseData = std::move(data);
    return 0;
}

int MachinePrivate::runCommand(uint16_t command, const std::vector<uint8_t>& data)
{
    m_Command = command;
    m_CommandData = data;
    return runCommand();
}

int MachinePrivate::runCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData)
{
    return runCommand(commandData.first, commandData.second);
}

void MachinePrivate::run()
{
    m_CommandQueue.push(GET_ROD_JSON_NORMAL);
    m_CommandQueue.push(GET_PD_JSON_NORMAL);
    m_IsRunning.store(true);
    mpRunThread = std::make_unique<std::thread>([this]
    {
        std::pair<uint16_t, std::vector<uint8_t>> cmd = GET_RT_JSON_NORMAL;
        while (m_IsRunning.load())
        {
            auto startTime = std::chrono::high_resolution_clock::now();
            cmd = GET_RT_JSON_NORMAL;
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                if (!m_CommandQueue.empty())
                {
                    cmd = m_CommandQueue.front();
                    m_CommandQueue.pop();
                }
            }
            auto runCode = runCommand(cmd);
            if (0 == runCode)
            {
                int jsonCode = 0;

                if (GET_PD_JSON_NORMAL == cmd)
                {
                    jsonCode = m_MachineData.jsonToPressData(m_ResponseData);

                    {
                        std::lock_guard<std::mutex> lock(m_MachineDataMutex);
                        if (mpMachineDataInterface != nullptr)
                        {
                            mpMachineDataInterface->onPressData(jsonCode, m_MachineData.getPressData());
                        }
                    }
                }
                else if (GET_ROD_JSON_NORMAL == cmd)
                {
                    jsonCode = m_MachineData.jsonToReadOnlyData(m_ResponseData);
                    {
                        std::lock_guard<std::mutex> lock(m_MachineDataMutex);
                        if (mpMachineDataInterface != nullptr)
                        {
                            mpMachineDataInterface->onReadOnlyData(jsonCode, m_RegisterNo, m_MachineData.getReadOnlyData());
                        }
                    }
                }
                else if (GET_RT_JSON_NORMAL == cmd)
                {
                    jsonCode = m_MachineData.jsonToRealTimeData(m_ResponseData);

                    {
                        std::lock_guard<std::mutex> lock(m_MachineDataMutex);
                        if (mpMachineDataInterface != nullptr)
                        {
                            mpMachineDataInterface->onRealTimeData(jsonCode, m_MachineData.getRealTimeData());
                        }
                    }
                }

                {
                    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
                    if (mpMachineDataInterface != nullptr)
                    {
                        mpMachineDataInterface->onDataError(cmd.first, m_ResponseData);
                    }
                }
                if (cmd.first & 0x1000)
                {
                    {
                        std::lock_guard<std::mutex> lock(m_ResponseMapMutex);
                        m_ResponsePair = {cmd.first, m_ResponseData};
                    }
                }
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
                    if (mpMachineDataInterface != nullptr)
                    {
                        mpMachineDataInterface->onCommandError(static_cast<RetCommand>(runCode), cmd.first);
                    }
                }
            }

            auto endTime = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            if (duration.count() < ONE_MILLION_SECOND)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(ONE_MILLION_SECOND - duration.count()));
            }
            else if (duration.count() > ONE_MILLION_SECOND)
            {
                {
                    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
                    if (mpMachineDataInterface != nullptr)
                    {
                        mpMachineDataInterface->onCommandPassWarningError(cmd.first);
                    }
                }
            }
        }
    });
}

void MachinePrivate::stop()
{
    m_IsRunning.store(false);
    if (mpRunThread->joinable())
    {
        mpRunThread->join();
    }
    mpRunThread.reset();
}

bool MachinePrivate::isRunning() const
{
    return m_IsRunning.load();
}

void MachinePrivate::addCommand(const std::pair<uint16_t, std::vector<uint8_t>>& commandData)
{
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        m_CommandQueue.emplace(commandData);
    }
    int out = 0xFF;
    auto now = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(HALF_MILLION_SECONDS);
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        {
            std::lock_guard<std::mutex> lock(m_ResponseMapMutex);
            if (m_ResponsePair.first == commandData.first && m_ResponsePair.second.size() > 0)
            {
                out = m_ResponsePair.second[0];
            }
        }
    }while (now > std::chrono::high_resolution_clock::now());
    m_ResponsePair = {0,{1}};
    qDebug () << "cmd out:" << out;
}

std::vector<std::string> MachinePrivate::getPortList()
{
    auto list = Serialport::getSerialPortInfo();
    std::vector<std::string> portList;
    for (auto port : list)
        portList.emplace_back(port.portName);
    return portList;
}

const char* MachinePrivate::getLastErrorInfo()
{
    return m_ErrorInfo;
}

void MachinePrivate::registerDataInterface(MachineDataInterface* dataInterface)
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    mpMachineDataInterface = dataInterface;
}

void MachinePrivate::unregisterDataInterface()
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    mpMachineDataInterface = nullptr;
}
