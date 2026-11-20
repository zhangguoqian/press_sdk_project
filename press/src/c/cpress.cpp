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
    //! 共享所有权：注销后仍在途的回调可继续安全访问 adapter
    std::shared_ptr<CMachineDataAdapter> callbackAdapter;
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

PRESS_C_EXPORT const char* cpress_get_temp_data_json(CPressContext* handle,
                                                     int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return nullptr;
    }

    handle->lastJson.clear();
    if (!handle->impl->getTempDataJson(handle->lastJson, isCompressed != 0))
    {
        return nullptr;
    }
    return handle->lastJson.c_str();
}

PRESS_C_EXPORT int cpress_set_temp_data_json(CPressContext* handle,
                                             const char* json,
                                             int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || json == nullptr)
    {
        return 0;
    }

    return handle->impl->setTempDataJson(json, isCompressed != 0) ? 1 : 0;
}

PRESS_C_EXPORT const char* cpress_get_mix_data_json(CPressContext* handle,
                                                    int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return nullptr;
    }

    handle->lastJson.clear();
    if (!handle->impl->getMixDataJson(handle->lastJson, isCompressed != 0))
    {
        return nullptr;
    }
    return handle->lastJson.c_str();
}

PRESS_C_EXPORT int cpress_set_mix_data_json(CPressContext* handle,
                                            const char* json,
                                            int isCompressed)
{
    if (handle == nullptr || handle->impl == nullptr || json == nullptr)
    {
        return 0;
    }

    return handle->impl->setMixDataJson(json, isCompressed != 0) ? 1 : 0;
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

// ==================== 压力单值 / 索引命令 ====================
PRESS_C_EXPORT int cpress_get_press_step_count(CPressContext* handle, uint8_t* step)
{
    if (handle == nullptr || handle->impl == nullptr || step == nullptr) return 0;
    return handle->impl->getPressStepCount(*step) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_press_step_count(CPressContext* handle, uint8_t step)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setPressStepCount(step) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_calibration_value(CPressContext* handle, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getCalibrationValue(*value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_calibration_value(CPressContext* handle, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setCalibrationValue(value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mold_type(CPressContext* handle, uint8_t* type)
{
    if (handle == nullptr || handle->impl == nullptr || type == nullptr) return 0;
    return handle->impl->getMoldType(*type) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mold_type(CPressContext* handle, uint8_t type)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMoldType(type) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_circle(CPressContext* handle, float* diameter)
{
    if (handle == nullptr || handle->impl == nullptr || diameter == nullptr) return 0;
    return handle->impl->getCircle(*diameter) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_circle(CPressContext* handle, float diameter)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setCircle(diameter) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_rectangle(CPressContext* handle, float* a, float* b)
{
    if (handle == nullptr || handle->impl == nullptr || a == nullptr || b == nullptr) return 0;
    return handle->impl->getRectangle(*a, *b) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_rectangle(CPressContext* handle, float a, float b)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setRectangle(a, b) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_air_time(CPressContext* handle, uint8_t* seconds)
{
    if (handle == nullptr || handle->impl == nullptr || seconds == nullptr) return 0;
    return handle->impl->getAirTime(*seconds) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_air_time(CPressContext* handle, uint8_t seconds)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setAirTime(seconds) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_press_value_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getPressValueIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_press_value_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setPressValueIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_after_value_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getAfterValueIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_after_value_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setAfterValueIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_keep_press_time(CPressContext* handle, uint8_t index, int32_t* ms)
{
    if (handle == nullptr || handle->impl == nullptr || ms == nullptr) return 0;
    return handle->impl->getKeepPressTime(index, *ms) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_keep_press_time(CPressContext* handle, uint8_t index, int32_t ms)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setKeepPressTime(index, ms) ? 1 : 0;
}

// ==================== 温度单值 / 索引命令 ====================
PRESS_C_EXPORT int cpress_get_temp_step_count(CPressContext* handle, uint8_t* step)
{
    if (handle == nullptr || handle->impl == nullptr || step == nullptr) return 0;
    return handle->impl->getTempStepCount(*step) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_temp_step_count(CPressContext* handle, uint8_t step)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setTempStepCount(step) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_temp_speed_mold(CPressContext* handle, uint8_t* mold)
{
    if (handle == nullptr || handle->impl == nullptr || mold == nullptr) return 0;
    return handle->impl->getTempSpeedMold(*mold) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_temp_speed_mold(CPressContext* handle, uint8_t mold)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setTempSpeedMold(mold) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_start_temp_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getStartTempIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_temp_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setTempIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_temp_speed_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getTempSpeedIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_temp_speed_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setTempSpeedIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_keep_temp_time(CPressContext* handle, uint8_t index, int32_t* ms)
{
    if (handle == nullptr || handle->impl == nullptr || ms == nullptr) return 0;
    return handle->impl->getKeepTempTime(index, *ms) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_keep_temp_time(CPressContext* handle, uint8_t index, int32_t ms)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setKeepTempTime(index, ms) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_max_water_value(CPressContext* handle, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getMaxWaterValue(*value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_max_water_value(CPressContext* handle, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMaxWaterValue(value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_min_water_value(CPressContext* handle, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getMinWaterValue(*value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_min_water_value(CPressContext* handle, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMinWaterValue(value) ? 1 : 0;
}

// ==================== 混合模式单值 / 索引命令 ====================
PRESS_C_EXPORT int cpress_get_mix_step_count(CPressContext* handle, uint8_t* step)
{
    if (handle == nullptr || handle->impl == nullptr || step == nullptr) return 0;
    return handle->impl->getMixStepCount(*step) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_step_count(CPressContext* handle, uint8_t step)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixStepCount(step) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_mold_type(CPressContext* handle, uint8_t* type)
{
    if (handle == nullptr || handle->impl == nullptr || type == nullptr) return 0;
    return handle->impl->getMixMoldType(*type) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_mold_type(CPressContext* handle, uint8_t type)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixMoldType(type) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_rectangle(CPressContext* handle, float* a, float* b)
{
    if (handle == nullptr || handle->impl == nullptr || a == nullptr || b == nullptr) return 0;
    return handle->impl->getMixRectangle(*a, *b) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_rectangle(CPressContext* handle, float a, float b)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixRectangle(a, b) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_circle(CPressContext* handle, float* diameter)
{
    if (handle == nullptr || handle->impl == nullptr || diameter == nullptr) return 0;
    return handle->impl->getMixCircle(*diameter) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_circle(CPressContext* handle, float diameter)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixCircle(diameter) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_is_press_index(CPressContext* handle, uint8_t index, int* isPress)
{
    if (handle == nullptr || handle->impl == nullptr || isPress == nullptr) return 0;
    bool value = false;
    if (!handle->impl->getMixIsPressIndex(index, value)) return 0;
    *isPress = value ? 1 : 0;
    return 1;
}

PRESS_C_EXPORT int cpress_set_mix_is_press_index(CPressContext* handle, uint8_t index, int isPress)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixIsPressIndex(index, isPress != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_press_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getMixPressIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_press_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixPressIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_after_press_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getMixAfterPressIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_after_press_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixAfterPressIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_is_heat_up_index(CPressContext* handle, uint8_t index, int* isHeatUp)
{
    if (handle == nullptr || handle->impl == nullptr || isHeatUp == nullptr) return 0;
    bool value = false;
    if (!handle->impl->getMixIsHeatUpIndex(index, value)) return 0;
    *isHeatUp = value ? 1 : 0;
    return 1;
}

PRESS_C_EXPORT int cpress_set_mix_is_heat_up_index(CPressContext* handle, uint8_t index, int isHeatUp)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixIsHeatUpIndex(index, isHeatUp != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_heat_up_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getMixHeatUpIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_heat_up_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixHeatUpIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_is_heat_down_index(CPressContext* handle, uint8_t index, int* isHeatDown)
{
    if (handle == nullptr || handle->impl == nullptr || isHeatDown == nullptr) return 0;
    bool value = false;
    if (!handle->impl->getMixIsHeatDownIndex(index, value)) return 0;
    *isHeatDown = value ? 1 : 0;
    return 1;
}

PRESS_C_EXPORT int cpress_set_mix_is_heat_down_index(CPressContext* handle, uint8_t index, int isHeatDown)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixIsHeatDownIndex(index, isHeatDown != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_heat_down_index(CPressContext* handle, uint8_t index, float* value)
{
    if (handle == nullptr || handle->impl == nullptr || value == nullptr) return 0;
    return handle->impl->getMixHeatDownIndex(index, *value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_heat_down_index(CPressContext* handle, uint8_t index, float value)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixHeatDownIndex(index, value) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_get_mix_keep_time(CPressContext* handle, uint8_t index, int32_t* ms)
{
    if (handle == nullptr || handle->impl == nullptr || ms == nullptr) return 0;
    return handle->impl->getMixKeepTime(index, *ms) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_mix_keep_time(CPressContext* handle, uint8_t index, int32_t ms)
{
    if (handle == nullptr || handle->impl == nullptr) return 0;
    return handle->impl->setMixKeepTime(index, ms) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_set_demolding(CPressContext* handle, int isDemolding)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->setDemolding(isDemolding != 0) ? 1 : 0;
}

PRESS_C_EXPORT int cpress_start_air(CPressContext* handle)
{
    if (handle == nullptr || handle->impl == nullptr)
    {
        return 0;
    }

    return handle->impl->startAir() ? 1 : 0;
}

PRESS_C_EXPORT void cpress_register_data_interface(CPressContext* handle,
                                                 const CPressDataCallbacks* callbacks,
                                                 int intervalSeconds)
{
    if (handle == nullptr || handle->impl == nullptr || callbacks == nullptr)
    {
        return;
    }

    // 先注销（停定时线程），再替换 adapter；adapter 由 SDK 与上下文共享持有，
    // 即使其他线程仍有在途同步回调，也不会访问已析构对象
    handle->impl->unregisterDataInterface();
    handle->callbackAdapter.reset();

    handle->callbacks = *callbacks;
    handle->callbackAdapter = std::make_shared<CMachineDataAdapter>(handle->callbacks);
    handle->impl->registerDataInterfaceOwned(handle->callbackAdapter, intervalSeconds);
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
