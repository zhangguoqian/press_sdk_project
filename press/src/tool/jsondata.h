//
// Created by 11518 on 2026/9/14.
//

#ifndef PRESS_SDK_PROJECT_MACHINEDATA_H
#define PRESS_SDK_PROJECT_MACHINEDATA_H

#include "presstype.h"
#include <string>
#include <vector>

class JsonData
{
public:
    JsonData() = default;
    ~JsonData() = default;
    ReadOnlyData getReadOnlyData() const;
    PressData getPressData() const;
    RealTimeData getRealTimeData() const;

    void setReadOnlyData(const ReadOnlyData &);
    void setPressData(const PressData &);
    void setRealTimeData(const RealTimeData &);

    int jsonToReadOnlyData(const std::string &json);
    int jsonToRealTimeData(const std::string &json);
    int jsonToPressData(const std::string &json);

    int jsonToReadOnlyData(const std::vector<uint8_t> &data);
    int jsonToRealTimeData(const std::vector<uint8_t> &data);
    int jsonToPressData(const std::vector<uint8_t> &data);

    /**获取只读数据**/
    ReadOnlyData m_ReadOnlyData{};
    PressData m_PressData{};
    RealTimeData m_RealTimeData{};
};


#endif //PRESS_SDK_PROJECT_MACHINEDATA_H