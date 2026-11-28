#!/usr/bin/env python3
"""Compile the actual LM95071 driver with GPIO mocks at O0/O2/O3."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'QINGJIAOBAN/CLASS/SCR/lm9507_code.c').read_bytes().decode('gb18030')
header = r'''
#include <stdint.h>
typedef struct GPIO_STR GPIO_STR;
struct GPIO_STR {
    void (*str_init)(GPIO_STR *);
    void (*write)(GPIO_STR *, uint8_t);
    uint8_t (*read)(GPIO_STR *);
};
typedef struct LM9507_CLASS_STR LM9507_STR;
struct LM9507_CLASS_STR {
    GPIO_STR *clk, *cs, *mosi, *miso;
    void (*init)(LM9507_STR *);
    void (*write)(LM9507_STR *,uint16_t);
    uint16_t (*read)(LM9507_STR *);
    float (*readTemp)(LM9507_STR *);
    void (*shoutDown)(LM9507_STR *);
    void (*str_init)(LM9507_STR *);
};
#define Bit_SET 1U
#define Bit_RESET 0U
void host_nop(void);
#define __NOP() host_nop()
void LM9507_DELAY(uint32_t time);
void LM9507_STR_INIT(LM9507_STR *);
'''
test = r'''
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lm9507_head.h"
static unsigned checks,nops,initializations;
static unsigned cs_low,cs_high,rises,falls,reads,writes;
static uint8_t cs=1,clock_level;
static uint16_t incoming,outgoing;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"LM95071 failure at line %d: %s\n",__LINE__,#x); abort(); \
} } while (0)
void host_nop(void) { ++nops; }
static void pin_init(GPIO_STR *pin) { (void)pin; ++initializations; }
static void clock_write(GPIO_STR *pin,uint8_t value) {
    (void)pin;
    CHECK(cs==0);
    if (value) { CHECK(clock_level==0); ++rises; }
    else { CHECK(clock_level==1); ++falls; }
    clock_level=value;
}
static void cs_write(GPIO_STR *pin,uint8_t value) {
    (void)pin;
    CHECK(value!=cs);
    cs=value;
    if (value) ++cs_high; else ++cs_low;
}
static void data_write(GPIO_STR *pin,uint8_t value) {
    (void)pin; CHECK(cs==0 && clock_level==0);
    outgoing=(uint16_t)((outgoing<<1)|value); ++writes;
}
static uint8_t data_read(GPIO_STR *pin) {
    uint8_t value;
    (void)pin; CHECK(cs==0 && clock_level==1 && reads<16);
    value=(uint8_t)((incoming>>(15U-reads))&1U); ++reads;
    return value;
}
static GPIO_STR clk_pin={pin_init,clock_write,0};
static GPIO_STR cs_pin={pin_init,cs_write,0};
static GPIO_STR mosi_pin={pin_init,data_write,0};
static GPIO_STR miso_pin={pin_init,0,data_read};
static LM9507_STR sensor={&clk_pin,&cs_pin,&mosi_pin,&miso_pin,0,0,0,0,0,LM9507_STR_INIT};
static void reset_transaction(void) {
    CHECK(cs==1 && clock_level==0);
    cs_low=cs_high=rises=falls=reads=writes=nops=0; outgoing=0;
}
int main(void) {
    const uint32_t delays[]={0,1,10,10003};
    unsigned i,tail;
    int count;
    for (i=0;i<sizeof(delays)/sizeof(delays[0]);++i) {
        nops=0; LM9507_DELAY(delays[i]); CHECK(nops==delays[i]);
    }
    sensor.str_init(&sensor); CHECK(initializations==4);
    CHECK(sensor.init && sensor.write && sensor.read && sensor.readTemp && sensor.shoutDown);
    for (count=-1280;count<=3200;++count) for (tail=0;tail<4;++tail) {
        float result;
        reset_transaction();
        incoming=(uint16_t)((uint16_t)(count*4)|tail);
        result=sensor.readTemp(&sensor);
        CHECK(fabsf(result-count*0.03125f)<0.000001f);
        CHECK(cs_low==1 && cs_high==1 && cs==1);
        CHECK(rises==16 && falls==16 && reads==16 && writes==0);
        CHECK(nops==160);
    }
    reset_transaction();
    cs_pin.write(&cs_pin,0); sensor.write(&sensor,0xA55A); cs_pin.write(&cs_pin,1);
    CHECK(outgoing==0xA55A && writes==16 && reads==0);
    CHECK(rises==16 && falls==16 && nops==320);
    reset_transaction(); sensor.shoutDown(&sensor);
    CHECK(cs_low==1 && cs_high==1 && outgoing==0x00FF && writes==16 && nops==320);
    printf("CG40 LM95071: PASS %u checks; delay count, 17924 signed temperatures/tails, CS/clock/data order and write/shutdown\n",checks);
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='cg40_temperature_') as temp:
    d = Path(temp)
    (d / 'lm9507_head.h').write_text(header, encoding='utf-8')
    (d / 'lm9507_code.c').write_text(source, encoding='utf-8')
    (d / 'test.c').write_text(test, encoding='utf-8')
    for level in ['-O0', '-O2', '-O3']:
        exe=d/('test'+('.exe' if os.name=='nt' else ''))
        cmd=[os.environ.get('CC','cc'),'-std=c99',level,'-Wall','-Wextra','-Werror',
             '-fstack-protector-all','-fno-omit-frame-pointer','-I',str(d)]
        if os.environ.get('HOST_SANITIZERS','1')!='0': cmd+=['-fsanitize=undefined']
        subprocess.run(cmd+[str(d/'lm9507_code.c'),str(d/'test.c'),'-o',str(exe)],check=True)
        print('Compiler optimization: '+level,flush=True)
        subprocess.run([str(exe)],check=True,env={**os.environ,'UBSAN_OPTIONS':'halt_on_error=1'})
