/******************************************************************************
 * pressinterface.hpp — asynchronous callback interface for data push notifications
 * pressinterface.hpp — 数据推送异步回调接口
 *
 *  Application code can derive from PressDataInterface and register the object
 *  with Press::registerDataInterface(). Once registered, device responses,
 *  parameter updates, and error events are delivered through these callbacks.
 *  应用代码可以从 PressDataInterface 派生，并通过 Press::registerDataInterface()
 *  注册回调对象。注册后，设备响应、参数更新和错误事件都会通过这些回调
 *  发送给上层。
 *
 *  Threading model / 线程模型:
 *    Callbacks are dispatched from the SDK's internal worker thread. UI code should
 *    not update widgets directly from this thread unless cross-thread dispatch is
 *    already handled.
 *    回调由 SDK 内部工作线程发出。UI 代码不应直接在该线程中更新控件，除非
 *    已自行处理跨线程分发。
 *
 *  Typical usage / 典型用法:
 *    - receive real-time state updates / 接收实时状态更新
 *    - receive pressure configuration updates / 接收压力参数更新
 *    - receive error notifications / 接收错误通知
 *****************************************************************************/

#ifndef PRESS_SDK_PROJECT_PRESSINTERFACE_HPP
#define PRESS_SDK_PROJECT_PRESSINTERFACE_HPP

#include "presstype.h"

#include <vector>


/******************************************************************************
 * PressDataInterface — Asynchronous data callback interface / 异步数据回调接口
 *
 *  实现此接口并通过 Machine::registerDataInterface() 注册，即可在只读数据、
 *  压力参数、实时状态到达时接收推送式通知。
 *****************************************************************************/

class PressDataInterface
{
public:
    virtual ~PressDataInterface() = default;

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