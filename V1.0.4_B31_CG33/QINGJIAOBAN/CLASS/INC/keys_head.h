#ifndef __KEYS_HEAD_H
#define __KEYS_HEAD_H
#include "main_head.h"
#include "gpio_head.h"

typedef enum
{
    long_down   =0,
    void_down     ,
    short_down    ,
    none_down     ,
    cnt2_down     ,
    cnt3_down     ,
    cnt4_down     ,
}KEY_STATE;


#define KEY_MATRIX_WIDTH_ADD    1
#define KEY_MATRIX_HIGH_ADD     0


/*
    按键参数
*/

typedef struct KEY_PARAM_CALSS_STR
{
    uint8_t   num;
    BitAction target_level;   //目标电平
    BitAction now_level;      //当前电平状态
    BitAction last_level;     //上次电平状态
    uint16_t  countDown_cnt;  //倒计时计数器
    uint16_t  countDown_thre; //倒计时阈值
    uint16_t  cnt;            //计数器
    uint16_t  time;           //计时器
    uint16_t  time_thre;      //计时器阈值
    KEY_STATE state;          //计时器状态
    
    KEY_STATE (*logic_check)(struct KEY_PARAM_CALSS_STR*);
}KEY_PARAM_STR;

#define KEY_PARAM_CREATE(_num,_target_level,_countDown_thre,_time_thre)\
{                                                                      \
    .num                       = _num,                                 \
    .target_level              = _target_level,                        \
    .countDown_cnt             = 0,                                    \
    .countDown_thre            = _countDown_thre,                      \
    .cnt                       = 0,                                    \
    .time                      = 0,                                    \
    .time_thre                 = _time_thre,                           \
    .state                     = void_down,                            \
    .logic_check               = KEYS_LOGIC_CHECK,                     \
}


/*
    矩阵按键
*/
typedef struct KEY_MATRIX_CLASS_STR
{
    GPIO_STR **width_gpio_str;       //横向IO
    GPIO_STR **high_gpio_str;        //纵向
    uint8_t width;                   //横向数量
    uint8_t high;                    //纵向数量
    uint8_t width_i;                 //现有横向数量
    uint8_t high_i;                  //现有纵向数量
    KEY_PARAM_STR**keys_param;        //参数列表
    
    void (*add_param)(struct KEY_MATRIX_CLASS_STR*,KEY_PARAM_STR*);
    void (*add_gpio)(struct KEY_MATRIX_CLASS_STR*,GPIO_STR*,uint8_t);
    void (*check)(struct KEY_MATRIX_CLASS_STR*);
    void (*logic)(struct KEY_MATRIX_CLASS_STR*);
    void (*init)(struct KEY_MATRIX_CLASS_STR*);
    void (*str_init)(struct KEY_MATRIX_CLASS_STR*);
}KEY_MATRIX_STR;

void KEYS_MATRIX_STR_INIT(KEY_MATRIX_STR*key_matrix);

#define KEY_MATRIX_CLASS_CREATE(_width,_high)\
{\
    .width           = _width,               \
    .high            = _high,                \
    .width_i         = 0,                    \
    .high_i          = 0,                    \
    .str_init        = KEYS_MATRIX_STR_INIT, \
}



/*
    普通按键
*/

typedef struct KEY_NORMAL_CLASS_STR
{
    GPIO_STR *gpio_str;       //按键IO
    KEY_PARAM_STR *key_param;  //按键参数
    
    void      (*str_init)(struct KEY_NORMAL_CLASS_STR*);    //结构体初始化
    void      (*init)(struct KEY_NORMAL_CLASS_STR*);        //初始化
    BitAction (*check)(struct KEY_NORMAL_CLASS_STR*);       //按键检测
    KEY_STATE (*logic)(struct KEY_NORMAL_CLASS_STR*);       //按键逻辑计算
}KEY_NORMAL_STR;



void KEYS_NORMAL_STR_INIT(KEY_NORMAL_STR * key_normal_str);

#define KEY_NORMAL_CLASS_CREATE(_gpio,_param)\
{\
    .gpio_str                  = _gpio,                       \
    .key_param                 = _param,                      \
    .str_init                  = KEYS_NORMAL_STR_INIT,        \
}














#define KEY_STK_SIZE    100
#define KEY_TASK_PRIO   8



 
void Key_Task_Create(void);




#define ONOFF_GPIO_PORT   GPIOA
#define ONOFF_GPIO_PIN    GPIO_Pin_0

#define LFRI_GPIO_PORT    GPIOA
#define LFRI_GPIO_PIN     GPIO_Pin_1

#define WIDTH1_GPIO_PORT  GPIOA
#define WIDTH1_GPIO_PIN   GPIO_Pin_4

#define WIDTH2_GPIO_PORT  GPIOA
#define WIDTH2_GPIO_PIN   GPIO_Pin_5

#define WIDTH3_GPIO_PORT  GPIOA
#define WIDTH3_GPIO_PIN   GPIO_Pin_6

#define HIGH1_GPIO_PORT   GPIOA
#define HIGH1_GPIO_PIN    GPIO_Pin_7

#define HIGH2_GPIO_PORT   GPIOC
#define HIGH2_GPIO_PIN    GPIO_Pin_13








#endif
