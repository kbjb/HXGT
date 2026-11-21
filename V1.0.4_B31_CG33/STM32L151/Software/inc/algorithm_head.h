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

/* GJ validity pertains to the last calculation; check link/storage separately.
 * Never display or export the 0.0f error sentinel as a measurement. */
float Get_GJ_Value(uint16_t GJ_Ad,float temp);
uint8_t GJ_Result_Is_Valid(void);


uint8_t Get_Driver_Power(uint16_t PowerAd);
#endif




