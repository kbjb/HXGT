#include <assert.h>
#include "pcf8563_head.h"
#include "param_head.h"
#include "oled_head.h"
#include "App_Eep.h"
#include "algorithm_head.h"

uint8_t menu_Id_Select, menu_Id_History, Set_Time[6];
I2C_HandleTypeDef hi2c2;
static unsigned labels, moves, calls, beeps, beep_calls;
static char gj_label[24], cg_label[24];
static uint8_t fresh, gj_valid;
static EEP_STORAGE_STATE storage;
uint32_t HAL_GetTick(void) {return 0;}
uint8_t Sensor_Data_Is_Fresh(void) {return fresh;}
EEP_STORAGE_STATE Eep_Get_Storage_State(void) {return storage;}
uint8_t GJ_Result_Is_Valid(void) {return gj_valid;}
float Get_GJ_Value(uint16_t ad,float t) {(void)ad;(void)t;return 1435.0f;}
float Get_CG_Value(int16_t ad,float t) {(void)ad;(void)t;return 0.0f;}
volatile uint8_t CG_Result_Status;
uint16_t GJ_Ad;
int16_t CG_Ad;
float Temp, GJ_Value, CG_Value, GJ_Value_History, CG_Value_History;
uint8_t Data_Show_Percision=1U, Goal_Set_Status;
float Goal_CJ_Min_Value=1434, Goal_CJ_Max_Value=1436, Goal_CG_Min_Value, Goal_CG_Max_Value;
int8_t left_And_Righe_Status, left_And_Righe_Status_History;
uint8_t Get_Driver_Power(uint16_t ad) {(void)ad;return 1;}
uint16_t ADC_ReadChannel9(void) {return 0;}
char power_0_ico[16],power_n_ico[16],power_1_ico[16],power_2_ico[16],power_3_ico[16],power_4_ico[16];
void OLED_ShowImage(int16_t x,int16_t y,uint8_t w,uint8_t h,const uint8_t *img)
{(void)x;(void)y;(void)w;(void)h;(void)img;}
uint8_t PCF8563_GetTime(I2C_HandleTypeDef *h,PCF8563_TimeTypeDef *t)
{(void)h;(void)t;return 1;}
void Main_Page_Init(void);
void Main_Page_Reflash(void);
void CH_Page_Init(void);
void CH_Page_Reflash(void);
static char seen[3][9];
static PCF8563_TimeTypeDef last;
void Menu_Page_Init(void);
void Menu_Page_Reflash(void);
void Change_Time_For_Set_Time(void);
void OLED_PrintChinese(uint8_t x, uint8_t y, char *s) {
    if(x==5) return;
    assert(x == 32 && y == 1 + 2*(labels % 3) && strlen(s)==8);
    memcpy(seen[labels++ % 3],s,9);
}
void OLED_PrintString(uint8_t x,uint8_t y,char *s,uint8_t font) {
    if(x==52 && (y==2 || y==5)) {
        assert(strlen(s)<24); strcpy(y==2 ? gj_label : cg_label,s); return;
    }
    if(x!=16) return;
    assert(x==16 && font==FONT_8X16 && (y==1 || y==3 || y==5));
    if (!strcmp(s,"->")) ++moves; else assert(!strcmp(s,"  "));
}
void set_PageId(PAGE_ID page) { assert(page==page_menu); }
void Beep_Ctrl(uint16_t ms,uint16_t n) {assert(ms==100); beeps=n; ++beep_calls;}
uint8_t PCF8563_SetTime(I2C_HandleTypeDef *h,PCF8563_TimeTypeDef *t) {
    assert(h==&hi2c2); last=*t; ++calls; return HAL_OK;
}
static void check(uint8_t y,uint8_t m,uint8_t d,int weekday) {
    unsigned old=calls;
    Set_Time[0]=y;Set_Time[1]=m;Set_Time[2]=d;
    Change_Time_For_Set_Time();
    if(weekday<0) {assert(calls==old && beeps==3);return;}
    assert(calls==old+1 && beeps==1 && last.weekday==weekday && !last.century);
    assert(last.year==y && last.month==m && last.day==d && last.hour==Set_Time[3] &&
           last.minute==Set_Time[4] && last.second==Set_Time[5]);
}
int main(void) {
    char middle[9];
    Menu_Page_Init();
    assert(labels==3 && moves==1);
    memcpy(middle,seen[2],9);
    menu_Id_Select=255U; Menu_Page_Reflash();
    assert(menu_Id_Select==4 && menu_Id_History==4 && labels==6 && moves==2);
    assert(!strcmp(seen[0],middle));
    Menu_Page_Reflash(); assert(labels==6);
    menu_Id_Select=0; Menu_Page_Reflash(); assert(labels==9 && moves==3);
    Set_Time[3]=23; Set_Time[4]=59; Set_Time[5]=59;
    check(0,1,1,6); check(0,2,29,2); check(24,2,29,4);
    check(26,11,20,5); check(99,12,31,4);
    check(0,0,1,-1); check(0,1,0,-1); check(1,2,29,-1);
    check(99,4,31,-1); check(24,2,30,-1); check(24,13,1,-1);
    Set_Time[3]=24;check(24,1,1,-1);
    Set_Time[3]=0;Set_Time[4]=60;check(24,1,1,-1);
    Set_Time[4]=0;Set_Time[5]=60;check(24,1,1,-1);
    /* Identical underlying numbers throughout: status changes must redraw. */
    beep_calls=0; Goal_Set_Status=1; fresh=0; gj_valid=1;
    storage=EEP_STATE_B3_VALID; CG_Result_Status=CG_RESULT_OK;
    Main_Page_Init(); Main_Page_Reflash();
    assert(!strcmp(gj_label,"NO SENSOR") && !strcmp(cg_label,"NO SENSOR") && beep_calls==0);
    storage=EEP_STATE_IO_ERROR; Main_Page_Reflash();
    assert(!strcmp(gj_label,"EEP ERR") && !strcmp(cg_label,"EEP ERR") && beep_calls==0);
    fresh=1; gj_valid=0; storage=EEP_STATE_B3_VALID; Main_Page_Reflash();
    assert(!strcmp(gj_label,"CAL ERROR") && !strcmp(cg_label,"+000.00") && beep_calls==0);
    gj_valid=1; Main_Page_Reflash();
    assert(!strcmp(gj_label,"+1435.00") && beep_calls==1 && beeps==3);
    GJ_Ad=0xFFFFU; Main_Page_Reflash();
    assert(!strcmp(gj_label,"NO SENSOR") && !strcmp(cg_label,"+000.00") && beep_calls==1);
    GJ_Ad=15000U;
    fresh=0; Main_Page_Reflash(); assert(!strcmp(gj_label,"NO SENSOR") && beep_calls==1);
    CH_Page_Init(); CH_Page_Reflash();
    assert(!strcmp(gj_label,"NOT READY") && !strcmp(cg_label,"NOT READY") && beep_calls==1);
    puts("CG34 UI: PASS; menu bounds and terminators, RTC date/weekday, GJ status + alarms, unavailable CH");
}
