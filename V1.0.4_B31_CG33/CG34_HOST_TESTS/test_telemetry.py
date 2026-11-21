#!/usr/bin/env python3
"""Test actual foreground telemetry guard and unchanged valid wire record."""
from pathlib import Path
import os
import subprocess
import tempfile

p = Path(__file__).resolve().parents[1] / "STM32L151/Core/Src/main.c"
c = p.read_text(encoding="gb18030")
part = c[c.index("      if(U2_PC_Work_Mode == 1)"):c.index("      Storage_Key_Check_Scn();")]
stub = r'''
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define CG_RESULT_OK 0
#define EEP_STATE_B3_VALID 1
#define UART2_ID 1
#define page_main 0
static uint32_t tick;
static uint32_t U2_Cycle_Send_Mode_Time_Current,U2_Cycle_Send_Mode_Time_History;
static unsigned U2_PC_Work_Mode=1,page,fresh=1,eep=1,gj_valid=1,cg_ok=1,calls;
static unsigned GJ_Ad,CG_Ad,CG_Result_Status;
static float Temp=20.0f;
static char sent[96];
static unsigned get_PageId(void){return page;}
static unsigned Sensor_Data_Is_Fresh(void){return fresh;}
static unsigned Eep_Get_Storage_State(void){return eep;}
static unsigned GJ_Result_Is_Valid(void){return gj_valid;}
static uint32_t HAL_GetTick(void){return tick;}
static float Get_GJ_Value(unsigned x,float t){(void)x;(void)t;return 1435.0f;}
static float Get_CG_Value(unsigned x,float t){(void)x;(void)t;CG_Result_Status=!cg_ok;return -22.5f;}
static void UART_Send_Data(unsigned u,uint8_t *d,uint16_t len)
{assert(u==UART2_ID && len<sizeof(sent));memcpy(sent,d,len);sent[len]=0;calls++;}
static void run(void) {
'''
tail = r'''
}
int main(void){
    tick=1000; fresh=0; run(); assert(calls==0);
    tick+=1000; fresh=1; eep=0; run(); assert(calls==0);
    tick+=1000; eep=1; gj_valid=0; run(); assert(calls==0);
    tick+=1000; gj_valid=1; cg_ok=0; run(); assert(calls==0);
    tick+=1000; cg_ok=1; page=1; run(); assert(calls==0);
    tick+=1000; page=page_main; run(); assert(calls==1);
    assert(strcmp(sent,"Type:0|Data1:1435.00|Data2:-22.50|Temp:20.00\r\n")==0);
    run(); assert(calls==1);
    puts("CG34 telemetry: PASS; stale, invalid calibration and unavailable CH blocked; valid wire record unchanged");
}
'''
with tempfile.TemporaryDirectory(prefix="cg34_telemetry_") as td:
    src, exe = Path(td) / "test.c", Path(td) / "test"
    src.write_text(stub+part+tail, encoding="utf-8")
    subprocess.run(["cc", "-std=c99", "-O1", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=undefined", str(src), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True, env={**os.environ,"UBSAN_OPTIONS":"halt_on_error=1"})
