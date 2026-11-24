#include "param_head.h"

/*1:左股  -1 右股*/
int8_t  left_And_Righe_Status = 1;              //当前左右股状态
int8_t  left_And_Righe_Status_History = -1;     //上一次左右股状态
uint32_t Driver_Calibration_Password = 997900;  //标定界面登录密码
PAGE_ID Page_id = page_size;                    //当前页面ID号
float  POWER_Vol = 3.3;                         //单片机供电电压 （ADC检测范围电压量程）



uint8_t menu_Id_Select = 0;                     //菜单界面 当前选择的菜单项
uint8_t menu_Id_History = 0;                    //菜单界面 上一次选择的菜单项

float   CG_Value_History = 0;                   //主界面   上一次的超高示值
float   GJ_Value_History = 0;                   //主界面   上一次的轨距示值
float   CG_Value = 180.12;                      //主界面   当前的超高示值
float   GJ_Value = 1435.11;                     //主界面   当前的轨距示值
float   CZ_Value = 0.0f;                     //查照数据
float   HB_Value = 0.0f;                    //护背数据

uint8_t Set_Time[6]    = {24,8,5,1,59,32};      //时间设置界面 时间输入缓存数组分别对应 年 月 日 时 分 秒
uint8_t Set_Time_Select = 0;                    //时间设置界面 当前选择要修改的时间项
uint8_t Set_Time_Select_History = 1;            //时间设置界面 上一次选择要修改的时间项

uint8_t Goal_Set_Status         = 0;            //目标设置界面 轨距超高目标设置状态：1目标模式开启  0目标模式失败
uint8_t Goal_Set_GJ_1           = 3;            //目标设置界面 目标轨距设置整数十分位
uint8_t Goal_Set_GJ_2           = 5;            //目标设置界面 目标轨距设置整数个分位
uint8_t Goal_Set_GJ_3           = 0;            //目标设置界面 目标轨距设置区间（小数位）

uint8_t Goal_Set_CG_1           = 0;            //目标设置界面 目标超高设置整数符号位
uint8_t Goal_Set_CG_2           = 0;            //目标设置界面 目标超高设置整数百分位
uint8_t Goal_Set_CG_3           = 0;            //目标设置界面 目标超高设置整数十分位
uint8_t Goal_Set_CG_4           = 0;            //目标设置界面 目标超高设置整数个分位
uint8_t Goal_Set_CG_5           = 0;            //目标设置界面 目标超高设置区间（小数位）
uint8_t Goal_Set_Select         = 0;            //目标设置界面 当前选择要设置的位项
uint8_t Goal_Set_Select_History = 0;            //目标设置界面 上一次选择要设置的位项

float   Goal_CJ_Max_Value = 0;                  //目标设置界面 目标轨距范围区间的最大值
float   Goal_CJ_Min_Value = 0;                  //目标设置界面 目标轨距范围区间的最小值
        
float   Goal_CG_Max_Value = 0;                  //目标设置界面 目标超高范围区间的最大值
float   Goal_CG_Min_Value = 0;                  //目标设置界面 目标超高范围区间的最小值

uint8_t Password_Input_Temp[6] = {0};           //密码输入界面/密码设置界面  密码输入缓存数组
uint8_t Password_Input_Select = 0;              //密码输入界面/密码设置界面  当前选择要输入的位项
uint8_t Password_Input_Select_History = 1;      //密码输入界面/密码设置界面  上一次选择要输入的位项

uint8_t Calibration_Menu_Select = 0;            //标定菜单界面  当前选择标定菜单项
uint8_t Calibration_Menu_Select_History = 1;    //标定菜单界面  上一次选择标定菜单项

uint8_t Percision_Menu_Select = 0;              //精度设置界面  当前选择精度项
uint8_t Percision_Menu_Select_History = 1;      //精度设置界面  上一次选择精度项
uint8_t Data_Show_Percision = 1;                //精度设置界面  当前设置精度  0：精度1    1：精度2

uint8_t CG_Cal_Save_Failed = 0;                 // [CG32-05] 0=OK,1=EEP,2=bad CG cal,3=no sensor
uint8_t CG_Calibration_Status = 0;              //超高标定界面  当前超高标定步骤
uint8_t CG_Calibration_Status_History = 1;      //超高标定界面  上一次超高标定步骤

uint8_t GJ_Calibration_Status = 0;              //轨距标定界面  当前轨距标定步骤
uint8_t GJ_Calibration_Status_History = 1;      //轨距标定界面  上一次轨距标定步骤

uint8_t CH_Cal_Save_Failed = 0U; /* [CG38] 1=EEP,2=cal,3=link,4=unstable */
uint8_t CH_Calibration_Status = 0;              //查护标定界面  当前查护标定步骤
uint8_t CH_Calibration_Status_History = 1;      //查护标定界面  上一次查护标定步骤


uint8_t U2_PC_Work_Mode = 0;                    //通讯模式
uint32_t U2_Cycle_Send_Mode_Time_Current = 0;                    //通讯模式
uint32_t U2_Cycle_Send_Mode_Time_History = 0;                    //通讯模式


/*超高算法原始参数数据*/
CG_Calibration_Param_Str CG_Param = {1,-12445,50,12513,20};
CG_Calibration_Param_Str CG_Param_Temp;
/*轨距算法原始参数数据*/
GJ_Calibration_Param_Str GJ_Param = {7781,15946,27395,20};
GJ_Calibration_Param_Str GJ_Param_Temp;
/*查护算法原始参数数据*/
CH_Calibration_Param_Str CH_Param = {10000,13000,20};
CH_Calibration_Param_Str CH_Param_Temp;
/*倾角板传感器原始数据*/
int16_t  CG_Ad;                                 //倾角传感器采集倾角AD值
uint16_t GJ_Ad;                                 //位移传感器采集距离AD值
float    Temp;                                  //传感器内部温度





PAGE_ID get_PageId(void)
{
    return Page_id;
}

void set_PageId(PAGE_ID pageId)
{
    Page_id = pageId;
}

















