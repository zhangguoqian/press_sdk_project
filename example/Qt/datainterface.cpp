#include "datainterface.h"
#include "ui/uihome.h"

DataInterface::DataInterface(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<ReadOnlyData>("ReadOnlyData");
    qRegisterMetaType<RealTimeData>("RealTimeData");
    qRegisterMetaType<PressData>("PressData");
    qRegisterMetaType<std::vector<uint8_t>>("std::vector<uint8_t>");
}

DataInterface::~DataInterface() = default;


void DataInterface::onReadOnlyData(int errorCode, uint64_t registerNo, const ReadOnlyData& readOnlyData)
{
    emit signalReadOnlyData(errorCode, registerNo, readOnlyData);
}

void DataInterface::onRealTimeData(int errorCode, const RealTimeData& realTimeData)
{
    emit signalRealTimeData(errorCode, realTimeData);
}

void DataInterface::onPressData(int errorCode, const PressData& pressData)
{
    emit signalPressData(errorCode, pressData);
}

void DataInterface::onError(uint16_t cmdCode, std::vector<uint8_t> response)
{
    emit signalError(cmdCode, response);
}
