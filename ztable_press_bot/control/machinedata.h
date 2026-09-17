//
// Created by 11518 on 2026/9/14.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_MACHINEDATA_H
#define ZTABLE_PRESS_BOT_PROJECT_MACHINEDATA_H

#include "machinetype.h"

class MachineData
{
public:
    MachineData();
    ~MachineData();
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
private:
    ReadOnlyData m_ReadOnlyData{};
    PressData m_PressData{};
    RealTimeData m_RealTimeData{};
};


#endif //ZTABLE_PRESS_BOT_PROJECT_MACHINEDATA_H
