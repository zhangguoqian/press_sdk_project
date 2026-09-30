//
// Created by 11518 on 2026/9/13.
//
#ifndef PRESS_BOT_PROJECT_MACHINEPRIVATE_H
#define PRESS_BOT_PROJECT_MACHINEPRIVATE_H

#include "port/portbase.h"
#include "tool/jsondata.h"
#include "press.hpp"
#include <memory>
#include <vector>
#include <cstdint>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

static constexpr uint16_t GET_MACHINE_TYPE_V = GET_MACHINE_TYPE;
static constexpr uint16_t GET_ROD_JSON_V     = GET_ROD_JSON;
static constexpr uint16_t GET_PD_JSON_V      = GET_PD_JSON;
static constexpr uint16_t SET_PD_JSON_V      = SET_PD_JSON;
static constexpr uint16_t GET_RT_JSON_V      = GET_RT_JSON;
static constexpr uint16_t SET_PRESS_V        = SET_PRESS;
static constexpr uint16_t SET_DEMOLD_V       = SET_DEMOLD;

enum class MachineError
{
    None = 0,
    PortNotOpen,
    WriteFailed,
    ResponseTooShort,
    ChecksumMismatch,
    RegisterNoMismatch,
    CommandMismatch,
    Timeout,
    Stopped,
    ConnectFailed,
    JsonParseFailed,

    OutSignalBlocked,
    ImmediateSignalBlocked,
    StateUnchanged,

    UnknownError,
};

struct FrameData
{
    uint16_t m_Command = 0;
    uint64_t m_Seq = 0;                     //!< 请求序号，用于匹配响应
    std::vector<uint8_t> m_FrameDataList = {};
};

class PressPrivate
{
public:
    PressPrivate();
    ~PressPrivate();

    bool connect(const char* portName, PortType portType);
    void disconnect();
    bool isConnected() const;
    uint64_t getMachineRegisterNo() const;

    bool getReadOnlyData(ReadOnlyData& data, bool isCompressed = false);
    bool getPressData(PressData& data, bool isCompressed = false);
    bool setPressData(const PressData& data, bool isCompressed = false);
    bool getRealTimeData(RealTimeData& data, bool isCompressed = false);
    bool setPressing(bool isPressing);
    bool setDemolding(bool isDemolding);

    void run();
    void stop();
    bool isRunning() const;

    static std::vector<std::string> getPortList();
    const char* getLastErrorInfo() const;

    void registerDataInterface(PressDataInterface* dataInterface);
    void unregisterDataInterface();

    const JsonData& getMachineData() const;

private:
    //! 启动调度线程，幂等：若已启动则直接返回
    void _startScheduler();
    //! 停止调度线程并 join，幂等：若未启动则直接返回
    void _stopScheduler();

    std::vector<uint8_t> _buildSendFrame(const FrameData& frameData);
    MachineError _parseResponseFrame(const std::vector<uint8_t>& response, uint16_t command, FrameData& out);
    bool _requestCommand(uint16_t command, const std::vector<uint8_t>& listData, std::vector<uint8_t>& responseListData);
    void _setError(MachineError err);
    std::shared_ptr<PressDataInterface> _safeGetInterface();
    bool _handleSetResponse(uint16_t cmd, const std::vector<uint8_t>& responseListData);

    std::mutex m_RequestSerialMutex{};

    std::mutex m_CommandMutex{};
    std::queue<FrameData> m_CommandQueue = {};
    std::condition_variable m_CommandCond{};

    std::mutex m_ResponseMutex{};
    std::queue<FrameData> m_ResponseQueue = {};
    std::condition_variable m_ResponseCond{};

    std::unique_ptr<std::thread> mpRunThread = nullptr;
    std::atomic<bool> m_IsRunning{false};

    std::atomic<uint64_t> m_RegisterNo{0};
    std::atomic<uint64_t> m_RequestSeq{0};     //!< 请求序号，超时后丢弃迟到响应
    std::unique_ptr<PortBase> mpPort = nullptr;

    std::atomic<MachineError> m_LastError{MachineError::None};

    JsonData m_JsonData{};
    std::shared_ptr<PressDataInterface> mpMachineDataInterface;
    std::mutex m_MachineDataMutex{};
};

#endif