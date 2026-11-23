#!/usr/bin/env python3
"""B31 CG_FIX host checks using the firmware's real calculation and key handler.

Runs with local GCC/Clang. No hardware or EEPROM is touched.
"""
from pathlib import Path
import subprocess, tempfile, os, re

root = Path(__file__).resolve().parents[2]

def get(rel):
    return (root / rel).read_bytes().decode('gb18030')

algo = get('STM32L151/Software/src/Algorithm_code.c')
key = get('STM32L151/Software/src/App_key.c')
display = get('STM32L151/Software/src/App_code.c')
main = get('STM32L151/Core/Src/main.c')
eep = get('STM32L151/Software/src/App_Eep.c')

# Static integration checks.
assert 'B3_Eeprom_Boot_OK = Eep_Data_Get();' in main
assert main.index('B3_Eeprom_Boot_OK = Eep_Data_Get();') < main.index('CG_Data_Init();')
assert main.count('CG_Data_Init();') == 1
assert 'CG_Param = c->cg;' in eep and 'B3_CalibrationToGlobals(new_data);' in eep
assert 'CG_Display_Status_History != state' in display
assert 'CG_Result_Status' in display and 'CG RANGE' not in display
assert 'CG_MAX_MEASURE_MM' not in algo and 'CG_Calibration_Sane' in algo
assert 'if(Main_Values_Need_Redraw || GJ_Value_History != GJ_Value)' in display
assert 'Main_Values_Need_Redraw = 1U;' in display
assert 'Main_Values_Need_Redraw = 0U;' in display
cg_section = display[display.index('void CG_Calibration_Page_Reflash(void)'):display.index('void GJ_Calibration_Page_Init(void)')]
case8 = cg_section.split('        case 8:', 1)[1].split('        case 9:', 1)[0]
case9 = cg_section.split('        case 9:', 1)[1].split('        case 10:', 1)[0]
assert '"成功"' not in case8 and '"成功"' in case9
assert 'CG_Cal_Save_Failed' in case8 and '"EEP ERROR"' in case8 and '"CAL ERROR"' in case8
assert 'EEP_B3_SERVICE_ENABLE_LEGACY_CAL_UPGRADE 0' in get('STM32L151/Software/inc/App_Eep.h')
assert 'EEP_B3_SERVICE_IMPORT_LEGACY_COUNT       0' in get('STM32L151/Software/inc/App_Eep.h')

def extract_c_function(text, sig):
    start = text.index(sig)
    begin = text.index('{', start)
    depth = 0
    for i in range(begin, len(text)):
        if text[i] == '{': depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0: return text[start:i+1]
    raise ValueError('cannot locate closing brace')

with tempfile.TemporaryDirectory(prefix='cgfix_') as temp:
    d = Path(temp)
    (d / 'Algorithm_head.h').write_text(r'''
#ifndef ALGORITHM_HEAD_STUB
#define ALGORITHM_HEAD_STUB
#include <stdint.h>
#include <math.h>
typedef struct { int16_t CG_Zero_Left_Ad, CG_Max_Left_Ad, CG_Zero_Right_Ad, CG_Max_Right_Ad; float temp; } CG_Calibration_Param_Str;
typedef struct { uint16_t GJ_1410_Ad, GJ_1435_Ad, GJ_1470_Ad; float temp; } GJ_Calibration_Param_Str;
extern CG_Calibration_Param_Str CG_Param;
extern GJ_Calibration_Param_Str GJ_Param;
extern int8_t left_And_Righe_Status;
#define CG_RESULT_OK 0U
#define CG_RESULT_CAL_ERROR 1U
#define CG_RESULT_TEMP_ERROR 3U
extern volatile uint8_t CG_Result_Status;
extern float POWER_Vol, GJ_Value;
void CG_Data_Init(void);
float Get_CG_Value(int16_t CG_Ad, float temp);
float Get_GJ_Value(uint16_t GJ_Ad, float temp);
uint8_t Get_Driver_Power(uint16_t PowerAd);
#endif
''')
    (d / 'adc_head.h').write_text('float ADC_GetVrefintVoltage(void);\n')
    guard = get('STM32L151/Software/inc/cg_cal_guard.h').replace('#include "param_head.h"', '#include "Algorithm_head.h"')
    (d / 'cg_cal_guard.h').write_text(guard, encoding='utf-8')
    (d / 'Algorithm_code.c').write_text(algo, encoding='utf-8')
    (d / 'test_math.c').write_text(r'''
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "Algorithm_head.h"
extern float CG_Value_t;

static void check_zero_edges(int16_t left_zero, int16_t right_zero,
                             int16_t left_max, int16_t right_max)
{
    int32_t ad;
    int16_t low = left_zero < right_zero ? left_zero : right_zero;
    int16_t high = left_zero > right_zero ? left_zero : right_zero;
    float left_step = 180.0f / ((float)low - left_max);
    float right_step = 180.0f / ((float)right_max - high);
    float largest_step = fmaxf(left_step, right_step);
    float previous;
    CG_Param = (CG_Calibration_Param_Str){left_zero,left_max,right_zero,right_max,23.0f};
    left_And_Righe_Status = 1;
    CG_Data_Init();
    assert(CG_Result_Status == CG_RESULT_OK);
    assert(fabsf(Get_CG_Value(left_max,23)-180.0f)<0.001f);
    assert(fabsf(Get_CG_Value(right_max,23)+180.0f)<0.001f);
    assert(fabsf(Get_CG_Value(low,23))<0.001f);
    assert(fabsf(Get_CG_Value(high,23))<0.001f);
    if (low>INT16_MIN) {
        Get_CG_Value((int16_t)(low-1),23);
        assert(fabsf(CG_Value_t-left_step)<0.001f);
    }
    if (high<INT16_MAX) {
        Get_CG_Value((int16_t)(high+1),23);
        assert(fabsf(CG_Value_t+right_step)<0.001f);
    }
    Get_CG_Value(INT16_MIN,23);
    previous=CG_Value_t;
    for (ad=(int32_t)INT16_MIN+1;ad<=INT16_MAX;++ad) {
        Get_CG_Value((int16_t)ad,23);
        assert(CG_Result_Status==CG_RESULT_OK);
        assert(CG_Value_t<=previous+0.001f);
        assert(previous-CG_Value_t<=largest_step+0.001f);
        previous=CG_Value_t;
    }
    left_And_Righe_Status=-1;
    assert(fabsf(Get_CG_Value(left_max,23)+180.0f)<0.001f);
    assert(fabsf(Get_CG_Value(right_max,23)-180.0f)<0.001f);
    left_And_Righe_Status=1;
    assert(fabsf(Get_CG_Value(low,25)-0.10f)<0.001f);
}

CG_Calibration_Param_Str CG_Param = {1,-12445,50,12513,20};
GJ_Calibration_Param_Str GJ_Param = {7781,15946,27395,20};
int8_t left_And_Righe_Status = 1;
float POWER_Vol = 3.3f, GJ_Value = 0;
float ADC_GetVrefintVoltage(void) {return 3.3f;}
int main(void) {
  /* Before the coefficients are initialized, all CG AD values map to 0 at 20C. */
  assert(Get_CG_Value(-12445,20) == 0.0f);
  assert(CG_Result_Status == CG_RESULT_CAL_ERROR);
  CG_Data_Init();
  assert(fabsf(Get_CG_Value(-12445,20) - 180.0f) < 0.051f);
  assert(fabsf(Get_CG_Value(12513,20) + 180.0f) < 0.051f);
  assert(fabsf(Get_CG_Value(10,20)) < 0.051f);
  left_And_Righe_Status = -1;
  assert(fabsf(Get_CG_Value(-12445,20) + 180.0f) < 0.051f);
  left_And_Righe_Status = 1;
  CG_Param.CG_Zero_Left_Ad=100;
  CG_Param.CG_Max_Left_Ad=-9000;
  CG_Param.CG_Zero_Right_Ad=150;
  CG_Param.CG_Max_Right_Ad=9000;
  CG_Data_Init();
  assert(fabsf(Get_CG_Value(-9000,20) - 180.0f) < 0.051f);
  assert(fabsf(Get_CG_Value(9000,20) + 180.0f) < 0.051f);
  assert(fabsf(Get_CG_Value(110,20)) < 0.051f);
  CG_Param.CG_Zero_Left_Ad=10; CG_Param.CG_Max_Left_Ad=0;
  CG_Data_Init();
  assert(CG_Result_Status == CG_RESULT_CAL_ERROR);
  assert(Get_CG_Value(-70,20)==0.0f);
  assert(CG_Result_Status == CG_RESULT_CAL_ERROR);
  CG_Param.CG_Zero_Left_Ad=1; CG_Param.CG_Max_Left_Ad=-12445;
  CG_Param.CG_Zero_Right_Ad=50; CG_Param.CG_Max_Right_Ad=12513;
  CG_Param.temp=23;
  CG_Data_Init();
  assert(fabsf(Get_CG_Value(25,23))<0.001f);
  assert(CG_Result_Status == CG_RESULT_OK);
  assert(fabsf(Get_CG_Value(25,25)-0.10f)<0.06f);
  assert(Get_CG_Value(-30000,23)>400.0f);
  assert(CG_Result_Status == CG_RESULT_OK);
  assert(Get_CG_Value(30000,23)<-400.0f);
  assert(CG_Result_Status == CG_RESULT_OK);
  assert(Get_CG_Value(100,105)==0.0f);
  assert(CG_Result_Status == CG_RESULT_TEMP_ERROR);

  /* All 65536 AD codes are swept in normal, reversed, and equal-zero cases.
   * Include large zero gaps, signed endpoints, and minimum allowed spans. */
  check_zero_edges(1,50,-12445,12513);
  check_zero_edges(50,1,-12445,12513);
  check_zero_edges(50,50,-12445,12513);
  check_zero_edges(750,-750,-3250,3250);
  check_zero_edges(-750,750,-4750,4750);
  check_zero_edges(28000,27000,24000,32000);
  check_zero_edges(-28000,-27000,-32768,-23000);
  puts("CG35 zero edges: PASS; full 16-bit sweeps, both zero orders, endpoints, direction, temperature");
  puts("CG math: PASS; startup/invalid spans, unbounded numeric output, endpoints, temp reference");
  return 0;
}
''')
    func = extract_c_function(key, 'void CG_Calibration_Storage(void)')
    (d / 'test_key.c').write_text(r'''
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "Algorithm_head.h"
#include "cg_cal_guard.h"
CG_Calibration_Param_Str CG_Param, CG_Param_Temp;
static uint8_t link_fresh=1, sample_stable=1;
uint8_t Sensor_Data_Is_Fresh(void) {return link_fresh;}
int16_t CG_Ad=0;
uint8_t Sensor_Get_Stable_CG(int16_t *value) {
    *value=CG_Ad; return sample_stable;
}
void Sensor_CG_Stability_Reset(void) {}
float Temp=20.5f;
uint8_t CG_Calibration_Status=0, CG_Calibration_Status_History=8, CG_Cal_Save_Failed=0;
static int save_calls=0, init_calls=0, beeps=0, allow_save=0;
uint8_t Eep_Save_CG_Calibration(CG_Calibration_Param_Str p) {
    ++save_calls; if (!allow_save) return 0;
    CG_Param=p; return 1;
}
void CG_Data_Init(void) { ++init_calls; }
void Beep_Ctrl(uint16_t period,uint8_t times) {(void)period; beeps=times;}
''' + func + r'''
int main(void) {
  link_fresh=0; CG_Ad=100; CG_Calibration_Storage();
  assert(CG_Calibration_Status==0 && CG_Cal_Save_Failed==3);
  link_fresh=1; sample_stable=0;
  CG_Calibration_Storage();
  assert(CG_Calibration_Status==0 && CG_Cal_Save_Failed==4 && save_calls==0);
  sample_stable=1;
  CG_Ad=100;   CG_Calibration_Storage(); assert(CG_Calibration_Status==1);
  CG_Calibration_Storage(); assert(CG_Calibration_Status==2);
  CG_Ad=150;   CG_Calibration_Storage(); assert(CG_Calibration_Status==3);
  CG_Calibration_Storage(); assert(CG_Calibration_Status==4);
  CG_Ad=-9000; CG_Calibration_Storage(); assert(CG_Calibration_Status==5);
  CG_Calibration_Storage(); assert(CG_Calibration_Status==6);
  CG_Ad=9000;  CG_Calibration_Storage(); assert(CG_Calibration_Status==7);
  CG_Calibration_Storage(); assert(CG_Calibration_Status==8 && save_calls==0);
  link_fresh=0; CG_Calibration_Storage();
  assert(CG_Calibration_Status==8 && CG_Cal_Save_Failed==3 && save_calls==0);
  link_fresh=1;
  CG_Calibration_Storage(); assert(CG_Calibration_Status==8 && save_calls==1);
  assert(CG_Cal_Save_Failed==1 && beeps==3 && init_calls==0);
  assert(CG_Calibration_Status_History==0xFFU);
  allow_save=1;
  CG_Calibration_Storage(); assert(CG_Calibration_Status==9 && save_calls==2);
  assert(CG_Cal_Save_Failed==0 && init_calls==1 && beeps==1);
  assert(CG_Param.CG_Zero_Left_Ad==100 && CG_Param.CG_Zero_Right_Ad==150);
  assert(CG_Param.CG_Max_Left_Ad==-9000 && CG_Param.CG_Max_Right_Ad==9000);
  assert(CG_Param.temp==20.5f);
  CG_Calibration_Storage(); assert(CG_Calibration_Status==10 && save_calls==2);
  CG_Calibration_Status=8; CG_Cal_Save_Failed=0;
  CG_Param_Temp.CG_Max_Left_Ad=99;
  CG_Calibration_Storage();
  assert(CG_Calibration_Status==8 && CG_Cal_Save_Failed==2);
  assert(save_calls==2 && init_calls==1);
  puts("CG key: PASS; save not premature, failed/implausible saves rejected");
  return 0;
}
''')

    display_func = extract_c_function(display, 'void CG_Calibration_Page_Reflash(void)')
    capture_enum = get('STM32L151/Software/inc/agreement_head.h').replace('\r\n','\n')
    enum_start = capture_enum.index('typedef enum\n{\n    CG_CAL_CAPTURE_READY')
    enum_end = capture_enum.index('} CG_CAL_WAIT_STATE;',enum_start)+len('} CG_CAL_WAIT_STATE;')
    capture_enum = capture_enum[enum_start:enum_end]
    (d / 'test_display.c').write_text(r"""
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "Algorithm_head.h"
#define FONT_8X16 1
#define page_calibration_menu 1
CG_Calibration_Param_Str CG_Param_Temp={123,-12345,-456,23456,20};
uint8_t CG_Calibration_Status, CG_Calibration_Status_History, CG_Cal_Save_Failed;
int16_t CG_Ad;
char CG_buffer[8];
static uint8_t fresh=1, ready=0;
static char live_text[16], saved_text[16], state_text[16];
static unsigned save_label;
static uint8_t occupied[8][128];
uint8_t Sensor_Data_Is_Fresh(void) { return fresh; }
uint8_t Sensor_Get_Stable_CG(int16_t *ad) { *ad=0; return ready; }
void set_PageId(unsigned page) { (void)page; }
static void rect(unsigned x,unsigned y,unsigned width,unsigned pages) {
    unsigned xx,yy;
    assert(x+width<=128 && y+pages<=8);
    for(yy=y;yy<y+pages;++yy) for(xx=x;xx<x+width;++xx) {
        assert(!occupied[yy][xx]); occupied[yy][xx]=1;
    }
}
void OLED_Clear(void) {
    memset(occupied,0,sizeof(occupied));
    live_text[0]=saved_text[0]=state_text[0]=0; save_label=0;
}
void OLED_PrintString(uint8_t x,uint8_t y,char *text,uint8_t font) {
    assert(font==FONT_8X16);
    rect(x,y,8U*(unsigned)strlen(text),2);
    if(y==2) { assert(x==28); snprintf(live_text,sizeof(live_text),"%s",text); }
    if(y==4 && x==60) snprintf(saved_text,sizeof(saved_text),"%s",text);
    if(y==4 && x==20 && strcmp(text,"SAVE:")==0) save_label=1;
    if(y==6) snprintf(state_text,sizeof(state_text),"%s",text);
}
void OLED_PrintChinese(uint8_t x,uint8_t y,char *text) {
    unsigned chars=0;
    const unsigned char *p=(const unsigned char *)text;
    while(*p) { if((*p&0xC0)!=0x80) chars++; p++; }
    rect(x,y,16U*chars,2);
}
""" + capture_enum + '\nCG_CAL_WAIT_STATE CG_Calibration_Wait_State=CG_CAL_WAIT_NOISE;\n' + display_func + r"""
int main(void) {
    const int16_t values[]={INT16_MIN,-1,0,1,INT16_MAX};
    const int16_t saved[]={123,-456,-12345,23456};
    const char *wait_text[]={"WAIT DATA","WAIT LINK","WAIT TIME","WAIT AD  ","WAIT DRFT"};
    unsigned page,i;
    char expected[16];
    for(page=0;page<10;++page) for(i=0;i<sizeof(values)/sizeof(values[0]);++i) {
        CG_Calibration_Status=(uint8_t)page; CG_Calibration_Status_History=0xFF;
        CG_Cal_Save_Failed=0; CG_Ad=values[i]; fresh=1; ready=0;
        CG_Calibration_Page_Reflash();
        snprintf(expected,sizeof(expected),"AD:%+06d",values[i]);
        assert(strcmp(live_text,expected)==0);
        if(page<8 && (page&1)) {
            snprintf(expected,sizeof(expected),"%+06d",saved[page/2]);
            assert(save_label && strcmp(saved_text,expected)==0);
        }
    }
    for(i=1;i<=5;++i) {
        CG_Calibration_Status=0; CG_Calibration_Status_History=0xFF;
        CG_Calibration_Wait_State=(CG_CAL_WAIT_STATE)i;
        CG_Calibration_Page_Reflash();
        assert(strcmp(state_text,wait_text[i-1])==0);
    }
    ready=1; CG_Calibration_Status_History=0xFF; CG_Calibration_Page_Reflash();
    assert(strcmp(state_text,"READY    ")==0);
    fresh=0; CG_Calibration_Status_History=0xFF; CG_Calibration_Page_Reflash();
    assert(strcmp(live_text,"AD:------")==0 && strcmp(state_text,"NO SENSOR")==0);
    puts("CG37 calibration display: PASS; live AD on 10 states, full signed range, saved means, wait reasons, 128x64 nonoverlap");
    return 0;
}
""",encoding='utf-8')

    for source, out, more in [('test_math.c','test_math',['Algorithm_code.c','-lm']), ('test_key.c','test_key',[]), ('test_display.c','test_display',[])]:
        cmd=['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
             '-fsanitize=undefined','-fno-omit-frame-pointer','-I',str(d),str(d/source)]
        for v in more:
            cmd.append(str(d/v) if v.endswith('.c') else v)
        cmd += ['-o',str(d/out)]
        subprocess.run(cmd,check=True,capture_output=True,text=True)
        subprocess.run([str(d/out)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('CG integration static checks: PASS; EEPROM production switches unchanged')
