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

    void onReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData) override;
    void onRealTimeData(int errorCode, const RealTimeData& realTimeData) override;
    void onPressData(int errorCode, const PressData& pressData) override;
    void onError(uint16_t cmdCode, std::vector<uint8_t> response) override;

signals:
    void signalError(uint16_t cmdCode, std::vector<uint8_t> response);
    void signalReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData);
    void signalRealTimeData(int errorCode, const RealTimeData& realTimeData);
    void signalPressData(int errorCode, const PressData& pressData);

};

#endif // PRESS_SDK_PROJECT_DATAINTERFACE_H
