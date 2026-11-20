#ifndef __ADC_HEAD_H
#define __ADC_HEAD_H

#include "main.h"



// STM32L151的Vrefint校准值地址（请根据实际芯片参考手册确认）
// 通常L1系列的Vrefint校准值在0x1FF80078
#define VREFINT_CAL_ADDR  ((uint16_t*)0x1FF80078)
#define VREFINT_TYPICAL_VOLTAGE         1.2f        // Vrefint典型电压1.2V
#define VDDA_TYPICAL                    3.0f        // STM32L系列典型VDDA为3.0V



// 函数声明
void ADC1_Init(void);
uint16_t ADC_ReadChannel(uint32_t channel);
uint16_t ADC_ReadChannel9(void);
uint16_t ADC_ReadVrefint(void);
float ADC_GetChannel9Voltage(void);
float ADC_GetVDDAVoltage(void);
float ADC_GetVrefintVoltage(void);


extern float  POWER_Vol;









#endif








