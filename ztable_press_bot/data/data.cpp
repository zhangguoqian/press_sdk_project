/*
 * data.cpp
 *
 *  Created on: 2023年4月11日
 *      Author: 11518
 */

#include "data.h"
#include "zfile.h"
//
//bool ActualPressData::openJsonFile(const std::string &fileName)
//{
//    std::string all = ZGLOBAL::readFile(fileName);
//    if(openJsonString(all))
//    {
//        return true;
//    }
//    return false;
//}
//
//bool ActualPressData::openJsonString(const std::string &str)
//{
//    if(str.empty())
//    {
//        return false;
//    }
//    //1.创建工厂对象
//    Json::CharReaderBuilder ReaderBuilder;
//    ReaderBuilder["emitUTF8"] = true;//utf8支持,不加这句,utf8的中文字符会编程\uxxx
//
//    JsonControl jsonControl;
//    Json::Value root;
//    Json::Reader reader;
//
//    if (!reader.parse(str,root, true))
//    {
//        return false;
//    }
//    s_Step = jsonControl.getInt("Step", root);
//    s_Type = jsonControl.getInt("Type", root);
//    s_D = jsonControl.getDouble("D", root);
//
//    s_A = jsonControl.getDouble("A", root);
//    s_B = jsonControl.getDouble("B", root);
//    //	s_PushSet = jsonControl.getInt("PushSet", root);
//
//    s_Ups = jsonControl.getVectorDouble("Ups", root,5);
//    s_Downs = jsonControl.getVectorDouble("Downs", root,5);
//    s_Times = jsonControl.getVectorInt("Times", root,5);
//
//    return true;
//}
//
////int s_Step = 5;
////int s_Type = 1;
////double s_D = 30.00;
//
////double s_A = 10.00;
////double s_B = 20.00;
////int s_PushSet = 5;
//
////std::vector<double> s_UpValues;
////std::vector<double> s_DownValues;
////std::vector<int> s_TimeValues;
//
//bool ActualPressData::savaJsonString(std::string& str) {
//    //1.写json的工厂对象
//    Json::StreamWriterBuilder writebuild;
//    writebuild["emitUTF8"] = true;//utf8支持,加这句,utf8的中文字符会编程\uxxx
//
//    Json::Value root;
//
//    root["Step"] = s_Step;
//    root["Type"] = s_Type;
//    root["D"] = s_D;
//
//    root["A"] = s_A;
//    root["B"] = s_B;
//    //	root["PushSet"] = s_PushSet;
//
//    Json::Value array;
//    for(auto &var:s_Ups){
//        array.append(var);
//    }
//    root["Ups"] = array;
//
//    array.clear();
//    for(auto &var:s_Downs){
//        array.append(var);
//    }
//    root["Downs"] = array;
//    array.clear();
//    for(auto &var:s_Times){
//        array.append(var);
//    }
//    root["Times"] = array;
//    array.clear();
//    str = root.toStyledString();
//
//    //	LOGD("string:%s",str.c_str());
//    return true;
//}
//
//bool ActualPressData::saveJsonFile(const std::string& fileName) {
//    std::string str;
//    savaJsonString(str);
//    return ZGLOBAL::writeFile(fileName, str);
//}
//
//double ActualPressData::maxUpValue() const {
//    return *std::max_element(s_Ups.begin(), s_Ups.begin()+s_Step);
//}
//
//void ActualPressData::init(int size) {
//    s_Step = 5;
//    s_Type = 2;
//    s_D = 30.00;
//    s_A = 10.00;
//    s_B = 20.00;
//    //	s_PushSet = 5;
//    s_Ups.resize(size);
//    s_Downs.resize(size);
//    s_Times.resize(size);
//    for(int i(0);i<size;i++){
//        s_Ups[i] = 5*(i+1);
//        s_Downs[i] = 4.9*(i+1);
//        s_Times[i] = 30;
//    }
//}
//
//void ReadOnlySetData::init() {
//    s_MaxLimit = 20.0;//s_MaxLimit = 30.0;
//    s_MinLimit = 0.2;//s_MinLimit = 0.3;
//    s_Max_Min = 0.1;//s_Max_Min = 0.3;
//    s_Diameter = 95.0;//s_Diameter = 110.0;
//    s_OverLoadUps.resize(10, 0.0);
//    s_MaxPushSet = 10.0;//s_MaxPushSet = 6.0;
//    s_PushSet = 5.0;
//    s_Beep = true;
//    s_Type = "HAP-20S";//s_Type = "Type";
//    s_Screenshot = false;
//    s_NameCN = "自动压片机";
//    s_NameEN = "AutoPressBot";
//    s_PressPrecision = 2;
//}
//
//bool ReadOnlySetData::openJsonFile(const std::string& fileName) {
//    std::string all =ZGLOBAL::readFile(fileName);
////    all = makeCode(all, 'z');
//    if(openJsonString(all))
//    {
//        return true;
//    }
//    return false;
//}
//
//bool ReadOnlySetData::openJsonString(const std::string& str) {
//    if(str.empty())
//    {
//        return 0;
//    }
//    //1.创建工厂对象
//    Json::CharReaderBuilder ReaderBuilder;
//    ReaderBuilder["emitUTF8"] = true;//utf8支持,不加这句,utf8的中文字符会编程\uxxx
//
//    JsonControl jsonControl;
//    Json::Value root;
//    Json::Reader reader;
//
//    if (!reader.parse(str,root, false))
//    {
//        return 0;
//    }
//    s_MaxLimit = jsonControl.getDouble("Max", root);
//    s_MinLimit = jsonControl.getDouble("Min", root);
//    s_Max_Min = jsonControl.getDouble("Max_Min", root);
//    s_Diameter = jsonControl.getDouble("Diameter", root);
//    s_OverLoadUps = jsonControl.getVectorDouble("OverLoads",root,10);
//    s_MaxPushSet = jsonControl.getDouble("MaxPushSet", root);
//    s_PushSet = jsonControl.getDouble("PushSet", root);
//    s_Beep = jsonControl.getBool("Beep", root);
//    s_Type = jsonControl.getString("Type", root);
//    s_Screenshot = jsonControl.getBool("Screenshot", root);
//    s_NameCN = jsonControl.getString("NameCN", root);
//    s_NameEN = jsonControl.getString("NameEN", root);
//    s_PressPrecision = jsonControl.getInt("PressPrecision", root);
//    return true;
//}
//
//bool ReadOnlySetData::savaJsonString(std::string& str) {
//    //1.写json的工厂对象
//    Json::StreamWriterBuilder writebuild;
//    writebuild["emitUTF8"] = true;//utf8支持,加这句,utf8的中文字符会编程\uxxx
//
//    Json::Value root;
//    root["Max"] = s_MaxLimit;
//    root["Min"] = s_MinLimit;
//    root["Max_Min"] = s_Max_Min;
//    root["Diameter"] = s_Diameter;
//    root["Beep"] = s_Beep;
//    root["Type"] = s_Type;
//    root["Screenshot"]=s_Screenshot;
//    root["NameCN"]=s_NameCN;
//    root["NameEN"]=s_NameEN;
//    root["PressPrecision"]=s_PressPrecision;
//
//    Json::Value array;
//    for(auto &var:s_OverLoadUps){
//        array.append(var);
//    }
//    root["OverLoads"] = array;
//    array.clear();
//    root["PushSet"] = s_PushSet;
//    root["MaxPushSet"] = s_MaxPushSet;
//
//    str = root.toStyledString();
////    str = makeCode(str, 'z');
//    //	LOGD("string:%s",str.c_str());
//    return 1;
//}
//
//bool ReadOnlySetData::saveJsonFile(const std::string& fileName) {
//    std::string str;
//    savaJsonString(str);
//    return ZGLOBAL::writeFile(fileName, str);
//}
