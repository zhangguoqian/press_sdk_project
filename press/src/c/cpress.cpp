#include "press/cpress.h"

#include "press.hpp"

#include <memory>
#include <string>
#include <vector>

namespace
{
class CMachineDataAdapter : public PressDataInterface
{
public:
    explicit CMachineDataAdapter(const PressCDataCallbacks& callbacks)
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
    PressCDataCallbacks m_callbacks;
};
} // namespace

struct PressCContext
{
    std::unique_ptr<Press> impl;
    std::unique_ptr<CMachineDataAdapter> callbackAdapter;
    PressCDataCallbacks callbacks;
};

PRESS_C_EXPORT PressCContext* cpress_create(void)
{
    PressCContext* context = new PressCContext();
    context->impl = std::unique_ptr<Press>(new Press());
    return context;
}

PRESS_C_EXPORT void cpress_destroy(PressCContext* handle)
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

PRESS_C_EXPORT int cpress_connect(PressCContext* handle, const char* portName, int portType)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->connect(portName, static_cast<PortType>(portType)) ? 1 : 0;
}

PRESS_C_EXPORT void cpress_disconnect(PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->disconnect();
}

PRESS_C_EXPORT int cpress_is_connected(const PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->isConnected() ? 1 : 0;
}

PRESS_C_EXPORT uint64_t cpress_get_machine_register_no(const PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->getMachineRegisterNo();
}

PRESS_C_EXPORT const char* cpress_get_last_error_info(const PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return "";
    }

    return handle->impl->getLastErrorInfo();
}

PRESS_C_EXPORT void cpress_run(PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->run();
}

PRESS_C_EXPORT void cpress_stop(PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->stop();
}

PRESS_C_EXPORT int cpress_is_running(const PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->isRunning() ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_read_only_data(PressCContext* handle,
                                            ReadOnlyData* data,
                                            int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->getReadOnlyData(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_press_data(PressCContext* handle,
                                        PressData* data,
                                        int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->getPressData(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_press_data(PressCContext* handle,
                                        const PressData* data,
                                        int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->setPressData(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_real_time_data(PressCContext* handle,
                                           RealTimeData* data,
                                           int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || data == nullptr)
    {
        return 0;
    }

    return handle->impl->getRealTimeData(*data, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_pressing(PressCContext* handle, int isPressing)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->setPressing(isPressing != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_demolding(PressCContext* handle, int isDemolding)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->setDemolding(isDemolding != 0) ? 1 : 0;
}

PRESS_C_EXPORT void cpress_register_data_interface(PressCContext* handle,
                                                 const PressCDataCallbacks* callbacks)
{
    if (handle == nullptr || handle->impl == nullptr || callbacks == nullptr)
    {
        return;
    }

    handle->callbacks = *callbacks;
    handle->callbackAdapter.reset(new CMachineDataAdapter(handle->callbacks));
    handle->impl->registerDataInterface(handle->callbackAdapter.get());
}

PRESS_C_EXPORT void cpress_unregister_data_interface(PressCContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return;
    }

    handle->impl->unregisterDataInterface();
    handle->callbackAdapter.reset();
    handle->callbacks = PressCDataCallbacks();
}

PRESS_C_EXPORT const char* cpress_version(void)
{
    static const std::string versionString = Press::version();
    return versionString.c_str();
}
