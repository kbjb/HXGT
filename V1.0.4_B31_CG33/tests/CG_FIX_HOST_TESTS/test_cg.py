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
static uint8_t link_fresh=1;
uint8_t Sensor_Data_Is_Fresh(void) {return link_fresh;}
int16_t CG_Ad=0;
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
  link_fresh=1;
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
    for source, out, more in [('test_math.c','test_math',['Algorithm_code.c','-lm']), ('test_key.c','test_key',[])]:
        cmd=['cc','-std=c99','-O1','-Wall','-Wextra','-Werror','-Wno-unused-parameter',
             '-fsanitize=undefined','-fno-omit-frame-pointer','-I',str(d),str(d/source)]
        for v in more:
            cmd.append(str(d/v) if v.endswith('.c') else v)
        cmd += ['-o',str(d/out)]
        subprocess.run(cmd,check=True,capture_output=True,text=True)
        subprocess.run([str(d/out)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
print('CG integration static checks: PASS; EEPROM production switches unchanged')
