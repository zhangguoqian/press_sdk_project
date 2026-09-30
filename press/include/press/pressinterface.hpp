/******************************************************************************
 * pressinterface.hpp — MachineDataInterface 异步回调接口
 *
 *  使用者继承此纯虚类、实现各回调方法，然后通过 Machine::registerDataInterface()
 *  注册。设备返回数据、运行状态变化、或错误/未处理响应时，对应的回调将在
 *  Machine 内部调度线程中被触发（非 UI 线程，Qt 用户需自行跨线程派发）。
 *
 *  线程说明: 所有回调默认在后台线程调用；UI 更新必须跨线程处理。
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_PRESSINTERFACE_HPP
#define PRESS_SDK_PROJECT_PRESSINTERFACE_HPP

#include "presstype.h"

#include <vector>


/******************************************************************************
 * MachineDataInterface — Asynchronous data callback interface / 异步数据回调接口
 *
 *  实现此接口并通过 Machine::registerDataInterface() 注册，即可在只读数据、
 *  压力参数、实时状态到达时接收推送式通知。
 *****************************************************************************/

class MachineDataInterface
{
public:
    virtual ~MachineDataInterface() = default;

    //! Read-only data callback / 只读数据回调
    //! @param errorCode   0 on success, non-zero on failure
    //! @param registerNo  Device register number (unique per connection)
    //! @param readOnlyData Decoded read-only data
    virtual void onReadOnlyData(int errorCode, uint64_t registerNo,
                                const ReadOnlyData& readOnlyData) = 0;

    //! Real-time data callback / 实时数据回调
    //! @param errorCode   0 on success, non-zero on failure
    //! @param realTimeData Decoded real-time state
    virtual void onRealTimeData(int errorCode, const RealTimeData& realTimeData) = 0;

    //! Pressure parameter callback / 压力参数回调
    //! @param errorCode   0 on success, non-zero on failure
    //! @param pressData   Decoded pressure parameters
    virtual void onPressData(int errorCode, const PressData& pressData) = 0;

    //! Error / unhandled response callback / 错误或未处理响应回调
    //! 当设备返回的命令 ID 不被 SDK 识别或解析失败时触发。
    //! @param cmdCode   Command ID that produced the response
    //! @param response  Raw response bytes from device
    virtual void onError(uint16_t cmdCode, std::vector<uint8_t> response) = 0;
};

#endif /* PRESS_SDK_PROJECT_PRESSINTERFACE_HPP */