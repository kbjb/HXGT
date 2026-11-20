#ifndef __GPIO_HEAD_H
#define __GPIO_HEAD_H

#include "main_head.h"

typedef struct GPIO_CLASS_STR
{
    // Ù–‘
    GPIO_TypeDef*      gpio;
    uint32_t           rcc;
    GPIO_InitTypeDef   initdef;
    //∑Ω∑®
    void      (*str_init)     (struct GPIO_CLASS_STR*);
    void      (*init)        (struct GPIO_CLASS_STR*);
    void      (*mode_set)    (struct GPIO_CLASS_STR*,GPIOMode_TypeDef);
    void      (*speed_set)   (struct GPIO_CLASS_STR*,GPIOSpeed_TypeDef);
    void      (*write)       (struct GPIO_CLASS_STR*,BitAction);
    BitAction (*read)        (struct GPIO_CLASS_STR*);
}GPIO_STR;






 void GPIO_STR_INIT(GPIO_STR *gpio_str);




#define GPIO_RCC_SELECT(gpio)       (gpio == GPIOA)?RCC_APB2Periph_GPIOA:   \
                                    (gpio == GPIOB)?RCC_APB2Periph_GPIOB:   \
                                    (gpio == GPIOC)?RCC_APB2Periph_GPIOC:   \
                                    (gpio == GPIOD)?RCC_APB2Periph_GPIOD:   \
                                    (gpio == GPIOE)?RCC_APB2Periph_GPIOE:   \
                                    (gpio == GPIOF)?RCC_APB2Periph_GPIOF:   \
                                    (gpio == GPIOG)?RCC_APB2Periph_GPIOG:   \
                                     RCC_APB2Periph_GPIOA



#define GPIO_CLASS_CREATE(_gpio,_gpio_pin,gpio_mode,_gpio_speed)            \
{                                                                           \
    .gpio                         = _gpio,                                  \
    .rcc                          = GPIO_RCC_SELECT(_gpio),                 \
    .initdef.GPIO_Mode            = gpio_mode,                              \
    .initdef.GPIO_Speed           = _gpio_speed,                            \
    .initdef.GPIO_Pin             = _gpio_pin,                              \
    .str_init                     = GPIO_STR_INIT,                          \
}









#endif




