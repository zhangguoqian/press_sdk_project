#ifndef PRESS_SDK_PROJECT_DATAINTERFACE_H
#define PRESS_SDK_PROJECT_DATAINTERFACE_H

#include <QObject>
#include "press.hpp"


class DataInterface : public QObject, public PressDataInterface
{
    Q_OBJECT
public:
    explicit DataInterface(QObject *parent = nullptr);
    ~DataInterface() override;

    //! JSON <-> SDK 结构体转换，供示例界面直接复用
    static ReadOnlyData parseReadOnlyData(const std::string& json);
    static PressData parsePressData(const std::string& json);
    static RealTimeData parseRealTimeData(const std::string& json);
    static std::string serializePressData(const PressData& data);

    void onReadOnlyData(int errorCode, uint64_t registerNo, const std::string& json) override;
    void onRealTimeData(int errorCode, const std::string& json) override;
    void onPressData(int errorCode, const std::string& json) override;
    void onTempData(int errorCode, const std::string& json) override;
    void onMixData(int errorCode, const std::string& json) override;
    void onError(uint16_t cmdCode, std::vector<uint8_t> response) override;

signals:
    void signalError(uint16_t cmdCode, std::vector<uint8_t> response);
    void signalReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData);
    void signalRealTimeData(int errorCode, const RealTimeData& realTimeData);
    void signalPressData(int errorCode, const PressData& pressData);
    void signalTempData(int errorCode, const std::string& json);
    void signalMixData(int errorCode, const std::string& json);

};

#endif // PRESS_SDK_PROJECT_DATAINTERFACE_H
