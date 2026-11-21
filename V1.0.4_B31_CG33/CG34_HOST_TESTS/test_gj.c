#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "algorithm_head.h"

/* Satisfy the unchanged cant and ADC routines in the production file. */
CG_Calibration_Param_Str CG_Param;
GJ_Calibration_Param_Str GJ_Param;
int8_t left_And_Righe_Status = 1;
float POWER_Vol;
float ADC_GetVrefintVoltage(void) { return 3.3f; }

/* Compute the original, uncentered normal equations in double precision.
 * Integer terms here are exact in double, including the subtraction. */
static double gauge_reference(uint16_t input)
{
    double a = GJ_Param.GJ_1410_Ad, b = GJ_Param.GJ_1435_Ad;
    double c = GJ_Param.GJ_1470_Ad, sum_x = a + b + c;
    double sum_y = 4315.0;
    double sum_xy = 1410.0*a + 1435.0*b + 1470.0*c;
    double slope = (3.0*sum_xy - sum_x*sum_y) /
        (3.0*(a*a + b*b + c*c) - sum_x*sum_x);
    return slope * input + (sum_y - slope * sum_x) / 3.0;
}

static void check(uint16_t a, uint16_t b, uint16_t c, uint16_t sample)
{
    float result;
    double expected;
    GJ_Param = (GJ_Calibration_Param_Str){a, b, c, 20.0f};
    expected = gauge_reference(sample);
    result = Get_GJ_Value(sample, -15.0f);
    assert(GJ_Result_Is_Valid());
    assert(fabs((double)result - expected) <= 0.001 + fabs(expected)*3e-7);
    assert(Get_GJ_Value(sample, 45.0f) == result); /* no invented drift term */
}

static void invalid(uint16_t a, uint16_t b, uint16_t c)
{
    GJ_Param = (GJ_Calibration_Param_Str){a, b, c, 20.0f};
    assert(Get_GJ_Value(16000, 20.0f) == 0.0f);
    assert(!GJ_Result_Is_Valid());
}

int main(void)
{
    uint32_t seed = 20261120U;
    uint16_t i;
    assert(!GJ_Result_Is_Valid());
    check(7781, 15946, 27395, 15946);    /* factory */
    check(27395, 15946, 7781, 15946);    /* reversed sensor polarity */
    check(30000, 30001, 30002, 30002);   /* old float denominator was zero */
    check(30002, 30001, 30000, 30000);
    check(65533, 65534, 65535, 65533);
    check(0, 32768, 65535, 65535);       /* wide uint16 range */
    invalid(30000, 30000, 30000);
    invalid(100, 100, 200);
    invalid(100, 200, 100);
    invalid(300, 100, 200);
    check(7781, 15946, 27395, 15946);    /* valid after invalid */
    for (i = 0; i < 10000; ++i) {
        uint16_t a, b, c, sample;
        seed = seed * 1664525U + 1013904223U;
        a = (uint16_t)(seed >> 18);
        seed = seed * 1664525U + 1013904223U;
        b = a + 1 + (uint16_t)(seed >> 18);
        seed = seed * 1664525U + 1013904223U;
        c = b + 1 + (uint16_t)(seed >> 18);
        seed = seed * 1664525U + 1013904223U;
        sample = (uint16_t)(seed >> 16);
        if ((i & 1U) == 0U) check(a, b, c, sample);
        else check(c, b, a, sample);
    }
    puts("GJ math: PASS (small spans, both polarities, invalid/recovery, 10,000 regression comparisons)");
    return 0;
}
