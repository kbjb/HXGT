#include <stdio.h>
#include <assert.h>
#include "adc161_head.h"
static uint16_t raw_bits;
static unsigned read_idx, clocks;
static void write_gpio(Gpio *x,uint8_t v) { (void)x; (void)v; clocks++; }
static uint8_t read_gpio(Gpio *x) {
    (void)x;
    assert(read_idx < 16U);
    return (raw_bits >> (15U-read_idx++)) & 1U;
}
static void init_gpio(Gpio *x) { (void)x; }
static Gpio gpio={write_gpio,read_gpio,init_gpio};
int main(void) {
    ADC161_STR f={&gpio,&gpio,&gpio,0,0,0,0};
    const uint16_t raw[] = {0U,1U,0x7FFFU,0x8000U,0xFFFFU,0xBFA0U,0x4000U};
    const int16_t expected[] = {0,1,32767,-32768,-1,-16480,16384};
    for (unsigned i=0;i<sizeof(raw)/sizeof(raw[0]);i++) {
        raw_bits=raw[i];read_idx=0U;clocks=0U;
        ADC161_READ(&f);
        assert(read_idx==16U && clocks==38U);
        assert(f.ad_data==expected[i]);
        assert(uint16_twos_complement_to_int16_original(raw[i])==expected[i]);
    }
    puts("CG32 ADC161 bitstream: PASS (7 full-sign cases, 18 clocks)");
    return 0;
}
