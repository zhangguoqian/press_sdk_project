//
// Created by 11518 on 2026/9/13.
//
#include "pressprivate.h"
#include "press.hpp"
#include "port/serialport.h"
#include "port/tcpsocket.h"
#include "common/unity.h"
#include <cstring>
#include <thread>
#include <iostream>
#include <cassert>

#ifdef _WIN32
#   include <winsock2.h>
#else
#   include <arpa/inet.h>
#endif

namespace {
constexpr uint64_t WAIT_MILLI_SECONDS = 300;
constexpr int CMD_LEN = 14;

const char* errorToString(MachineError err)
{
    switch (err)
    {
    case MachineError::None:                return "No error";
    case MachineError::PortNotOpen:         return "Port is not open";
    case MachineError::WriteFailed:         return "Failed to write to port";
    case MachineError::ResponseTooShort:    return "Response data is too short";
    case MachineError::ChecksumMismatch:    return "Checksum mismatch";
    case MachineError::RegisterNoMismatch: return "Register number mismatch";
    case MachineError::CommandMismatch:     return "Command mismatch";
    case MachineError::Timeout:             return "Request timeout";
    case MachineError::Stopped:             return "Machine has stopped";
    case MachineError::ConnectFailed:       return "Failed to connect port";
    case MachineError::JsonParseFailed:     return "Failed to parse JSON data";
    case MachineError::OutSignalBlocked:    return "Output signal blocked";
    case MachineError::ImmediateSignalBlocked: return "Immediate signal blocked";
    case MachineError::StateUnchanged:      return "State unchanged";
    case MachineError::UnknownError:        return "Unknown error";
    default:                                return "Unknown error";
    }
}
} // namespace

PressPrivate::PressPrivate() = default;

PressPrivate::~PressPrivate()
{
    stop();
}

bool PressPrivate::connect(const char* portName, PortType portType)
{
    if (portName == nullptr || std::strlen(portName) == 0)
    {
        _setError(MachineError::ConnectFailed);
        return false;
    }

    if (mpPort != nullptr)
    {
        _stopScheduler();
        mpPort->close();
        mpPort.reset();
        m_RegisterNo.store(0);
    }

    if (portType == SerialPortType)
    {
        mpPort = std::make_unique<SerialPort>();
    }
    else if (portType == TcpSocketPortType)
    {
        mpPort = std::make_unique<TcpSocket>();
    }
    else
    {
        _setError(MachineError::ConnectFailed);
        return false;
    }

    mpPort->setPortName(portName);
    bool ret = mpPort->open();
    if (!ret)
    {
        _setError(MachineError::ConnectFailed);
        mpPort.reset();
        return false;
    }
    _startScheduler();
    return true;
}

void PressPrivate::disconnect()
{
    _stopScheduler();
    if (mpPort != nullptr)
    {
        mpPort->close();
        mpPort.reset();
    }
}

bool PressPrivate::isConnected() const
{
    if (mpPort == nullptr)
    {
        return false;
    }
    return mpPort->isOpen();
}

uint64_t PressPrivate::getMachineRegisterNo() const
{
    return m_RegisterNo;
}

bool PressPrivate::getReadOnlyData(ReadOnlyData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_ROD_JSON_V, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onReadOnlyData(static_cast<int>(m_LastError.load()), m_RegisterNo.load(), ReadOnlyData{});
        }
        return false;
    }
    int jsonErr = m_JsonData.jsonToReadOnlyData(responseListData);
    if (0 != jsonErr)
    {
        std::cerr << "jsonToReadOnlyData failed at field code: " << jsonErr << std::endl;
        _setError(MachineError::JsonParseFailed);
        if (auto iface = _safeGetInterface())
        {
            iface->onReadOnlyData(static_cast<int>(MachineError::JsonParseFailed), m_RegisterNo.load(), ReadOnlyData{});
        }
        return false;
    }
    data = m_JsonData.getReadOnlyData();
    if (auto iface = _safeGetInterface())
    {
        iface->onReadOnlyData(0, m_RegisterNo.load(), data);
    }
    return true;
}

bool PressPrivate::getPressData(PressData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_PD_JSON_V, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onPressData(static_cast<int>(m_LastError.load()), PressData{});
        }
        return false;
    }
    int jsonErr = m_JsonData.jsonToPressData(responseListData);
    if (0 != jsonErr)
    {
        std::cerr << "jsonToPressData failed at field code: " << jsonErr << std::endl;
        _setError(MachineError::JsonParseFailed);
        if (auto iface = _safeGetInterface())
        {
            iface->onPressData(static_cast<int>(MachineError::JsonParseFailed), PressData{});
        }
        return false;
    }
    data = m_JsonData.getPressData();
    if (auto iface = _safeGetInterface())
    {
        iface->onPressData(0, data);
    }
    return true;
}

bool PressPrivate::setPressData(const PressData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    std::vector<uint8_t> commandListData;
    commandListData.emplace_back(static_cast<uint8_t>(isCompressed));
    std::vector<uint8_t> tempDataList = toFrameData(data);
    commandListData.insert(commandListData.end(), tempDataList.begin(), tempDataList.end());
    if (!_requestCommand(SET_PD_JSON_V, commandListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_PD_JSON_V, responseListData);
        return false;
    }
    return _handleSetResponse(SET_PD_JSON_V, responseListData);
}

bool PressPrivate::getRealTimeData(RealTimeData& data, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_RT_JSON_V, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onRealTimeData(static_cast<int>(m_LastError.load()), RealTimeData{});
        }
        return false;
    }
    int jsonErr = m_JsonData.jsonToRealTimeData(responseListData);
    if (0 != jsonErr)
    {
        std::cerr << "jsonToRealTimeData failed at field code: " << jsonErr << std::endl;
        _setError(MachineError::JsonParseFailed);
        if (auto iface = _safeGetInterface())
        {
            iface->onRealTimeData(static_cast<int>(MachineError::JsonParseFailed), RealTimeData{});
        }
        return false;
    }
    data = m_JsonData.getRealTimeData();
    if (auto iface = _safeGetInterface())
    {
        iface->onRealTimeData(0, data);
    }
    return true;
}

bool PressPrivate::setPressing(bool isPressing)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_PRESS_V, {static_cast<uint8_t>(isPressing)}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_PRESS_V, responseListData);
        return false;
    }
    return _handleSetResponse(SET_PRESS_V, responseListData);
}

bool PressPrivate::setDemolding(bool isDemolding)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_DEMOLD_V, {static_cast<uint8_t>(isDemolding)}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_DEMOLD_V, responseListData);
        return false;
    }
    return _handleSetResponse(SET_DEMOLD_V, responseListData);
}

bool PressPrivate::_handleSetResponse(uint16_t cmd, const std::vector<uint8_t>& responseListData)
{
    if (responseListData.size() == 1)
    {
        switch (responseListData[0])
        {
        case 0x00: return true;
        case 0x01:
            _setError(MachineError::OutSignalBlocked);
            if (auto iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        case 0x02:
            _setError(MachineError::ImmediateSignalBlocked);
            if (auto iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        case 0x03:
            _setError(MachineError::StateUnchanged);
            if (auto iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        default:
            _setError(MachineError::UnknownError);
            if (auto iface = _safeGetInterface()) iface->onError(cmd, responseListData);
            return false;
        }
    }
    if (auto iface = _safeGetInterface()) iface->onError(cmd, responseListData);
    return false;
}

std::vector<uint8_t> PressPrivate::_buildSendFrame(const FrameData& frameData)
{
    assert(frameData.m_FrameDataList.size() <= 0xFFFF && "Frame data length exceeds uint16_t range");

    std::vector<uint8_t> data;
    data.reserve(CMD_LEN + frameData.m_FrameDataList.size());

    // 注册号按大端（网络字节序）写入帧
    uint64_t registerNo = m_RegisterNo.load();
    uint8_t regBytes[8] = {
        static_cast<uint8_t>(registerNo >> 56),
        static_cast<uint8_t>(registerNo >> 48),
        static_cast<uint8_t>(registerNo >> 40),
        static_cast<uint8_t>(registerNo >> 32),
        static_cast<uint8_t>(registerNo >> 24),
        static_cast<uint8_t>(registerNo >> 16),
        static_cast<uint8_t>(registerNo >> 8),
        static_cast<uint8_t>(registerNo)
    };
    data.insert(data.end(), regBytes, regBytes + 8);

    uint16_t cmd = frameData.m_Command;
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&cmd),
                reinterpret_cast<const uint8_t*>(&cmd) + sizeof(cmd));

    uint16_t len = htons(static_cast<uint16_t>(frameData.m_FrameDataList.size()));
    data.insert(data.end(), reinterpret_cast<const uint8_t*>(&len),
                reinterpret_cast<const uint8_t*>(&len) + sizeof(len));

    data.insert(data.end(), frameData.m_FrameDataList.begin(), frameData.m_FrameDataList.end());

    uint8_t checkSum = Unity::getChecksum(data);
    data.emplace_back(checkSum);
    return data;
}

MachineError PressPrivate::_parseResponseFrame(const std::vector<uint8_t>& response, uint16_t command, FrameData& out)
{
    out.m_Command = command;

    if (response.size() < CMD_LEN)
    {
        return MachineError::ResponseTooShort;
    }
    if (response.back() != Unity::getChecksum(response.begin(), response.end() - 1))
    {
        return MachineError::ChecksumMismatch;
    }

    // 注册号按大端（网络字节序）存储，手动组装为主机字节序
    const uint8_t* p = response.data();
    uint64_t readRegisterNo =
        (static_cast<uint64_t>(p[0]) << 56) |
        (static_cast<uint64_t>(p[1]) << 48) |
        (static_cast<uint64_t>(p[2]) << 40) |
        (static_cast<uint64_t>(p[3]) << 32) |
        (static_cast<uint64_t>(p[4]) << 24) |
        (static_cast<uint64_t>(p[5]) << 16) |
        (static_cast<uint64_t>(p[6]) << 8)  |
        (static_cast<uint64_t>(p[7]));
    uint64_t expectedNo = m_RegisterNo.load();
    if (expectedNo == 0)
    {
        m_RegisterNo.store(readRegisterNo);
    }
    else if (readRegisterNo != expectedNo)
    {
        return MachineError::RegisterNoMismatch;
    }

    uint16_t readCommand = 0;
    std::memcpy(&readCommand, response.data() + 8, sizeof(readCommand));
    if (readCommand != command)
    {
        return MachineError::CommandMismatch;
    }

    uint16_t readDataLen = 0;
    std::memcpy(&readDataLen, response.data() + 10, sizeof(readDataLen));
    readDataLen = ntohs(readDataLen);

    // 边界检查：12 字节头 + readDataLen 数据 + 1 字节校验和不能超过响应长度
    if (static_cast<size_t>(12) + readDataLen + 1 > response.size())
    {
        return MachineError::ResponseTooShort;
    }

    // SET 位检测：readCommand 是网络字节序，需先转主机序再判断
    if (IS_SET_CMD(ntohs(readCommand)))
    {
        out.m_FrameDataList.push_back(response[12]);
    }
    else
    {
        out.m_FrameDataList.assign(response.begin() + 12, response.begin() + 12 + readDataLen);
    }

    return MachineError::None;
}

bool PressPrivate::_requestCommand(uint16_t command, const std::vector<uint8_t>& listData, std::vector<uint8_t>& responseListData)
{
    std::lock_guard<std::mutex> serialLock(m_RequestSerialMutex);

    _setError(MachineError::None);

    if (!m_IsRunning.load())
    {
        _setError(MachineError::Stopped);
        return false;
    }

    {
        std::lock_guard<std::mutex> rlock(m_ResponseMutex);
        while (!m_ResponseQueue.empty())
        {
            m_ResponseQueue.pop();
        }
    }

    uint64_t mySeq = ++m_RequestSeq;
    FrameData request;
    request.m_Command = htons(command);
    request.m_Seq = mySeq;
    request.m_FrameDataList = listData;

    {
        std::lock_guard<std::mutex> lock(m_CommandMutex);
        m_CommandQueue.push(std::move(request));
    }
    m_CommandCond.notify_one();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_MILLI_SECONDS);
    while (true)
    {
        std::unique_lock<std::mutex> lock(m_ResponseMutex);
        auto remaining = deadline - std::chrono::steady_clock::now();
        if (remaining <= std::chrono::milliseconds(0))
        {
            _setError(MachineError::Timeout);
            return false;
        }
        if (!m_ResponseCond.wait_for(lock, remaining, [this] {
            return !m_ResponseQueue.empty() || !m_IsRunning.load();
        }))
        {
            _setError(MachineError::Timeout);
            return false;
        }
        if (!m_IsRunning.load() && m_ResponseQueue.empty())
        {
            _setError(MachineError::Stopped);
            return false;
        }

        // 取出队列中所有响应，找匹配序号的；丢弃迟到的旧响应
        while (!m_ResponseQueue.empty())
        {
            FrameData resp = std::move(m_ResponseQueue.front());
            m_ResponseQueue.pop();
            if (resp.m_Seq == mySeq)
            {
                if (m_LastError.load() != MachineError::None)
                {
                    return false;
                }
                responseListData = std::move(resp.m_FrameDataList);
                return true;
            }
            // 序号不匹配，是迟到的旧响应，丢弃继续
        }
        // 队列中没有匹配的响应，继续循环等待
    }
}

void PressPrivate::_startScheduler()
{
    if (mpRunThread != nullptr)
    {
        return;
    }
    m_IsRunning.store(true);
    mpRunThread = std::make_unique<std::thread>([this]
    {
        while (m_IsRunning.load())
        {
            FrameData request;
            {
                std::unique_lock<std::mutex> lock(m_CommandMutex);
                m_CommandCond.wait(lock, [this] {
                    return !m_CommandQueue.empty() || !m_IsRunning.load();
                });
                if (!m_IsRunning.load() && m_CommandQueue.empty())
                {
                    break;
                }
                request = std::move(m_CommandQueue.front());
                m_CommandQueue.pop();
            }

            if (mpPort == nullptr)
            {
                _setError(MachineError::PortNotOpen);
                FrameData flag;
                flag.m_Seq = request.m_Seq;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            std::vector<uint8_t> sendData = _buildSendFrame(request);
            auto writeRet = mpPort->write(sendData.data(), sendData.size());
            if (writeRet != sendData.size())
            {
                _setError(MachineError::WriteFailed);
                FrameData flag;
                flag.m_Seq = request.m_Seq;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            std::vector<uint8_t> rawResponse;
            rawResponse.reserve(CMD_LEN);
            auto readDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(WAIT_MILLI_SECONDS);
            while (std::chrono::steady_clock::now() < readDeadline)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                uint8_t tmp[256];
                auto readRet = mpPort->read(tmp, sizeof(tmp));
                if (readRet > 0)
                {
                    rawResponse.insert(rawResponse.end(), tmp, tmp + readRet);
                }
                if (rawResponse.size() >= CMD_LEN)
                {
                    uint16_t len = (static_cast<uint16_t>(rawResponse[10]) << 8) | rawResponse[11];
                    if (rawResponse.size() >= CMD_LEN + len)
                    {
                        break;
                    }
                }
            }
            FrameData parsed;
            parsed.m_Seq = request.m_Seq;
            MachineError parseErr = _parseResponseFrame(rawResponse, request.m_Command, parsed);
            if (parseErr != MachineError::None)
            {
                _setError(parseErr);
                FrameData flag;
                flag.m_Seq = request.m_Seq;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            _setError(MachineError::None);
            {
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(parsed));
            }
            m_ResponseCond.notify_one();
        }
    });
}

void PressPrivate::_stopScheduler()
{
    m_IsRunning.store(false);
    m_CommandCond.notify_all();
    m_ResponseCond.notify_all();
    if (mpRunThread && mpRunThread->joinable())
    {
        mpRunThread->join();
    }
    mpRunThread.reset();
}

void PressPrivate::run()
{
    _startScheduler();
}

void PressPrivate::stop()
{
    _stopScheduler();
    disconnect();
}

bool PressPrivate::isRunning() const
{
    return m_IsRunning.load();
}

std::vector<std::string> PressPrivate::getPortList()
{
    auto list = SerialPort::listAvailablePorts();
    std::vector<std::string> portList;
    portList.reserve(list.size());
    for (const auto& port : list)
        portList.emplace_back(port.name);
    return portList;
}

void PressPrivate::_setError(MachineError err)
{
    m_LastError.store(err);
}

const char* PressPrivate::getLastErrorInfo() const
{
    return errorToString(m_LastError.load());
}

void PressPrivate::registerDataInterface(PressDataInterface* dataInterface)
{
    if (dataInterface == nullptr)
    {
        unregisterDataInterface();
        return;
    }
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    // 使用空删除器：shared_ptr 仅用于线程安全的引用计数，不管理对象生命周期
    mpMachineDataInterface = std::shared_ptr<PressDataInterface>(dataInterface, [](PressDataInterface*){});
}

void PressPrivate::unregisterDataInterface()
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    mpMachineDataInterface.reset();
}

std::shared_ptr<PressDataInterface> PressPrivate::_safeGetInterface()
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    return mpMachineDataInterface;
}

const JsonData& PressPrivate::getMachineData() const
{
    return m_JsonData;
}