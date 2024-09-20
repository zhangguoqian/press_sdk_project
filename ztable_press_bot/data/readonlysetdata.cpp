//
// Created by 11518 on 2024/8/6.
//

#include "readonlysetdata.h"

void ReadOnlySetData::init() {
    s_MaxLimit = 20.0;//s_MaxLimit = 30.0;
    s_MinLimit = 0.2;//s_MinLimit = 0.3;
    s_Max_Min = 0.1;//s_Max_Min = 0.3;
    s_Diameter = 95.0;//s_Diameter = 110.0;
    s_OverLoadUps.resize(10, 0.0);
    s_MaxPushSet = 10.0;//s_MaxPushSet = 6.0;
    s_PushSet = 5.0;
    s_Beep = true;
    s_Type = "HAP-20S";//s_Type = "Type";
    s_Screenshot = false;
    s_NameCN = "自动压片机";
    s_NameEN = "AutoPressBot";
    s_PressPrecision = 1;
}

bool ReadOnlySetData::openJsonFile(const QString &fileName) {
    return false;
}

bool ReadOnlySetData::saveJsonFile(const QString &fileName) {
    return false;
}
