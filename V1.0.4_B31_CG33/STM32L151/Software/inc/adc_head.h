#ifndef __ADC_HEAD_H
#define __ADC_HEAD_H

#include "main.h"

#define BATTERY_DIVIDER_TOP_KOHM        100.0f
#define BATTERY_DIVIDER_BOTTOM_KOHM      33.0f
#define BATTERY_ADC_SAMPLES                  8U
#define BATTERY_LEVEL_1_V                  7.20f
#define BATTERY_LEVEL_2_V                  7.44f
#define BATTERY_LEVEL_3_V                  7.68f
#define BATTERY_LEVEL_4_V                  7.92f
#define BATTERY_LEVEL_HYST_V               0.06f
#define BATTERY_LOW_ENTER_V                7.20f
#define BATTERY_SAMPLE_PERIOD_MS            5000U
#define BATTERY_FLASH_PERIOD_MS              350U

enum { BATTERY_ADC_WAIT, BATTERY_ADC_VALID, BATTERY_ADC_FAILED };
uint8_t ADC_Battery_Step(float *volts);
void ADC_Battery_Cancel(void);
uint8_t Battery_Level_For_Voltage(float volts, uint8_t previous_level);

#endif
