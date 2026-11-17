//
// Created by 11518 on 2026/9/13.
//
#include "pressprivate.h"
#include "press.hpp"
#include "port/serialport.h"
#include "port/tcpsocket.h"
#include "common/unity.h"
#include "presstype.h"
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
//! 响应数据区长度上限：超过即判定为伪帧头（设备 JSON 最大约 2KB）
constexpr uint16_t FRAME_MAX_DATA_LEN = 8192;

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

bool PressPrivate::connect(const char* portName, PressPortType portType)
{
    std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);

    if (portName == nullptr || std::strlen(portName) == 0)
    {
        _setError(MachineError::ConnectFailed);
        return false;
    }

    if (mpPort != nullptr)
    {
        _stopScheduler();
        _closePort();
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
    // 版本查询作为连接握手，失败则回滚到未连接状态
    VersionInfo versionInfo{};
    if (!getVersionInfo(versionInfo))
    {
        _stopScheduler();
        _closePort();
        return false;
    }
    return true;
}

void PressPrivate::disconnect()
{
    std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);
    _stopScheduler();
    _closePort();
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

bool PressPrivate::getReadOnlyDataJson(std::string& json, bool isCompressed)
{
    json.clear();
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_ROD_JSON_V, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onReadOnlyData(static_cast<int>(m_LastError.load()), m_RegisterNo.load(), std::string());
        }
        return false;
    }
    if (responseListData.empty())
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface())
        {
            iface->onReadOnlyData(static_cast<int>(m_LastError.load()), m_RegisterNo.load(), std::string());
        }
        return false;
    }
    json.assign(responseListData.begin(), responseListData.end());
    if (auto iface = _safeGetInterface())
    {
        iface->onReadOnlyData(0, m_RegisterNo.load(), json);
    }
    return true;
}

bool PressPrivate::getPressDataJson(std::string& json, bool isCompressed)
{
    json.clear();
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_PD_JSON_V, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onPressData(static_cast<int>(m_LastError.load()), std::string());
        }
        return false;
    }
    if (responseListData.empty())
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface())
        {
            iface->onPressData(static_cast<int>(m_LastError.load()), std::string());
        }
        return false;
    }
    json.assign(responseListData.begin(), responseListData.end());
    if (auto iface = _safeGetInterface())
    {
        iface->onPressData(0, json);
    }
    return true;
}

bool PressPrivate::setPressDataJson(const std::string& json, bool isCompressed)
{
    // payload: [压缩标志][JSON 字节...]
    std::vector<uint8_t> requestListData;
    requestListData.emplace_back(static_cast<uint8_t>(isCompressed));
    requestListData.insert(requestListData.end(), json.begin(), json.end());
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_PD_JSON_V, requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_PD_JSON_V, responseListData);
        return false;
    }
    // 下位机写成功后回显 JSON，非空即成功
    return _handleSetResponse(SET_PD_JSON_V, responseListData);
}

bool PressPrivate::getRealTimeDataJson(std::string& json, bool isCompressed)
{
    json.clear();
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_RT_JSON_V, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onRealTimeData(static_cast<int>(m_LastError.load()), std::string());
        }
        return false;
    }
    if (responseListData.empty())
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface())
        {
            iface->onRealTimeData(static_cast<int>(m_LastError.load()), std::string());
        }
        return false;
    }
    json.assign(responseListData.begin(), responseListData.end());
    if (auto iface = _safeGetInterface())
    {
        iface->onRealTimeData(0, json);
    }
    return true;
}

bool PressPrivate::getVersionInfo(VersionInfo& versionInfo)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_VERSION_V, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(GET_VERSION_V, responseListData);
        return false;
    }
    // 协议规定版本响应为 4 字节：AppNo / Major / Minor / Patch
    if (!VERSION_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(GET_VERSION_V, responseListData);
        return false;
    }
    versionInfo.m_AppNo = static_cast<AppType>(responseListData[0]);
    versionInfo.m_Major = responseListData[1];
    versionInfo.m_Minor = responseListData[2];
    versionInfo.m_Patch = responseListData[3];
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        m_JsonData.m_VersionInfo = versionInfo;
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
    // 写命令响应分两类（依据下位机 receive.cpp 实现）：
    //   控制类命令（SET_START_*/SELECT_MODEL_STATE）：返回 1 字节状态码
    //     0x00=停止成功 0x01=启动成功 0x02=立即信号阻塞 0x03=状态未变(拒绝)
    //   参数类命令（JSON/索引/单值写）：回显完整数据，非空即成功
    if (responseListData.empty())
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(cmd, responseListData);
        return false;
    }
    if (responseListData.size() == 1)
    {
        switch (responseListData[0])
        {
        case 0x00:
        case 0x01: return true;    // 停止成功 / 启动成功
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
    // 多字节回显数据：参数类写命令成功
    return true;
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

    // 命令与长度均显式按大端（网络字节序）写入，m_Command 为主机序原始值，
    // 不再依赖" htons 值 + 小端主机"的隐含假设
    uint16_t cmd = frameData.m_Command;
    data.emplace_back(static_cast<uint8_t>(cmd >> 8));
    data.emplace_back(static_cast<uint8_t>(cmd & 0xFF));

    uint16_t lenValue = static_cast<uint16_t>(frameData.m_FrameDataList.size());
    data.emplace_back(static_cast<uint8_t>(lenValue >> 8));
    data.emplace_back(static_cast<uint8_t>(lenValue & 0xFF));

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

    const uint8_t* p = response.data();

    uint16_t readDataLen = 0;
    std::memcpy(&readDataLen, p + 10, sizeof(readDataLen));
    readDataLen = ntohs(readDataLen);

    // 边界检查：12 字节头 + readDataLen 数据 + 1 字节校验和不能超过响应长度
    if (static_cast<size_t>(12) + readDataLen + 1 > response.size())
    {
        return MachineError::ResponseTooShort;
    }

    // 校验和位于数据区之后（偏移 12 + readDataLen），不能取缓冲区最后一字节：
    // 端口残留迟到数据时最后一字节可能不属于本帧
    size_t frameLen = static_cast<size_t>(12) + readDataLen;
    if (response[frameLen] != Unity::getChecksum(response.begin(), response.begin() + frameLen))
    {
        return MachineError::ChecksumMismatch;
    }

    // 注册号按大端（网络字节序）存储，手动组装为主机字节序
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
    std::memcpy(&readCommand, p + 8, sizeof(readCommand));
    // 线上为大端，转主机序后再比较与判断 SET 位（m_Command 现为主机序原始值）
    readCommand = ntohs(readCommand);
    if (readCommand != command)
    {
        return MachineError::CommandMismatch;
    }

    // SET（写）命令响应为回显数据（参数类）或 1 字节状态码（控制类），
    // 统一拷贝完整数据区，由上层 _handleSetResponse 按命令类型解释
    out.m_FrameDataList.assign(response.begin() + 12, response.begin() + 12 + readDataLen);

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
    request.m_Command = command;   // 主机序原始值，组帧时显式转大端
    request.m_Seq = mySeq;
    request.m_FrameDataList = listData;

    {
        std::lock_guard<std::mutex> lock(m_CommandMutex);
        m_CommandQueue.push(std::move(request));
    }
    m_CommandCond.notify_one();

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_RequestTimeoutMs.load());
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
                // 错误码随响应携带，避免读到其他请求（如定时器）设置的全局错误
                if (resp.m_Error != MachineError::None)
                {
                    _setError(resp.m_Error);
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
                flag.m_Error = MachineError::PortNotOpen;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            // 发送前清掉上次超时交互残留在端口缓冲中的字节，避免与本次响应错位拼接
            mpPort->flushInput();

            std::vector<uint8_t> sendData = _buildSendFrame(request);
            auto writeRet = mpPort->write(sendData.data(), sendData.size());
            if (writeRet != sendData.size())
            {
                _setError(MachineError::WriteFailed);
                FrameData flag;
                flag.m_Seq = request.m_Seq;
                flag.m_Error = MachineError::WriteFailed;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            std::vector<uint8_t> rawResponse;
            rawResponse.reserve(CMD_LEN);
            auto readDeadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_RequestTimeoutMs.load());
            bool frameReady = false;
            while (!frameReady && std::chrono::steady_clock::now() < readDeadline)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                uint8_t tmp[256];
                auto readRet = mpPort->read(tmp, sizeof(tmp));
                if (readRet > 0)
                {
                    rawResponse.insert(rawResponse.end(), tmp, tmp + readRet);
                }
                // 在缓冲中扫描完整帧（帧长 = 13 + len）：
                // 长度字段非法或校验和不符时丢弃首字节重新同步，避免错位后永久无法恢复
                while (!frameReady && rawResponse.size() >= CMD_LEN)
                {
                    uint16_t len = (static_cast<uint16_t>(rawResponse[10]) << 8) | rawResponse[11];
                    if (len > FRAME_MAX_DATA_LEN)
                    {
                        rawResponse.erase(rawResponse.begin());
                        continue;
                    }
                    size_t frameLen = static_cast<size_t>(13) + len;
                    if (rawResponse.size() < frameLen)
                    {
                        break;  // 数据不全，继续读
                    }
                    uint8_t expect = Unity::getChecksum(rawResponse.begin(), rawResponse.begin() + (frameLen - 1));
                    if (rawResponse[frameLen - 1] == expect)
                    {
                        rawResponse.resize(frameLen);  // 截掉帧后可能混入的多余字节
                        frameReady = true;
                    }
                    else
                    {
                        rawResponse.erase(rawResponse.begin());
                    }
                }
            }
            if (!frameReady)
            {
                _setError(MachineError::Timeout);
                FrameData flag;
                flag.m_Seq = request.m_Seq;
                flag.m_Error = MachineError::Timeout;
                std::lock_guard<std::mutex> rlock(m_ResponseMutex);
                m_ResponseQueue.push(std::move(flag));
                m_ResponseCond.notify_one();
                continue;
            }

            FrameData parsed;
            parsed.m_Seq = request.m_Seq;
            MachineError parseErr = _parseResponseFrame(rawResponse, request.m_Command, parsed);
            if (parseErr != MachineError::None)
            {
                _setError(parseErr);
                FrameData flag;
                flag.m_Seq = request.m_Seq;
                flag.m_Error = parseErr;
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
    // 调度线程重启（如重连）后，若配置过定时周期则恢复实时数据定时采集
    if (m_TimerInterval.load() > 0)
    {
        _startRealTimeTimer(m_TimerInterval.load());
    }
}

void PressPrivate::_stopScheduler()
{
    _stopRealTimeTimer();
    m_IsRunning.store(false);
    m_CommandCond.notify_all();
    m_ResponseCond.notify_all();
    if (mpRunThread && mpRunThread->joinable())
    {
        mpRunThread->join();
    }
    mpRunThread.reset();
}


void PressPrivate::_closePort()
{
    if (mpPort != nullptr)
    {
        mpPort->close();
        mpPort.reset();
    }
    // 注册号随连接失效，防止下次连接误用旧机器的注册号
    m_RegisterNo.store(0);
}

void PressPrivate::run()
{
    std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);
    _startScheduler();
}

void PressPrivate::stop()
{
    std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);
    _stopScheduler();
    _closePort();
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

void PressPrivate::registerDataInterface(PressDataInterface* dataInterface, int intervalSeconds)
{
    std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);

    if (dataInterface == nullptr)
    {
        _unregisterInterfaceLocked();
        return;
    }
    {
        std::lock_guard<std::mutex> lock(m_MachineDataMutex);
        // 使用空删除器：shared_ptr 仅用于线程安全的引用计数，不管理对象生命周期
        mpMachineDataInterface = std::shared_ptr<PressDataInterface>(dataInterface, [](PressDataInterface*){});
    }
    // intervalSeconds <= 0 时不启动定时线程（与 press.hpp 文档一致），
    // 避免 wait_for 非正周期立即超时导致高频空转轰炸设备
    if (intervalSeconds > 0)
    {
        _startRealTimeTimer(intervalSeconds);
    }
    else
    {
        _stopRealTimeTimer();
        m_TimerInterval.store(0);
    }
}

void PressPrivate::_unregisterInterfaceLocked()
{
    // 回调已注销，定时采集失去意义：停止定时线程并清除周期配置
    _stopRealTimeTimer();
    m_TimerInterval.store(0);
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    mpMachineDataInterface.reset();
}

void PressPrivate::unregisterDataInterface()
{
    std::lock_guard<std::mutex> lifecycleLock(m_LifecycleMutex);
    _unregisterInterfaceLocked();
}

std::shared_ptr<PressDataInterface> PressPrivate::_safeGetInterface()
{
    std::lock_guard<std::mutex> lock(m_MachineDataMutex);
    return mpMachineDataInterface;
}

JsonData PressPrivate::getMachineData() const
{
    // 返回加锁拷贝，避免与定时线程的解析写入产生数据竞争
    std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
    return m_JsonData;
}

bool PressPrivate::getSerialNumber(std::string& serialNumber)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_SERIAL_NUMBER_V, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(GET_SERIAL_NUMBER_V, responseListData);
        return false;
    }
    // 协议规定序列号最长 32 字节
    if (!SERIAL_NUMBER_TYPE_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(GET_SERIAL_NUMBER_V, responseListData);
        return false;
    }
    serialNumber.assign(responseListData.begin(), responseListData.end());
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        std::strncpy(m_JsonData.m_ReadOnlyData.m_SerialNumber, serialNumber.c_str(),
                     sizeof(m_JsonData.m_ReadOnlyData.m_SerialNumber) - 1);
        m_JsonData.m_ReadOnlyData.m_SerialNumber[sizeof(m_JsonData.m_ReadOnlyData.m_SerialNumber) - 1] = '\0';
    }
    return true;
}

void PressPrivate::setRequestTimeoutMs(std::uint32_t ms)
{
    // 0 恢复默认；上限 60 秒，防止误传导致请求近乎永久阻塞
    if (ms == 0)
    {
        ms = static_cast<std::uint32_t>(WAIT_MILLI_SECONDS);
    }
    m_RequestTimeoutMs.store(ms > 60000u ? 60000u : ms);
}

std::uint32_t PressPrivate::requestTimeoutMs() const
{
    return m_RequestTimeoutMs.load();
}

void PressPrivate::_startRealTimeTimer(int intervalSeconds)
{
    // 若已有定时线程则先停止，再按新周期重启
    _stopRealTimeTimer();
    m_TimerInterval.store(intervalSeconds);
    m_TimerRunning.store(true);
    mpRealTimeTimerThread = std::make_unique<std::thread>([this]
    {
        while (m_TimerRunning.load() && m_IsRunning.load())
        {
            {
                std::unique_lock<std::mutex> lock(m_TimerMutex);
                m_TimerCond.wait_for(lock, std::chrono::seconds(m_TimerInterval.load()), [this] {
                    return !m_TimerRunning.load() || !m_IsRunning.load();
                });
            }
            if (!m_TimerRunning.load() || !m_IsRunning.load())
            {
                break;
            }

            std::string realTimeJson;
            if (getRealTimeDataJson(realTimeJson))
            {
                m_TimerConsecutiveFailures.store(0);
            }
            else if (m_TimerConsecutiveFailures.fetch_add(1) + 1 == TIMER_FAIL_REPORT_THRESHOLD)
            {
                // 连续失败达到阈值：上报一次连接异常，避免设备拔线后上层无感知
                if (auto iface = _safeGetInterface())
                {
                    iface->onError(GET_RT_JSON_V, {});
                }
            }
        }
    });
}

void PressPrivate::_stopRealTimeTimer()
{
    m_TimerRunning.store(false);
    m_TimerCond.notify_all();
    if (mpRealTimeTimerThread && mpRealTimeTimerThread->joinable())
    {
        mpRealTimeTimerThread->join();
    }
    mpRealTimeTimerThread.reset();
}

// ==================== 协议数据辅助 ====================
// 协议约定（依据下位机 receive.cpp / data 实现整理）：
//   1. 数值编码为 4 字节本机字节序（小端 IEEE754 / int32，union 直接拷贝）
//   2. 读命令请求 payload 固定 1 字节（索引 / 语言代码 / 压缩标志 / 哑元 0）
//   3. 写命令响应分两类：
//      - 控制类（SET_START_*/SELECT_MODEL_STATE）：1 字节状态码
//      - 参数类（索引/单值/JSON/系统写）：回显写入数据，非空即成功
namespace
{
float _readHostFloat(const std::vector<uint8_t>& data, size_t offset)
{
    float value = 0.0f;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

int32_t _readHostInt32(const std::vector<uint8_t>& data, size_t offset)
{
    int32_t value = 0;
    std::memcpy(&value, data.data() + offset, sizeof(value));
    return value;
}

void _appendHostFloat(std::vector<uint8_t>& data, float value)
{
    uint8_t bytes[sizeof(float)];
    std::memcpy(bytes, &value, sizeof(bytes));
    data.insert(data.end(), bytes, bytes + sizeof(bytes));
}

void _appendHostInt32(std::vector<uint8_t>& data, int32_t value)
{
    uint8_t bytes[sizeof(int32_t)];
    std::memcpy(bytes, &value, sizeof(bytes));
    data.insert(data.end(), bytes, bytes + sizeof(bytes));
}
} // namespace


bool PressPrivate::getStringValue(uint16_t command, uint8_t arg, std::string& text)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(command, {arg}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(command, responseListData);
        return false;
    }
    // 响应为 [arg, string...]（首字节回显请求参数）；协议限长 32 字节
    if (responseListData.size() < 2 || responseListData.size() > 32)
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(command, responseListData);
        return false;
    }
    text.assign(responseListData.begin() + 1, responseListData.end());
    return true;
}

// ==================== 只读设备命令（ROD） ====================

bool PressPrivate::getName(std::string& name, uint8_t language)
{
    if (!getStringValue(CMDID_NAME, language, name))
    {
        return false;
    }
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        if (language == 0x02)
        {
            std::strncpy(m_JsonData.m_ReadOnlyData.m_NameEN, name.c_str(),
                         sizeof(m_JsonData.m_ReadOnlyData.m_NameEN) - 1);
            m_JsonData.m_ReadOnlyData.m_NameEN[sizeof(m_JsonData.m_ReadOnlyData.m_NameEN) - 1] = '\0';
        }
        else
        {
            std::strncpy(m_JsonData.m_ReadOnlyData.m_NameZH, name.c_str(),
                         sizeof(m_JsonData.m_ReadOnlyData.m_NameZH) - 1);
            m_JsonData.m_ReadOnlyData.m_NameZH[sizeof(m_JsonData.m_ReadOnlyData.m_NameZH) - 1] = '\0';
        }
    }
    return true;
}

bool PressPrivate::getMachineType(std::string& type)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_MACHINE_TYPE_V, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(GET_MACHINE_TYPE_V, responseListData);
        return false;
    }
    // 响应为纯字符串（无语言前缀），协议限长 32 字节
    if (responseListData.empty() || !MACHINE_TYPE_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(GET_MACHINE_TYPE_V, responseListData);
        return false;
    }
    type.assign(responseListData.begin(), responseListData.end());
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        std::strncpy(m_JsonData.m_ReadOnlyData.m_Type, type.c_str(),
                     sizeof(m_JsonData.m_ReadOnlyData.m_Type) - 1);
        m_JsonData.m_ReadOnlyData.m_Type[sizeof(m_JsonData.m_ReadOnlyData.m_Type) - 1] = '\0';
    }
    return true;
}

bool PressPrivate::getPressParameter(PressParameter& param)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_PRESS_PARAMETER, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_PRESS_PARAMETER, responseListData);
        return false;
    }
    // 固件实际组包为 21 字节（readonlydata.cpp getPressParameterData 含 OutType）
    if (responseListData.size() != 21)
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_PRESS_PARAMETER, responseListData);
        return false;
    }
    param.m_MaxPLimit = _readHostFloat(responseListData, 0);
    param.m_MinPLimit = _readHostFloat(responseListData, 4);
    param.m_MaxMin = _readHostFloat(responseListData, 8);
    param.m_Diameter = _readHostFloat(responseListData, 12);
    param.m_MaxPStep = responseListData[16];
    param.m_PDecimal = responseListData[17];
    param.m_PressDecimal = responseListData[18];
    param.m_PModel = responseListData[19];
    param.m_OutType = responseListData[20];
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        m_JsonData.m_ReadOnlyData.m_MaxPLimit = param.m_MaxPLimit;
        m_JsonData.m_ReadOnlyData.m_MinPLimit = param.m_MinPLimit;
        m_JsonData.m_ReadOnlyData.m_Max_Min = param.m_MaxMin;
        m_JsonData.m_ReadOnlyData.m_Diameter = param.m_Diameter;
        m_JsonData.m_ReadOnlyData.m_MaxPStep = param.m_MaxPStep;
        m_JsonData.m_ReadOnlyData.m_PDecimal = param.m_PDecimal;
        m_JsonData.m_ReadOnlyData.m_PressDecimal = param.m_PressDecimal;
        m_JsonData.m_ReadOnlyData.m_PModel = param.m_PModel;
        m_JsonData.m_ReadOnlyData.m_OutType = param.m_OutType;
    }
    return true;
}

bool PressPrivate::getTempParameter(TempParameter& param)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_TEMP_PARAMETER, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_TEMP_PARAMETER, responseListData);
        return false;
    }
    if (!TEMP_PARAMETER_TYPE_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_TEMP_PARAMETER, responseListData);
        return false;
    }
    param.m_MaxTLimit = _readHostFloat(responseListData, 0);
    param.m_MinTLimit = _readHostFloat(responseListData, 4);
    param.m_MaxTStep = responseListData[8];
    param.m_TDecimal = responseListData[9];
    param.m_IsHasWater = responseListData[10];
    param.m_IsHasSpeed = responseListData[11];
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        m_JsonData.m_ReadOnlyData.m_MaxTLimit = param.m_MaxTLimit;
        m_JsonData.m_ReadOnlyData.m_MinTLimit = param.m_MinTLimit;
        m_JsonData.m_ReadOnlyData.m_MaxTStep = param.m_MaxTStep;
        m_JsonData.m_ReadOnlyData.m_TDecimal = param.m_TDecimal;
        m_JsonData.m_ReadOnlyData.m_IsHasWater = param.m_IsHasWater;
        m_JsonData.m_ReadOnlyData.m_IsHasSpeed = param.m_IsHasSpeed;
    }
    return true;
}

bool PressPrivate::getOtherInfo(OtherInfo& info)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_OTHER_INFO, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_OTHER_INFO, responseListData);
        return false;
    }
    if (!OTHER_INFO_TYPE_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_OTHER_INFO, responseListData);
        return false;
    }
    info.m_Screenshot = responseListData[0];
    info.m_StartDelay = responseListData[1];
    info.m_IsHideLang = responseListData[2];
    info.m_Remote = responseListData[3];
    info.m_VersionType = responseListData[4];
    {
        std::lock_guard<std::mutex> jsonLock(m_JsonDataMutex);
        m_JsonData.m_ReadOnlyData.m_Screenshot = info.m_Screenshot;
        m_JsonData.m_ReadOnlyData.m_StartDelay = info.m_StartDelay;
        m_JsonData.m_ReadOnlyData.m_IsHideLang = info.m_IsHideLang;
        m_JsonData.m_ReadOnlyData.m_Remote = info.m_Remote;
        m_JsonData.m_ReadOnlyData.m_VersionType = info.m_VersionType;
    }
    return true;
}

// ==================== 段参数命令 ====================

bool PressPrivate::getPressStepParameter(uint8_t index, PressStepParameter& param)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_INDEX_PSTEP_PARAMETER, {index}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_INDEX_PSTEP_PARAMETER, responseListData);
        return false;
    }
    if (!INDEX_PSTEP_PARAMETER_BOOL(responseListData.size()) || responseListData[0] != index)
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_INDEX_PSTEP_PARAMETER, responseListData);
        return false;
    }
    param.m_SetPValue = _readHostFloat(responseListData, 1);
    param.m_AfterValue = _readHostFloat(responseListData, 5);
    param.m_KPTime = static_cast<uint32_t>(_readHostInt32(responseListData, 9));
    return true;
}

bool PressPrivate::setPressStepParameter(uint8_t index, const PressStepParameter& param)
{
    std::vector<uint8_t> requestListData;
    requestListData.push_back(index);
    _appendHostFloat(requestListData, param.m_SetPValue);
    _appendHostFloat(requestListData, param.m_AfterValue);
    _appendHostInt32(requestListData, static_cast<int32_t>(param.m_KPTime));
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_INDEX_PSTEP_PARAMETER), requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_INDEX_PSTEP_PARAMETER), responseListData);
        return false;
    }
    if (responseListData.size() != 13 || responseListData[0] != index)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_INDEX_PSTEP_PARAMETER), responseListData);
        return false;
    }
    return true;
}

bool PressPrivate::getTempStepParameter(uint8_t index, TempStepParameter& param)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_INDEX_TSTEP_PARAMETER, {index}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_INDEX_TSTEP_PARAMETER, responseListData);
        return false;
    }
    if (!INDEX_TSTEP_PARAMETER_BOOL(responseListData.size()) || responseListData[0] != index)
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_INDEX_TSTEP_PARAMETER, responseListData);
        return false;
    }
    param.m_TempValue = _readHostFloat(responseListData, 1);
    param.m_SpeedValue = _readHostFloat(responseListData, 5);
    param.m_KTTime = static_cast<uint32_t>(_readHostInt32(responseListData, 9));
    return true;
}

bool PressPrivate::setTempStepParameter(uint8_t index, const TempStepParameter& param)
{
    std::vector<uint8_t> requestListData;
    requestListData.push_back(index);
    _appendHostFloat(requestListData, param.m_TempValue);
    _appendHostFloat(requestListData, param.m_SpeedValue);
    _appendHostInt32(requestListData, static_cast<int32_t>(param.m_KTTime));
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_INDEX_TSTEP_PARAMETER), requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_INDEX_TSTEP_PARAMETER), responseListData);
        return false;
    }
    if (responseListData.size() != 13 || responseListData[0] != index)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_INDEX_TSTEP_PARAMETER), responseListData);
        return false;
    }
    return true;
}

bool PressPrivate::getMixStepParameter(uint8_t index, MixStepParameter& param)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_MIX_INDEX_STEP_PARAMETER, {index}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_MIX_INDEX_STEP_PARAMETER, responseListData);
        return false;
    }
    // 实际布局为 24 字节（index + 23），下位机校验宏 23 疑似 off-by-one，按实际布局解析
    if (responseListData.size() != 24 || responseListData[0] != index)
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_MIX_INDEX_STEP_PARAMETER, responseListData);
        return false;
    }
    param.m_IsPress = responseListData[1];
    param.m_SetPValue = _readHostFloat(responseListData, 2);
    param.m_AfterValue = _readHostFloat(responseListData, 6);
    param.m_IsUpHeat = responseListData[10];
    param.m_UpTemp = _readHostFloat(responseListData, 11);
    param.m_IsDownHeat = responseListData[15];
    param.m_DownTemp = _readHostFloat(responseListData, 16);
    param.m_KTime = static_cast<uint32_t>(_readHostInt32(responseListData, 20));
    return true;
}

bool PressPrivate::setMixStepParameter(uint8_t index, const MixStepParameter& param)
{
    std::vector<uint8_t> requestListData;
    requestListData.push_back(index);
    requestListData.push_back(param.m_IsPress);
    _appendHostFloat(requestListData, param.m_SetPValue);
    _appendHostFloat(requestListData, param.m_AfterValue);
    requestListData.push_back(param.m_IsUpHeat);
    _appendHostFloat(requestListData, param.m_UpTemp);
    requestListData.push_back(param.m_IsDownHeat);
    _appendHostFloat(requestListData, param.m_DownTemp);
    _appendHostInt32(requestListData, static_cast<int32_t>(param.m_KTime));
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_MIX_INDEX_STEP_PARAMETER), requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_MIX_INDEX_STEP_PARAMETER), responseListData);
        return false;
    }
    if (responseListData.size() != 24 || responseListData[0] != index)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_MIX_INDEX_STEP_PARAMETER), responseListData);
        return false;
    }
    return true;
}

// ==================== 系统命令 ====================

bool PressPrivate::getDateTime(DateTimeInfo& dateTime)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_DATE_TIME, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_DATE_TIME, responseListData);
        return false;
    }
    if (!DATE_TIME_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_DATE_TIME, responseListData);
        return false;
    }
    // 布局: [秒][分][时][日][月][年]
    dateTime.m_Second = responseListData[0];
    dateTime.m_Minute = responseListData[1];
    dateTime.m_Hour = responseListData[2];
    dateTime.m_Day = responseListData[3];
    dateTime.m_Month = responseListData[4];
    dateTime.m_Year = responseListData[5];
    return true;
}

bool PressPrivate::setDateTime(const DateTimeInfo& dateTime)
{
    // 布局: [秒][分][时][日][月][年]，写成功后下位机回显当前时间
    std::vector<uint8_t> requestListData = {
        dateTime.m_Second, dateTime.m_Minute, dateTime.m_Hour,
        dateTime.m_Day, dateTime.m_Month, dateTime.m_Year
    };
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_DATE_TIME), requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_DATE_TIME), responseListData);
        return false;
    }
    if (responseListData.size() != 6)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_DATE_TIME), responseListData);
        return false;
    }
    return true;
}

bool PressPrivate::getBrightness(uint8_t& brightness)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_BRIGHTNESS, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_BRIGHTNESS, responseListData);
        return false;
    }
    if (!GET_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_BRIGHTNESS, responseListData);
        return false;
    }
    brightness = responseListData[0];
    return true;
}

bool PressPrivate::setBrightness(uint8_t brightness)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_BRIGHTNESS), {brightness}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_BRIGHTNESS), responseListData);
        return false;
    }
    // 参数类写命令：回显校验
    if (responseListData.size() != 1 || responseListData[0] != brightness)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_BRIGHTNESS), responseListData);
        return false;
    }
    return true;
}

bool PressPrivate::getButtonSound(bool& enabled)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_BUTTON_SOUND, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_BUTTON_SOUND, responseListData);
        return false;
    }
    if (!GET_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_BUTTON_SOUND, responseListData);
        return false;
    }
    enabled = (responseListData[0] != 0);
    return true;
}

bool PressPrivate::setButtonSound(bool enabled)
{
    const uint8_t value = enabled ? 1 : 0;
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_BUTTON_SOUND), {value}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_BUTTON_SOUND), responseListData);
        return false;
    }
    if (responseListData.size() != 1 || responseListData[0] != value)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_BUTTON_SOUND), responseListData);
        return false;
    }
    return true;
}

bool PressPrivate::getLanguage(uint8_t& language)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_LANGUAGE, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_LANGUAGE, responseListData);
        return false;
    }
    if (!GET_BOOL(responseListData.size()))
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_LANGUAGE, responseListData);
        return false;
    }
    language = responseListData[0];
    return true;
}

bool PressPrivate::setLanguage(uint8_t language)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_CMD(CMDID_LANGUAGE), {language}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_LANGUAGE), responseListData);
        return false;
    }
    if (responseListData.size() != 1 || responseListData[0] != language)
    {
        _setError(MachineError::UnknownError);
        if (auto iface = _safeGetInterface()) iface->onError(SET_CMD(CMDID_LANGUAGE), responseListData);
        return false;
    }
    return true;
}

// ==================== 状态与控制命令 ====================

bool PressPrivate::getMachineState(MachineState& state)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(CMDID_GET_CURRENT_STATE, {0x00}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_GET_CURRENT_STATE, responseListData);
        return false;
    }
    // 下位机 RTData::getRtData 二进制布局（字段数依模式分支），见 MachineState 说明。
    // 头部 u8 数 h ∈ {2,3,4}（flags/pStep[/tStep[/t1Step]]），
    // 尾部时间 u32 数 t ∈ {1,2,3}，总长 = 1 + h + 12 + 4t + 1。
    // PTT: h=2,t=1 → 20 字节；默认/P_TT: h=3,t=2 → 25 字节；P_T_T: h=4,t=3 → 30 字节
    const size_t size = responseListData.size();
    size_t headBytes = 0;
    size_t timeBytes = 0;
    if (size == 20)      { headBytes = 2; timeBytes = 1; }
    else if (size == 25) { headBytes = 3; timeBytes = 2; }
    else if (size == 30) { headBytes = 4; timeBytes = 3; }
    else
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface()) iface->onError(CMDID_GET_CURRENT_STATE, responseListData);
        return false;
    }
    state = MachineState{};
    state.m_ModelState = responseListData[0];
    state.m_Flags = responseListData[1];
    state.m_PressStep = responseListData[2];
    if (headBytes >= 3)
    {
        state.m_TempStep = responseListData[3];
    }
    if (headBytes >= 4)
    {
        state.m_Temp1Step = responseListData[4];
    }
    const size_t floatOffset = 1 + headBytes;
    state.m_PressValue = _readHostFloat(responseListData, floatOffset);
    state.m_TempUpValue = _readHostFloat(responseListData, floatOffset + 4);
    state.m_TempDownValue = _readHostFloat(responseListData, floatOffset + 8);
    const size_t timeOffset = floatOffset + 12;
    state.m_PTime = static_cast<uint32_t>(_readHostInt32(responseListData, timeOffset));
    if (timeBytes >= 2)
    {
        state.m_TTime = static_cast<uint32_t>(_readHostInt32(responseListData, timeOffset + 4));
    }
    if (timeBytes >= 3)
    {
        state.m_T1Time = static_cast<uint32_t>(_readHostInt32(responseListData, timeOffset + 8));
    }
    state.m_FlowChange = responseListData[size - 1];
    state.m_Count = static_cast<uint8_t>(size);
    return true;
}

bool PressPrivate::setStartHeat(bool isHeating)
{
    std::vector<uint8_t> responseListData;
    uint16_t command = CMDID_SET_START_HEAT;
    if (!_requestCommand(command, {static_cast<uint8_t>(isHeating)}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(command, responseListData);
        return false;
    }
    return _handleSetResponse(command, responseListData);
}

bool PressPrivate::setStart(bool isStart)
{
    std::vector<uint8_t> responseListData;
    uint16_t command = CMDID_SET_START;
    if (!_requestCommand(command, {static_cast<uint8_t>(isStart)}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(command, responseListData);
        return false;
    }
    return _handleSetResponse(command, responseListData);
}

bool PressPrivate::selectModelState(uint8_t model)
{
    std::vector<uint8_t> responseListData;
    uint16_t command = CMDID_SELECT_MODEL_STATE;
    if (!_requestCommand(command, {model}, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(command, responseListData);
        return false;
    }
    return _handleSetResponse(command, responseListData);
}

// ==================== JSON 批量命令（字符串形式） ====================

bool PressPrivate::getTempDataJson(std::string& json, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_TD_JSON, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onTempData(static_cast<int>(m_LastError.load()), std::string());
            iface->onError(GET_TD_JSON, responseListData);
        }
        return false;
    }
    if (responseListData.empty())
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface())
        {
            iface->onTempData(static_cast<int>(m_LastError.load()), std::string());
            iface->onError(GET_TD_JSON, responseListData);
        }
        return false;
    }
    json.assign(responseListData.begin(), responseListData.end());
    if (auto iface = _safeGetInterface())
    {
        iface->onTempData(0, json);
    }
    return true;
}

bool PressPrivate::setTempDataJson(const std::string& json, bool isCompressed)
{
    // payload: [压缩标志][JSON 字节...]
    std::vector<uint8_t> requestListData;
    requestListData.push_back(static_cast<uint8_t>(isCompressed));
    requestListData.insert(requestListData.end(), json.begin(), json.end());
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_TD_JSON, requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_TD_JSON, responseListData);
        return false;
    }
    // 下位机写成功后回显 JSON，非空即成功
    return _handleSetResponse(SET_TD_JSON, responseListData);
}

bool PressPrivate::getMixDataJson(std::string& json, bool isCompressed)
{
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(GET_MD_JSON, {static_cast<uint8_t>(isCompressed)}, responseListData))
    {
        if (auto iface = _safeGetInterface())
        {
            iface->onMixData(static_cast<int>(m_LastError.load()), std::string());
            iface->onError(GET_MD_JSON, responseListData);
        }
        return false;
    }
    if (responseListData.empty())
    {
        _setError(MachineError::ResponseTooShort);
        if (auto iface = _safeGetInterface())
        {
            iface->onMixData(static_cast<int>(m_LastError.load()), std::string());
            iface->onError(GET_MD_JSON, responseListData);
        }
        return false;
    }
    json.assign(responseListData.begin(), responseListData.end());
    if (auto iface = _safeGetInterface())
    {
        iface->onMixData(0, json);
    }
    return true;
}

bool PressPrivate::setMixDataJson(const std::string& json, bool isCompressed)
{
    // payload: [压缩标志][JSON 字节...]
    std::vector<uint8_t> requestListData;
    requestListData.push_back(static_cast<uint8_t>(isCompressed));
    requestListData.insert(requestListData.end(), json.begin(), json.end());
    std::vector<uint8_t> responseListData;
    if (!_requestCommand(SET_MD_JSON, requestListData, responseListData))
    {
        if (auto iface = _safeGetInterface()) iface->onError(SET_MD_JSON, responseListData);
        return false;
    }
    // 下位机写成功后回显 JSON，非空即成功
    return _handleSetResponse(SET_MD_JSON, responseListData);
}
