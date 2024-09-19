//
// Created by 11518 on 2024/8/6.
//

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "actualpressdata.h"

//int ActualPressData::getSStep() const {
//    return s_Step;
//}
//
//void ActualPressData::setSStep(int sStep) {
//    s_Step = sStep;
//}
//
//int ActualPressData::getSType() const {
//    return s_Type;
//}
//
//void ActualPressData::setSType(int sType) {
//    s_Type = sType;
//}
//
//double ActualPressData::getSB() const {
//    return s_B;
//}
//
//void ActualPressData::setSB(double sB) {
//    s_B = sB;
//}
//
//double ActualPressData::getSD() const {
//    return s_D;
//}
//
//void ActualPressData::setSD(double sD) {
//    s_D = sD;
//}
//
//double ActualPressData::getSA() const {
//    return s_A;
//}
//
//void ActualPressData::setSA(double sA) {
//    s_A = sA;
//}
//
//const QVector<double> &ActualPressData::getSUps() const {
//    return s_Ups;
//}
//
//void ActualPressData::setSUps(const QVector<double> &sUps) {
//    s_Ups = sUps;
//}
//
//const QVector<double> &ActualPressData::getSDowns() const {
//    return s_Downs;
//}
//
//void ActualPressData::setSDowns(const QVector<double> &sDowns) {
//    s_Downs = sDowns;
//}
//
//const QVector<int> &ActualPressData::getSTimes() const {
//    return s_Times;
//}
//
//void ActualPressData::setSTimes(const QVector<int> &sTimes) {
//    s_Times = sTimes;
//}

void ActualPressData::init(int size) {
    s_Step = 5;
    s_Type = 2;
    s_D = 30.00;
    s_A = 10.00;
    s_B = 20.00;
    s_PushSet = 5;
    s_Ups.resize(size);
    s_Downs.resize(size);
    s_Times.resize(size);
    for(int i(0);i<size;i++){
        s_Ups[i] = 5*(i+1);
        s_Downs[i] = 4.9*(i+1);
        s_Times[i] = 30;
    }
}

double ActualPressData::maxUpValue() const {
    return *std::max_element(s_Ups.begin(), s_Ups.begin()+s_Step);
}

bool ActualPressData::openJsonFile(const QString &fileName) {
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly)){
        return false;
    }
    QByteArray byteArray = file.readAll();
    file.close();

    return openJsonString(byteArray);
}

bool ActualPressData::saveJsonFile(const QString &fileName) {
//    QJsonObject jsonObject;
//    jsonObject["Step"] = s_Step;
//    jsonObject["Type"] = s_Type;
//    jsonObject["D"] = s_D;
//
//    jsonObject["A"] = s_A;
//    jsonObject["B"] = s_B;
//    //	root["PushSet"] = s_PushSet;
//
//    QJsonArray jsonArrayUp;
//    for(auto &var:s_Ups){
//        jsonArrayUp.append(var);
//    }
//    jsonObject["Ups"] = jsonArrayUp;
//
//
//    QJsonArray jsonArrayDown;
//    for(auto &var:s_Downs){
//        jsonArrayDown.append(var);
//    }
//    jsonObject["Downs"] = jsonArrayDown;
//
//
//    QJsonArray jsonArrayTime;
//    for(auto &var:s_Times){
//        jsonArrayTime.append(var);
//    }
//    jsonObject["Times"] = jsonArrayTime;
//
//    QJsonDocument jsonDocument;
//    jsonDocument.setObject(jsonObject);
    QByteArray byteArray = saveJsonString();

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly)){
        return false;
    }
    file.write(byteArray);
    file.close();
    return true;
}
//
//void ActualPressData::setSUpValueIndex(int index, double value) {
//    if(index < s_Ups.size()){
//        s_Ups[index] = value;
//    }
//}
//
//void ActualPressData::setSDownValueIndex(int index, double value) {
//    if(index < s_Downs.size()){
//        s_Downs[index] = value;
//    }
//}
//
//void ActualPressData::setSTImeValueIndex(int index, int value) {
//    if(index < s_Times.size()){
//        s_Times[index] = value;
//    }
//}
//
QVector<PressCmd> ActualPressData::getPressCmdList() const {
    QVector<PressCmd> pressCmdList;
    for (int i = 0; i < s_Step; ++i) {
        if(i>=1){
            if(s_Ups[i] < s_Ups[i-1]) {
                pressCmdList.append(PressCmd{i,CmdTypeStop, 0, 0,0});
            }
        }
        pressCmdList.append(PressCmd{i,CmdTypeSet,s_Ups[i],s_Downs[i],this->s_Times[i]});
        for (int j = 0; j < s_Times[i]; ++j) {
            pressCmdList.append(PressCmd{i,CmdTypeGet,s_Ups[i],s_Downs[i],0});
        }
    }
    pressCmdList.append(PressCmd{1,CmdTypeStop,0,0});
    return pressCmdList;
}

bool ActualPressData::openJsonString(const QByteArray &byteArray) {
    QJsonParseError error{};
    QJsonDocument jsonDocument = QJsonDocument::fromJson(byteArray,&error);
    if(QJsonParseError::NoError!=error.error){
        return false;
    }
    QJsonObject jsonObject = jsonDocument.object();

    s_Step = jsonObject["Step"].toInt();
    s_Type = jsonObject["Type"].toInt();
    s_D = jsonObject["D"].toDouble();

    s_A = jsonObject["A"].toDouble();
    s_B = jsonObject["B"].toDouble();
    s_PushSet = jsonObject["PushSet"].toDouble();

    QJsonArray arrayUps = jsonObject["Ups"].toArray();
    int size = arrayUps.size();
    s_Ups.resize(size);
    for (int i = 0; i < size; ++i) {
        s_Ups[i] = arrayUps[i].toDouble();
    }
    QJsonArray arrayDowns = jsonObject["Downs"].toArray();
    size = arrayDowns.size();
    s_Downs.resize(size);
    for (int i = 0; i < size; ++i) {
        s_Downs[i] = arrayDowns[i].toDouble();
    }
    QJsonArray arrayTimes = jsonObject["Times"].toArray();
    size = arrayTimes.size();
    s_Times.resize(size);
    for (int i = 0; i < size; ++i) {
        s_Times[i] = arrayTimes[i].toInt();
    }
    return true;
}

QByteArray ActualPressData::saveJsonString() {
    QJsonObject jsonObject;
    jsonObject["Step"] = s_Step;
    jsonObject["Type"] = s_Type;
    jsonObject["D"] = s_D;

    jsonObject["A"] = s_A;
    jsonObject["B"] = s_B;
    jsonObject["PushSet"] = s_PushSet;

    QJsonArray jsonArrayUp;
    for(auto &var:s_Ups){
        jsonArrayUp.append(var);
    }
    jsonObject["Ups"] = jsonArrayUp;


    QJsonArray jsonArrayDown;
    for(auto &var:s_Downs){
        jsonArrayDown.append(var);
    }
    jsonObject["Downs"] = jsonArrayDown;


    QJsonArray jsonArrayTime;
    for(auto &var:s_Times){
        jsonArrayTime.append(var);
    }
    jsonObject["Times"] = jsonArrayTime;

    QJsonDocument jsonDocument;
    jsonDocument.setObject(jsonObject);
    return jsonDocument.toJson();
}

QString ActualPressData::typeString() const {
    if(s_Type == 0){
        return "异性";
    }else if(s_Type == 1){
        return "圆形";
    }else if(s_Type == 2){
        return "矩形";
    }else{
        return "错误";
    }
}

QString ActualPressData::sizeString() const {
    if(s_Type == 0){
        return "";
    }else if(s_Type == 1){
        return QString("%1mm").arg(s_D);
    }else if(s_Type == 2){
        return QString("%1mm X %2mm").arg(s_A).arg(s_B);
    }else{
        return "错误";
    }
}

QVector<PressCmd> ActualPressData::getPushCmdList() const {
    QVector<PressCmd> pressCmdList;
    pressCmdList.append(PressCmd{0,CmdTypeSet,s_PushSet,s_PushSet-0.3,3});
    for (int j = 0; j < 3; ++j) {
        pressCmdList.append(PressCmd{0,CmdTypeGet,s_PushSet,s_PushSet-0.3,0});
    }

    return pressCmdList;
}
