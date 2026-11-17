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
                        const std::string& json) override
    {
        if (m_callbacks.onReadOnlyData != nullptr)
        {
            m_callbacks.onReadOnlyData(m_callbacks.userData,
                                      errorCode,
                                      registerNo,
                                      json.c_str());
        }
    }

    void onRealTimeData(int errorCode,
                        const std::string& json) override
    {
        if (m_callbacks.onRealTimeData != nullptr)
        {
            m_callbacks.onRealTimeData(m_callbacks.userData,
                                      errorCode,
                                      json.c_str());
        }
    }

    void onPressData(int errorCode,
                     const std::string& json) override
    {
        if (m_callbacks.onPressData != nullptr)
        {
            m_callbacks.onPressData(m_callbacks.userData,
                                   errorCode,
                                   json.c_str());
        }
    }

    void onTempData(int errorCode,
                    const std::string& json) override
    {
        if (m_callbacks.onTempData != nullptr)
        {
            m_callbacks.onTempData(m_callbacks.userData,
                                   errorCode,
                                   json.c_str());
        }
    }

    void onMixData(int errorCode,
                   const std::string& json) override
    {
        if (m_callbacks.onMixData != nullptr)
        {
            m_callbacks.onMixData(m_callbacks.userData,
                                  errorCode,
                                  json.c_str());
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
    //! 最近一次 JSON 查询结果，返回指针在该缓冲区中保持有效，
    //! 直到同一上下文的下一次 GET 调用（单线程使用假设）。
    std::string lastJson;
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

PRESS_C_EXPORT const char* cpress_get_read_only_data_json(CPressContext* handle,
                                                          int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return nullptr;
    }

    handle->lastJson.clear();
    if (!handle->impl->getReadOnlyDataJson(handle->lastJson, isCompressed != 0))
    {
        return nullptr;
    }
    return handle->lastJson.c_str();
}

PRESS_C_EXPORT const char* cpress_get_press_data_json(CPressContext* handle,
                                                      int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return nullptr;
    }

    handle->lastJson.clear();
    if (!handle->impl->getPressDataJson(handle->lastJson, isCompressed != 0))
    {
        return nullptr;
    }
    return handle->lastJson.c_str();
}

PRESS_C_EXPORT int cpress_set_press_data_json(CPressContext* handle,
                                              const char* json,
                                              int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || json == nullptr)
    {
        return 0;
    }

    return handle->impl->setPressDataJson(json, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT const char* cpress_get_real_time_data_json(CPressContext* handle,
                                                          int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return nullptr;
    }

    handle->lastJson.clear();
    if (!handle->impl->getRealTimeDataJson(handle->lastJson, isCompressed != 0))
    {
        return nullptr;
    }
    return handle->lastJson.c_str();
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
