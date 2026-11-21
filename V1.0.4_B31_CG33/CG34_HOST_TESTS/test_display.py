#!/usr/bin/env python3
"""Compile the production OLED and RTC paths against a fault-injected HAL."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
src = root / "STM32L151/Software/src"
oled = (src / "oled_code.c").read_text(encoding="latin1")
app = (src / "App_code.c").read_text(encoding="latin1")
macros = oled[oled.index("#define OLED_CS_LOW"):oled.index("static uint8_t oled_buffer")]
core = oled[oled.index("static uint8_t oled_buffer"):oled.index("void OLED_PrintChar")]
refresh = oled[oled.index("void OLED_Refresh(void)\n{"):]
rtc = app[app.index("static uint32_t RTC_LastPoll;"):app.index("static int8_t  Power_0_Ico_Show_Timer_Status")]
stub = r'''
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#define OLED_WIDTH 128
#define OLED_PAGE_NUM 8
#define OLED_SPI_HANDLE hspi2
#define OLED_CS_PORT 0
#define OLED_CS_PIN 0
#define OLED_DC_PORT 0
#define OLED_DC_PIN 1
#define OLED_RESET_PORT 0
#define OLED_RESET_PIN 2
#define GPIO_PIN_RESET 0
#define GPIO_PIN_SET 1
#define FONT_6X8 0
typedef enum {HAL_OK, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT} HAL_StatusTypeDef;
typedef struct {uint8_t year, month, day, hour, minute;} PCF8563_TimeTypeDef;
static int hspi2, hi2c2, cs = 1;
static unsigned writes, page_writes, reads, renders, timeout_seen;
static uint32_t tick;
static HAL_StatusTypeDef spi_result, rtc_result;
static char headline[32];
static PCF8563_TimeTypeDef test_time = {26, 11, 20, 20, 45};
static uint32_t HAL_GetTick(void) {return tick;}
static void HAL_Delay(uint32_t ms) {tick += ms;}
static void HAL_GPIO_WritePin(int port,int pin,int level)
{(void)port; if (pin == OLED_CS_PIN) cs=level;}
static HAL_StatusTypeDef HAL_SPI_Transmit(void *h,uint8_t *p,uint16_t n,uint32_t timeout)
{(void)h; (void)p; assert(cs==0); writes++; timeout_seen=timeout;
 if(n==OLED_WIDTH) page_writes++;
 if(spi_result==HAL_TIMEOUT) tick+=timeout;
 return spi_result;}
static HAL_StatusTypeDef PCF8563_GetTime(void *h,PCF8563_TimeTypeDef *t)
{(void)h; reads++; if(rtc_result==HAL_OK) *t=test_time; return rtc_result;}
static void OLED_PrintString(uint8_t x,uint8_t y,char* s,uint8_t f)
{(void)x;(void)y;(void)f; renders++; snprintf(headline,sizeof(headline),"%s",s);}
void OLED_Init(void);
void OLED_Refresh(void);
void OLED_Clear(void);
'''
tests = r'''
int main(void)
{
    unsigned before;
    OLED_Clear(); assert(page_writes==8 && writes==32 && cs==1);
    spi_result=HAL_TIMEOUT; tick=UINT32_MAX-30U;
    OLED_Clear(); assert(timeout_seen==10 && writes==33 && cs==1 && OLED_HasFault());
    before=writes; OLED_WriteData(1); OLED_Clear();
    assert(writes==before && !OLED_TryRecover());
    tick+=99U; assert(!OLED_TryRecover() && writes==before);
    tick+=1U; spi_result=HAL_BUSY;
    assert(!OLED_TryRecover() && OLED_HasFault() && writes==before+1U && cs==1);
    tick+=100U; spi_result=HAL_OK;
    assert(OLED_TryRecover() && !OLED_HasFault() && page_writes==16U);
    tick=UINT32_MAX-10U; RTC_ReflashHeader();
    assert(reads==1 && renders==1 && strcmp(headline,"26/11/20 20:45")==0);
    tick+=999U; RTC_ReflashHeader();
    assert(reads==1 && renders==1);
    tick+=1U; RTC_ReflashHeader(); assert(reads==2 && renders==1);
    RTC_HeaderRedraw=1; RTC_ReflashHeader(); assert(reads==2 && renders==2);
    tick+=1000U; rtc_result=HAL_TIMEOUT; RTC_ReflashHeader();
    assert(reads==3 && renders==3 && strcmp(headline,"RTC ERR       ")==0);
    tick+=1000U; rtc_result=HAL_OK; test_time.month=15; RTC_ReflashHeader();
    assert(reads==4 && renders==3);
    tick+=1000U; test_time.month=12; RTC_ReflashHeader();
    assert(reads==5 && renders==4 && strcmp(headline,"26/12/20 20:45")==0);
    puts("CG34 display: PASS; bounded SPI fault/recovery, CS release, wrap; RTC pacing/failure");
}
'''
with tempfile.TemporaryDirectory(prefix="cg34_display_") as td:
    src_path = Path(td) / "test.c"
    exe = Path(td) / "test"
    src_path.write_text(stub + macros + core + refresh + rtc + tests, encoding="latin1")
    subprocess.run(["cc", "-std=c99", "-O1", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=undefined", str(src_path), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True, env={**os.environ, "UBSAN_OPTIONS":"halt_on_error=1"})
