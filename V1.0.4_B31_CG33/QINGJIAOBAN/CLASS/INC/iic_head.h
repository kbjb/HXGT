#ifndef __IIC_HEAD_H
#define __IIC_HEAD_H

#include "main_head.h"
#include "gpio_head.h"


typedef struct IIC_CLASS_STR
{
    GPIO_STR *sclk_gpio;
    GPIO_STR *sda_gpio;
    uint32_t  delay_cnt;
    GPIOMode_TypeDef sda_output_mode;
    void      (*str_init)(struct IIC_CLASS_STR*) ;
    void      (*init)(struct IIC_CLASS_STR*);
    void      (*start)(struct IIC_CLASS_STR *);
    void      (*stop)(struct IIC_CLASS_STR*);
    uint8_t   (*wait_ack)(struct IIC_CLASS_STR*);
    void      (*ack)(struct IIC_CLASS_STR*);
    void      (*nack)(struct IIC_CLASS_STR*);
    void      (*send_byte)(struct IIC_CLASS_STR*,uint8_t); 
    uint8_t   (*read_byte)(struct IIC_CLASS_STR*);
    void      (*delay)(struct IIC_CLASS_STR*);
}IIC_STR;




void IIC_STR_INIT(IIC_STR *iic_str);
     
 
 
 
#define IIC_CLASS_CREATE(_sclk,_sda,_delay_cnt,_delay)\
{\
    .sclk_gpio = _sclk,\
    .sda_gpio  = _sda, \
    .delay_cnt = _delay_cnt,\
    .delay     = _delay,\
    .str_init  = IIC_STR_INIT,\
}




#endif

