#include "App_oled.h"
#include <string.h>
#include "pcf8563_head.h"
#include "Algorithm_head.h"
#include "agreement_head.h" /* [CG32-04] sensor-frame freshness */
#include "App_Eep.h"        /* [CG32-04] EEPROM readiness */
#include "adc_head.h"

static PAGE_FUN_T Page_Fun_Buffere[page_size];
/* [CGFIX-02] Force the first reading to render after OLED_Clear(), even 0.00. */
static uint8_t Main_Values_Need_Redraw = 1U;
static uint8_t CG_Display_Status_History = 0xFFU;
static uint8_t GJ_Display_Status_History = 0xFFU;






/************************logo界面***************************/

static uint32_t ReflashTime = 0;

void logo_Page_Init(void)
{
    //OLED_ShowImage(18,6,92,27,(uint8_t *)logo_ico);
    OLED_PrintChinese(32,2,"华夏高铁");
    ReflashTime = HAL_GetTick();
}

void logo_Page_Reflash(void)
{
    uint32_t CurrentTime=HAL_GetTick();
    
    OLED_PrintString(40,4,"CG34-RC2",FONT_8X16);
    if((CurrentTime-ReflashTime)>1500)
    {
        set_PageId(page_main);
    }
}

/*************************主界面***************************/
void Main_Page_Init(void)
{
    Main_Values_Need_Redraw = 1U;
    CG_Display_Status_History = 0xFFU;
    GJ_Display_Status_History = 0xFFU;
    CG_Value_History = 0;
    GJ_Value_History = 0;
    left_And_Righe_Status_History = 1-left_And_Righe_Status;
    OLED_PrintChinese(5,2,"轨距");
    OLED_PrintString(44,2,":",FONT_8X16);
    OLED_PrintChinese(5,5,"超高");
    OLED_PrintString(44,5,":",FONT_8X16);
}


extern I2C_HandleTypeDef hi2c2;

/* Shared main/CH header: at most one RTC transaction per second, and
 * redraw the header on page entry or on visible clock/status changes. */
static uint32_t RTC_LastPoll;
static uint8_t RTC_Polled;
static uint8_t RTC_HeaderRedraw = 1U;
static char RTC_Header[16];

static void RTC_ReflashHeader(void)
{
    uint32_t now = HAL_GetTick();
    if (!RTC_Polled || (uint32_t)(now - RTC_LastPoll) >= 1000U)
    {
        PCF8563_TimeTypeDef time;
        char next[sizeof(RTC_Header)] = "RTC ERR       ";
        RTC_LastPoll = now;
        RTC_Polled = 1U;
        if (PCF8563_GetTime(&hi2c2, &time) == HAL_OK &&
            time.year <= 99U && time.month >= 1U && time.month <= 12U &&
            time.day >= 1U && time.day <= 31U && time.hour < 24U &&
            time.minute < 60U)
            snprintf(next, sizeof(next), "%02u/%02u/%02u %02u:%02u",
                     (unsigned)time.year, (unsigned)time.month, (unsigned)time.day,
                     (unsigned)time.hour, (unsigned)time.minute);
        if (strcmp(RTC_Header, next) != 0)
        {
            strcpy(RTC_Header, next);
            RTC_HeaderRedraw = 1U;
        }
    }
    if (RTC_HeaderRedraw)
    {
        OLED_PrintString(5, 0, RTC_Header, FONT_6X8);
        RTC_HeaderRedraw = 0U;
    }
}

static int8_t  Power_0_Ico_Show_Timer_Status = 0;
static uint16_t Power_Ico_Show_Timer_Freq = 5000;
static uint32_t Power_Ico_Show_Timer_History = 0;
static uint32_t Power_Ico_Show_Timer_Now = 0;
void Main_Page_Reflash(void)
{
    char buffer[20];
    uint8_t fresh = Sensor_Data_Is_Fresh();
    uint8_t eep_valid = (uint8_t)(Eep_Get_Storage_State() == EEP_STATE_B3_VALID);
    uint8_t gj_state;
    GJ_Value = Get_GJ_Value(GJ_Ad, Temp);
    gj_state = !eep_valid ? CG_RESULT_EEP_ERROR :
               ((!fresh || GJ_Ad > 0x7FFFU) ? CG_RESULT_NO_SENSOR :
                (GJ_Result_Is_Valid() ? CG_RESULT_OK : CG_RESULT_CAL_ERROR));
    if (Main_Values_Need_Redraw || GJ_Display_Status_History != gj_state ||
        (gj_state == CG_RESULT_OK && GJ_Value_History != GJ_Value))
    {
        OLED_PrintString(52,2,"         ",FONT_8X16);
        if (gj_state == CG_RESULT_OK)
            snprintf(buffer,sizeof(buffer),Data_Show_Percision == 1 ? "%+.2f" : "%+.1f",GJ_Value);
        else
            snprintf(buffer,sizeof(buffer),"%s",gj_state == CG_RESULT_EEP_ERROR ? "EEP ERR" :
                     (gj_state == CG_RESULT_NO_SENSOR ? "NO SENSOR" : "CAL ERROR"));
        OLED_PrintString(52,2,buffer,FONT_8X16);
        GJ_Display_Status_History = gj_state;
        GJ_Value_History = GJ_Value;
    }

    /* [CG32-04] Error status takes precedence over a numeric zero.
     * Leave the raw CG_Ad and CG_Value_t available in Keil Watch. */
    CG_Value = Get_CG_Value(CG_Ad,Temp);
    {
        uint8_t state = CG_Result_Status;
        if (!fresh) state = CG_RESULT_NO_SENSOR;
        if (!eep_valid)
            state = CG_RESULT_EEP_ERROR;
        if (Main_Values_Need_Redraw ||
            CG_Display_Status_History != state ||
            (state == CG_RESULT_OK && CG_Value_History != CG_Value))
        {
            /* Clear the whole old number, including previous sign/digits.
             * Nine 8px characters fit in x=52..123 of a 128px OLED. */
            OLED_PrintString(52,5,"         ",FONT_8X16);
            if (state == CG_RESULT_OK)
            {
                if(Data_Show_Percision == 1)
                    snprintf(buffer,sizeof(buffer),"%+07.2f",CG_Value);
                else
                    snprintf(buffer,sizeof(buffer),"%+06.1f",CG_Value);
            }
            else if (state == CG_RESULT_CAL_ERROR)
                snprintf(buffer,sizeof(buffer),"CAL ERROR");
            else if (state == CG_RESULT_TEMP_ERROR)
                snprintf(buffer,sizeof(buffer),"TEMP ERR");
            else if (state == CG_RESULT_EEP_ERROR)
                snprintf(buffer,sizeof(buffer),"EEP ERR");
            else
                snprintf(buffer,sizeof(buffer),"NO SENSOR");
            OLED_PrintString(52,5,buffer,FONT_8X16);
            CG_Display_Status_History = state;
            CG_Value_History = CG_Value;
        }
        /* Only use a VALID physical measurement for goal alarm. */
        if (state != CG_RESULT_OK) CG_Value_History = 0.0f;
    }

    Main_Values_Need_Redraw = 0U; /* [CGFIX-02] */
    /*显示时间*/
    //获取时间数据
    RTC_ReflashHeader();
    
    
    Power_Ico_Show_Timer_Now =HAL_GetTick();
    if((Power_Ico_Show_Timer_Now - Power_Ico_Show_Timer_History)>Power_Ico_Show_Timer_Freq)
    {
        uint8_t Driver_Power_Level = Get_Driver_Power(ADC_ReadChannel9());
        /*显示电量信息*/
        if(Driver_Power_Level == 0)
        {
            Power_Ico_Show_Timer_Freq = 500;
            Power_0_Ico_Show_Timer_Status = 1-Power_0_Ico_Show_Timer_Status;
            if(Power_0_Ico_Show_Timer_Status == 0)
                OLED_ShowImage(105,0,16,8,(uint8_t*)power_0_ico);
            else
                OLED_ShowImage(105,0,16,8,(uint8_t*)power_n_ico);
        }
        else
        {
            Power_Ico_Show_Timer_Freq = 5000;
            if(Driver_Power_Level == 4)
                OLED_ShowImage(105,0,16,8,(uint8_t*)power_4_ico);
            else if(Driver_Power_Level == 3)
                OLED_ShowImage(105,0,16,8,(uint8_t*)power_3_ico);
            else if(Driver_Power_Level == 2)
                OLED_ShowImage(105,0,16,8,(uint8_t*)power_2_ico);
            else
                OLED_ShowImage(105,0,16,8,(uint8_t*)power_1_ico);
        }
        Power_Ico_Show_Timer_History = Power_Ico_Show_Timer_Now;
    }
    
    
    
    
    /*显示目标提示*/
    if(Goal_Set_Status == 1)
    {
        OLED_PrintString(5,1,"MUBIAO",FONT_6X8);
        if (CG_Result_Status == CG_RESULT_OK && Sensor_Data_Is_Fresh() &&
            Eep_Get_Storage_State() == EEP_STATE_B3_VALID &&
            CG_Value>Goal_CG_Min_Value && CG_Value<Goal_CG_Max_Value)
        {
            Beep_Ctrl(100,3);
        }
        if(gj_state == CG_RESULT_OK && GJ_Value>Goal_CJ_Min_Value&&GJ_Value<Goal_CJ_Max_Value)
        {
            Beep_Ctrl(100,3);
        }
    }
    else
        OLED_PrintString(5,1,"      ",FONT_6X8);
    /*显示左右股状态*/
    if(left_And_Righe_Status_History!= left_And_Righe_Status)
    {
        if(left_And_Righe_Status == 1)
        {
            OLED_PrintString(96,1,"     ",FONT_6X8);
            OLED_PrintString(96,1,"LEFT",FONT_6X8);
        }
        else
        {
            OLED_PrintString(96,1,"     ",FONT_6X8);
            OLED_PrintString(96,1,"RIGHT",FONT_6X8);
        }
        left_And_Righe_Status_History = left_And_Righe_Status;
    }
    /*显示电量数据*/
}

/*************************查照护背界面***************************/

void CH_Page_Init(void)
{
    OLED_PrintChinese(5,2,"查照");
    OLED_PrintString(44,2,":",FONT_8X16);
    OLED_PrintChinese(5,5,"护背");
    OLED_PrintString(44,5,":",FONT_8X16);
    /* This checkout has no live CZ/HB producers. Never show demo numbers. */
    OLED_PrintString(52,2,"NOT READY",FONT_8X16);
    OLED_PrintString(52,5,"NOT READY",FONT_8X16);
}


void CH_Page_Reflash(void)
{
    RTC_ReflashHeader();
    /*显示电量数据*/
    OLED_ShowImage(105,0,16,8,(uint8_t*)power_4_ico);
}

/************************菜单选择界面***************************/

/* Four two-byte GBK glyphs plus a terminating NUL per menu label. */
static char Menu_buffer[MENU_ID_NUMBER][9] = {"时间设置","目标设置","标定设置","密码设置","精度设置"};
static void Menu_Draw_Labels(uint8_t first)
{
    uint8_t row;
    for (row = 0U; row < 3U; ++row)
        OLED_PrintChinese(32U, (uint8_t)(1U + 2U * row), Menu_buffer[first + row]);
}

void Menu_Page_Init(void)
{
    menu_Id_History = 0U;
    menu_Id_Select = 0U;
    OLED_PrintString(16,1,"->",FONT_8X16);
    Menu_Draw_Labels(0U);
}

void Menu_Page_Reflash(void)
{
    uint8_t first;
    if (menu_Id_Select >= MENU_ID_NUMBER) menu_Id_Select = MENU_ID_NUMBER - 1U;
    if (menu_Id_History == menu_Id_Select) return;
    first = (menu_Id_Select > 2U) ? (uint8_t)(menu_Id_Select - 2U) : 0U;
    OLED_PrintString(16,1,"  ",FONT_8X16);
    OLED_PrintString(16,3,"  ",FONT_8X16);
    OLED_PrintString(16,5,"  ",FONT_8X16);
    OLED_PrintString(16, (uint8_t)(1U + 2U * (menu_Id_Select - first)), "->", FONT_8X16);
    Menu_Draw_Labels(first);
    menu_Id_History = menu_Id_Select;
}

/************************时间设置界面***************************/

void Time_Set_Page_Init(void)
{
    PCF8563_TimeTypeDef timeT = {0};
    if (PCF8563_GetTime(&hi2c2,&timeT) != HAL_OK)
    {
        /* Initialize an editable date rather than showing stack garbage. */
        timeT.month = 1U;
        timeT.day = 1U;
    }
    OLED_PrintString(9,2,"Y",FONT_8X16);
    OLED_PrintString(49,2,"M",FONT_8X16);
    OLED_PrintString(89,2,"D",FONT_8X16);
    OLED_PrintString(9,5,"h",FONT_8X16);
    OLED_PrintString(49,5,"m",FONT_8X16);
    OLED_PrintString(89,5,"s",FONT_8X16);
    
    Set_Time[0]     = timeT.year;
    Set_Time[1]     = timeT.month;
    Set_Time[2]     = timeT.day;
    Set_Time[3]     = timeT.hour;
    Set_Time[4]     = timeT.minute;
    Set_Time[5]     = timeT.second;
    Set_Time_Select = 0;
    Set_Time_Select_History = 1;
    
}

void Time_Set_Page_Reflash(void)
{
    char buffer[8]; /* [B1] Time fields have room for unexpected values. */
    snprintf(buffer,sizeof(buffer),"%02d",Set_Time[0]);
    OLED_PrintString(20,2,buffer,FONT_8X16);
    snprintf(buffer,sizeof(buffer),"%02d",Set_Time[1]);
    OLED_PrintString(60,2,buffer,FONT_8X16);
    snprintf(buffer,sizeof(buffer),"%02d",Set_Time[2]);
    OLED_PrintString(100,2,buffer,FONT_8X16);
    snprintf(buffer,sizeof(buffer),"%02d",Set_Time[3]);
    OLED_PrintString(20,5,buffer,FONT_8X16);
    snprintf(buffer,sizeof(buffer),"%02d",Set_Time[4]);
    OLED_PrintString(60,5,buffer,FONT_8X16);
    snprintf(buffer,sizeof(buffer),"%02d",Set_Time[5]);
    OLED_PrintString(100,5,buffer,FONT_8X16);
    if(Set_Time_Select_History!=Set_Time_Select)
    {
        switch(Set_Time_Select)
        {
            case 0:
                OLED_PrintLine(100,116,7,0x00);
                OLED_PrintLine(20,36,4,0x01);
                break;
            case 1:
                OLED_PrintLine(20,36,4,0x00);
                OLED_PrintLine(60,76,4,0x01);
                break;
            case 2:
                OLED_PrintLine(60,76,4,0x00);
                OLED_PrintLine(100,116,4,0x01);
                break;
            case 3:
                OLED_PrintLine(100,116,4,0x00);
                OLED_PrintLine(20,36,7,0x01);
                break;
            case 4:
                OLED_PrintLine(20,36,7,0x00);
                OLED_PrintLine(60,76,7,0x01);
                break;
            case 5:
                OLED_PrintLine(60,76,7,0x00);
                OLED_PrintLine(100,116,7,0x01);
                break;
        }
        Set_Time_Select_History = Set_Time_Select;
    }
}

/*************************目标设置界面***************************/

void Goal_Set_Page_Init(void)
{
    OLED_PrintChinese(7,1,"轨距");
    OLED_PrintChinese(71,1,"±");
    OLED_PrintString(39,1,":",FONT_8X16);
    OLED_PrintString(87,1,"0.",FONT_8X16);
    OLED_PrintChinese(7,4,"超高");
    OLED_PrintChinese(80,4,"±");
    OLED_PrintString(39,4,":",FONT_8X16);
    OLED_PrintString(95,4,"0.",FONT_8X16);
    Goal_Set_GJ_1 = 3;
    Goal_Set_GJ_2 = 5;
    Goal_Set_GJ_3 = 1;
    Goal_Set_CG_1 = 0;
    Goal_Set_CG_2 = 0;
    Goal_Set_CG_3 = 0;
    Goal_Set_CG_4 = 0;
    Goal_Set_CG_5 = 1;
    Goal_Set_Select_History = 1;
    Goal_Set_Select = 0;
}
uint8_t Goal_Set_Select_Show_Ico_Addr[8][2]=
{
    {55, 3},
    {63, 3},
    {103,3},
    {47 ,6},
    {55 ,6},
    {63 ,6},
    {71 ,6},
    {111,6},
};
void Goal_Set_Page_Reflash(void)
{
    OLED_PrintChar(55,1,(Goal_Set_GJ_1+'0'),FONT_8X16);
    OLED_PrintChar(63,1,(Goal_Set_GJ_2+'0'),FONT_8X16);
    OLED_PrintChar(103,1,(Goal_Set_GJ_3+'0'),FONT_8X16);
    
    OLED_PrintChar(47 , 4,(Goal_Set_CG_1+'+'),FONT_8X16);
    OLED_PrintChar(55 , 4,(Goal_Set_CG_2+'0'),FONT_8X16);
    OLED_PrintChar(63 , 4,(Goal_Set_CG_3+'0'),FONT_8X16);
    OLED_PrintChar(71 , 4,(Goal_Set_CG_4+'0'),FONT_8X16);
    OLED_PrintChar(111, 4,(Goal_Set_CG_5+'0'),FONT_8X16);
    if(Goal_Set_Select_History != Goal_Set_Select)
    {
        OLED_PrintChar(Goal_Set_Select_Show_Ico_Addr[Goal_Set_Select_History][0] , Goal_Set_Select_Show_Ico_Addr[Goal_Set_Select_History][1],' ',FONT_6X8);
        OLED_PrintChar(Goal_Set_Select_Show_Ico_Addr[Goal_Set_Select][0] , Goal_Set_Select_Show_Ico_Addr[Goal_Set_Select][1],'-',FONT_6X8);
        Goal_Set_Select_History = Goal_Set_Select; 
    }
}

/*************************密码设置界面***************************/

void Password_Set_Page_Init(void)
{
    OLED_PrintChinese(24,1,"请输入密码");

    for(uint8_t i=0;i<6;i++)
        Password_Input_Temp[i] = 0;
    Password_Input_Select = 0;
    Password_Input_Select_History = 1;
}

void Password_Set_Page_Reflash(void)
{
    for(uint8_t i=0;i<6;i++)
        OLED_PrintChar(40+8*i,4,(Password_Input_Temp[i]+'0'),FONT_8X16);
    if(Password_Input_Select_History != Password_Input_Select)
    {
        OLED_PrintChar(40+8*Password_Input_Select_History,6,' ',FONT_6X8);
        OLED_PrintChar(40+8*Password_Input_Select,6,'-',FONT_6X8);
        Password_Input_Select_History = Password_Input_Select;
    }
}


/*************************密码修改界面***************************/

void Password_Change_Page_Init(void)
{
    OLED_PrintChinese(24,1,"请输入密码");

    for(uint8_t i=0;i<6;i++)
        Password_Input_Temp[i] = 0;
    Password_Input_Select = 0;
    Password_Input_Select_History = 1;
}

void Password_Change_Page_Reflash(void)
{
    for(uint8_t i=0;i<6;i++)
        OLED_PrintChar(40+8*i,4,(Password_Input_Temp[i]+'0'),FONT_8X16);
    if(Password_Input_Select_History != Password_Input_Select)
    {
        OLED_PrintChar(40+8*Password_Input_Select_History,6,' ',FONT_6X8);
        OLED_PrintChar(40+8*Password_Input_Select,6,'-',FONT_6X8);
        Password_Input_Select_History = Password_Input_Select;
    }
}

/*************************标定数据菜单界面***************************/

void Calibration_Menu_Page_Init(void)
{
    OLED_PrintChinese(32,1,"超高标定");
    OLED_PrintChinese(32,3,"轨距标定");
    OLED_PrintChinese(32,5,"查照标定");
    Calibration_Menu_Select = 1;
    Calibration_Menu_Select_History = 3;
}

void Calibration_Menu_Page_Reflash(void)
{
    if(Calibration_Menu_Select_History!=Calibration_Menu_Select)
    {
        OLED_PrintString(16,Calibration_Menu_Select_History,"  ",FONT_8X16);
        OLED_PrintString(16,Calibration_Menu_Select,"->",FONT_8X16);
        Calibration_Menu_Select_History = Calibration_Menu_Select;
    }
}

/*************************精度设置菜单界面***************************/
void Percision_Menu_Page_Init(void)
{
    OLED_PrintChinese(32,1,"精度一");
    OLED_PrintChinese(32,3,"精度二");
    Percision_Menu_Select = 1;
    Percision_Menu_Select_History = 3;
    
}

void Percision_Menu_Page_Reflash(void)
{
    if(Percision_Menu_Select_History!=Percision_Menu_Select)
    {
        OLED_PrintString(16,Percision_Menu_Select_History,"  ",FONT_8X16);
        OLED_PrintString(16,Percision_Menu_Select,"->",FONT_8X16);
        Percision_Menu_Select_History = Percision_Menu_Select;
    }
}

/*************************超高标定设置界面***************************/

//static uint32_t CG_Calibration_Tim_Current = 0;
//static uint32_t CG_Calibration_Tim_History = 0;

static char *Calibration_Error_Label(uint8_t error)
{
    if (error == CAL_SAVE_NO_SENSOR) return "NO SENSOR";
    if (error == CAL_SAVE_TEMP_ERROR) return "TEMP ERR";
    if (error == CAL_SAVE_SENSOR_ERROR) return "SENSOR ERR";
    if (error == CAL_SAVE_BAD_POINTS) return "CAL ERROR";
    return "SAVE FAIL";
}

void CG_Calibration_Page_Init(void)
{
    CG_Calibration_Status = 0;
    CG_Calibration_Status_History = 0xFFU;
    CG_Cal_Save_Failed = CAL_SAVE_OK;
//    CG_Calibration_Tim_Current = 0;
//    CG_Calibration_Tim_History = 0;
}


char CG_buffer[8] = {0}; /* [B1] Signed 16-bit: -32768 + NUL needs 7 bytes. */
void CG_Calibration_Page_Reflash(void)
{
    if(CG_Calibration_Status_History!=CG_Calibration_Status)
    {
        OLED_Clear();
//        CG_Calibration_Tim_History = HAL_GetTick();
        CG_Calibration_Status_History = CG_Calibration_Status;
    }
    switch(CG_Calibration_Status)
    {
        case 0:
            OLED_PrintChinese(40,1,"水平处");
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 1;
//            }
            break;
        case 1:
            OLED_PrintChinese(40,1,"水平处");
            snprintf(CG_buffer,sizeof(CG_buffer),"%05d",CG_Param_Temp.CG_Zero_Left_Ad);
            OLED_PrintString(44,4,CG_buffer,FONT_8X16);
            break;
        case 2:
            OLED_PrintChinese(24,1,"水平处掉头");
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 3;
//            }
            break;
        case 3:
            OLED_PrintChinese(24,1,"水平处掉头");
            snprintf(CG_buffer,sizeof(CG_buffer),"%05d",CG_Param_Temp.CG_Zero_Right_Ad);
            OLED_PrintString(44,4,CG_buffer,FONT_8X16);
            break;
        case 4:
            OLED_PrintString(44,1,"180mm",FONT_8X16);
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 5;
//            }
            break;
        case 5:
            OLED_PrintString(44,1,"180mm",FONT_8X16);
            snprintf(CG_buffer,sizeof(CG_buffer),"%05d",CG_Param_Temp.CG_Max_Left_Ad);
            OLED_PrintString(44,4,CG_buffer,FONT_8X16);
            break;
        case 6:
            OLED_PrintString(28,1,"180mm",FONT_8X16);
            OLED_PrintChinese(68,1,"掉头");
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 7;
//            }
            break;
        case 7:
            OLED_PrintString(28,1,"180mm",FONT_8X16);
            OLED_PrintChinese(68,1,"掉头");
            snprintf(CG_buffer,sizeof(CG_buffer),"%05d",CG_Param_Temp.CG_Max_Right_Ad);
            OLED_PrintString(44,4,CG_buffer,FONT_8X16);
            break;
        case 8:
            /* [CGFIX-03] Collected four AD points, NOT saved yet. */
            OLED_PrintChinese(32,1,"超高标定");
            if (CG_Cal_Save_Failed == CAL_SAVE_FAILED)
                OLED_PrintString(28,4,"EEP ERROR",FONT_8X16);
            else if (CG_Cal_Save_Failed != CAL_SAVE_OK)
                OLED_PrintString(28,4,Calibration_Error_Label(CG_Cal_Save_Failed),FONT_8X16);
            else
                OLED_PrintChinese(24,4,"存储键保存");
            break;
        case 9:
            /* Only reached after EEPROM CRC/readback commit succeeds. */
            OLED_PrintChinese(32,1,"超高标定");
            OLED_PrintChinese(48,4,"成功");
            break;
        case 10:
            set_PageId(page_calibration_menu);
            break;
    }
    /* [CG32-05] Error also visible on the four capture prompt pages. */
    if (CG_Cal_Save_Failed != CAL_SAVE_OK && CG_Calibration_Status < 8U)
        OLED_PrintString(24,6,Calibration_Error_Label(CG_Cal_Save_Failed),FONT_8X16);
}

/*************************轨距标定设置界面***************************/
void GJ_Calibration_Page_Init(void)
{
    GJ_Calibration_Status = 0;
    GJ_Calibration_Status_History = 0xFFU;
    GJ_Cal_Save_Failed = CAL_SAVE_OK;
}

char GJ_buffer[8] = {0}; /* [B1] Signed 16-bit: -32768 + NUL needs 7 bytes. */
void CJ_Calibration_Page_Reflash(void)
{
    if(GJ_Calibration_Status_History!=GJ_Calibration_Status)
    {
        OLED_Clear();
        GJ_Calibration_Status_History = GJ_Calibration_Status;
    }
    switch(GJ_Calibration_Status)
    {
        case 0:
            OLED_PrintChinese(24,1,"当前标定值");
            OLED_PrintString(36,3,"1410mm",FONT_8X16);
            break;
        case 1:
            OLED_PrintString(36,1,"1410mm",FONT_8X16);
            snprintf(GJ_buffer,sizeof(GJ_buffer),"%05u",(unsigned)GJ_Param_Temp.GJ_1410_Ad);
            OLED_PrintString(44,4,GJ_buffer,FONT_8X16);
            break;
        case 2:
            OLED_PrintChinese(24,1,"当前标定值");
            OLED_PrintString(36,3,"1435mm",FONT_8X16);
            break;
        case 3:
            OLED_PrintString(36,1,"1435mm",FONT_8X16);
            snprintf(GJ_buffer,sizeof(GJ_buffer),"%05u",(unsigned)GJ_Param_Temp.GJ_1435_Ad);
            OLED_PrintString(44,4,GJ_buffer,FONT_8X16);
            break;
        case 4:
            OLED_PrintChinese(24,1,"当前标定值");
            OLED_PrintString(36,3,"1470mm",FONT_8X16);
            break;
        case 5:
            OLED_PrintString(36,1,"1470mm",FONT_8X16);
            snprintf(GJ_buffer,sizeof(GJ_buffer),"%05u",(unsigned)GJ_Param_Temp.GJ_1470_Ad);
            OLED_PrintString(44,4,GJ_buffer,FONT_8X16);
            break;
        case 6:
            OLED_PrintChinese(32,1,"轨距标定");
            if (GJ_Cal_Save_Failed != CAL_SAVE_OK)
                OLED_PrintString(24,4,Calibration_Error_Label(GJ_Cal_Save_Failed),FONT_8X16);
            else
                OLED_PrintChinese(24,4,"存储键保存");
            break;
        case 7:
            OLED_PrintChinese(32,1,"轨距标定");
            OLED_PrintChinese(48,4,"成功"); /* EEPROM commit has succeeded. */
            break;
        case 8:
            set_PageId(page_calibration_menu);
            break;
    }
    if (GJ_Cal_Save_Failed != CAL_SAVE_OK && GJ_Calibration_Status < 6U)
        OLED_PrintString(24,6,Calibration_Error_Label(GJ_Cal_Save_Failed),FONT_8X16);
}

/*************************查护标定设置界面***************************/
void CH_Calibration_Page_Init(void)
{
    CH_Calibration_Status = 0;
    CH_Calibration_Status_History = 0xFFU;
    CH_Cal_Save_Failed = CAL_SAVE_OK;
}

char CH_buffer[8] = {0}; /* [B1] Signed 16-bit: -32768 + NUL needs 7 bytes. */
void CH_Calibration_Page_Reflash(void)
{
    if(CH_Calibration_Status_History!=CH_Calibration_Status)
    {
        OLED_Clear();
        CH_Calibration_Status_History = CH_Calibration_Status;
    }
    switch(CH_Calibration_Status)
    {
        case 0:
            OLED_PrintChinese(24,1,"查照标定值");
            OLED_PrintString(36,3,"1391mm",FONT_8X16);
            break;
        case 1:
            OLED_PrintChinese(32,1,"查");
            OLED_PrintString(48,1,"1391mm",FONT_8X16);
            snprintf(CH_buffer,sizeof(CH_buffer),"%05u",(unsigned)CH_Param_Temp.CZ_Ad);
            OLED_PrintString(44,4,CH_buffer,FONT_8X16);
            break;
        case 2:
            OLED_PrintChinese(24,1,"护背标定值");
            OLED_PrintString(36,3,"1348mm",FONT_8X16);
            break;
        case 3:
            OLED_PrintChinese(32,1,"护");
            OLED_PrintString(48,1,"1348mm",FONT_8X16);
            snprintf(CH_buffer,sizeof(CH_buffer),"%05u",(unsigned)CH_Param_Temp.HB_Ad);
            OLED_PrintString(44,4,CH_buffer,FONT_8X16);
            break;
        case 4:
            OLED_PrintChinese(32,1,"查护标定");
            if (CH_Cal_Save_Failed != CAL_SAVE_OK)
                OLED_PrintString(24,4,Calibration_Error_Label(CH_Cal_Save_Failed),FONT_8X16);
            else
                OLED_PrintChinese(24,4,"存储键保存");
            break;
        case 5:
            OLED_PrintChinese(32,1,"查护标定");
            OLED_PrintChinese(48,4,"成功"); /* EEPROM commit has succeeded. */
            break;
        case 6:
            set_PageId(page_calibration_menu);
            break;
    }
    if (CH_Cal_Save_Failed != CAL_SAVE_OK && CH_Calibration_Status < 4U)
        OLED_PrintString(24,6,Calibration_Error_Label(CH_Cal_Save_Failed),FONT_8X16);
}



















/************************系统相关函数**********************/



static PAGE_ID Page_Id_History = page_size;
void oled_Reflash(void)
{
    PAGE_ID            page_id = get_PageId();
    static PAGE_FUN_T  page_fun;
    if (OLED_HasFault())
    {
        if (!OLED_TryRecover()) return;
        Page_Id_History = page_size; /* redraw all fields after re-init */
    }
    
    if(page_id != Page_Id_History)
    {
        //更新页面
        page_fun = Page_Fun_Buffere[page_id];
        Page_Id_History = page_id;
        RTC_HeaderRedraw = 1U;
        //清除屏幕
        OLED_Clear();
        //刷新初始界面
        if(page_fun.init != NULL)
            page_fun.init();
    }
    
    if(page_fun.refresh != NULL)
        page_fun.refresh();
}


void Page_init(void)
{
    Page_Fun_Buffere[page_logo].init                 = logo_Page_Init;
    Page_Fun_Buffere[page_logo].refresh              = logo_Page_Reflash;
    Page_Fun_Buffere[page_main].init                 = Main_Page_Init;
    Page_Fun_Buffere[page_main].refresh              = Main_Page_Reflash;
    Page_Fun_Buffere[page_ch  ].init                 = CH_Page_Init;
    Page_Fun_Buffere[page_ch]  .refresh              = CH_Page_Reflash;
    Page_Fun_Buffere[page_menu].init                 = Menu_Page_Init;
    Page_Fun_Buffere[page_menu].refresh              = Menu_Page_Reflash;
    Page_Fun_Buffere[page_time].init                 = Time_Set_Page_Init;
    Page_Fun_Buffere[page_time].refresh              = Time_Set_Page_Reflash;
    Page_Fun_Buffere[page_goal].init                 = Goal_Set_Page_Init;
    Page_Fun_Buffere[page_goal].refresh              = Goal_Set_Page_Reflash;
    Page_Fun_Buffere[page_password].init             = Password_Set_Page_Init;
    Page_Fun_Buffere[page_password].refresh          = Password_Set_Page_Reflash;
    Page_Fun_Buffere[page_calibration].init          = Password_Set_Page_Init;
    Page_Fun_Buffere[page_calibration].refresh       = Password_Set_Page_Reflash;
    Page_Fun_Buffere[page_password_change].init      = Password_Change_Page_Init;
    Page_Fun_Buffere[page_password_change].refresh   = Password_Change_Page_Reflash;
    Page_Fun_Buffere[page_calibration_menu].init     = Calibration_Menu_Page_Init;
    Page_Fun_Buffere[page_calibration_menu].refresh  = Calibration_Menu_Page_Reflash;
    Page_Fun_Buffere[page_percision].init            = Percision_Menu_Page_Init;
    Page_Fun_Buffere[page_percision].refresh         = Percision_Menu_Page_Reflash;
    Page_Fun_Buffere[page_cg_calibration].init       = CG_Calibration_Page_Init;
    Page_Fun_Buffere[page_cg_calibration].refresh    = CG_Calibration_Page_Reflash;
    Page_Fun_Buffere[page_gj_calibration].init       = GJ_Calibration_Page_Init;
    Page_Fun_Buffere[page_gj_calibration].refresh    = CJ_Calibration_Page_Reflash;
    Page_Fun_Buffere[page_ch_calibration].init       = CH_Calibration_Page_Init;
    Page_Fun_Buffere[page_ch_calibration].refresh    = CH_Calibration_Page_Reflash;
    
    

    


    set_PageId(page_logo);
}












