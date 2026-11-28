#ifndef __ALGORITHM_HEAD_H
#define __ALGORITHM_HEAD_H

#include "main.h"
#include "param_head.h"
#include "math.h"




/* [CGFIX-01] Rebuild the CG coefficients after EEPROM load or calibration.
 * [CG32-03] An invalid reading returns 0 but must NEVER be shown as 0 mm. */
#define CG_RESULT_OK           0U
#define CG_RESULT_CAL_ERROR    1U
#define CG_RESULT_TEMP_ERROR   3U
#define CG_RESULT_NO_SENSOR    4U
#define CG_RESULT_EEP_ERROR    5U
extern volatile uint8_t CG_Result_Status;
void CG_Data_Init(void);
float Get_CG_Value(int16_t CG_Ad,float temp);

float Get_GJ_Value(uint16_t GJ_Ad,float temp);
/* [CG38] Shared displacement scale, independent CH reference positions. */
#define CH_CZ_REFERENCE_MM 1391.0f
#define CH_HB_REFERENCE_MM 1348.0f
uint8_t GJ_Get_Valid_Scale(float *scale);
uint8_t Get_CH_Values(uint16_t ad, float *cz, float *hb);


#endif




