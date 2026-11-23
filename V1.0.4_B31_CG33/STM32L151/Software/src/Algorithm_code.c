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


float Get_GJ_Value(uint16_t GJ_Ad,float temp)
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
    // 防止分母为零（理论上不会发生，因三点不共线时分母非零）
    if (denominator == 0.0f) {
        // 退化为使用中点参考值，实际应用中应根据需求处理
        return (y1 + y2 + y3) / n;
    }

    float k = (n * sumXY - sumX * sumY) / denominator;
    float b = (sumY - k * sumX) / n;

    // 计算实时AD值对应的长度
    GJ_Value = k * GJ_Ad + b;
    return GJ_Value;
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















