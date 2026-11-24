#include "App_key.h"
#include "pcf8563_head.h"
#include "key_head.h"
#include "App_Eep.h"
#include "Algorithm_head.h" /* [CGFIX-01] */
#include "cg_cal_guard.h" /* [CG32-03] */
#include "agreement_head.h" /* [CG32-05] guard stale calibration input */





static uint8_t Power_Status = 0;
void Power_Ctrl(void)
{
    if(Power_Status == 1)
    {
        //蜂鸣器响动1秒
        Power_Status = 0;
        Beep_Ctrl(100,1);
        //关机
        HAL_GPIO_WritePin(Power_Ctrl_GPIO_Port,Power_Ctrl_Pin,GPIO_PIN_RESET);
    }
    else
    {
        //开机
        HAL_GPIO_WritePin(Power_Ctrl_GPIO_Port,Power_Ctrl_Pin,GPIO_PIN_SET);
        //蜂鸣器响动1秒
        Power_Status = 1;
        Beep_Ctrl(100,1);
    }
}





/*电源键相关功能*/
uint8_t Get_Power_Status(void)
{
    return Power_Status;
}

void GoTo_Menu_Page(void)
{
    set_PageId(page_menu);
}
/*撤销键相关功能*/

void GoTo_Main_Page(void)
{
    set_PageId(page_main);
}

void GoTo_Calibration_Page(void)
{
    set_PageId(page_calibration);
}
void GoTo_Calibration_Menu_Page(void)
{
    set_PageId(page_calibration_menu);
}
void Cancel_Goal_Set(void)
{
    Goal_Set_Status = 0;
    Beep_Ctrl(100,1);
    
}
/*左按键相关功能*/
void GoTo_Ch_Page(void)
{
    set_PageId(page_ch);
}

void Reduce_Time_For_Time_Select(void)
{
    static const uint8_t maxima[6]={99,12,31,23,59,59};
    uint8_t minimum,maximum;
    if (Set_Time_Select>=6U) Set_Time_Select=0U;
    PCF8563_NormalizeEdit(Set_Time);
    minimum=(Set_Time_Select==1U || Set_Time_Select==2U) ? 1U : 0U;
    maximum=Set_Time_Select==2U ? PCF8563_DaysInMonth(Set_Time[0],Set_Time[1],0U) : maxima[Set_Time_Select];
    Set_Time[Set_Time_Select]=(Set_Time[Set_Time_Select]<=minimum) ? maximum : Set_Time[Set_Time_Select]-1U;
    PCF8563_NormalizeEdit(Set_Time);
    Set_Time_Error=0U;
}

void Reduce_Goal_For_Goal_Select(void)
{
    switch(Goal_Set_Select)
    {
        case 0:
            Goal_Set_GJ_1 = Goal_Set_GJ_1==1?7:Goal_Set_GJ_1-1;
            break;
        case 1:
            Goal_Set_GJ_2 = Goal_Set_GJ_2==0?9:Goal_Set_GJ_2-1;
            break;
        case 2:
            Goal_Set_GJ_3 = Goal_Set_GJ_3==1?9:Goal_Set_GJ_3-1;
            break;
        case 3:
            Goal_Set_CG_1 = Goal_Set_CG_1==0?2:0;
            break;
        case 4:
            Goal_Set_CG_2 = Goal_Set_CG_2==0?1:Goal_Set_CG_2-1;
            break;
        case 5:
            Goal_Set_CG_3 = Goal_Set_CG_3==0?9:Goal_Set_CG_3-1;
            break;
        case 6:
            Goal_Set_CG_4 = Goal_Set_CG_4==0?9:Goal_Set_CG_4-1;
            break;
        case 7:
            Goal_Set_CG_5 = Goal_Set_CG_5==1?9:Goal_Set_CG_5-1;
            break;
    }
}

void Reduce_Password_For_Password_Select(void)
{
    Password_Input_Temp[Password_Input_Select] = Password_Input_Temp[Password_Input_Select] == 0?9:Password_Input_Temp[Password_Input_Select]-1;
}
void Reduce_Calibration_For_Menu_Select(void)
{
    Calibration_Menu_Select = Calibration_Menu_Select == 1?5:Calibration_Menu_Select-2;
}

/*右按键相关功能*/

void InCrease_Time_For_Time_Select(void)
{
    static const uint8_t maxima[6]={99,12,31,23,59,59};
    uint8_t minimum,maximum;
    if (Set_Time_Select>=6U) Set_Time_Select=0U;
    PCF8563_NormalizeEdit(Set_Time);
    minimum=(Set_Time_Select==1U || Set_Time_Select==2U) ? 1U : 0U;
    maximum=Set_Time_Select==2U ? PCF8563_DaysInMonth(Set_Time[0],Set_Time[1],0U) : maxima[Set_Time_Select];
    Set_Time[Set_Time_Select]=(Set_Time[Set_Time_Select]>=maximum) ? minimum : Set_Time[Set_Time_Select]+1U;
    PCF8563_NormalizeEdit(Set_Time);
    Set_Time_Error=0U;
}
void InCrease_Goal_For_Goal_Select(void)
{
    switch(Goal_Set_Select)
    {
        case 0:
            Goal_Set_GJ_1 = Goal_Set_GJ_1==7?1:Goal_Set_GJ_1+1;
            break;
        case 1:
            Goal_Set_GJ_2 = Goal_Set_GJ_2==9?0:Goal_Set_GJ_2+1;
            break;
        case 2:
            Goal_Set_GJ_3 = Goal_Set_GJ_3==9?1:Goal_Set_GJ_3+1;
            break;
        case 3:
            Goal_Set_CG_1 = Goal_Set_CG_1==2?0:2;
            break;
        case 4:
            Goal_Set_CG_2 = Goal_Set_CG_2==1?0:Goal_Set_CG_2+1;
            break;
        case 5:
            Goal_Set_CG_3 = Goal_Set_CG_3==9?0:Goal_Set_CG_3+1;
            break;
        case 6:
            Goal_Set_CG_4 = Goal_Set_CG_4==9?0:Goal_Set_CG_4+1;
            break;
        case 7:
            Goal_Set_CG_5 = Goal_Set_CG_5==9?1:Goal_Set_CG_5+1;
            break;
    }
}

void InCrease_Password_For_Password_Select(void)
{
    Password_Input_Temp[Password_Input_Select] = Password_Input_Temp[Password_Input_Select] == 9?0:Password_Input_Temp[Password_Input_Select]+1;
}

void InCrease_Calibration_For_Menu_Select(void)
{
    Calibration_Menu_Select = Calibration_Menu_Select == 5?1:Calibration_Menu_Select+2;
}
/*切换键相关功能*/
void Change_Left_Or_Right(void)
{
    if(left_And_Righe_Status == 1)
        left_And_Righe_Status = -1;
    else
        left_And_Righe_Status = 1;
}

void Change_Menu_Menu_Id(void)
{
    menu_Id_Select = menu_Id_Select == (MENU_ID_NUMBER-1)?0:menu_Id_Select+1;
}

void Change_Time_Select_Id(void)
{
    Set_Time_Select=(Set_Time_Select>=5U) ? 0U : Set_Time_Select+1U;
}

void Change_Goal_Set_Select_Id(void)
{
    Goal_Set_Select = Goal_Set_Select == 7?0:Goal_Set_Select+1;
}

void Change_Password_Set_Select_Id(void)
{
    Password_Input_Select = Password_Input_Select == 5?0:Password_Input_Select+1;
}

void Change_Percision_Set_Select_Id(void)
{
    Percision_Menu_Select = Percision_Menu_Select == 3?1:3;
}


/*确认键相关功能*/
void Change_Menu_Page_For_Menu_Id(void)
{
    PAGE_ID Goal_Page = (PAGE_ID)(menu_Id_Select+page_time);
    if(Goal_Page>=page_time && Goal_Page< page_cg_calibration)
        set_PageId(Goal_Page);
}
void Change_Goal(void)
{
    Goal_CJ_Max_Value = 1400+Goal_Set_GJ_1*10+Goal_Set_GJ_2+Goal_Set_GJ_3*0.1;
    Goal_CJ_Min_Value = 1400+Goal_Set_GJ_1*10+Goal_Set_GJ_2-Goal_Set_GJ_3*0.1;
    
    Goal_CG_Max_Value = Goal_Set_CG_2*100+Goal_Set_CG_3*10+Goal_Set_CG_4+Goal_Set_CG_5*0.1;
    Goal_CG_Min_Value = Goal_Set_CG_2*100+Goal_Set_CG_3*10+Goal_Set_CG_4-Goal_Set_CG_5*0.1;
    if(Goal_Set_CG_1 == 2)
    {
        Goal_CG_Max_Value = Goal_CG_Max_Value*-1;
        Goal_CG_Min_Value = Goal_CG_Min_Value*-1;
    }
    Goal_Set_Status = 1;
    Beep_Ctrl(100,1);
    set_PageId(page_main);
    
}

void Password_Input_Confirm(void)
{
    uint32_t Password_Input_temp_Value = Password_Input_Temp[0]*100000+Password_Input_Temp[1]*10000
                                        +Password_Input_Temp[2]*1000+Password_Input_Temp[3]*100+Password_Input_Temp[4]*10+Password_Input_Temp[5];
    if(Password_Input_temp_Value == Driver_Calibration_Password)
    {
        if(get_PageId() == page_password)
            set_PageId(page_password_change);
        if(get_PageId() == page_calibration)
            set_PageId(page_calibration_menu);
    }
    else
        set_PageId(page_menu);
}

void Calibration_Menu_Select_Confirm(void)
{
    switch(Calibration_Menu_Select)
    {
        case 1:
            set_PageId(page_cg_calibration);
            break;
        case 3:
            set_PageId(page_gj_calibration);
            break;
        case 5:
            set_PageId(page_ch_calibration);
            break;
    }
}

void Percision_Show_Set_Confirm(void)
{
    if(Percision_Menu_Select == 1)
        Data_Show_Percision = 0;
    else
        Data_Show_Percision = 1;
    set_PageId(page_menu);
}

void Password_Set_Confirm(void)
{
    uint32_t Password_Temp = Password_Input_Temp[0]*100000+
                             Password_Input_Temp[1]*10000+
                             Password_Input_Temp[2]*1000+
                             Password_Input_Temp[3]*100+
                             Password_Input_Temp[4]*10+
                             Password_Input_Temp[5];
    /* [B3] Commit the complete CRC-protected calibration/password snapshot. */
    if(Eep_Save_Password(Password_Temp) == 1)
        Beep_Ctrl(100,1);
    else
        Beep_Ctrl(100,3);
    set_PageId(page_menu);
}

extern I2C_HandleTypeDef hi2c2;
void Change_Time_For_Set_Time(void)
{
    PCF8563_TimeTypeDef time={0};
    time.year=Set_Time[0]; time.month=Set_Time[1]; time.day=Set_Time[2];
    time.hour=Set_Time[3]; time.minute=Set_Time[4]; time.second=Set_Time[5];
    time.century=0U; /* Six-field editor uses 2000..2099. */
    time.weekday=PCF8563_Weekday(time.year,time.month,time.day);
    if (!PCF8563_TimeIsValid(&time))
    {
        Set_Time_Error=2U;
        Beep_Ctrl(100,3);
        return;
    }
    if (PCF8563_SetTime(&hi2c2,&time)==HAL_OK)
    {
        Set_Time_Error=0U;
        Beep_Ctrl(100,1);
        set_PageId(page_menu);
    }
    else
    {
        Set_Time_Error=3U;
        Beep_Ctrl(100,3);
    }
}

/*存储键相关功能*/


void CG_Calibration_Storage(void)
{
    int16_t captured_ad = 0;
    /* [CG32-05] Require fresh sensor data before capture or commit. */
    if ((CG_Calibration_Status == 0U || CG_Calibration_Status == 2U ||
         CG_Calibration_Status == 4U || CG_Calibration_Status == 6U ||
         CG_Calibration_Status == 8U) && !Sensor_Data_Is_Fresh())
    {
        CG_Cal_Save_Failed = 3U;
        CG_Calibration_Status_History = 0xFFU;
        Beep_Ctrl(100,3);
        return;
    }
    if ((CG_Calibration_Status == 0U || CG_Calibration_Status == 2U ||
         CG_Calibration_Status == 4U || CG_Calibration_Status == 6U) &&
        !Sensor_Get_Stable_CG(&captured_ad))
    {
        CG_Cal_Save_Failed = 4U; /* Still moving or not enough fresh samples. */
        CG_Calibration_Status_History = 0xFFU;
        Beep_Ctrl(100,3);
        return;
    }
    if (CG_Cal_Save_Failed == 3U || CG_Cal_Save_Failed == 4U)
    {
        CG_Cal_Save_Failed = 0U;
        CG_Calibration_Status_History = 0xFFU;
    }
    CG_Calibration_Status++;
    switch(CG_Calibration_Status)
    {
        case 1:
            CG_Param_Temp.CG_Zero_Left_Ad  = captured_ad;
            break;
        case 3:
            CG_Param_Temp.CG_Zero_Right_Ad = captured_ad;
            break;
        case 5:
            CG_Param_Temp.CG_Max_Left_Ad   = captured_ad;
            break;
        case 7:
            CG_Param_Temp.CG_Max_Right_Ad  = captured_ad;
            break;
        case 9:
            /* [B3] All five CG fields are written together to a new
             * validated A/B snapshot, never to legacy overlapping bytes. */
            CG_Param_Temp.temp = Temp;
            if (!CG_Calibration_Sane(&CG_Param_Temp))
            {
                CG_Calibration_Status = 8U;
                CG_Cal_Save_Failed = 2U; /* Invalid 180mm AD span/zero gap */
                CG_Calibration_Status_History = 0xFFU;
                Beep_Ctrl(100,3);
                return;
            }
            if (Eep_Save_CG_Calibration(CG_Param_Temp) == 0)
            {
                CG_Calibration_Status = 8; /* Retrying the storage key is safe. */
                CG_Cal_Save_Failed = 1U; /* [CGFIX-03] Do not claim success. */
                CG_Calibration_Status_History = 0xFFU; /* force OLED_Clear() */
                Beep_Ctrl(100,3);
                return;
            }
            /* [CGFIX-01] Eep_Save_CG_Calibration updates CG_Param only
             * after verified commit. Recompute K/B for immediate use. */
            CG_Data_Init();
            CG_Cal_Save_Failed = 0U;
            Beep_Ctrl(100,1);
            break;
    }
    if (CG_Calibration_Status < 8U) Sensor_CG_Stability_Reset();
}




void GJ_Calibration_Storage(void)
{
    GJ_Calibration_Status++;
    switch(GJ_Calibration_Status)
    {
        case 1:
            GJ_Param_Temp.GJ_1410_Ad   = GJ_Ad;
            break;
        case 3:
            GJ_Param_Temp.GJ_1435_Ad   = GJ_Ad;
            break;
        case 5:
            GJ_Param_Temp.GJ_1470_Ad   = GJ_Ad;
            break;
        case 7:
            /* [B3] One validated atomic-ish A/B update for GJ points. */
            GJ_Param_Temp.temp = Temp;
            if (Eep_Save_GJ_Calibration(GJ_Param_Temp) == 0)
            {
                GJ_Calibration_Status = 6;
                Beep_Ctrl(100,3);
                return;
            }
            Beep_Ctrl(100,1);
            break;
    }
}

void CH_Calibration_Storage(void)
{
    uint16_t captured_ad=0U;
    float scale;
    if (CH_Calibration_Status==0U || CH_Calibration_Status==2U ||
        CH_Calibration_Status==4U)
    {
        if (!Sensor_Data_Is_Fresh()) CH_Cal_Save_Failed=3U;
        else if (!GJ_Get_Valid_Scale(&scale) ||
                 !(Temp>=-40.0f && Temp<=100.0f)) CH_Cal_Save_Failed=2U;
        else if (CH_Calibration_Status!=4U &&
                 !Sensor_Get_Stable_GJ(&captured_ad,scale)) CH_Cal_Save_Failed=4U;
        else CH_Cal_Save_Failed=0U;
        if (CH_Cal_Save_Failed!=0U)
        {
            CH_Calibration_Status_History=0xFFU;
            Beep_Ctrl(100,3);
            return;
        }
    }
    switch (CH_Calibration_Status)
    {
        case 0U:
            CH_Param_Temp.CZ_Ad=captured_ad;
            CH_Calibration_Status=1U;
            break;
        case 1U:
            CH_Calibration_Status=2U;
            Sensor_GJ_Stability_Reset();
            break;
        case 2U:
            CH_Param_Temp.HB_Ad=captured_ad;
            CH_Calibration_Status=3U;
            break;
        case 3U:
            CH_Calibration_Status=4U; /* collected, not committed */
            break;
        case 4U:
            CH_Param_Temp.temp=Temp;
            if (Eep_Save_CH_Calibration(CH_Param_Temp)==0U)
            {
                CH_Cal_Save_Failed=1U;
                CH_Calibration_Status_History=0xFFU;
                Beep_Ctrl(100,3);
                return;
            }
            CH_Cal_Save_Failed=0U;
            CH_Calibration_Status=5U; /* only verified commit is success */
            break;
        case 5U:
            CH_Calibration_Status=6U;
            break;
        default:
            return;
    }
    Beep_Ctrl(100,1);
}


















KEY_FUN_T Key_Funs[] = {
/*开机键相关功能路由表*/
{ KEY_ID_ON     ,KEY_STATUS_LONG     ,page_size              ,Power_Ctrl},//长按开关机
{ KEY_ID_ON     ,KEY_STATUS_SHORT    ,page_main              ,GoTo_Menu_Page},
/*取消键相关功能路由表*/
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_menu              ,GoTo_Main_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_ch                ,GoTo_Main_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_goal              ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_password          ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_percision         ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_time              ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_calibration       ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_password_change   ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_calibration_menu  ,GoTo_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_cg_calibration    ,GoTo_Calibration_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_gj_calibration    ,GoTo_Calibration_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_ch_calibration    ,GoTo_Calibration_Menu_Page},
{ KEY_ID_CANCEL ,KEY_STATUS_SHORT    ,page_main              ,Cancel_Goal_Set},

/*左键相关功能路由表*/
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_main              ,GoTo_Ch_Page},
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_ch                ,GoTo_Main_Page},
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_time              ,Reduce_Time_For_Time_Select},
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_goal              ,Reduce_Goal_For_Goal_Select},
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_password          ,Reduce_Password_For_Password_Select},
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_calibration       ,Reduce_Password_For_Password_Select},
{ KEY_ID_LEFT   ,KEY_STATUS_SHORT    ,page_password_change   ,Reduce_Password_For_Password_Select},


/*右键相关功能路由表*/
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_main              ,GoTo_Ch_Page},
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_ch                ,GoTo_Main_Page},
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_time              ,InCrease_Time_For_Time_Select},
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_goal              ,InCrease_Goal_For_Goal_Select},
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_password          ,InCrease_Password_For_Password_Select},
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_calibration       ,InCrease_Password_For_Password_Select},
{ KEY_ID_RIGHT  ,KEY_STATUS_SHORT    ,page_password_change   ,InCrease_Password_For_Password_Select},

/*切换键相关功能路由表*/
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_main              ,Change_Left_Or_Right},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_menu              ,Change_Menu_Menu_Id},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_time              ,Change_Time_Select_Id},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_goal              ,Change_Goal_Set_Select_Id},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_password          ,Change_Password_Set_Select_Id},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_calibration       ,Change_Password_Set_Select_Id},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_calibration_menu  ,InCrease_Calibration_For_Menu_Select},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_percision         ,Change_Percision_Set_Select_Id},
{ KEY_ID_CHANGE ,KEY_STATUS_SHORT    ,page_password_change   ,Change_Password_Set_Select_Id},


/*确认键相关功能路由表*/
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_menu              ,Change_Menu_Page_For_Menu_Id},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_time              ,Change_Time_For_Set_Time},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_goal              ,Change_Goal},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_password          ,Password_Input_Confirm},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_calibration       ,Password_Input_Confirm},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_calibration_menu  ,Calibration_Menu_Select_Confirm},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_percision         ,Percision_Show_Set_Confirm},
{ KEY_ID_SET    ,KEY_STATUS_SHORT    ,page_password_change   ,Password_Set_Confirm},
 
/*存储键相关功能路由表*/
{ KEY_ID_STORAGE,KEY_STATUS_SHORT    ,page_cg_calibration    ,CG_Calibration_Storage},
{ KEY_ID_STORAGE,KEY_STATUS_SHORT    ,page_gj_calibration    ,GJ_Calibration_Storage},
{ KEY_ID_STORAGE,KEY_STATUS_SHORT    ,page_ch_calibration    ,CH_Calibration_Storage},

};


uint8_t Key_Funs_Number = sizeof(Key_Funs)/sizeof(KEY_FUN_T);




























