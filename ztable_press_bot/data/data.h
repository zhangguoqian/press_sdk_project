/*
 * data.h
 *
 *  Created on: 2023年4月11日
 *      Author: 11518
 */

#ifndef JNI_DATA_DATA_H_
#define JNI_DATA_DATA_H_

#include <vector>
//#include "jsoncontrol.h"

//struct ActualPressData{
//	int s_Step;
//	int s_Type;
//	double s_D;
//	double s_A;
//	double s_B;
////	double s_PushSet;
//	std::vector<double> s_Ups;
//	std::vector<double> s_Downs;
//	std::vector<int> s_Times;
////	std::vector<int> s_MpaValues;
//
//	bool openJsonString(const std::string &str);
//	bool openJsonFile(const std::string &fileName);
//	bool savaJsonString(std::string &str);
//	bool saveJsonFile(const std::string &fileName);
//
//	double maxUpValue() const;
//
//	void init(int size);
//};

//struct ReadOnlySetData{
//	double s_MaxLimit = 30.0;
//	double s_MinLimit = 0.3;
//	double s_Max_Min = 0.3;
//	double s_Diameter = 110.0;
//	std::vector<double> s_OverLoadUps;
//	double s_MaxPushSet = 6.0;
//	double s_PushSet = 5.0;
//	bool s_Beep = true;
//	std::string s_Type = "Type";
//	bool s_Screenshot = false;
////	int s_Language = 0;
//	std::string s_NameCN = "自动压片机";
//	std::string s_NameEN = "AutomaticTabletPress";
//	int s_PressPrecision = 2;
//
//	void init();
//	bool openJsonString(const std::string &str);
//	bool openJsonFile(const std::string &fileName);
//	bool savaJsonString(std::string &str);
//	bool saveJsonFile(const std::string &fileName);
//
//private:
//	char makecodeChar(char c,int key){
//	    return c=c^key;
//	}
//	//加密
//	std::string makeCode(std::string str,char pkey){
//	    int len=str.size();//获取长度
//	    for(int i=0;i<len;i++)
//	        str[i]=makecodeChar(str[i],pkey);
//	    return str;
//	}
//
//};


#endif /* JNI_DATA_DATA_H_ */
