#ifndef _AT24C02_HEAD_H
#define _AT24C02_HEAD_H
#include "main_head.h"
#include "gpio_head.h"
#include "iic_head.h"



#define AT24C02_CHECK_ADDR  2
#define AT24C02_CHECK_VALUE 50

#define AT24C02_INFO_ADDR   1
#define AT24C02_MAX_SIZE    254

#define AT24C02_ADDR        0xA0

typedef struct AT24C02_CLASS_STR
{
    IIC_STR *  iic;
    uint16_t   size;
    uint16_t   used_memory;
    uint8_t    addr;
    
    void       (*str_init)(struct AT24C02_CLASS_STR*);
    void       (*init)(struct AT24C02_CLASS_STR*);
    uint8_t    (*writebyte)(struct AT24C02_CLASS_STR*,uint8_t,uint8_t);
    uint8_t    (*readbyte)(struct AT24C02_CLASS_STR*,uint8_t);
    uint8_t    (*read)(struct AT24C02_CLASS_STR*,uint8_t,uint8_t,uint8_t*);
    uint8_t    (*write)(struct AT24C02_CLASS_STR*,uint8_t,uint8_t,uint8_t*);
    uint8_t    (*check)(struct AT24C02_CLASS_STR*);
}AT24C02_STR;

void AT24C02_STR_INIT(AT24C02_STR * at24c02_str);

#define AT24C02_CLASS_CREATE(_iic,_size,_addr)\
{\
    .iic      = _iic,               \
    .size     = _size,              \
    .addr     = _addr,              \
    .str_init = AT24C02_STR_INIT,   \
}

#define  AT24C02_IIC_SCL_PORT   GPIOB
#define  AT24C02_IIC_SCL_PIN    GPIO_Pin_6
#define  AT24C02_IIC_SDA_PORT   GPIOB
#define  AT24C02_IIC_SDA_PIN    GPIO_Pin_7





#define   AT24C02_STK_SIZE      120
#define   AT24C02_TASK_PRO      10


void AT24C02_Task_Create(void);






#endif


