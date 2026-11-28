#include "App_oled.h"
#include "pcf8563_head.h"
#include "Algorithm_head.h"
#include "agreement_head.h" /* [CG32-04] sensor-frame freshness */
#include "App_Eep.h"        /* [CG32-04] EEPROM readiness */
#include "adc_head.h"
#include "usart_head.h"
#include "key_head.h"
#include <string.h>

static PAGE_FUN_T Page_Fun_Buffere[page_size];
/* [CGFIX-02] Force the first reading to render after OLED_Clear(), even 0.00. */
static uint8_t Main_Values_Need_Redraw = 1U;
static uint8_t CG_Display_Status_History = 0xFFU;
static PAGE_ID Page_Id_History = page_size;
static uint8_t Goal_Marker_History = 0xFFU;






/************************logo界面***************************/

static uint32_t ReflashTime = 0;

void logo_Page_Init(void)
{
    //OLED_ShowImage(18,6,92,27,(uint8_t *)logo_ico);
    OLED_PrintChinese(32,2,"华夏高铁");
    OLED_PrintString(28,4,"CG41 TEST",FONT_8X16);
    ReflashTime = HAL_GetTick();
}

void logo_Page_Reflash(void)
{
    uint32_t CurrentTime=HAL_GetTick();
    
    if((CurrentTime-ReflashTime)>1500)
    {
        set_PageId(page_main);
    }
}

extern I2C_HandleTypeDef hi2c2;
/* Battery and clock caches are shared by both measurement pages. */
static uint8_t Battery_Level = 0xFFU;
static uint8_t Battery_Force_Redraw = 1U;
static uint8_t Battery_Last_Icon = 0xFFU;
static uint8_t Battery_Low_Detected, Battery_Warned, Battery_Sampling;
static uint32_t Battery_Sample_Tick, Battery_Warning_Tick;

static uint8_t Clock_Force_Redraw = 1U;
static uint32_t Clock_Read_Tick;
static char Clock_Text[20] = "--/--/-- --:--";
static char Clock_Displayed[20];
static const uint8_t Charge_Text[128] = {
    0x00, 0x00, 0x04, 0x04, 0xE4, 0xFC, 0x84, 0x04, 0x04, 0x74, 0x7C, 0xC4, 0x40, 0x00, 0x00, 0x00,
    0x00, 0xFC, 0x4C, 0x44, 0x4C, 0xFC, 0x10, 0x90, 0x90, 0x10, 0x10, 0xFE, 0xFE, 0x10, 0x10, 0x00,
    0x00, 0x08, 0x08, 0xC8, 0xE8, 0xB8, 0x98, 0x8E, 0x88, 0xE8, 0x68, 0xC8, 0x88, 0x08, 0x00, 0x00,
    0x00, 0xF0, 0xF8, 0x48, 0x48, 0x48, 0xFE, 0xFE, 0x48, 0x48, 0x48, 0xF8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x30, 0x18, 0x2E, 0x23, 0x20, 0x31, 0x13, 0x0C, 0x0C, 0x1F, 0x11, 0x30, 0x20, 0x00, 0x00,
    0x00, 0x1F, 0x04, 0x04, 0x04, 0x0F, 0x00, 0x00, 0x03, 0x00, 0x20, 0x3F, 0x1F, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x20, 0x30, 0x18, 0x0F, 0x00, 0x00, 0x1F, 0x3F, 0x20, 0x20, 0x30, 0x38, 0x00, 0x00,
    0x00, 0x07, 0x0F, 0x04, 0x04, 0x04, 0x3F, 0x3F, 0x24, 0x24, 0x24, 0x27, 0x20, 0x30, 0x00, 0x00,
};

void Measurement_Power_Session(void)
{
    uint32_t now = HAL_GetTick();
    Battery_Level = 0xFFU;
    Battery_Low_Detected = 0U;
    Battery_Warned = 0U;
    Battery_Sampling = 0U;
    Battery_Sample_Tick = now - BATTERY_SAMPLE_PERIOD_MS + 25U;
    Clock_Read_Tick = now - 1000U;
    Clock_Force_Redraw = 1U;
    Page_Id_History = page_size;
    set_PageId(page_logo);
}

void Measurement_Battery_Service(void)
{
    uint32_t now = HAL_GetTick();
    float volts;
    uint8_t result;
    if (!Battery_Sampling && (uint32_t)(now - Battery_Sample_Tick) < BATTERY_SAMPLE_PERIOD_MS) return;
    Battery_Sampling = 1U;
    result = ADC_Battery_Step(&volts);
    if (result == BATTERY_ADC_WAIT) return;
    Battery_Sampling = 0U;
    Battery_Sample_Tick = now;
    if (result == BATTERY_ADC_VALID)
    {
        Battery_Level = Battery_Level_For_Voltage(volts, Battery_Level);
        if (volts <= BATTERY_LOW_ENTER_V) Battery_Low_Detected = 1U;
    }
}

static void Measurement_Battery_Refresh(void)
{
    static const char * const icons[] = {
        power_0_ico, power_1_ico, power_2_ico, power_3_ico, power_4_ico
    };
    uint32_t now = HAL_GetTick();
    uint32_t age;
    uint8_t icon = Battery_Level;
    if (!Battery_Warned && Battery_Low_Detected)
    {
        Battery_Warned = 1U;
        Battery_Warning_Tick = now;
        Beep_Low_Battery();
    }

    age = (uint32_t)(now - Battery_Warning_Tick);
    if (Battery_Warned && age < 4U * BATTERY_FLASH_PERIOD_MS &&
        (age / BATTERY_FLASH_PERIOD_MS) % 2U == 0U)
        icon = 5U;
    /* Retire the visual timer once the two flashes are done, even across tick wrap. */
    if (Battery_Warned == 1U && age >= 4U * BATTERY_FLASH_PERIOD_MS) Battery_Warned = 2U;
    if (Battery_Warned == 2U) icon = Battery_Level;
    if (!Battery_Force_Redraw && icon == Battery_Last_Icon) return;

    if (icon == 5U)
    {
        OLED_ShowImage(64,0,64,16,Charge_Text);
    }
    else
    {
        if (Battery_Last_Icon == 5U)
        {
            OLED_PrintLine(64,128,0,0U);
            OLED_PrintLine(64,128,1,0U);
            Clock_Force_Redraw = 1U;
            left_And_Righe_Status_History = 0;
        }
        OLED_ShowImage(105,0,16,8,(const uint8_t *)(icon <= 4U ? icons[icon] : power_n_ico));
    }
    Battery_Last_Icon = icon;
    Battery_Force_Redraw = 0U;
}

static void Measurement_Time_Refresh(void)
{
    uint32_t now = HAL_GetTick();
    PCF8563_TimeTypeDef time;
    if ((uint32_t)(now - Clock_Read_Tick) >= 1000U)
    {
        Clock_Read_Tick = now;
        if (PCF8563_GetTime(&hi2c2,&time)==HAL_OK && time.century==0U)
            snprintf(Clock_Text,sizeof(Clock_Text),"%02u/%02u/%02u %02u:%02u",
                     (unsigned)time.year,(unsigned)time.month,(unsigned)time.day,
                     (unsigned)time.hour,(unsigned)time.minute);
        else strcpy(Clock_Text,"--/--/-- --:--");
    }
    if (Battery_Last_Icon != 5U &&
        (Clock_Force_Redraw || strcmp(Clock_Displayed, Clock_Text) != 0))
    {
        OLED_PrintString(5,0,Clock_Text,FONT_6X8);
        strcpy(Clock_Displayed, Clock_Text);
        Clock_Force_Redraw = 0U;
    }
}

/*************************主界面***************************/
void Main_Page_Init(void)
{
    Battery_Force_Redraw=1U;
    Clock_Force_Redraw=1U;
    Main_Values_Need_Redraw = 1U;
    CG_Display_Status_History = 0xFFU;
    Goal_Marker_History = 0xFFU;
    CG_Value_History = 0;
    GJ_Value_History = 0;
    left_And_Righe_Status_History = 1-left_And_Righe_Status;
    OLED_PrintChinese(5,2,"轨距");
    OLED_PrintString(44,2,":",FONT_8X16);
    OLED_PrintChinese(5,5,"超高");
    OLED_PrintString(44,5,":",FONT_8X16);
}


void Main_Page_Reflash(void)
{
    char buffer[20];
    /*显示轨距数值*/
    GJ_Value = Get_GJ_Value(GJ_Ad,Temp);
    //获取轨距数据
    if(Main_Values_Need_Redraw || GJ_Value_History != GJ_Value)
    {
        if(Data_Show_Percision == 1)
            snprintf(buffer,sizeof(buffer),"%+.2f",GJ_Value);
        else
            snprintf(buffer,sizeof(buffer),"%+.1f",GJ_Value);
        OLED_PrintString(52,2,buffer,FONT_8X16);
        GJ_Value_History  =  GJ_Value;
    }
    
    /*显示超高数值*/
    /* [CG32-04] Error status takes precedence over a numeric zero.
     * Leave the raw CG_Ad and CG_Value_t available in Keil Watch. */
    CG_Value = Get_CG_Value(CG_Ad,Temp);
    {
        uint8_t state = CG_Result_Status;
        if (!Sensor_Data_Is_Fresh()) state = CG_RESULT_NO_SENSOR;
        if (Eep_Get_Storage_State() != EEP_STATE_B3_VALID)
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
    Measurement_Battery_Refresh();
    Measurement_Time_Refresh();
    if (Goal_Marker_History != Goal_Set_Status)
    {
        OLED_PrintString(5,1,Goal_Set_Status == 1 ? "MUBIAO" : "      ",FONT_6X8);
        Goal_Marker_History = Goal_Set_Status;
    }
    {
        uint8_t in_range = Goal_Set_Status == 1 &&
            ((CG_Result_Status == CG_RESULT_OK && Sensor_Data_Is_Fresh() &&
              Eep_Get_Storage_State() == EEP_STATE_B3_VALID &&
              CG_Value > Goal_CG_Min_Value && CG_Value < Goal_CG_Max_Value) ||
             (GJ_Value > Goal_CJ_Min_Value && GJ_Value < Goal_CJ_Max_Value));
        if (in_range) Beep_Ctrl(100U,3U);
    }
    /*显示左右股状态*/
    if (Battery_Last_Icon != 5U && left_And_Righe_Status_History != left_And_Righe_Status)
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

static float CZ_Value_S = 0,HB_Value_S = 0;
static uint8_t CH_Display_Status_History=0xFFU;
static uint8_t CH_Values_Need_Redraw=1U;
void CH_Page_Init(void)
{
    Battery_Force_Redraw=1U;
    Clock_Force_Redraw=1U;
    CZ_Value_S = 0,HB_Value_S = 0;
    CH_Display_Status_History=0xFFU;
    CH_Values_Need_Redraw=1U;
    OLED_PrintChinese(5,2,"查照");
    OLED_PrintString(44,2,":",FONT_8X16);
    OLED_PrintChinese(5,5,"护背");
    OLED_PrintString(44,5,":",FONT_8X16);
}


void CH_Page_Reflash(void)
{
    char buffer[20];
    uint8_t state=0U;
    const char *error="";
    if (!Sensor_Data_Is_Fresh()) { state=1U; error="NO SENSOR"; }
    else if (Eep_Get_Storage_State()!=EEP_STATE_B3_VALID) { state=2U; error="EEP ERROR"; }
    else if (GJ_Ad==0U || GJ_Ad>0x7FFFU) { state=3U; error="AD ERROR "; }
    else if (!Get_CH_Values(GJ_Ad,&CZ_Value,&HB_Value)) { state=4U; error="CAL ERROR"; }
    if (CH_Display_Status_History!=state)
    {
        OLED_PrintString(52,2,"         ",FONT_8X16);
        OLED_PrintString(52,5,"         ",FONT_8X16);
        CH_Values_Need_Redraw=1U;
        CH_Display_Status_History=state;
    }
    if (state!=0U)
    {
        if (CH_Values_Need_Redraw)
        {
            OLED_PrintString(52,2,(char *)error,FONT_8X16);
            OLED_PrintString(52,5,(char *)error,FONT_8X16);
        }
    }
    else
    {
        if (CH_Values_Need_Redraw || CZ_Value_S!=CZ_Value)
        {
            int length=snprintf(buffer,sizeof(buffer),Data_Show_Percision==1 ? "%+.2f" : "%+.1f",CZ_Value);
            /* Pad in one write: erase old digits without blanking the row. */
            if (length<0 || length>9) snprintf(buffer,sizeof(buffer),"RANGE    ");
            else { while(length<9) buffer[length++]=' '; buffer[9]='\0'; }
            OLED_PrintString(52,2,buffer,FONT_8X16);
            CZ_Value_S=CZ_Value;
        }
        if (CH_Values_Need_Redraw || HB_Value_S!=HB_Value)
        {
            int length=snprintf(buffer,sizeof(buffer),Data_Show_Percision==1 ? "%+.2f" : "%+.1f",HB_Value);
            /* Pad in one write: erase old digits without blanking the row. */
            if (length<0 || length>9) snprintf(buffer,sizeof(buffer),"RANGE    ");
            else { while(length<9) buffer[length++]=' '; buffer[9]='\0'; }
            OLED_PrintString(52,5,buffer,FONT_8X16);
            HB_Value_S=HB_Value;
        }
    }
    CH_Values_Need_Redraw=0U;
    Measurement_Battery_Refresh();
    Measurement_Time_Refresh();
}

/************************菜单选择界面***************************/

static const char Menu_buffer[MENU_ID_NUMBER-1U][9] = {
    "时间设置","目标设置","标定设置","密码设置","精度设置"
};
static const uint8_t Bluetooth_Menu_Text[128] = {
    0x00, 0x04, 0xE4, 0x04, 0x04, 0xEE, 0x04, 0x84, 0xE4, 0x7E, 0xC4, 0x44, 0x44, 0x44, 0x00, 0x00,
    0x00, 0x00, 0x46, 0x76, 0x5E, 0x46, 0x46, 0xC6, 0xC6, 0xFE, 0x46, 0x46, 0x44, 0x40, 0x00, 0x00,
    0x00, 0x80, 0x86, 0x86, 0x86, 0xFE, 0xC6, 0x86, 0x86, 0xFE, 0xFE, 0x86, 0x86, 0x80, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x10, 0x12, 0x1C, 0x10, 0xF0, 0x10, 0x1E, 0x12, 0x10, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x20, 0x3D, 0x3C, 0x24, 0x3F, 0x3C, 0x25, 0x3C, 0x3C, 0x25, 0x3D, 0x3C, 0x20, 0x00, 0x00,
    0x00, 0x30, 0x30, 0x18, 0x08, 0x0C, 0x06, 0x23, 0x20, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x30, 0x30, 0x1C, 0x07, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x21, 0x21, 0x31, 0x11, 0x19, 0x0F, 0x03, 0x07, 0x0D, 0x19, 0x31, 0x31, 0x21, 0x00, 0x00,
};
static uint8_t Bluetooth_Menu_History;

void Menu_Page_Init(void)
{
    menu_Id_History = 0xFFU;
    menu_Id_Select = 0U;
    Bluetooth_Menu_History = 0xFFU;
}

void Menu_Page_Reflash(void)
{
    uint8_t status = Bluetooth_Status();
    uint8_t start, row;
    if (menu_Id_Select >= MENU_ID_NUMBER) menu_Id_Select = 0U;
    if (menu_Id_History == menu_Id_Select && Bluetooth_Menu_History == status) return;
    start = menu_Id_Select > 2U ? menu_Id_Select - 2U : 0U;
    for (row = 0U; row < 3U; ++row)
    {
        uint8_t item = start + row;
        uint8_t page = 1U + row * 2U;
        OLED_PrintString(16,page,item == menu_Id_Select ? "->" : "  ",FONT_8X16);
        OLED_PrintLine(96,128,page,0U);
        OLED_PrintLine(96,128,page+1U,0U);
        if (item == MENU_ID_NUMBER-1U)
        {
            const char *label = status == BT_ON ? "ON" : status == BT_OFF ? "OFF" :
                                status == BT_BUSY ? "..." : "ERR";
            OLED_ShowImage(32,page*8U,64,16,Bluetooth_Menu_Text);
            OLED_PrintString(99,page,(char *)label,FONT_6X8);
        }
        else OLED_PrintChinese(32,page,(char *)Menu_buffer[item]);
    }
    menu_Id_History = menu_Id_Select;
    Bluetooth_Menu_History = status;
}

/************************时间设置界面***************************/

void Time_Set_Page_Init(void)
{
    PCF8563_TimeTypeDef timeT;
    uint8_t i;
    static const uint8_t fallback[6]={0,1,1,0,0,0};
    Set_Time_Error=0U;
    for (i=0U;i<6U;++i) Set_Time[i]=fallback[i];
    if (PCF8563_GetTime(&hi2c2,&timeT)==HAL_OK && timeT.century==0U)
    {
        Set_Time[0]=timeT.year; Set_Time[1]=timeT.month; Set_Time[2]=timeT.day;
        Set_Time[3]=timeT.hour; Set_Time[4]=timeT.minute; Set_Time[5]=timeT.second;
    }
    else Set_Time_Error=1U;
    PCF8563_NormalizeEdit(Set_Time);
    OLED_PrintString(9,2,"Y",FONT_8X16);
    OLED_PrintString(49,2,"M",FONT_8X16);
    OLED_PrintString(89,2,"D",FONT_8X16);
    OLED_PrintString(9,5,"h",FONT_8X16);
    OLED_PrintString(49,5,"m",FONT_8X16);
    OLED_PrintString(89,5,"s",FONT_8X16);
    Set_Time_Select=0U;
    Set_Time_Select_History=0xFFU;
}

void Time_Set_Page_Reflash(void)
{
    static const uint8_t xpos[6]={20,60,100,20,60,100};
    static const uint8_t ypos[6]={2,2,2,5,5,5};
    uint8_t i;
    char text[4];
    const char *status="20YY             ";
    PCF8563_NormalizeEdit(Set_Time);
    if (Set_Time_Select>=6U) Set_Time_Select=0U;
    for (i=0U;i<6U;++i)
    {
        /* Two digits plus one space removes any old third digit (e.g. 999).
         * Day/seconds end at x=123, within the 128 px display. */
        snprintf(text,sizeof(text),"%02u ",(unsigned)(Set_Time[i]%100U));
        OLED_PrintString(xpos[i],ypos[i],text,FONT_8X16);
    }
    if (Set_Time_Select_History!=Set_Time_Select)
    {
        for (i=0U;i<6U;++i)
            OLED_PrintLine(xpos[i],xpos[i]+16U,ypos[i]+2U,i==Set_Time_Select ? 0x01 : 0x00);
        Set_Time_Select_History=Set_Time_Select;
    }
    if (Set_Time_Error==1U) status="RTC READ ERROR   ";
    else if (Set_Time_Error==2U) status="INVALID DATE     ";
    else if (Set_Time_Error==3U) status="RTC WRITE ERROR  ";
    OLED_PrintString(5,0,(char *)status,FONT_6X8);
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

void CG_Calibration_Page_Init(void)
{
    CG_Calibration_Status = 0;
    Sensor_CG_Stability_Reset();
    CG_Cal_Save_Failed = 0U; /* [CGFIX-03] */
//    CG_Calibration_Tim_Current = 0;
//    CG_Calibration_Tim_History = 0;
}


char CG_buffer[8] = {0}; /* [B1] Signed 16-bit: -32768 + NUL needs 7 bytes. */
void CG_Calibration_Page_Reflash(void)
{
    char live_ad_text[16];
    uint8_t sensor_fresh = Sensor_Data_Is_Fresh();
    if(CG_Calibration_Status_History!=CG_Calibration_Status)
    {
        OLED_Clear();
//        CG_Calibration_Tim_History = HAL_GetTick();
        CG_Calibration_Status_History = CG_Calibration_Status;
    }
    switch(CG_Calibration_Status)
    {
        case 0:
            OLED_PrintChinese(40,0,"水平处");
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 1;
//            }
            break;
        case 1:
            OLED_PrintChinese(40,0,"水平处");
            snprintf(CG_buffer,sizeof(CG_buffer),"%+06d",CG_Param_Temp.CG_Zero_Left_Ad);
            OLED_PrintString(60,4,CG_buffer,FONT_8X16);
            break;
        case 2:
            OLED_PrintChinese(24,0,"水平处掉头");
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 3;
//            }
            break;
        case 3:
            OLED_PrintChinese(24,0,"水平处掉头");
            snprintf(CG_buffer,sizeof(CG_buffer),"%+06d",CG_Param_Temp.CG_Zero_Right_Ad);
            OLED_PrintString(60,4,CG_buffer,FONT_8X16);
            break;
        case 4:
            OLED_PrintString(44,0,"180mm",FONT_8X16);
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 5;
//            }
            break;
        case 5:
            OLED_PrintString(44,0,"180mm",FONT_8X16);
            snprintf(CG_buffer,sizeof(CG_buffer),"%+06d",CG_Param_Temp.CG_Max_Left_Ad);
            OLED_PrintString(60,4,CG_buffer,FONT_8X16);
            break;
        case 6:
            OLED_PrintString(28,0,"180mm",FONT_8X16);
            OLED_PrintChinese(68,0,"掉头");
            OLED_PrintChinese(24,4,"存储键保存");
//            CG_Calibration_Tim_Current = HAL_GetTick();
//            if((CG_Calibration_Tim_Current - CG_Calibration_Tim_History)>1000)
//            {
//                CG_Calibration_Status = 7;
//            }
            break;
        case 7:
            OLED_PrintString(28,0,"180mm",FONT_8X16);
            OLED_PrintChinese(68,0,"掉头");
            snprintf(CG_buffer,sizeof(CG_buffer),"%+06d",CG_Param_Temp.CG_Max_Right_Ad);
            OLED_PrintString(60,4,CG_buffer,FONT_8X16);
            break;
        case 8:
            /* [CGFIX-03] Collected four AD points, NOT saved yet. */
            OLED_PrintChinese(32,0,"超高标定");
            if(CG_Cal_Save_Failed == 3U)
                OLED_PrintString(28,4,"NO SENSOR",FONT_8X16);
            else if(CG_Cal_Save_Failed == 2U)
                OLED_PrintString(28,4,"CAL ERROR",FONT_8X16);
            else if(CG_Cal_Save_Failed)
                OLED_PrintString(28,4,"EEP ERROR",FONT_8X16);
            else
                OLED_PrintChinese(24,4,"存储键保存");
            break;
        case 9:
            /* Only reached after EEPROM CRC/readback commit succeeds. */
            OLED_PrintChinese(32,0,"超高标定");
            OLED_PrintChinese(48,4,"成功");
            break;
        case 10:
            set_PageId(page_calibration_menu);
            break;
    }
    /* [CG37] Live received AD stays visible before and after capture.
     * Six signed digits fully overwrite previous values, including -32768. */
    if (CG_Calibration_Status < 10U)
    {
        if (sensor_fresh)
            snprintf(live_ad_text,sizeof(live_ad_text),"AD:%+06d",CG_Ad);
        else
            snprintf(live_ad_text,sizeof(live_ad_text),"AD:------");
        OLED_PrintString(28,2,live_ad_text,FONT_8X16);
    }
    if (CG_Calibration_Status < 8U)
    {
        int16_t stable_ad;
        char *status = "SAVED    ";
        if ((CG_Calibration_Status & 1U) != 0U)
            OLED_PrintString(20,4,"SAVE:",FONT_8X16);
        if (!sensor_fresh) status = "NO SENSOR";
        else if ((CG_Calibration_Status & 1U) == 0U)
        {
            if (Sensor_Get_Stable_CG(&stable_ad)) status = "READY    ";
            else
            {
                switch (CG_Calibration_Wait_State)
                {
                    case CG_CAL_WAIT_LINK:  status = "WAIT LINK"; break;
                    case CG_CAL_WAIT_TIME:  status = "WAIT TIME"; break;
                    case CG_CAL_WAIT_NOISE: status = "WAIT AD  "; break;
                    case CG_CAL_WAIT_DRIFT: status = "WAIT DRFT"; break;
                    default:               status = "WAIT DATA"; break;
                }
            }
        }
        OLED_PrintString(28,6,status,FONT_8X16);
    }
}

/*************************轨距标定设置界面***************************/
void GJ_Calibration_Page_Init(void)
{
    GJ_Calibration_Status = 0;
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
            snprintf(GJ_buffer,sizeof(GJ_buffer),"%05d",GJ_Ad);
            OLED_PrintString(44,4,GJ_buffer,FONT_8X16);
            break;
        case 2:
            OLED_PrintChinese(24,1,"当前标定值");
            OLED_PrintString(36,3,"1435mm",FONT_8X16);
            break;
        case 3:
            OLED_PrintString(36,1,"1435mm",FONT_8X16);
            snprintf(GJ_buffer,sizeof(GJ_buffer),"%05d",GJ_Ad);
            OLED_PrintString(44,4,GJ_buffer,FONT_8X16);
            break;
        case 4:
            OLED_PrintChinese(24,1,"当前标定值");
            OLED_PrintString(36,3,"1470mm",FONT_8X16);
            break;
        case 5:
            OLED_PrintString(36,1,"1470mm",FONT_8X16);
            snprintf(GJ_buffer,sizeof(GJ_buffer),"%05d",GJ_Ad);
            OLED_PrintString(44,4,GJ_buffer,FONT_8X16);
            break;
        case 6:
            OLED_PrintChinese(32,1,"轨距标定");
            OLED_PrintChinese(48,4,"成功");
            break;
        case 7:
            set_PageId(page_calibration_menu);
            break;
    }
}

/*************************查护标定设置界面***************************/
void CH_Calibration_Page_Init(void)
{
    CH_Calibration_Status=0U;
    CH_Calibration_Status_History=0xFFU;
    CH_Cal_Save_Failed=0U;
    CH_Param_Temp=CH_Param;
    Sensor_GJ_Stability_Reset();
}

char CH_buffer[8] = {0}; /* [B1] Signed 16-bit: -32768 + NUL needs 7 bytes. */
void CH_Calibration_Page_Reflash(void)
{
    char text[17];
    const char *status="CAPTURED    ";
    uint8_t fresh=Sensor_Data_Is_Fresh();
    if (CH_Calibration_Status_History!=CH_Calibration_Status)
    {
        OLED_Clear();
        CH_Calibration_Status_History=CH_Calibration_Status;
    }
    if (CH_Calibration_Status>=6U)
    {
        set_PageId(page_calibration_menu);
        return;
    }
    if (CH_Calibration_Status<2U) OLED_PrintString(20,0,"CZ 1391mm",FONT_8X16);
    else if (CH_Calibration_Status<4U) OLED_PrintString(20,0,"HB 1348mm",FONT_8X16);
    else OLED_PrintChinese(32,0,"查护标定");
    if (fresh) snprintf(text,sizeof(text),"AD:%05u",(unsigned)GJ_Ad);
    else snprintf(text,sizeof(text),"AD:-----");
    OLED_PrintString(28,2,text,FONT_8X16);
    if (CH_Calibration_Status==1U || CH_Calibration_Status==3U)
    {
        snprintf(text,sizeof(text),"CAP:%05u",(unsigned)(CH_Calibration_Status==1U ?
                 CH_Param_Temp.CZ_Ad : CH_Param_Temp.HB_Ad));
        OLED_PrintString(28,4,text,FONT_8X16);
    }
    else if (CH_Calibration_Status==5U)
    {
        OLED_PrintChinese(48,4,"成功");
        status="SAVED       ";
    }
    else
    {
        OLED_PrintChinese(24,4,"存储键保存");
        if (CH_Calibration_Status==4U)
        {
            status="SAVE?       ";
            if (CH_Cal_Save_Failed==1U) status="EEP ERROR   ";
            else if (CH_Cal_Save_Failed==2U) status="CAL ERROR   ";
            else if (CH_Cal_Save_Failed==3U) status="NO SENSOR   ";
        }
        else
        {
            float scale;
            uint16_t stable_ad;
            if (!GJ_Get_Valid_Scale(&scale)) status="GJ CAL ERROR";
            else if (!(Temp>=-40.0f && Temp<=100.0f)) status="TEMP ERROR  ";
            else if (Sensor_Get_Stable_GJ(&stable_ad,scale)) status="READY       ";
            else
            {
                switch (CH_Calibration_Wait_State)
                {
                    case CH_CAL_WAIT_LINK: status="WAIT LINK   "; break;
                    case CH_CAL_WAIT_TIME: status="WAIT TIME   "; break;
                    case CH_CAL_WAIT_NOISE: status="WAIT NOISE  "; break;
                    case CH_CAL_WAIT_DRIFT: status="WAIT DRIFT  "; break;
                    case CH_CAL_WAIT_SCALE: status="GJ CAL ERROR"; break;
                    default: status="WAIT DATA   "; break;
                }
            }
        }
    }
    if (!fresh && CH_Calibration_Status!=5U) status="NO SENSOR   ";
    OLED_PrintString(16,6,(char *)status,FONT_8X16);
}



















/************************系统相关函数**********************/



void oled_Reflash(void)
{
    PAGE_ID            page_id = get_PageId();
    static PAGE_FUN_T  page_fun;
    static uint32_t render_tick;
    uint32_t now = HAL_GetTick();
    
    if(page_id != Page_Id_History)
    {
        //更新页面
        page_fun = Page_Fun_Buffere[page_id];
        Page_Id_History = page_id;
        //清除屏幕
        OLED_Clear();
        //刷新初始界面
        if(page_fun.init != NULL)
            page_fun.init();
    }
    
    else if ((uint32_t)(now - render_tick) < 20U) return;

    render_tick = now;
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












