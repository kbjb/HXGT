#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "cg_angle_filter.h"
static unsigned checks = 0;
#define CHECK(p) do { assert(p); checks++; } while(0)
static void stabilize(CG_ANGLE_FILTER *f, int16_t v)
{
    unsigned i;
    for (i=0; i<CG_ANGLE_WINDOW_SIZE+5; ++i) CG_AngleFilter_Push(f,v);
}
int main(void)
{
    CG_ANGLE_FILTER f;
    CG_AngleFilter_Init(&f);
    CHECK(CG_AngleFilter_Push(&f,1500)==1500); /* never average 29 zeroes */
    stabilize(&f,1500); CHECK(f.output_value==1500);
    stabilize(&f,1500 + CG_ANGLE_DEADBAND_AD - 1); CHECK(f.output_value==1500); /* Below threshold. */
    stabilize(&f,1500 + CG_ANGLE_DEADBAND_AD); CHECK(f.output_value==1500 + CG_ANGLE_DEADBAND_AD); /* At threshold. */
    CG_AngleFilter_Init(&f);
    stabilize(&f,1500);
    CG_AngleFilter_Push(&f,32767); /* one gross spike is trimmed */
    CHECK(f.output_value==1500);
    CG_AngleFilter_Push(&f,-32768); /* another gross spike is trimmed */
    CHECK(f.output_value==1500);
    CG_AngleFilter_Init(&f);
    stabilize(&f,-32768); CHECK(f.filtered_value==-32768);
    stabilize(&f,32767); CHECK(f.filtered_value==32767);
    stabilize(&f,-12445); CHECK(f.output_value==-12445);
    stabilize(&f,12513); CHECK(f.output_value==12513);
    stabilize(&f,-12445); CHECK(f.output_value==-12445);
    CG_AngleFilter_Init(&f);
    stabilize(&f,600);
    for (int v=601;v<=630;v++) {
        stabilize(&f,(int16_t)v);
        CHECK(f.output_value<=v && v-f.output_value<CG_ANGLE_DEADBAND_AD);
    }
    /* A stationary alternating +/-10 AD disturbance averages to zero. */
    CG_AngleFilter_Init(&f);
    for (unsigned i=0; i<200; ++i) {
        CG_AngleFilter_Push(&f, (int16_t)(1000 + ((i&1) ? 10 : -10)));
        if (i>=CG_ANGLE_WINDOW_SIZE) CHECK(f.filtered_value==1000);
    }
    /* An ideal step fully replaces the 800 ms window in at most 40 samples. */
    for (unsigned i=0; i<CG_ANGLE_WINDOW_SIZE; ++i) CG_AngleFilter_Push(&f,12000);
    CHECK(f.filtered_value==12000 && f.output_value==12000);
    printf("CG32 filter: PASS (%u checks, -32768..32767, %ld AD, spike/return)\n", checks, CG_ANGLE_DEADBAND_AD);
    return 0;
}
