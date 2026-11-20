#ifndef ADC_HEAD_H
#define ADC_HEAD_H

#include "main_head.h"
#include "gpio_head.h"


#define      MAX_CH_NUM             17

typedef struct ADC_CH_CLASS_STR
{
    GPIO_STR    *ch_gpio;
    uint8_t     ch;
    uint8_t     SampleTime;
    
    void (*init)(struct ADC_CH_CLASS_STR *);
}ADC_CH_STR;

void ADC_CH_INIT(ADC_CH_STR *adc_ch_str);

#define ADC_CH_CLASS_CREATE(_gpio,_ch,_SampleTime)\
{\
    .ch_gpio     = _gpio,       \
    .ch          = _ch,         \
    .SampleTime  = _SampleTime, \
    .init        = ADC_CH_INIT, \
}

typedef struct ADC_CLASS_STR
{
    ADC_TypeDef*    adc;
    ADC_CH_STR **   chs;
    uint8_t         ch_num;
    uint32_t        rcc;
    uint32_t        rcc_pclk;
    ADC_InitTypeDef adc_info;
    
    void     (*str_init)(struct ADC_CLASS_STR*);
    void     (*init)(struct ADC_CLASS_STR*);
    void     (*ch_add)(struct ADC_CLASS_STR*,ADC_CH_STR*);
    uint16_t (*get_adc)(struct ADC_CLASS_STR*,ADC_CH_STR);
    uint16_t (*get_aver)(struct ADC_CLASS_STR*,ADC_CH_STR,uint16_t);
}ADC_STR;


#define ADC_RCC_SELECT(_ADC) (_ADC == ADC1)?RCC_APB2Periph_ADC1:\
                             (_ADC == ADC2)?RCC_APB2Periph_ADC2:\
                              RCC_APB2Periph_ADC1

void ADC_STR_INIT(ADC_STR *adc_str);

#define ADC_CALSS_CREATE(_ADC,_ADC_CHS,_RCC_PCLK)\
{\
    .adc        = _ADC,                 \
    .chs        = _ADC_CHS,             \
    .rcc        = ADC_RCC_SELECT(_ADC), \
    .rcc_pclk   = _RCC_PCLK,            \
    .ch_num     = 0,                    \
    .str_init   = ADC_STR_INIT,         \
}




void ADC_CH_INIT(ADC_CH_STR *adc_ch_str);
void ADC_STR_INIT(ADC_STR *adc_str);


#define ADC_STK_SIZE    120
#define ADC_TASK_PRIO   8

void Adc_Task_Create(void);




#define CHARGE_PORT     GPIOB
#define CHARGE_PIN      GPIO_Pin_1










#endif


