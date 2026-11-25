#include "Algorithm_head.h"
#include "adc_head.h"
#include "cg_cal_guard.h" /* [CG32-03] */
#include <stdlib.h>
#include <float.h>

float TempOffset = 0;
float CG_Value_r = 0;
float CG_Value_t = 0;
float CG_Zero_Offset_t = 0;
float CG_Zero_Offset = 0;

//static float K_Center_Offset;
//static float B_Center_Offset;

static float K_CG_Left;
static float B_CG_Left;
static float K_CG_Right;
static float B_CG_Right;
static uint8_t CG_Coefficients_Valid = 0U;
volatile uint8_t CG_Result_Status = CG_RESULT_CAL_ERROR;

void CG_Data_Init(void)
{
    int16_t zero_low, zero_high;
    /* [CG32-03] Never divide by a tiny or reversed AD separation. */
    CG_Coefficients_Valid = CG_Calibration_Sane(&CG_Param);
    K_CG_Left = 0.0f; B_CG_Left = 0.0f;
    K_CG_Right = 0.0f; B_CG_Right = 0.0f;
    if (!CG_Coefficients_Valid)
    {
        CG_Result_Status = CG_RESULT_CAL_ERROR;
        return;
    }
    /* [CG35] Anchor each slope to the edge where that branch is entered.
     * Both horizontal points define the zero interval in either ordering.
     * Keep the measured negative/positive AD endpoints mapped to +/-180 mm.
     * The existing span >=4000 and zero gap <=1500 checks ensure both
     * sorted-edge denominators remain nonzero (at least 2500 AD). */
    zero_low = (CG_Param.CG_Zero_Left_Ad < CG_Param.CG_Zero_Right_Ad) ?
                CG_Param.CG_Zero_Left_Ad : CG_Param.CG_Zero_Right_Ad;
    zero_high = (CG_Param.CG_Zero_Left_Ad > CG_Param.CG_Zero_Right_Ad) ?
                 CG_Param.CG_Zero_Left_Ad : CG_Param.CG_Zero_Right_Ad;
    K_CG_Left = 180.0f / ((float)CG_Param.CG_Max_Left_Ad - (float)zero_low);
    B_CG_Left = -K_CG_Left * (float)zero_low;
    K_CG_Right = -180.0f / ((float)CG_Param.CG_Max_Right_Ad - (float)zero_high);
    B_CG_Right = -K_CG_Right * (float)zero_high;
    CG_Result_Status = CG_RESULT_OK;
}

float Get_CG_Value(int16_t adc_value, float temp)
{
    int16_t zero_low, zero_high;
    /* [CG32-03] Returned 0 on errors is NOT a measurement; callers must
     * inspect CG_Result_Status and show an error, not 0.00 mm. */
    CG_Value_r = 0.0f;
    CG_Value_t = 0.0f;
    if (!CG_Coefficients_Valid || !CG_Calibration_Sane(&CG_Param))
    {
        CG_Result_Status = CG_RESULT_CAL_ERROR;
        return 0.0f;
    }
    if (!(temp >= -40.0f && temp <= 100.0f))
    {
        CG_Result_Status = CG_RESULT_TEMP_ERROR;
        return 0.0f;
    }
    /* The coefficient is unchanged; the offset is now referenced to
     * the ACTUAL temperature recorded with the four-point calibration.
     * For factory CG_Param.temp=20, results match the previous formula. */
    TempOffset = -0.054637f * (temp - CG_Param.temp);
    zero_low = (CG_Param.CG_Zero_Left_Ad < CG_Param.CG_Zero_Right_Ad) ?
                CG_Param.CG_Zero_Left_Ad : CG_Param.CG_Zero_Right_Ad;
    zero_high = (CG_Param.CG_Zero_Left_Ad > CG_Param.CG_Zero_Right_Ad) ?
                 CG_Param.CG_Zero_Left_Ad : CG_Param.CG_Zero_Right_Ad;
    if (adc_value < zero_low)
        CG_Value_t = (float)adc_value * K_CG_Left + B_CG_Left;
    else if (adc_value > zero_high)
        CG_Value_t = (float)adc_value * K_CG_Right + B_CG_Right;
    else
        CG_Value_t = 0.0f;

    /* [CG33-01] Display out-of-range numeric measurements for diagnostics.
     * Do not suppress values beyond +/-240 mm. This does not establish
     * accuracy or replace the calibration and sensor-integrity checks. */
    CG_Value_r = roundf((CG_Value_t - TempOffset) / 0.05f) *
                 0.05f * (float)left_And_Righe_Status;
    CG_Result_Status = CG_RESULT_OK;
    return CG_Value_r;
}


/* [CG38] One fit shared by gauge and CH; retain the existing three-point
 * least-squares arithmetic and normal gauge results. No second filter. */
static uint8_t GJ_Fit_Coefficients(float *k, float *b)
{
    float x1 = (float)GJ_Param.GJ_1410_Ad;
    float y1 = 1410.0f;
    float x2 = (float)GJ_Param.GJ_1435_Ad;
    float y2 = 1435.0f;
    float x3 = (float)GJ_Param.GJ_1470_Ad;
    float y3 = 1470.0f;

    // 计算线性回归参数：y = k * x + b
    float sumX = x1 + x2 + x3;
    float sumY = y1 + y2 + y3;
    float sumXY = x1 * y1 + x2 * y2 + x3 * y3;
    float sumX2 = x1 * x1 + x2 * x2 + x3 * x3;

    int n = 3;
    float denominator = n * sumX2 - sumX * sumX;
    if (denominator <= 0.0f) return 0U;
    *k = (n * sumXY - sumX * sumY) / denominator;
    *b = (sumY - *k * sumX) / n;
    return 1U;
}

float Get_GJ_Value(uint16_t GJ_Ad,float temp)
{
    float k, b;
    (void)temp; /* Gauge has no temperature correction in the current model. */
    if (!GJ_Fit_Coefficients(&k,&b)) return (1410.0f+1435.0f+1470.0f)/3.0f;
    GJ_Value = k * GJ_Ad + b;
    return GJ_Value;
}

uint8_t GJ_Get_Valid_Scale(float *scale)
{
    uint16_t a=GJ_Param.GJ_1410_Ad, b=GJ_Param.GJ_1435_Ad, c=GJ_Param.GJ_1470_Ad;
    float offset;
    if (scale == NULL || a==0U || b==0U || c==0U ||
        a>0x7FFFU || b>0x7FFFU || c>0x7FFFU ||
        !(((a<b)&&(b<c)) || ((a>b)&&(b>c))) ||
        !(GJ_Param.temp>=-40.0f && GJ_Param.temp<=100.0f)) return 0U;
    if (!GJ_Fit_Coefficients(scale,&offset)) return 0U;
    return ((*scale>0.0f || *scale<0.0f) && fabsf(*scale)<=FLT_MAX) ? 1U : 0U;
}

uint8_t Get_CH_Values(uint16_t ad, float *cz, float *hb)
{
    float scale;
    if (cz==NULL || hb==NULL || ad==0U || ad>0x7FFFU ||
        CH_Param.CZ_Ad==0U || CH_Param.CZ_Ad>0x7FFFU ||
        CH_Param.HB_Ad==0U || CH_Param.HB_Ad>0x7FFFU ||
        !(CH_Param.temp>=-40.0f && CH_Param.temp<=100.0f) ||
        !GJ_Get_Valid_Scale(&scale)) return 0U;
    /* Independent contact references may have identical AD. Subtract in
     * signed arithmetic so a movement below either reference stays negative.
     * Shared mechanical ratio/direction must be checked on the actual heads. */
    *cz = CH_CZ_REFERENCE_MM + scale * ((int32_t)ad-(int32_t)CH_Param.CZ_Ad);
    *hb = CH_HB_REFERENCE_MM + scale * ((int32_t)ad-(int32_t)CH_Param.HB_Ad);
    return 1U;
}


static uint8_t  Power_Ad_Cache_Index = 0,Power_Ad_Cache_Count = 0;
static uint16_t Power_Ad_Cache[10] = {0};
static uint32_t Power_Ad_Sum = 0;

uint8_t Get_Driver_Power(uint16_t PowerAd)
{ 
    POWER_Vol = ADC_GetVrefintVoltage();
    float PowerVol_T;
    
    if(Power_Ad_Cache_Count == 10)//缓存区已满
        Power_Ad_Sum -= Power_Ad_Cache[Power_Ad_Cache_Index];
    else
        Power_Ad_Cache_Count++;
    // 添加新值到缓冲区
    Power_Ad_Cache[Power_Ad_Cache_Index] = PowerAd;
    Power_Ad_Sum += PowerAd;
    Power_Ad_Cache_Index = (Power_Ad_Cache_Index + 1) % 10;
    PowerVol_T = Power_Ad_Sum/Power_Ad_Cache_Count*POWER_Vol/4096.0*133/33;
    
    if(PowerVol_T>7.92f)
        return 4;
    else if(PowerVol_T>7.44f)
        return 3;
    else if(PowerVol_T>6.96f)
        return 2;
    else if(PowerVol_T>6.48f)
        return 1;
    else
        return 0;
}















