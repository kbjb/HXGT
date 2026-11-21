#include "Algorithm_head.h"
#include "adc_head.h"
#include "cg_cal_guard.h" /* [CG32-03] */
#include <stdlib.h>

float TempOffset = 0;
float CG_Value_r = 0;
float CG_Value_t = 0;
float CG_Zero_Offset_t = 0;
float CG_Zero_Offset = 0;

//static float K_Center_Offset;
//static float B_Center_Offset;

static float K_CG_Left;
static int16_t CG_Zero_Low;
static float K_CG_Right;
static int16_t CG_Zero_High;
static uint8_t CG_Coefficients_Valid = 0U;
volatile uint8_t CG_Result_Status = CG_RESULT_CAL_ERROR;

void CG_Data_Init(void)
{
    /* [CG32-03] Never divide by a tiny or reversed AD separation. */
    CG_Coefficients_Valid = CG_Calibration_Sane(&CG_Param);
    K_CG_Left = K_CG_Right = 0.0f;
    CG_Zero_Low = CG_Zero_High = 0;
    if (!CG_Coefficients_Valid)
    {
        CG_Result_Status = CG_RESULT_CAL_ERROR;
        return;
    }
    /* The zero band uses sorted endpoints. Anchor the outer branches
     * to those very endpoints, even when the recorded zeroes cross, so
     * there is no discontinuity. Keep +/-180 mm reference points fixed.
     * Guarded physical span exceeds the maximum zero gap. */
    CG_Zero_Low = CG_Param.CG_Zero_Left_Ad;
    CG_Zero_High = CG_Param.CG_Zero_Right_Ad;
    if (CG_Zero_Low > CG_Zero_High)
    {
        int16_t old_low = CG_Zero_Low;
        CG_Zero_Low = CG_Zero_High;
        CG_Zero_High = old_low;
    }
    K_CG_Left = 180.0f / (float)((int32_t)CG_Param.CG_Max_Left_Ad - CG_Zero_Low);
    K_CG_Right = -180.0f / (float)((int32_t)CG_Param.CG_Max_Right_Ad - CG_Zero_High);
    CG_Result_Status = CG_RESULT_OK;
}

float Get_CG_Value(int16_t adc_value, float temp)
{
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
    if (adc_value < CG_Zero_Low)
        CG_Value_t = (float)((int32_t)adc_value - CG_Zero_Low) * K_CG_Left;
    else if (adc_value > CG_Zero_High)
        CG_Value_t = (float)((int32_t)adc_value - CG_Zero_High) * K_CG_Right;
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


static uint8_t GJ_Result_Valid = 0U;

uint8_t GJ_Result_Is_Valid(void)
{
    return GJ_Result_Valid;
}

float Get_GJ_Value(uint16_t GJ_Ad, float temp)
{
    int32_t x1 = GJ_Param.GJ_1410_Ad;
    int32_t x2 = GJ_Param.GJ_1435_Ad;
    int32_t x3 = GJ_Param.GJ_1470_Ad;
    float d21, d31, d32, slope, centered, value;

    (void)temp; /* There is no experimentally established GJ thermal model. */
    GJ_Result_Valid = 0U;
    if (GJ_Ad > 0x7FFFU) return 0.0f; /* unavailable ADS1110 gauge data */
    /* As in EEPROM validation, either sensor direction is permitted, but
     * equal or nonmonotonic points cannot calibrate this measurement. */
    if (!((x1 < x2 && x2 < x3) || (x1 > x2 && x2 > x3)))
        return 0.0f; /* Error sentinel, NOT a gauge reading. */

    d21 = (float)(x2 - x1);
    d31 = (float)(x3 - x1);
    d32 = (float)(x3 - x2);
    /* Least-squares covariance/variance via pairwise differences:
     * identical model, without subtracting large near-equal numbers. */
    slope = (25.0f * d21 + 60.0f * d31 + 35.0f * d32) /
            (d21*d21 + d31*d31 + d32*d32);
    centered = (float)((int32_t)GJ_Ad - x2) - (d32 - d21) / 3.0f;
    value = (4315.0f / 3.0f) + slope * centered;
    GJ_Result_Valid = 1U;
    return value;
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















