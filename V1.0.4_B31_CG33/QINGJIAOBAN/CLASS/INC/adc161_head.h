#ifndef __ADC161_HEAD_H
#define __ADC161_HEAD_H

#include "main_head.h"
#include "gpio_head.h"
#include "data_head.h"

typedef struct ADC161_CLASS_STR
{
    GPIO_STR    *cs;
    GPIO_STR    *clk;
    GPIO_STR    *out;
    int16_t    ad_data;
    void (*str_init)(struct ADC161_CLASS_STR*);
    void (*init)(struct ADC161_CLASS_STR*);
    void (*read)(struct ADC161_CLASS_STR*);
}ADC161_STR;


void ADC161_STR_INIT(ADC161_STR *adc161_str);



#define ADC161_CLASS_CREATE(_cs,_clk,_out)\
{\
    .cs      = _cs,                       \
    .clk      = _clk,                     \
    .out      = _out,                     \
    .ad_data  = 0,                        \
    .str_init = ADC161_STR_INIT,          \
}





#define ADC161_STK_SIZE 120
#define ADC161_TASK_PRO 8




#define ADC161_CS_PORT   GPIOA
#define ADC161_CS_PIN    GPIO_Pin_4
#define ADC161_CLK_PORT  GPIOA
#define ADC161_CLK_PIN   GPIO_Pin_5
#define ADC161_OUT_PORT  GPIOA
#define ADC161_OUT_PIN   GPIO_Pin_6



void ADC161_Task_Create(void);










#endif


