/* CG40: execute the real Algorithm_code.c against an independent model. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "Algorithm_head.h"

extern float CG_Value_t, TempOffset;
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { \
    fprintf(stderr,"CG40 check failed at line %d: %s\n",__LINE__,#x); \
    abort(); } } while (0)

CG_Calibration_Param_Str CG_Param = {1,-12445,50,12513,20};
GJ_Calibration_Param_Str GJ_Param = {7781,15946,27395,20};
CH_Calibration_Param_Str CH_Param = {10318,10318,20};
int8_t left_And_Righe_Status = 1;
float POWER_Vol = 3.3f, GJ_Value = 0;
float ADC_GetVrefintVoltage(void) { return 3.3f; }

static double reference_raw(int16_t ad)
{
    double middle = ((double)CG_Param.CG_Zero_Left_Ad +
                     (double)CG_Param.CG_Zero_Right_Ad) / 2.0;
    double span;
    if ((double)ad < middle) {
        span = (double)CG_Param.CG_Zero_Left_Ad - CG_Param.CG_Max_Left_Ad;
        return (middle - ad) * 180.0 / span;
    }
    span = (double)CG_Param.CG_Max_Right_Ad - CG_Param.CG_Zero_Right_Ad;
    return -(ad - middle) * 180.0 / span;
}

static void check_case(int16_t zl, int16_t zr, int16_t al, int16_t ar)
{
    int32_t ad;
    double mid = ((double)zl + zr) / 2.0;
    double left_step = 180.0 / ((double)zl - al);
    double right_step = 180.0 / ((double)ar - zr);
    double max_step = fmax(left_step, right_step);
    float previous = 0.0f;
    float output, normal;
    CG_Param = (CG_Calibration_Param_Str){zl,al,zr,ar,23.0f};
    left_And_Righe_Status = 1;
    CG_Data_Init();
    CHECK(CG_Result_Status == CG_RESULT_OK);
    for (ad=INT16_MIN; ad<=INT16_MAX; ++ad) {
        double expected = reference_raw((int16_t)ad);
        output = Get_CG_Value((int16_t)ad,23);
        CHECK(CG_Result_Status == CG_RESULT_OK);
        CHECK(fabs((double)CG_Value_t - expected) < 0.002);
        CHECK(fabs((double)output - expected) <= 0.026);
        CHECK(fabsf(output/0.05f - roundf(output/0.05f)) < 0.01f);
        CHECK(output != 0.0f || !signbit(output));
        if (ad > INT16_MIN) {
            CHECK(CG_Value_t <= previous);
            CHECK((double)previous-CG_Value_t <= max_step+0.002);
        }
        previous = CG_Value_t;
    }

    /* Distinct horizontal captures are measurements, not a zero plateau. */
    Get_CG_Value(zl,23);
    CHECK(fabs((double)CG_Value_t-reference_raw(zl)) < 0.002);
    if (zl != zr) CHECK(CG_Value_t != 0.0f);
    Get_CG_Value(zr,23);
    CHECK(fabs((double)CG_Value_t-reference_raw(zr)) < 0.002);
    if (zl != zr) CHECK(CG_Value_t != 0.0f);
    if (floor(mid) == mid) {
        CHECK(Get_CG_Value((int16_t)mid,23) == 0.0f);
        CHECK(CG_Value_t == 0.0f);
    } else {
        Get_CG_Value((int16_t)floor(mid),23); CHECK(CG_Value_t > 0.0f);
        Get_CG_Value((int16_t)ceil(mid),23); CHECK(CG_Value_t < 0.0f);
    }

    /* A stored endpoint includes the bench's residual tilt. The 180 mm
     * increment is measured from its SAME-orientation zero, not from mid. */
    Get_CG_Value(al,23);
    CHECK(fabs(CG_Value_t - (180.0 + (mid-zl)*left_step)) < 0.002);
    Get_CG_Value(ar,23);
    CHECK(fabs(CG_Value_t - (-180.0 + (mid-zr)*right_step)) < 0.002);
    if (mid+(double)al-zl >= INT16_MIN && floor(mid)==mid) {
        CHECK(fabsf(Get_CG_Value((int16_t)(mid+al-zl),23)-180.0f)<0.001f);
    }
    if (mid+(double)ar-zr <= INT16_MAX && floor(mid)==mid) {
        CHECK(fabsf(Get_CG_Value((int16_t)(mid+ar-zr),23)+180.0f)<0.001f);
    }

    /* Direction and the existing temperature term remain post-processing. */
    for (ad=INT16_MIN; ad<=INT16_MAX; ad+=257) {
        normal=Get_CG_Value((int16_t)ad,23);
        left_And_Righe_Status=-1;
        output=Get_CG_Value((int16_t)ad,23);
        CHECK(output == -normal);
        CHECK(output != 0.0f || !signbit(output));
        left_And_Righe_Status=1;
        output=Get_CG_Value((int16_t)ad,25);
        CHECK(fabsf(TempOffset - (-0.054637f*2.0f)) < 0.000001f);
        CHECK(fabs((double)output-(reference_raw((int16_t)ad)+0.109274))<=0.026);
    }
}

static void check_bench_residual(void)
{
    const int16_t residuals[] = {-700,-115,0,115,700};
    const int16_t heights[] = {-180,-150,-100,-50,0,50,100,150,180};
    unsigned i,j;
    for (i=0;i<sizeof(residuals)/sizeof(residuals[0]);++i) {
        int16_t r=residuals[i];
        /* Synthetic linear sensor: absolute zero=1000, -100 AD/mm.
         * Reversing the bench inclination changes the residual sign. */
        CG_Param=(CG_Calibration_Param_Str){(int16_t)(1000+r),
            (int16_t)(1000+r-18000),(int16_t)(1000-r),
            (int16_t)(1000-r+18000),25};
        CG_Data_Init(); CHECK(CG_Result_Status==CG_RESULT_OK);
        for (j=0;j<sizeof(heights)/sizeof(heights[0]);++j) {
            float h=(float)heights[j];
            CHECK(fabsf(Get_CG_Value((int16_t)(1000-100*heights[j]),25)-h)<0.001f);
        }
    }
    /* Regression: r=115 gives 49.50 mm with CG35's cross-orientation
     * edge/slope; the same synthetic absolute input must now be 50.00. */
    CG_Param=(CG_Calibration_Param_Str){115,-17885,-115,17885,25};
    CG_Data_Init(); CHECK(Get_CG_Value(-5000,25)==50.0f);
}

static void check_invalid(void)
{
    const CG_Calibration_Param_Str bad[] = {
        {0,0,0,10000,20}, {0,-3999,0,10000,20},
        {0,10000,0,10000,20}, {0,-10000,0,3999,20},
        {0,-10000,0,-10000,20}, {751,-10000,-750,10000,20},
        {0,-10000,0,10000,-40.01f}, {0,-10000,0,10000,100.01f},
        {0,-10000,0,10000,NAN}, {0,-10000,0,10000,INFINITY}
    };
    const float bad_t[] = {-40.01f,100.01f,NAN,INFINITY,-INFINITY};
    unsigned i;
    for (i=0;i<sizeof(bad)/sizeof(bad[0]);++i) {
        CG_Param=bad[i]; CG_Data_Init();
        CHECK(CG_Result_Status==CG_RESULT_CAL_ERROR);
        CHECK(Get_CG_Value(-5000,20)==0.0f);
        CHECK(CG_Result_Status==CG_RESULT_CAL_ERROR);
    }
    CG_Param=(CG_Calibration_Param_Str){0,-10000,0,10000,20};
    CG_Data_Init();
    for (i=0;i<sizeof(bad_t)/sizeof(bad_t[0]);++i) {
        CHECK(Get_CG_Value(0,bad_t[i])==0.0f);
        CHECK(CG_Result_Status==CG_RESULT_TEMP_ERROR);
    }
    Get_CG_Value(0,-40); CHECK(CG_Result_Status==CG_RESULT_OK);
    Get_CG_Value(0,100); CHECK(CG_Result_Status==CG_RESULT_OK);
    CG_Param.CG_Max_Left_Ad=1; /* Runtime integrity check remains active. */
    CHECK(Get_CG_Value(-5000,20)==0.0f);
    CHECK(CG_Result_Status==CG_RESULT_CAL_ERROR);
    CG_Param=(CG_Calibration_Param_Str){0,-10000,0,10000,20};
    CG_Data_Init();
    CHECK(Get_CG_Value(INT16_MIN,20)>400.0f);
    CHECK(CG_Result_Status==CG_RESULT_OK);
    CHECK(Get_CG_Value(INT16_MAX,20)<-400.0f);
    CHECK(CG_Result_Status==CG_RESULT_OK);
}

static void check_rounding_and_signed_zero(void)
{
    int ad, direction;
    char text[32];
    /* Exactly one display step per two counts; +/-half rounds away from 0. */
    CG_Param=(CG_Calibration_Param_Str){0,-7200,0,7200,20};
    CG_Data_Init();
    CHECK(Get_CG_Value(-1,20)==0.05f);
    CHECK(Get_CG_Value(1,20)==-0.05f);
    /* Check tiny positive/negative results and exact 0 in both directions. */
    CG_Param=(CG_Calibration_Param_Str){0,-18000,0,18000,20};
    CG_Data_Init();
    for (direction=-1;direction<=1;direction+=2) {
        left_And_Righe_Status=(int8_t)direction;
        for (ad=-2;ad<=2;++ad) {
            float result=Get_CG_Value((int16_t)ad,20);
            CHECK(result==0.0f && !signbit(result));
            snprintf(text,sizeof(text),"%+07.2f",result);
            CHECK(strcmp(text,"+000.00")==0);
        }
        CHECK(Get_CG_Value(-3,20)==0.05f*direction);
        CHECK(Get_CG_Value(3,20)==-0.05f*direction);
        CHECK(Get_CG_Value(0,20.1f)==0.0f && !signbit(Get_CG_Value(0,20.1f)));
        CHECK(Get_CG_Value(0,19.9f)==0.0f && !signbit(Get_CG_Value(0,19.9f)));
    }
    left_And_Righe_Status=1;
}

int main(void)
{
    CHECK(Get_CG_Value(-12445,20)==0.0f);
    CHECK(CG_Result_Status==CG_RESULT_CAL_ERROR);
    check_case(1,50,-12445,12513);
    check_case(50,1,-12445,12513);
    check_case(50,50,-12445,12513);
    check_case(750,-750,-3250,3250);
    check_case(-750,750,-4750,4750);
    check_case(28000,27000,24000,32000);
    check_case(-28000,-27000,-32768,-23000);
    check_case(-51,-2,-12445,12513); /* Negative half-count midpoint. */
    check_bench_residual();
    check_invalid();
    check_rounding_and_signed_zero();
    printf("CG40 math: PASS %u checks; 8 full signed-AD sweeps, midpoint, same-orientation spans, residual tilt, guards, temperature, 0.05 mm and +0.00\n",checks);
    return 0;
}
