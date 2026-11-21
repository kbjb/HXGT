#ifndef __PARAM_HEAD_H
#define __PARAM_HEAD_H

#include "main.h"



typedef enum
{
    page_logo = 0,          //开启启动页
    page_main,              //轨距超高页面
    page_ch,                //查照护背页面
    page_menu,              //快捷菜单页面
    page_time,              //时间设置页面
    page_goal,              //目标设置页面
    page_calibration,       //数据标定页面（密码登录）
    page_password,          //密码设置页面（密码登录）
    page_percision,         //精度设置页面
    page_cg_calibration,    //超高标定页面
    page_gj_calibration,    //轨距标定页面
    page_ch_calibration,    //查护标定页面
    page_password_change,   //密码修改界面
    page_calibration_menu,  //标定菜单界面
    page_size,
}PAGE_ID;


/*超高标定系数参数*/
typedef struct
{
    int16_t CG_Zero_Left_Ad;
    int16_t CG_Max_Left_Ad;
    int16_t CG_Zero_Right_Ad;
    int16_t CG_Max_Right_Ad;
    float   temp;
}CG_Calibration_Param_Str;


/*轨距标定系数参数*/
typedef struct
{
    uint16_t GJ_1410_Ad;
    uint16_t GJ_1435_Ad;
    uint16_t GJ_1470_Ad;
    float   temp;
}GJ_Calibration_Param_Str;

/*查护标定系数参数*/
typedef struct
{
    uint16_t CZ_Ad;
    uint16_t HB_Ad;
    float   temp;
}CH_Calibration_Param_Str;

/*无符号32位与浮点数联合体*/
typedef union
{
    uint32_t inf;
    float    flo;
}FToI;

/*无符号16位与浮有符号16位联合体*/
typedef union
{
    int16_t  inf;
    uint16_t uinf;
}UToI;



typedef enum
{
    /*矩阵按键*/
    KEY_ID_ON= 0,
    KEY_ID_CHANGE,
    KEY_ID_SET ,
    KEY_ID_CANCEL,
    KEY_ID_LEFT,
    KEY_ID_RIGHT,
    KEY_ID_STORAGE,
    /*按键总数*/
    KEY_ID_MAX
}KEY_ID;

/*按键状态*/
typedef enum
{
    KEY_STATUS_SHORT,
    KEY_STATUS_LONG,
    /*按键总数*/
    KEY_STATUS_MAX,
}KEY_STATUS;

/*按键、当前界面与对应回调函数路由表结构体*/
typedef struct
{
    KEY_ID      Key_id;
    KEY_STATUS  key_status;
    PAGE_ID     page_id;
    void (*Key_Fun)(void);
}KEY_FUN_T;


/*按键检测状态机 $未使用*/
typedef enum {
    STATE_RELEASED = 0,     //按键空闲
    STATE_DEBOUNCE_PRESS,   //按下消抖中
    STATE_PRESSED,          //按下中
    STATE_DEBOUNCE_RELEASE, //按键释放
} Key_State_t;



/*按键参数结构体 $未使用*/
typedef struct
{
    Key_State_t     current_state;    // 当前状态
    GPIO_PinState   last_phy_state;   // 上一次物理状态
    GPIO_PinState   goal_phy_state;   // 上一次物理状态
    uint32_t        press_start_tick; // 按下开始的时刻
    uint8_t         is_debouncing;    // 消抖中标志
    uint16_t        debounce_count;   // 消抖计数器
    
}KEY_PARAM_T;

#pragma pack(1) // 禁用对齐，保证内存连续
typedef struct
{
    uint8_t  year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  type;
    uint32_t data1;
    uint32_t data2;
}EEP_NODE;
#pragma pack()




PAGE_ID get_PageId(void); 
void set_PageId(PAGE_ID pageId);



#define MENU_ID_NUMBER  5





extern int8_t  left_And_Righe_Status;
extern int8_t  left_And_Righe_Status_History;
extern PAGE_ID Page_id;
extern uint8_t menu_Id_Select ;
extern uint8_t menu_Id_History;
extern float   CG_Value_History ;
extern float   GJ_Value_History ;
extern float   CG_Value;
extern float   GJ_Value;
extern float   CZ_Value;
extern float   HB_Value;
extern uint8_t Set_Time[6] ;
    
extern uint8_t Set_Time_Select;
extern uint8_t Set_Time_Select_History;

extern uint8_t Goal_Set_GJ_1;
extern uint8_t Goal_Set_GJ_2;
extern uint8_t Goal_Set_GJ_3;
                                
extern uint8_t Goal_Set_Status;
extern uint8_t Goal_Set_CG_1;
extern uint8_t Goal_Set_CG_2;
extern uint8_t Goal_Set_CG_3;
extern uint8_t Goal_Set_CG_4;
extern uint8_t Goal_Set_CG_5;
extern uint8_t Goal_Set_Select;
extern uint8_t Goal_Set_Select_History;

extern float   Goal_CJ_Max_Value;
extern float   Goal_CJ_Min_Value;
extern float   Goal_CG_Max_Value;
extern float   Goal_CG_Min_Value;

extern uint8_t Password_Input_Temp[6];
extern uint8_t Password_Input_Select;
extern uint8_t Password_Input_Select_History;

extern uint8_t Calibration_Menu_Select ;
extern uint8_t Calibration_Menu_Select_History ;

extern uint8_t Percision_Menu_Select ;
extern uint8_t Percision_Menu_Select_History ;
extern uint8_t Data_Show_Percision ;

enum
{
    CAL_SAVE_OK = 0,
    CAL_SAVE_FAILED,
    CAL_SAVE_BAD_POINTS,
    CAL_SAVE_NO_SENSOR,
    CAL_SAVE_TEMP_ERROR,
    CAL_SAVE_SENSOR_ERROR
};
extern uint8_t CG_Cal_Save_Failed;
extern uint8_t GJ_Cal_Save_Failed;
extern uint8_t CH_Cal_Save_Failed;
extern uint8_t CG_Calibration_Status;
extern uint8_t CG_Calibration_Status_History;

extern uint8_t GJ_Calibration_Status;
extern uint8_t GJ_Calibration_Status_History;

extern uint8_t CH_Calibration_Status;
extern uint8_t CH_Calibration_Status_History;

extern uint32_t Driver_Calibration_Password;

extern float  POWER_Vol;
extern uint8_t U2_PC_Work_Mode;
extern uint32_t U2_Cycle_Send_Mode_Time_Current;
extern uint32_t U2_Cycle_Send_Mode_Time_History;




/*按键逻辑与界面操作路由表*/
extern KEY_FUN_T Key_Funs[];
extern uint8_t Key_Funs_Number;
/*倾角板传感器原始数据*/
extern int16_t  CG_Ad;
extern uint16_t GJ_Ad;
extern float    Temp;
/*超高算法原始参数数据*/
extern CG_Calibration_Param_Str CG_Param;
extern CG_Calibration_Param_Str CG_Param_Temp;

/*轨距算法原始参数数据*/
extern GJ_Calibration_Param_Str GJ_Param;
extern GJ_Calibration_Param_Str GJ_Param_Temp;

/*查护算法原始参数数据*/
extern CH_Calibration_Param_Str CH_Param;
extern CH_Calibration_Param_Str CH_Param_Temp;

#endif


