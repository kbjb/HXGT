#ifndef __LM9507_HEAD_H
#define __LM9507_HEAD_H
#include "main_head.h"
#include "spi_head.h"


typedef struct LM9507_CLASS_STR
{
    GPIO_STR *clk;
    GPIO_STR *cs;
    GPIO_STR *mosi;
    GPIO_STR *miso;
    
    void    (*init)(struct LM9507_CLASS_STR*);
    void    (*write)(struct LM9507_CLASS_STR*,uint16_t);
    uint16_t (*read)(struct LM9507_CLASS_STR*);
    float   (*readTemp)(struct LM9507_CLASS_STR*);
    void    (*shoutDown)(struct LM9507_CLASS_STR*);
    void    (*str_init)(struct LM9507_CLASS_STR*);
}LM9507_STR;




void LM9507_STR_INIT(LM9507_STR *lm9507_str);

#define LM9507_CLASS_STR(_clk,_cs,_mosi,_miso)\
{\
    .clk      = _clk,           \
    .cs       = _cs,            \
    .mosi     = _mosi,          \
    .miso     = _miso,          \
    .str_init = LM9507_STR_INIT,\
}


#define LM9507_CLK_PORT  GPIOB
#define LM9507_CLK_PIN   GPIO_Pin_13

#define LM9507_CS_PORT   GPIOB
#define LM9507_CS_PIN    GPIO_Pin_12

#define LM9507_MOSI_PORT GPIOB
#define LM9507_MOSI_PIN  GPIO_Pin_15

#define LM9507_MISO_PORT GPIOB
#define LM9507_MISO_PIN  GPIO_Pin_14








#define LM9531_STK_SIZE     120
#define LM9531_TASK_PRIO    8



void LM9531_Task_Create(void);











#endif


