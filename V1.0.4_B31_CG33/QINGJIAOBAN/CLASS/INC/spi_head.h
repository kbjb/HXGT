#ifndef __SPI_HEAD_H
#define __SPI_HEAD_H

#include "stm32f10x.h"
#include "gpio_head.h"



 //CPOL  CPHA
typedef enum
{
    mode0 = 0, //CPOL = 0  CPHA = 0
    mode1 = 1, //CPOL = 0  CPHA = 1
    mode2 = 2, //CPOL = 1  CPHA = 0
    mode3 = 3, //CPOL = 1  CPHA = 1
}spi_mode;



typedef struct SPI_CLASS_STR
{
    spi_mode  mode;
    GPIO_STR *clk_gpio;
    GPIO_STR *miso_gpio;
    GPIO_STR *mosi_gpio;
    GPIO_STR *cs_gpio;
    
    
    void (*str_init)(struct SPI_CLASS_STR*);
    void (*init)(struct SPI_CLASS_STR*);
    uint8_t (*readWrite)(struct SPI_CLASS_STR*,uint8_t);
}SPI_STR;

void SPI_STR_INIT(SPI_STR *spi_str);

#define SPI_CLASS_CREATE(_clk,_miso,_mosi,_cs,_mode)\
{\
    .clk_gpio  = _clk,        \
    .miso_gpio =_miso,        \
    .mosi_gpio = _mosi,       \
    .cs_gpio   = _cs,         \
    .mode      = _mode,       \
    .str_init  = SPI_STR_INIT,\
}




#endif


