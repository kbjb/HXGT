#!/usr/bin/env python3
"""CG39 battery/calendar regression compiled from actual firmware functions.

Fake HAL I2C checks register bytes and injects failed reads/writes. No hardware.
CC selects compiler; HOST_SANITIZERS=0 disables UBSan explicitly if unavailable.
"""
from pathlib import Path
import datetime, os, re, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
def read(p): return (root/p).read_bytes().decode('gb18030').replace('\r\n','\n')
def extract(s,sig):
    a=s.index(sig); b=s.index('{',a); depth=0
    for i in range(b,len(s)):
        depth+=(s[i]=='{')-(s[i]=='}')
        if depth==0:return s[a:i+1]+'\n'
    raise ValueError(sig)
oled=read('STM32L151/Software/src/App_code.c')
key=read('STM32L151/Software/src/App_key.c')
rtc=read('STM32L151/Software/src/pcf8563_code.c')
header=read('STM32L151/Software/inc/pcf8563_head.h')
for name in ['Main','CH']:
    assert 'Battery_Force_Redraw=1U;' in extract(oled,'void '+name+'_Page_Init(')
    page=extract(oled,'void '+name+'_Page_Reflash(')
    assert 'Measurement_Battery_Refresh();' in page
    assert 'Measurement_Time_Refresh();' in page
    assert 'power_4_ico' not in page
routes={'KEY_ID_LEFT':'Reduce_Time_For_Time_Select','KEY_ID_RIGHT':'InCrease_Time_For_Time_Select','KEY_ID_CHANGE':'Change_Time_Select_Id','KEY_ID_SET':'Change_Time_For_Set_Time','KEY_ID_CANCEL':'GoTo_Menu_Page'}
for btn,handler in routes.items():
    assert re.search(r'\{\s*'+btn+r'\s*,KEY_STATUS_SHORT\s*,page_time\s*,'+handler,key)
defs='\n'.join(x for x in header.splitlines() if x.startswith('#define PCF8563_'))
a=header.index('typedef struct');b=header.index('} PCF8563_TimeTypeDef;',a)+len('} PCF8563_TimeTypeDef;')
code=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct {unsigned id;} I2C_HandleTypeDef;
typedef enum {HAL_OK=0,HAL_ERROR=1,HAL_BUSY=2,HAL_TIMEOUT=3} HAL_StatusTypeDef;
static I2C_HandleTypeDef hi2c2={2};
static uint32_t tick=0;
uint32_t HAL_GetTick(void){return tick;}
static unsigned checks=0;
#define CHECK(x) do {assert(x);checks++;} while(0)
static uint8_t registers[256],reg_pointer;
static unsigned tx_calls=0,rx_calls=0,write_calls=0;
static uint8_t fail_tx=0,fail_rx=0;
HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef *h,uint16_t addr,uint8_t *data,uint16_t n,uint32_t timeout){
    CHECK(h==&hi2c2 && addr==0xA2 && timeout<=100U && data && n>=1 && n<=8);
    tx_calls++;if(fail_tx)return HAL_TIMEOUT;reg_pointer=data[0];
    if(n>1){CHECK((unsigned)data[0]+n-1<=256);memcpy(registers+data[0],data+1,n-1);write_calls++;}
    return HAL_OK;
}
HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef *h,uint16_t addr,uint8_t *data,uint16_t n,uint32_t timeout){
    CHECK(h==&hi2c2 && addr==0xA3 && timeout<=100U && data && n==7);
    rx_calls++;if(fail_rx)return HAL_TIMEOUT;memcpy(data,registers+reg_pointer,n);return HAL_OK;
}
'''+defs+'\n'+header[a:b]+'\n'
for sig in ['uint8_t PCF8563_DaysInMonth(', 'uint8_t PCF8563_TimeIsValid(', 'uint8_t PCF8563_Weekday(', 'void PCF8563_NormalizeEdit(', 'static uint8_t PCF8563_BCDIsValid(', 'uint8_t DEC_to_BCD(', 'uint8_t BCD_to_DEC(', 'uint8_t PCF8563_SetTime(', 'uint8_t PCF8563_GetTime(']:
    code+=extract(rtc,sig)
code+=r'''
uint8_t Set_Time[6],Set_Time_Select,Set_Time_Select_History,Set_Time_Error;
static unsigned beeps=0,page_id=99;
#define page_menu 3
void Beep_Ctrl(unsigned ms,unsigned n){CHECK(ms==100);beeps=n;}
void set_PageId(unsigned p){page_id=p;}
#define FONT_8X16 1
#define FONT_6X8 2
static char pixels[8][128],last_text[80];
static uint8_t lines[6];
void OLED_PrintString(unsigned x,unsigned y,char *text,unsigned font){
    unsigned width=font==FONT_8X16?8U:6U,i;
    CHECK(y+(font==FONT_8X16?2U:1U)<=8 && x+strlen(text)*width<=128);
    for(i=0;i<strlen(text);i++)pixels[y][x+i*width]=text[i];
    snprintf(last_text,sizeof(last_text),"%s",text);
}
void OLED_PrintLine(unsigned a,unsigned b,unsigned y,uint8_t data){
    unsigned index;
    if (y<2){CHECK(a==64 && b==128 && data==0);return;}
    CHECK(a<b && b<=128 && (y==4 || y==7));
    index=(y==4?0U:3U)+(a==20?0U:a==60?1U:2U);lines[index]=data;
}
static const char power_0_ico[16]={0},power_1_ico[16]={1},power_2_ico[16]={2},power_3_ico[16]={3},power_4_ico[16]={4},power_n_ico[16]={255};
static uint8_t last_icon=99, adc_ok=1;
static unsigned samples=0,draws=0,low_beeps=0;
static float fake_volts=8.10f;
int8_t left_And_Righe_Status_History=1;
#define page_size 20
#define page_logo 0
static unsigned Page_Id_History;
enum { BATTERY_ADC_WAIT, BATTERY_ADC_VALID, BATTERY_ADC_FAILED };
uint8_t ADC_Battery_Step(float *volts){samples++;*volts=fake_volts;return adc_ok ? BATTERY_ADC_VALID : BATTERY_ADC_FAILED;}
void Beep_Low_Battery(void){low_beeps++;}
void OLED_ShowImage(unsigned x,unsigned y,unsigned w,unsigned h,const uint8_t *bitmap){
    CHECK(y==0);
    if(x==64) {CHECK(w==64 && h==16);last_icon=5;}
    else {CHECK(x==105 && w==16 && h==8);last_icon=bitmap[0];}
    draws++;
}
'''
adc=read('STM32L151/Software/src/adc_code.c')
adc_header=read('STM32L151/Software/inc/adc_head.h')
code+='\n'.join(x for x in adc_header.splitlines() if x.startswith('#define BATTERY_'))+'\n'
code+='\n'.join(x for x in adc.splitlines() if x.startswith(('#define VREFINT_', '#define ADC_MAX_CODE')))+'\n'
code+=extract(adc,'static float Battery_VDDA(')
keys=read('STM32L151/Software/src/key_code.c')
code+="""
#define Beep_Ctrl_GPIO_Port 1U
#define Beep_Ctrl_Pin 2U
#define GPIO_PIN_SET 1U
#define GPIO_PIN_RESET 0U
static unsigned beep_pin;
static void HAL_GPIO_WritePin(unsigned port,unsigned pin,unsigned value)
{ CHECK(port==1U && pin==2U); beep_pin=value; }
"""
code+=keys[keys.index('static uint32_t Beep_Last_Time'):keys.index('void Key_Fun_Select')].replace('Beep_Low_Battery(', 'Test_Low_Battery(').replace('void Beep_Ctrl(', 'void Test_Beep_Ctrl(')
code+=extract(adc,'uint8_t Battery_Level_For_Voltage(')
a=oled.index('static uint8_t Battery_Level ='); b=oled.index('/*************************',a)
code+=oled[a:b]
for sig in ['void Time_Set_Page_Init(', 'void Time_Set_Page_Reflash(']:code+=extract(oled,sig)
for sig in ['void Reduce_Time_For_Time_Select(', 'void InCrease_Time_For_Time_Select(', 'void Change_Time_Select_Id(', 'void Change_Time_For_Set_Time(']:code+=extract(key,sig)
oracles=[]
for y in range(100):
    for month in range(1,13):
        start=datetime.date(2000+y,month,1)
        end=datetime.date(2000+y+(month==12),1 if month==12 else month+1,1)
        oracles.append('{%d,%d}'%((end-start).days,(start.weekday()+1)%7))
code+='static const uint8_t calendar_oracle[1200][2]={'+','.join(oracles)+'};\n'
code+=r'''
static void edit(uint8_t y,uint8_t mo,uint8_t day,uint8_t h,uint8_t mi,uint8_t sec){
    Set_Time[0]=y;Set_Time[1]=mo;Set_Time[2]=day;Set_Time[3]=h;Set_Time[4]=mi;Set_Time[5]=sec;
}
static void test_calendar(void){
    unsigned y,m,d,index;uint8_t expected;
    for(y=0;y<100;y++)for(m=1;m<=12;m++){
        index=y*12U+m-1U;CHECK(PCF8563_DaysInMonth((uint8_t)y,(uint8_t)m,0)==calendar_oracle[index][0]);
        for(d=1;d<=calendar_oracle[index][0];d++){
            expected=(uint8_t)((calendar_oracle[index][1]+d-1)%7);
            CHECK(PCF8563_Weekday((uint8_t)y,(uint8_t)m,(uint8_t)d)==expected);
            edit((uint8_t)y,(uint8_t)m,(uint8_t)d,23,59,59);Set_Time_Select=2;
            InCrease_Time_For_Time_Select();CHECK(Set_Time[2]==(d==calendar_oracle[index][0]?1:d+1));
            Reduce_Time_For_Time_Select();CHECK(Set_Time[2]==d);
        }
    }
    CHECK(PCF8563_DaysInMonth(0,2,0)==29 && PCF8563_DaysInMonth(0,2,1)==28);
    CHECK(PCF8563_DaysInMonth(26,0,0)==0 && PCF8563_DaysInMonth(26,13,0)==0);
    edit(24,2,29,0,0,0);Set_Time_Select=0;InCrease_Time_For_Time_Select();CHECK(Set_Time[0]==25 && Set_Time[2]==28);
    edit(26,1,31,0,0,0);Set_Time_Select=1;InCrease_Time_For_Time_Select();CHECK(Set_Time[1]==2 && Set_Time[2]==28);
    edit(26,12,31,0,0,0);Set_Time_Select=1;InCrease_Time_For_Time_Select();CHECK(Set_Time[1]==1);Reduce_Time_For_Time_Select();CHECK(Set_Time[1]==12);
    edit(99,1,1,0,0,0);Set_Time_Select=0;InCrease_Time_For_Time_Select();CHECK(Set_Time[0]==0);Reduce_Time_For_Time_Select();CHECK(Set_Time[0]==99);
    for(index=0;index<6;index++)for(d=0;d<256;d++){
        PCF8563_TimeTypeDef t={0};
        edit(26,11,24,12,30,15);Set_Time[index]=(uint8_t)d;Set_Time_Select=(uint8_t)index;
        InCrease_Time_For_Time_Select();Reduce_Time_For_Time_Select();
        t.year=Set_Time[0];t.month=Set_Time[1];t.day=Set_Time[2];t.hour=Set_Time[3];t.minute=Set_Time[4];t.second=Set_Time[5];
        CHECK(PCF8563_TimeIsValid(&t));
    }
    Set_Time_Select=255;InCrease_Time_For_Time_Select();CHECK(Set_Time_Select==0);
    Set_Time_Select=255;Change_Time_Select_Id();CHECK(Set_Time_Select==0);
    Set_Time_Select=5;Change_Time_Select_Id();CHECK(Set_Time_Select==0);
    puts("CG39 calendar: PASS; 2000..2099 date/weekday oracle, all field-byte inputs, wrap, leap-year and month clamping");
}
static void good_rtc(void){
    PCF8563_TimeTypeDef t={26,11,24,2,12,30,15,0};fail_tx=fail_rx=0;
    CHECK(PCF8563_SetTime(&hi2c2,&t)==HAL_OK);
}
static void bad_read(unsigned offset,uint8_t value){
    PCF8563_TimeTypeDef output,before;good_rtc();registers[2+offset]=value;
    memset(&output,0xA5,sizeof(output));before=output;
    CHECK(PCF8563_GetTime(&hi2c2,&output)!=HAL_OK && memcmp(&output,&before,sizeof(output))==0);
}
static void test_rtc_and_editor(void){
    PCF8563_TimeTypeDef t,backup;unsigned before,i;
    good_rtc();Time_Set_Page_Init();CHECK(Set_Time[0]==26 && Set_Time[1]==11 && Set_Time[2]==24 && Set_Time_Error==0);
    edit(26,11,24,22,58,59);page_id=99;Change_Time_For_Set_Time();
    CHECK(page_id==page_menu && Set_Time_Error==0 && beeps==1);
    CHECK(registers[2]==0x59 && registers[3]==0x58 && registers[4]==0x22 && registers[5]==0x24);
    CHECK(registers[6]==2 && registers[7]==0x11 && registers[8]==0x26);
    CHECK(PCF8563_GetTime(&hi2c2,&t)==HAL_OK && t.weekday==2 && t.century==0);
    before=write_calls;edit(26,2,29,0,0,0);Change_Time_For_Set_Time();CHECK(Set_Time_Error==2 && write_calls==before);
    edit(26,11,24,12,30,15);fail_tx=1;page_id=99;Change_Time_For_Set_Time();
    CHECK(Set_Time_Error==3 && beeps==3 && page_id==99);fail_tx=0;
    Change_Time_For_Set_Time();CHECK(Set_Time_Error==0 && page_id==page_menu);
    bad_read(0,0x80);bad_read(1,0x1A);bad_read(2,0x24);bad_read(3,0);bad_read(4,7);bad_read(5,0);bad_read(6,0xFF);
    good_rtc();registers[5]=0x31;registers[7]=0x04;CHECK(PCF8563_GetTime(&hi2c2,&t)!=HAL_OK);
    good_rtc();fail_rx=1;memset(&t,0xA5,sizeof(t));backup=t;
    CHECK(PCF8563_GetTime(&hi2c2,&t)!=HAL_OK && memcmp(&t,&backup,sizeof(t))==0);
    Time_Set_Page_Init();CHECK(Set_Time_Error==1 && Set_Time[0]==0 && Set_Time[1]==1 && Set_Time[2]==1);
    Time_Set_Page_Reflash();CHECK(strstr(last_text,"RTC READ ERROR")!=0);
    InCrease_Time_For_Time_Select();CHECK(Set_Time_Error==0);
    Measurement_Time_Refresh();CHECK(strcmp(last_text,"--/--/-- --:--")==0);fail_rx=0;
    good_rtc();tick+=1000U;Measurement_Time_Refresh();CHECK(strcmp(last_text,"26/11/24 12:30")==0);
    registers[7]|=0x80;Time_Set_Page_Init();CHECK(Set_Time_Error==1);good_rtc();
    edit(26,11,24,12,30,15);Set_Time_Select=255;Set_Time_Select_History=255;
    for(i=0;i<6;i++)pixels[i<3?2:5][(i%3)*40+36]='9';
    Time_Set_Page_Reflash();CHECK(Set_Time_Select==0);
    for(i=0;i<6;i++)CHECK(pixels[i<3?2:5][(i%3)*40+36]==' ');
    for(i=0;i<6;i++){
        unsigned j;Set_Time_Select=(uint8_t)i;Set_Time_Select_History=255;Time_Set_Page_Reflash();
        for(j=0;j<6;j++)CHECK(lines[j]==(i==j?1:0));
    }
    Set_Time[0]=255;Set_Time[1]=255;Set_Time[2]=255;Time_Set_Page_Reflash();CHECK(Set_Time[0]<=99 && Set_Time[1]==1 && Set_Time[2]==31);
    t=(PCF8563_TimeTypeDef){26,0,1,0,0,0,0,0};before=tx_calls;CHECK(PCF8563_SetTime(&hi2c2,&t)!=HAL_OK && tx_calls==before);
    CHECK(PCF8563_GetTime(&hi2c2,0)!=HAL_OK && PCF8563_SetTime(&hi2c2,0)!=HAL_OK);
    puts("CG39 RTC/editor: PASS; register encoding, invalid BCD/date/VL, I2C failures, complete save fields, error/retry, 999 suffix erase, selection and OLED bounds");
}
static void test_battery(void){
    unsigned read_before, draw_before, beeps_before, elapsed, pulses=0, previous=0;
    uint8_t fallback=99;
    CHECK(Battery_VDDA(1500.0f,1650,&fallback)==3.3f && !fallback);
    CHECK(Battery_VDDA(0.0f,1650,&fallback)==0.0f);
    CHECK(Battery_VDDA(4095.0f,1650,&fallback)==0.0f);
    CHECK(Battery_VDDA(1500.0f,0xFFFF,&fallback)>3.3f && fallback);
    CHECK(Battery_VDDA(1500.0f,0,&fallback)>3.3f && fallback);
    tick=UINT32_MAX-10U; fake_volts=7.19f;
    Measurement_Power_Session();
    Measurement_Battery_Service();
    CHECK(samples==0 && low_beeps==0);
    tick+=25; Measurement_Battery_Service(); Measurement_Battery_Refresh();
    CHECK(samples==1 && last_icon==5 && low_beeps==1);
    tick+=349; Measurement_Battery_Refresh(); CHECK(last_icon==5);
    tick++; Measurement_Battery_Refresh(); CHECK(last_icon==0);
    tick+=350; Measurement_Battery_Refresh(); CHECK(last_icon==5);
    tick+=350; Measurement_Battery_Refresh(); CHECK(last_icon==0);
    tick+=350; Measurement_Battery_Refresh();
    CHECK(Battery_Warned==2 && low_beeps==1);
    fake_volts=7.95f; tick+=5000; Measurement_Battery_Service();
    Measurement_Battery_Refresh(); CHECK(last_icon==3);
    fake_volts=8.01f; tick+=5000; Measurement_Battery_Service();
    Measurement_Battery_Refresh(); CHECK(last_icon==4);
    fake_volts=7.91f; tick+=5000; Measurement_Battery_Service();
    Measurement_Battery_Refresh(); CHECK(last_icon==4);
    fake_volts=7.10f; tick+=5000; Measurement_Battery_Service();
    Measurement_Battery_Refresh(); CHECK(last_icon==0 && low_beeps==1);
    adc_ok=0; fake_volts=8.2f; tick+=5000; Measurement_Battery_Service();
    Measurement_Battery_Refresh(); CHECK(last_icon==0 && low_beeps==1);
    adc_ok=1;
    draw_before=draws; Battery_Force_Redraw=1; Measurement_Battery_Refresh();
    CHECK(draws==draw_before+1);
    beeps_before=low_beeps; Measurement_Power_Session();
    fake_volts=7.0f; tick+=25; Measurement_Battery_Service(); Measurement_Battery_Refresh();
    CHECK(last_icon==5 && low_beeps==beeps_before+1);
    tick+=1400; Measurement_Battery_Refresh();
    read_before=rx_calls; Clock_Force_Redraw=1;
    Measurement_Time_Refresh(); Measurement_Time_Refresh();
    CHECK(rx_calls==read_before+1);
    tick+=999; Measurement_Time_Refresh(); CHECK(rx_calls==read_before+1);
    tick++; Measurement_Time_Refresh(); CHECK(rx_calls==read_before+2);
    tick=UINT32_MAX-50U;
    Test_Low_Battery();
    for(elapsed=0;elapsed<400U;++elapsed,++tick){
        Test_Beep_Ctrl(100U,0U);
        Beep_Reflash();
        if(beep_pin && !previous) pulses++;
        CHECK(beep_pin==(elapsed<360U && elapsed%120U<60U));
        previous=beep_pin;
    }
    CHECK(pulses==3);
    puts("CG41 battery/RTC: PASS once per power cycle, two flashes, recovery/retrigger rejection, ADC failure, hysteresis, time throttling");
}
int main(void){test_calendar();test_rtc_and_editor();test_battery();printf("CG39 UI tests: PASS %u checks\n",checks);return 0;}
'''
with tempfile.TemporaryDirectory(prefix='cg39_') as tmp:
    d=Path(tmp);source=d/'test.c';exe=d/('test.exe' if os.name=='nt' else 'test')
    source.write_text(code,encoding='utf-8')
    cmd=[os.environ.get('CC','cc'),'-std=c99','-O1','-Wall','-Wextra','-Werror','-fstack-protector-all','-fno-omit-frame-pointer']
    if os.environ.get('HOST_SANITIZERS','1')!='0':cmd+=['-fsanitize=undefined']
    subprocess.run(cmd+[str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
