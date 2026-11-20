#include "gpio_head.h"

void GPIO_INIT(GPIO_STR* gpio_str)
{
    RCC_APB2PeriphClockCmd(gpio_str->rcc,ENABLE);
    GPIO_Init(gpio_str->gpio,&gpio_str->initdef);
}

void GPIO_MODE_SET(GPIO_STR*gpio_str,GPIOMode_TypeDef gpio_mode)
{
    gpio_str ->initdef.GPIO_Mode = gpio_mode;
    gpio_str->init(gpio_str);
}

void GPIO_SPEED_SET(GPIO_STR*gpio_str,GPIOSpeed_TypeDef gpio_speed)
{
    gpio_str ->initdef.GPIO_Speed = gpio_speed;
    gpio_str->init(gpio_str);
}

void GPIO_WRITE(GPIO_STR* gpio_str,BitAction bitAction)
{
    GPIO_WriteBit(gpio_str->gpio,gpio_str->initdef.GPIO_Pin,bitAction);
}

BitAction GPIO_READ(GPIO_STR* gpio_str)
{
    return (BitAction)GPIO_ReadInputDataBit(gpio_str->gpio,gpio_str->initdef.GPIO_Pin);
}

void GPIO_STR_INIT(GPIO_STR* gpio_str)
{
    gpio_str->init = GPIO_INIT;
    gpio_str->mode_set = GPIO_MODE_SET;
    gpio_str->speed_set = GPIO_SPEED_SET;
    gpio_str->write = GPIO_WRITE;
    gpio_str->read  = GPIO_READ; 
    gpio_str->init(gpio_str);
}


/*********************************************************************OS***************************************************/








