#include "press/press.h"

#include "press.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace
{
class CMachineDataAdapter : public PressDataInterface
{
public:
    explicit CMachineDataAdapter(const CPressDataCallbacks& callbacks)
        : m_callbacks(callbacks)
    {
    }

    void onReadOnlyData(int errorCode,
                        uint64_t registerNo,
                        const ReadOnlyData& readOnlyData) override
    {
        if (m_callbacks.onReadOnlyData != nullptr)
        {
            m_callbacks.onReadOnlyData(m_callbacks.userData,
                                      errorCode,
                                      registerNo,
                                      &readOnlyData);
        }
    }

    void onRealTimeData(int errorCode,
                        const RealTimeData& realTimeData) override
    {
        if (m_callbacks.onRealTimeData != nullptr)
        {
            m_callbacks.onRealTimeData(m_callbacks.userData,
                                      errorCode,
                                      &realTimeData);
        }
    }

    void onPressData(int errorCode,
                     const PressData& pressData) override
    {
        if (m_callbacks.onPressData != nullptr)
        {
            m_callbacks.onPressData(m_callbacks.userData,
                                   errorCode,
                                   &pressData);
        }
    }

    void onError(uint16_t cmdCode,
                 std::vector<uint8_t> response) override
    {
        if (m_callbacks.onError != nullptr)
        {
            const uint8_t* data = response.empty() ? nullptr : response.data();
            m_callbacks.onError(m_callbacks.userData,
                                cmdCode,
                                data,
                                response.size());
        }
    }

private:
    CPressDataCallbacks m_callbacks;
};
} // namespace

struct CPressContext
{
    std::unique_ptr<Press> impl;
    std::unique_ptr<CMachineDataAdapter> callbackAdapter;
    CPressDataCallbacks callbacks;
};

PRESS_C_EXPORT CPressContext* cpress_create(void)
{
    CPressContext* context = new CPressContext();
    context->impl = std::unique_ptr<Press>(new Press());
    return context;
}

PRESS_C_EXPORT void cpress_destroy(CPressContext* handle)
{
    if (handle == nullptr)
    {
        return;
    }

    if (handle->impl != nullptr)
    {
        handle->impl->disconnect();
    }

    delete handle;
}

PRESS_C_EXPORT int cpress_connect(CPressContext* handle, const char* portName, int portType)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->connect(portName, static_cast<PressPortType>(portType)) ? 1 : 0;
}

PRESS_C_EXPORT void cpress_disconnect(CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->disconnect();
}

PRESS_C_EXPORT int cpress_is_connected(const CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->isConnected() ? 1 : 0;
}

PRESS_C_EXPORT uint64_t cpress_get_machine_register_no(const CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->getMachineRegisterNo();
}

PRESS_C_EXPORT const char* cpress_get_last_error_info(const CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return "";
    }

    return handle->impl->getLastErrorInfo();
}

PRESS_C_EXPORT void cpress_run(CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->run();
}

PRESS_C_EXPORT void cpress_stop(CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->stop();
}

PRESS_C_EXPORT int cpress_is_running(const CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->isRunning() ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_read_only_data_json(CPressContext* handle,
                                            ReadOnlyData* data,
                                            int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->getReadOnlyDataJson(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_press_data_json(CPressContext* handle,
                                        PressData* data,
                                        int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->getPressDataJson(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_press_data_json(CPressContext* handle,
                                        const PressData* data,
                                        int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->setPressDataJson(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_real_time_data_json(CPressContext* handle,
                                           RealTimeData* data,
                                           int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->getRealTimeDataJson(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_serial_number(CPressContext* handle, char* buf, size_t bufSize)
{
    if (handle == nullptr || handle->impl == nullptr || buf == nullptr || bufSize == 0)
    {
        return 0;
    }

    std::string serialNumber;
    if (!handle->impl->getSerialNumber(serialNumber))
    {
        return 0;
    }
    std::strncpy(buf, serialNumber.c_str(), bufSize - 1);
    buf[bufSize - 1] = '\0';
    return 1;
}

PRESS_C_EXPORT void cpress_set_request_timeout(CPressContext* handle, unsigned int ms)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }
    handle->impl->setRequestTimeout(ms);
}

PRESS_C_EXPORT int cpress_set_pressing(CPressContext* handle, int isPressing)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->setPressing(isPressing != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_demolding(CPressContext* handle, int isDemolding)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->setDemolding(isDemolding != 0) ? 1 : 0;
}

PRESS_C_EXPORT void cpress_register_data_interface(CPressContext* handle,
                                                 const CPressDataCallbacks* callbacks,
                                                 int intervalSeconds)
{
    if (handle == nullptr || handle->impl == nullptr || callbacks == nullptr)
    {
        return;
    }

    // 先注销（停定时线程、保证无在途回调），再替换 adapter，避免旧 adapter
    // 被析构后 impl 内的非拥有指针仍被回调访问
    handle->impl->unregisterDataInterface();
    handle->callbackAdapter.reset();

    handle->callbacks = *callbacks;
    handle->callbackAdapter.reset(new CMachineDataAdapter(handle->callbacks));
    handle->impl->registerDataInterface(handle->callbackAdapter.get(), intervalSeconds);
}

PRESS_C_EXPORT void cpress_unregister_data_interface(CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->unregisterDataInterface();
    handle->callbackAdapter.reset();
    handle->callbacks = CPressDataCallbacks();
}

PRESS_C_EXPORT const char* cpress_version(void)
{
    static const std::string versionString = Press::version();
    return versionString.c_str();
}
