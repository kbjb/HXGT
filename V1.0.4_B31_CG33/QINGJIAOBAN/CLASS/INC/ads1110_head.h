#ifndef __ADS1110_HEAD_H
#define __ADS1110_HEAD_H

#include "main_head.h"
#include "iic_head.h"



#define ADS1110_READ_REG        0x91
#define ADS1110_WRITE_REG       0x90

#define ADS1110_CONFIG_REG_DATA 0x0C

/*
    工作模式  单次采集模式、连续采集模式
*/
typedef enum
{
    ads1110_sigle = 0,
    ads1110_continue,
}CONVERSION;
/*
    转换速率  240SPS\60SPS\30SPS\15SPS
*/
typedef enum
{
    speed_240 =0,
    speed_60,
    speed_30,
    speed_15,
}ADS1110_RATE;
/*
    增益倍率
*/
typedef enum
{
    gain1   =0,
    gain2,
    gain4,
    gain8,
}ADS1110_GAIN;

typedef struct ADS1110_CLASS_STR
{
    IIC_STR *iic_str;
    
    uint8_t  config;
    uint16_t ad_data;
    void (*str_init)(struct ADS1110_CLASS_STR *);
    void (*init)(struct ADS1110_CLASS_STR *);
    uint8_t (*check)(struct ADS1110_CLASS_STR *);
    void (*read)(struct ADS1110_CLASS_STR *);
    void (*write)(struct ADS1110_CLASS_STR *,uint8_t);
    
}ADS1110_STR;

void ADS1110_STR_INIT(ADS1110_STR *ads1110_str);


#define ADS1110_CLASS_STR(_iic)     \
{\
    .iic_str  = _iic,               \
    .config   = 0,                  \
    .ad_data  = 0,                  \
    .str_init = ADS1110_STR_INIT,   \
}






#define ADS1110_SCL_PORT    GPIOB
#define ADS1110_SCL_PIN     GPIO_Pin_6


#define ADS1110_SDA_PORT    GPIOB
#define ADS1110_SDA_PIN     GPIO_Pin_7





#define ADS1110_STK_SIZE    120
#define ADS1110_TASK_PRO    8

void ADS1110_Task_Create(void);








#endif

