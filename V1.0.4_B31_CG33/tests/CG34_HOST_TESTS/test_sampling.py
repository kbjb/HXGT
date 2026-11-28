from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'QINGJIAOBAN/USER/main.c').read_bytes().decode('latin1')
begin = source.index('        elapsed = (uint32_t)(now - sample_tick);')
end = source.index('        /* Displacement and temperature', begin)
sample_step = source[begin:end]
test = r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#include "cg_angle_filter.h"
#define SENSOR_SAMPLE_PERIOD_MS 20U
static uint32_t sensor_tick_ms, sample_tick, last_sample_tick;
static uint32_t CG_Sample_Interval_Ms, CG_Sample_Overrun_Count;
static CG_ANGLE_FILTER CG_Angle_Filter;
static int16_t adc161_aver;
static unsigned reads;
struct FakeADC { int16_t ad_data; uint8_t (*read)(struct FakeADC *); };
static uint8_t spi_ok = 1U;
static uint8_t read_adc(struct FakeADC *adc) { reads++; adc->ad_data=1234; return spi_ok; }
static struct FakeADC adc161_str = {0, read_adc};
static struct { int16_t realValue, StableValue, PreTempValue;
    uint8_t tempCnt, flag; int32_t tempSumValue; } AngleParam;
static void step(void) {
    uint32_t now=sensor_tick_ms, elapsed;
''' + sample_step + r'''
}
int main(void) {
    unsigned i;
    CG_AngleFilter_Init(&CG_Angle_Filter);
    for (i=0;i<20;i++) { sensor_tick_ms=i; step(); }
    assert(reads==0);
    for (i=20;i<=1000;i++) { sensor_tick_ms=i; step(); }
    assert(reads==50 && CG_Angle_Filter.count==40 && adc161_aver==1234);
    assert(CG_Sample_Overrun_Count==0 && CG_Sample_Interval_Ms==20);
    sensor_tick_ms=1061; step();
    assert(reads==51 && CG_Sample_Overrun_Count==1 && CG_Angle_Filter.count==1);
    step(); assert(reads==51); /* No burst of repeated catch-up samples. */
    sample_tick=UINT32_MAX-9U; last_sample_tick=sample_tick; sensor_tick_ms=10U;
    step(); assert(reads==52 && CG_Sample_Interval_Ms==20);
    assert(AngleParam.tempSumValue==0 && AngleParam.tempCnt==0);
    spi_ok=0U; sensor_tick_ms+=20U; step();
    assert(reads==53 && CG_Angle_Filter.count==2 && adc161_aver==1234);
    puts("CG34 sampling: PASS; 50 Hz, missed-slot recovery, no catch-up burst, tick wrap");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cg34_timing_') as temp:
    c = Path(temp)/'timing.c'
    exe = Path(temp)/'timing.exe'
    c.write_text(test, encoding='utf-8')
    subprocess.run([os.environ.get('CC','cc'),'-std=c99','-O1','-Wall','-Wextra','-Werror','-I',str(root/'QINGJIAOBAN/USER'),str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
