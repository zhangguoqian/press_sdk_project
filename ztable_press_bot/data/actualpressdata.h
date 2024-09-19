//
// Created by 11518 on 2024/8/6.
//

#ifndef ZTABLE_PRESS_BOT_PROJECT_ACTUALPRESSDATA_H
#define ZTABLE_PRESS_BOT_PROJECT_ACTUALPRESSDATA_H

#include <QVector>

enum CmdType{
    CmdTypeSet,
    CmdTypeGet,
    CmdTypeStop,
};

struct PressCmd{
    int s_Step;
    CmdType s_CmdType;
    double s_UpValue;
    double s_DownValue;
    int s_Timer;
};

class ActualPressData {
public:
	bool openJsonString(const QByteArray &byteArray);
	bool openJsonFile(const QString &fileName);
    QByteArray saveJsonString();
	bool saveJsonFile(const QString &fileName);
	double maxUpValue() const;
	void init(int size);
    QString typeString() const;
    QString sizeString() const;
//
//public:
//    int getSStep() const;
//
//    void setSStep(int sStep);
//
//    int getSType() const;
//
//    void setSType(int sType);
//
//    double getSB() const;
//
//    void setSB(double sB);
//
//    double getSD() const;
//
//    void setSD(double sD);
//
//    double getSA() const;
//
//    void setSA(double sA);
//
//    const QVector<double> &getSUps() const;
//
//    void setSUps(const QVector<double> &sUps);
//
//    void setSUpValueIndex(int index,double value);
//
//    const QVector<double> &getSDowns() const;
//
//    void setSDowns(const QVector<double> &sDowns);
//    void setSDownValueIndex(int index,double value);
//
//    const QVector<int> &getSTimes() const;
//
//    void setSTimes(const QVector<int> &sTimes);
//    void setSTImeValueIndex(int index,int value);
//
    QVector<PressCmd> getPressCmdList() const;
    QVector<PressCmd> getPushCmdList() const;
//private:
    int s_Step;
    int s_Type;
	double s_D;
	double s_A;
	double s_B;
	double s_PushSet;
	QVector<double> s_Ups;
	QVector<double> s_Downs;
	QVector<int> s_Times;
};


#endif //ZTABLE_PRESS_BOT_PROJECT_ACTUALPRESSDATA_H
