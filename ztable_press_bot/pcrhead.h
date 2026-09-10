//
// Created by 11518 on 2024/8/5.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_PCRHEAD_H
#define ZTABLE_PRESS_BOT_PROJECT_PCRHEAD_H

#include <QSize>
#include <QVersionNumber>
#include <cmath>
#include "data/actualpressdata.h"
#include "data/readonlysetdata.h"
#include "control/machine.h"
#include "sql/datacontrol.h"

#ifdef __WIN32__
#include <windows.h>
#endif

#define g 9.8
#define PI 3.14159265
#define DEFAULT_MAX_STEP 30

#define VERSION_MAJ 0
#define VERSION_MIN 0
#define VERSION_MIC 1

extern QVersionNumber VersionNumber;

extern ReadOnlySetData *epReadOnlySetData;

namespace APP{
    inline QSize AppUiSize() {
#ifdef Q_OS_WIN
        return QSize(GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN));

#elif Q_OS_LINUX
        return QSize(1280,800);
#else
    return QSize(1280,800);
#endif
    }

    inline double getS(double ton,int type, double dmm, double amm, double bmm){
        double S = 0.0;
        if(type == 0){
            return 0.0;
        }else if(type == 1 && dmm != 0.0){
            S = PI * pow(dmm/2.0 * 0.001,2);
        }else if(type == 2 && amm !=0.0 && bmm!=0.0){
            S = amm * bmm * 0.000001;
        }else{
            return 0.0;
        }
        return S;
    }

    inline double typeMpaTonTon(double mpa,int type, double dmm, double amm, double bmm){
        double S = 0.0;
        if(type == 0){
            return 0.0;
        }else if(type == 1 && dmm != 0.0){
            S = PI * pow(dmm/2.0 * 0.001,2);
        }else if(type == 2 && amm !=0.0 && bmm!=0.0){
            S = amm * bmm * 0.000001;
        }else{
            return 0.0;
        }
        return (mpa * 1000000.0 * S) / (1000.0 * g);
    }

    inline double tonToTypeMpa(double ton,int type, double dmm, double amm, double bmm){
        double S = 0.0;
        if(type == 0){
            return 0.0;
        }else if(type == 1 && dmm != 0.0){
            S = PI * pow(dmm/2.0 * 0.001,2);
        }else if(type == 2 && amm !=0.0 && bmm!=0.0){
            S = amm * bmm * 0.000001;
        }else{
            return 0.0;
        }
        return ton * 1000.0 * g / S / 1000000.0;
    }

    inline int tonToMachineKpa(double value){
        double dmm = epReadOnlySetData->s_Diameter * 0.001;
        double S = PI * pow(dmm/2,2);
        return std::round((value*g)/S);
    }

    inline double machineKpaToTon(int value){
        double dmm = epReadOnlySetData->s_Diameter * 0.001;
        double S = PI * pow(dmm/2,2);
        return value*S/g;
    }

    inline int overLoadValue(double upValue) {
        int maxValue = epReadOnlySetData->s_MaxLimit;
        double dmm = epReadOnlySetData->s_Diameter * 0.001;
        double S = PI * pow(dmm/2,2);
        int index = int(upValue*10)/maxValue;
        if(index>10){
            return 0;
        }
        return round(((upValue+epReadOnlySetData->s_OverLoadUps[index-1])*g)/S);
    }


    static const char *ActualPressDataPath = "actual.json";
    static const char *ReadOnlySetDataPath = "readonly.json";
}

extern ActualPressData *epActualPressData;

extern Machine *epMachine;

extern DataControl *epDataControl;


#endif //ZTABLE_PRESS_BOT_PROJECT_PCRHEAD_H
