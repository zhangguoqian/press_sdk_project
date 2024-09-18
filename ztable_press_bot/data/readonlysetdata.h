//
// Created by 11518 on 2024/8/6.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_READONLYSETDATA_H
#define ZTABLE_PRESS_BOT_PROJECT_READONLYSETDATA_H

#include <QString>
#include <vector>

class ReadOnlySetData {
public:
    double s_MaxLimit = 30.0;
    double s_MinLimit = 0.3;
    double s_Max_Min = 0.3;
    double s_Diameter = 110.0;
    std::vector<double> s_OverLoadUps;
    double s_MaxPushSet = 6.0;
    double s_PushSet = 5.0;
    bool s_Beep = true;
    QString s_Type = "Type";
    bool s_Screenshot = false;
//	int s_Language = 0;
    QString s_NameCN = "自动压片机";
    QString s_NameEN = "AutomaticTabletPress";
    int s_PressPrecision = 2;

    void init();
    bool openJsonFile(const QString &fileName);
    bool saveJsonFile(const QString &fileName);
};


#endif //ZTABLE_PRESS_BOT_PROJECT_READONLYSETDATA_H
