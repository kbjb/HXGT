#include <stdio.h>
#include <assert.h>
#include "adc161_head.h"
static uint16_t raw_bits;
static unsigned read_idx, clocks;
static uint8_t cs_level, clk_level;
static Gpio cs_pin, clk_pin;
static void assert_idle(void) { assert(cs_level == Bit_SET && clk_level == Bit_SET); }
static void write_gpio(Gpio *x,uint8_t v) {
    if (x == &cs_pin) cs_level = v;
    else if (x == &clk_pin) clk_level = v;
    else assert(0);
    clocks++;
}
static uint8_t read_gpio(Gpio *x) {
    (void)x;
    assert(cs_level == Bit_RESET && clk_level == Bit_SET);
    assert(read_idx < 16U);
    return (raw_bits >> (15U-read_idx++)) & 1U;
}
static void init_gpio(Gpio *x) {
    if (x == &cs_pin) cs_level = Bit_RESET;
    if (x == &clk_pin) clk_level = Bit_RESET;
}
static Gpio cs_pin={write_gpio,0,init_gpio};
static Gpio clk_pin={write_gpio,0,init_gpio};
static Gpio out_pin={0,read_gpio,init_gpio};
int main(void) {
    ADC161_STR f={&cs_pin,&clk_pin,&out_pin,0,0,0,0};
    const uint16_t raw[] = {0U,1U,0x7FFFU,0x8000U,0xFFFFU,0xBFA0U,0x4000U};
    const int16_t expected[] = {0,1,32767,-32768,-1,-16480,16384};
    ADC161_STR_INIT(&f);
    assert_idle();
    for (unsigned i=0;i<sizeof(raw)/sizeof(raw[0]);i++) {
        raw_bits=raw[i];read_idx=0U;clocks=0U;
        ADC161_READ(&f);
        assert(read_idx==16U && clocks==38U);
        assert_idle();
        assert(f.ad_data==expected[i]);
        assert(uint16_twos_complement_to_int16_original(raw[i])==expected[i]);
    }
    puts("CG32 ADC161 bitstream: PASS (7 full-sign cases, 18 clocks, idle pins)");
    return 0;
}
