#!/usr/bin/env python3
"""CG38 checks compiled from the actual CH math, receiver, key and OLED code.

CC selects the compiler. Set HOST_SANITIZERS=0 only when UBSan is unavailable.
No hardware is accessed. EEPROM persistence/fault injection lives in B31 tests.
"""
from pathlib import Path
import os, re, subprocess, tempfile

root=Path(__file__).resolve().parents[2]
def read(p):
    return (root/p).read_bytes().decode('gb18030').replace('\r\n','\n')
def function(s,sig):
    a=s.index(sig); b=s.index('{',a); depth=0
    for i in range(b,len(s)):
        depth+=(s[i]=='{')-(s[i]=='}')
        if depth==0: return s[a:i+1]+'\n'
    raise ValueError(sig)
algorithm=read('STM32L151/Software/src/Algorithm_code.c')
receiver=read('STM32L151/Software/src/agreement_code.c')
key=read('STM32L151/Software/src/App_key.c')
oled=read('STM32L151/Software/src/App_code.c')
header=read('STM32L151/Software/inc/agreement_head.h')
algo_header=read('STM32L151/Software/inc/algorithm_head.h')
font_source=read('STM32L151/Software/src/oledfont.c')
for label in re.findall(r'OLED_PrintChinese\([^;]*?"([^"]+)"',function(oled,'void CH_Calibration_Page_Reflash(')):
    assert all(char in font_source for char in label), ('Missing OLED glyph',label)
definitions='\n'.join(line for line in header.splitlines() if line.startswith('#define SENSOR_') or line.startswith('#define CG_CAL_') or line.startswith('#define CH_CAL_'))
definitions+='\n'+'\n'.join(line for line in algo_header.splitlines() if line.startswith('#define CH_'))+'\n'
for name in ['CG','CH']:
    a=header.index('typedef enum\n{\n    '+name+'_CAL_CAPTURE_READY')
    b=header.index('} '+name+'_CAL_WAIT_STATE;',a)+len('} '+name+'_CAL_WAIT_STATE;')
    definitions+=header[a:b]+'\n'
rxprefix=receiver[:receiver.index('int16_t Unsigned_Six_To_Signed_Six(')]
rxprefix=re.sub(r'^#include.*\n','',rxprefix,flags=re.M)
code=r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
typedef struct {uint16_t GJ_1410_Ad,GJ_1435_Ad,GJ_1470_Ad;float temp;} GJ_Calibration_Param_Str;
typedef struct {uint16_t CZ_Ad,HB_Ad;float temp;} CH_Calibration_Param_Str;
typedef union {uint16_t uinf;int16_t inf;} UToI;
typedef union {uint32_t inf;float flo;} FToI;
GJ_Calibration_Param_Str GJ_Param={7781,15946,27395,20.0f};
CH_Calibration_Param_Str CH_Param={10318,10318,20.0f},CH_Param_Temp;
uint16_t GJ_Ad; int16_t CG_Ad; float Temp=20.0f,GJ_Value,CZ_Value,HB_Value;
uint8_t CH_Calibration_Status,CH_Calibration_Status_History,CH_Cal_Save_Failed,Data_Show_Percision=1;
static uint32_t tick=1000;
uint32_t HAL_GetTick(void){return tick;}
static unsigned checks=0;
#define CHECK(x) do{assert(x);checks++;}while(0)
'''+definitions+rxprefix
for sig in ['int16_t Unsigned_Six_To_Signed_Six(', 'float Unsigned_Six_To_Float(', 'uint8_t Get_Add_Verify(', 'uint8_t Sensor_Data_Receive_For_Uart1(']:
    code+=function(receiver,sig)
for sig in ['static uint8_t GJ_Fit_Coefficients(', 'float Get_GJ_Value(', 'uint8_t GJ_Get_Valid_Scale(', 'uint8_t Get_CH_Values(']:
    code+=function(algorithm,sig)
code+=r'''
static unsigned save_calls=0,beeps=0; static uint8_t save_ok=0;
uint8_t Eep_Save_CH_Calibration(CH_Calibration_Param_Str value) {
    save_calls++; if(!save_ok)return 0;CH_Param=value;return 1;
}
void Beep_Ctrl(unsigned ms,unsigned n){(void)ms;beeps=n;}
#define FONT_8X16 1
#define FONT_6X8 2
#define EEP_STATE_B3_VALID 1
#define page_calibration_menu 7
static unsigned storage_state=1,page_id=0;
unsigned Eep_Get_Storage_State(void){return storage_state;}
typedef struct {unsigned year,month,day,hour,minute;} PCF8563_TimeTypeDef;
static int hi2c2;
void PCF8563_GetTime(void *h,PCF8563_TimeTypeDef *t){(void)h;*t=(PCF8563_TimeTypeDef){26,11,23,12,0};}
void set_PageId(unsigned p){page_id=p;}
static char texts[100][40];static unsigned ntexts=0,clears=0;
void OLED_Clear(void){clears++;}
static void rect(unsigned x,unsigned y,unsigned w,unsigned h){CHECK(x+w<=128 && y+h<=8);}
static uint8_t power_4_ico[16];
void OLED_ShowImage(unsigned x,unsigned y,unsigned w,unsigned h,uint8_t *data){(void)data;rect(x,y,w,(h+7U)/8U);}
void OLED_PrintString(unsigned x,unsigned y,char *s,unsigned font){
    rect(x,y,(unsigned)strlen(s)*(font==FONT_8X16?8U:6U),font==FONT_8X16?2U:1U);
    assert(ntexts<100);snprintf(texts[ntexts++],40,"%s",s);
}
void OLED_PrintChinese(unsigned x,unsigned y,char *s){
    unsigned chars=0;const unsigned char*p=(unsigned char*)s;
    while(*p){if((*p&0xC0)!=0x80)chars++;p++;}
    rect(x,y,chars*16U,2U);
    assert(ntexts<100);snprintf(texts[ntexts++],40,"%s",s);
}
static void screen_reset(void){ntexts=0;}
static int saw(const char *s){
    unsigned i;size_t a=strlen(s);while(a && s[a-1]==' ')a--;
    for(i=0;i<ntexts;i++){size_t b=strlen(texts[i]);while(b && texts[i][b-1]==' ')b--;if(a==b && strncmp(s,texts[i],a)==0)return 1;}return 0;
}
static float CZ_Value_S=0,HB_Value_S=0;
static uint8_t CH_Display_Status_History=0xFFU,CH_Values_Need_Redraw=1;
'''
for sig in ['void CH_Page_Init(', 'void CH_Page_Reflash(', 'void CH_Calibration_Page_Init(', 'void CH_Calibration_Page_Reflash(']:
    code+=function(oled,sig)
code+=function(key,'void CH_Calibration_Storage(')
code+=r'''
static void frame(uint16_t ad){
    uint8_t f[13]={0x5A,10,0,0,0,0,0,0,0,0,0,13,10};FToI t;
    t.flo=Temp;f[2]=(uint8_t)(ad>>8);f[3]=(uint8_t)ad;
    f[6]=(uint8_t)(t.inf>>24);f[7]=(uint8_t)(t.inf>>16);f[8]=(uint8_t)(t.inf>>8);f[9]=(uint8_t)t.inf;
    f[10]=Get_Add_Verify(f,10);tick+=100;CHECK(Sensor_Data_Receive_For_Uart1(f,13)==3);
}
static void window(uint16_t ad){unsigned i;for(i=0;i<10;i++)frame(ad);}
static void test_math(void){
    unsigned ad;float cz,hb,k,g0,g,prev;
    GJ_Calibration_Param_Str original=GJ_Param;
    CHECK(GJ_Get_Valid_Scale(&k));
    CHECK(Get_CH_Values(10318,&cz,&hb) && cz==1391.0f && hb==1348.0f);
    CHECK(Get_CH_Values(10317,&cz,&hb) && cz<1391.0f && hb<1348.0f);
    CHECK(Get_CH_Values(10319,&cz,&hb) && cz>1391.0f && hb>1348.0f);
    g0=Get_GJ_Value(10318,20);prev=-1.0e9f;
    for(ad=1;ad<=32767;ad++){
        CHECK(Get_CH_Values((uint16_t)ad,&cz,&hb));
        g=Get_GJ_Value((uint16_t)ad,20);
        CHECK(fabsf((cz-1391.0f)-(g-g0))<0.001f);
        CHECK(cz>=prev && fabsf((cz-hb)-43.0f)<0.001f);prev=cz;
    }
    CH_Param.HB_Ad=11000;CHECK(Get_CH_Values(11000,&cz,&hb) && hb==1348.0f && cz>1391.0f);
    CH_Param.CZ_Ad=11000;CH_Param.HB_Ad=10318;
    CHECK(Get_CH_Values(11000,&cz,&hb) && cz==1391.0f && hb>1348.0f);
    GJ_Param=(GJ_Calibration_Param_Str){11000,8500,5000,20};CH_Param.CZ_Ad=CH_Param.HB_Ad=10318;
    CHECK(GJ_Get_Valid_Scale(&k) && k<0);
    CHECK(Get_CH_Values(10418,&cz,&hb) && fabsf(cz-1390.0f)<0.001f);
    CHECK(Get_CH_Values(10218,&cz,&hb) && fabsf(hb-1349.0f)<0.001f);
    GJ_Param.GJ_1435_Ad=GJ_Param.GJ_1410_Ad;CHECK(!Get_CH_Values(10318,&cz,&hb));
    GJ_Param=original;CHECK(!Get_CH_Values(0,&cz,&hb));CHECK(!Get_CH_Values(0x8000,&cz,&hb));
    CHECK(!Get_CH_Values(10318,NULL,&hb));
    CH_Param.CZ_Ad=0;CHECK(!Get_CH_Values(10318,&cz,&hb));CH_Param.CZ_Ad=10318;
    CH_Param.temp=NAN;CHECK(!Get_CH_Values(10318,&cz,&hb));CH_Param.temp=20;
    puts("CG38 math: PASS; full positive ADC sweep, shared gauge scale, independent/equal zeros, signed deltas, reversed slope, invalid data");
}
static void test_calibration(void){
    unsigned i;uint16_t ad;float k;CH_Calibration_Page_Init();
    CH_Calibration_Storage();CHECK(CH_Calibration_Status==0 && save_calls==0);
    frame(10318);CH_Calibration_Storage();CHECK(CH_Calibration_Status==0 && CH_Cal_Save_Failed==4);
    window(10318);screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("READY       ")&&saw("AD:10318"));
    CH_Calibration_Storage();CHECK(CH_Calibration_Status==1 && CH_Param_Temp.CZ_Ad==10318 && save_calls==0);
    frame(10325);screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("AD:10325")&&saw("CAP:10318")&&saw("CAPTURED    "));
    CH_Calibration_Storage();CHECK(CH_Calibration_Status==2);
    CH_Calibration_Storage();CHECK(CH_Calibration_Status==2 && CH_Cal_Save_Failed==4);
    window(10318);CH_Calibration_Storage();CHECK(CH_Calibration_Status==3 && CH_Param_Temp.HB_Ad==10318);
    CH_Calibration_Storage();CHECK(CH_Calibration_Status==4 && save_calls==0);
    screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("SAVE?       ") && !saw("成功"));
    CH_Calibration_Storage();CHECK(CH_Calibration_Status==4 && CH_Cal_Save_Failed==1 && save_calls==1 && beeps==3);
    screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("EEP ERROR   ")&&!saw("成功"));
    tick+=1500;save_ok=1;CH_Calibration_Storage();CHECK(CH_Calibration_Status==4 && save_calls==1 && CH_Cal_Save_Failed==3);
    frame(10318);CH_Calibration_Storage();CHECK(CH_Calibration_Status==5 && save_calls==2 && beeps==1);
    CHECK(CH_Param.CZ_Ad==10318 && CH_Param.HB_Ad==10318);
    screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("成功")&&saw("SAVED       "));
    tick+=1500;screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("AD:-----")&&saw("SAVED       "));
    CH_Calibration_Storage();screen_reset();CH_Calibration_Page_Reflash();CHECK(page_id==page_calibration_menu);
    CH_Calibration_Page_Init();CHECK(CH_Calibration_Status==0 && CH_Cal_Save_Failed==0);
    window(10318);Temp=NAN;CH_Calibration_Storage();CHECK(CH_Cal_Save_Failed==2 && save_calls==2);Temp=20;
    CHECK(GJ_Get_Valid_Scale(&k));CHECK(Sensor_Get_Stable_GJ(&ad,k));
    CH_Calibration_Page_Init();CHECK(!Sensor_Get_Stable_GJ(&ad,k));
    for(i=0;i<6;i++){
        CH_Calibration_Status=(uint8_t)i;CH_Calibration_Status_History=0xFF;frame(32767);
        screen_reset();CH_Calibration_Page_Reflash();CHECK(saw("AD:32767"));
    }
    puts("CG38 calibration: PASS; live vs captured AD, window reset, equal zeros, save confirmation/failure/retry, stale/temperature guards, OLED bounds");
}
static void test_measurement(void){
    float k;char expect[20];CH_Param=(CH_Calibration_Param_Str){10318,10318,20};
    frame(10318);screen_reset();CH_Page_Init();CH_Page_Reflash();CHECK(saw("+1391.00")&&saw("+1348.00"));
    frame(10418);screen_reset();CH_Page_Reflash();CHECK(GJ_Get_Valid_Scale(&k));
    snprintf(expect,sizeof(expect),"%+.2f",1391.0f+100*k);CHECK(saw(expect));
    tick+=1500;screen_reset();CH_Page_Reflash();CHECK(saw("NO SENSOR"));
    frame(10418);screen_reset();CH_Page_Reflash();CHECK(saw(expect));
    storage_state=0;screen_reset();CH_Page_Reflash();CHECK(saw("EEP ERROR"));storage_state=1;
    frame(0);screen_reset();CH_Page_Reflash();CHECK(saw("AD ERROR "));
    frame(10318);CH_Param.CZ_Ad=0;screen_reset();CH_Page_Reflash();CHECK(saw("CAL ERROR"));CH_Param.CZ_Ad=10318;
    Data_Show_Percision=0;screen_reset();CH_Page_Init();CH_Page_Reflash();CHECK(saw("+1391.0")&&saw("+1348.0"));
    puts("CG38 measurement: PASS; live values, precision, reentry, link loss/recovery, EEPROM/ADC/calibration errors");
}
int main(void){test_math();test_calibration();test_measurement();printf("CG38 CH tests: PASS %u checks\n",checks);return 0;}
'''
with tempfile.TemporaryDirectory(prefix='cg38_') as tmp:
    d=Path(tmp);c=d/'ch.c';exe=d/('ch.exe' if os.name=='nt' else 'ch')
    c.write_text(code,encoding='utf-8')
    cmd=[os.environ.get('CC','cc'),'-std=c99','-O1','-Wall','-Wextra','-Werror','-Wno-unused-parameter','-fstack-protector-all','-fno-omit-frame-pointer']
    if os.environ.get('HOST_SANITIZERS','1')!='0':cmd+=['-fsanitize=undefined']
    cmd += [str(c),'-lm','-o',str(exe)]
    subprocess.run(cmd,check=True)
    subprocess.run([str(exe)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
