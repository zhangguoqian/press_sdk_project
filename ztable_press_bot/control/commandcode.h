//
// Created by 11518 on 2026/9/3.
//

#ifndef ZTABLE_PRESS_BOT_COMMANDCODE_H
#define ZTABLE_PRESS_BOT_COMMANDCODE_H

#define SET_CMD(CMDID)    (CMDID | 0x1000)
#define IS_SET_CMD(CMDID) ((CMDID & 0x1000) >> 12)

#define GET_BOOL(size)				(size == 1)

enum ROD_CMDID{
	CMDID_VERSION  		   = 0x0000,     //获取版本
	CMDID_NAME             = 0x0001,     //获取名字
	CMDID_MACHINE_TYPE     = 0x0002,     //获取机器型号
	CMDID_SERIAL_NUMBER    = 0x0003,     //序列号
	CMDID_PRESS_PARAMETER  = 0x0004,     //压力参数
	CMDID_TEMP_PARAMETER   = 0x0005,     //温度参数
	CMDID_OTHER_INFO       = 0x0006,     //其他信息

	CMDID_ROD_JSON         = 0x00FF,     //json信息
};

#define VERSION_BOOL(size)				(size == 4)
#define NAME_BOOL(size)					(size > 1 && size < 32)
#define MACHINE_TYPE_BOOL(size)			(size <= 32)
#define SERIAL_NUMBER_TYPE_BOOL(size)	(size <= 32)
#define PRESS_PARAMETER_TYPE_BOOL(size) (size == 20)
#define TEMP_PARAMETER_TYPE_BOOL(size)	(size == 12)
#define OTHER_INFO_TYPE_BOOL(size)		(size == 5)


enum PRESS_CMDID{
	CMDID_PRESS_STEP       		= 0x0100,
	CMDID_PRESS_CALIBRATE		= 0x0101,
	CMDID_MOLD_TYPE             = 0x0102,
	CMDID_CIRCLE_D              = 0x0103,
	CMDID_RECT_AB    			= 0x0104,
	CMDID_RING_OUT_IN 			= 0x0105,
	CMDID_AIR_TIME              = 0x0106,
	CMDID_INDEX_PRESS           = 0x0107,
	CMDID_INDEX_AFTER           = 0x0108,
	CMDID_INDEX_KPTIME          = 0x0109,
	CMDID_INDEX_PSTEP_PARAMETER = 0x010A,
	CMDID_PRESS_DECIMAL 		= 0x010B,
	CMDID_PRESSURE_DECIMAL 		= 0x010C,
	CMDID_PMODEL 				= 0x010D,
	CMDID_DEMOLD_VALUE 			= 0x010E,


	CMDID_PD_JSON        		= 0x01FF,
};

#define PRESS_STEP_BOOL(size) 				(size == 1)
#define PRESS_CALIBRATE_BOOL(size) 			(size == 4)
#define MOLD_TYPE_BOOL(size) 				(size == 1)
#define CIRCLE_D_BOOL(size) 				(size == 4)
#define RECT_AB_BOOL(size)					(size == 8)
#define RING_OUT_IN_BOOL(size) 				(size == 8)
#define AIR_TIME_BOOL(size) 				(size == 1)
#define INDEX_PRESS_BOOL(size) 				(size == 5)
#define INDEX_AFTER_BOOL(size)				(size == 5)
#define INDEX_KPTIME_BOOL(size)				(size == 5)
#define INDEX_PSTEP_PARAMETER_BOOL(size)	(size == 13)

#define PRESS_DECIMAL_BOOL(size)			(size == 1)
#define PRESSURE_DECIMAL_BOOL(size)			(size == 1)
#define PMODEL_BOOL(size)					(size == 1)
#define DEMOLD_VALUE_BOOL(size)				(size == 4)


enum TEMP_CMDID{
	CMDID_TEMP_STEP       		= 0x0200,
	CMDID_SPEED_MOLD			= 0x0201,
	CMDID_INDEX_TEMP            = 0x0202,
	CMDID_INDEX_SPEED           = 0x0203,
	CMDID_INDEX_KTTIME          = 0x0204,
	CMDID_INDEX_TSTEP_PARAMETER = 0x0205,
	CMDID_WATER_UP              = 0x0206,
	CMDID_WATER_DOWN            = 0x0207,

	CMDID_TD_JSON        		= 0x02FF,
};

#define TEMP_STEP_BOOL(size) 				(size == 1)
#define SPEED_MOLD_BOOL(size) 				(size == 1)
#define INDEX_TEMP_BOOL(size) 				(size == 5)
#define INDEX_SPEED_BOOL(size) 				(size == 5)
#define INDEX_KTTIME_BOOL(size) 			(size == 5)
#define INDEX_TSTEP_PARAMETER_BOOL(size) 	(size == 13)
#define WATER_UP_BOOL(size) 				(size == 4)
#define WATER_DOWN_BOOL(size) 				(size == 4)


enum MIX_CMDID{
	CMDID_MIX_STEP       			= 0x0300,
	CMDID_MIX_MOLD_TYPE       		= 0x0301,
	CMDID_MIX_CIRCLE_D          	= 0x0302,
	CMDID_MIX_RECT_AB    			= 0x0303,
	CMDID_MIX_RING_OUT_IN 			= 0x0304,
	CMDID_MIX_INDEX_ISPRESS     	= 0x0305,
	CMDID_MIX_INDEX_PRESS       	= 0x0306,
	CMDID_MIX_INDEX_AFTER       	= 0x0307,
	CMDID_MIX_INDEX_ISHEAT_UP  	 	= 0x0308,
	CMDID_MIX_INDEX_TEMP_UP    	 	= 0x0309,
	CMDID_MIX_INDEX_ISHEAT_DOWN 	= 0x030A,
	CMDID_MIX_INDEX_TEMP_DOWN   	= 0x030B,
	CMDID_MIX_INDEX_KTIME          	= 0x030C,
	CMDID_MIX_INDEX_STEP_PARAMETER 	= 0x030D,

	CMDID_MD_JSON        		    = 0x03FF,
};

#define MIX_STEP_BOOL(size) 					(size == 1)
#define MIX_MOLD_TYPE_BOOL(size) 				(size == 1)
#define MIX_CIRCLE_D_BOOL(size) 				(size == 4)
#define MIX_RECT_AB_BOOL(size) 				(size == 8)
#define MIX_RING_OUT_IN_BOOL(size) 			(size == 8)
#define MIX_INDEX_ISPRESS_BOOL(size) 			(size == 2)
#define MIX_INDEX_PRESS_BOOL(size) 			(size == 5)
#define MIX_INDEX_AFTER_BOOL(size) 			(size == 5)
#define MIX_INDEX_ISHEAT_UP_BOOL(size) 		(size == 2)
#define MIX_INDEX_TEMP_UP_BOOL(size) 			(size == 5)
#define MIX_INDEX_ISHEAT_DOWN_BOOL(size) 		(size == 2)
#define MIX_INDEX_TEMP_DOWN_BOOL(size) 		(size == 5)
#define MIX_INDEX_KTIME_BOOL(size) 			(size == 5)
#define MIX_INDEX_STEP_PARAMETER_BOOL(size)	(size == 23)


enum SYSTEM_CMDID{
	CMDID_DATE_TIME       		= 0x0400,
	CMDID_BRIGHTNESS			= 0x0401,
	CMDID_BUTTON_SOUND          = 0x0402,
	CMDID_LANGUAGE              = 0x0403,
};

#define DATE_TIME_BOOL(size) 				(size == 6)
#define BRIGHTNESS_BOOL(size) 				(size == 1)
#define BUTTON_SOUND_BOOL(size) 			(size == 1)
#define LANGUAGE_BOOL(size) 				(size == 1)

enum STATE_CMDID{
	CMDID_GET_CURRENT_STATE     = 0x0500,
	CMDID_SET_START_PRESS		= 0x1521,
	CMDID_SET_START_HEAT		= 0x1522,
	CMDID_SET_START_DEMOLD		= 0x1523,

	CMDID_SET_START				= 0x1511,

	CMDID_SET_START_P			= 0x1531,
	CMDID_SET_START_TU			= 0x1532,
	CMDID_SET_START_TD			= 0x1533,

	CMDID_SELECT_MODEL_STATE         = 0x1507,

	CMDID_GET_CURRENT_STATE_JSON     = 0x05FF,

};

#define SET_START_BOOL(size) 				(size == 1)
#define SET_JSON_BOOL(size) 				(size > 3)

#define CMID_TYPE(CMDID)  ((CMDID & 0x0F00) >> 8)


#endif //ZTABLE_PRESS_BOT_COMMANDCODE_H
