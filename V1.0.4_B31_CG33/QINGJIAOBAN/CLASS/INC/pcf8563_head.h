#ifndef __PCF8563_HEAD_H
#define __PCF8563_HEAD_H


#include "main_head.h"
#include "gpio_head.h"
#include "iic_head.h"


#define PCF8563_Check_Data                       (uint8_t)0x55  //检测用，可用其他数值

#define PCF8563_ADDR                             (uint8_t)0x51 //7位地址
#define PCF8563_Write                            (uint8_t)0xa2  //写命令
#define PCF8563_Read                             (uint8_t)0xa3  //读命令，或者用（PCF8563_Write + 1）

//
//电源复位功能
//
#define PCF8563_PowerResetEnable                 (uint8_t)0x08
#define PCF8563_PowerResetDisable                (uint8_t)0x09

//
//世纪位操作定义
//
#define PCF_Century_SetBitC                      (uint8_t)0x80

#define PCF_Century_19xx                         (uint8_t)0x03
#define PCF_Century_20xx                         (uint8_t)0x04

//
//数据格式
//
#define PCF_Format_BIN                           (uint8_t)0x01
#define PCF_Format_BCD                           (uint8_t)0x02

//
//设置PCF8563模式用
//
#define PCF_Mode_Normal                          (uint8_t)0x05

/******************************************************************************
                             参数寄存器地址宏定义
******************************************************************************/

#define PCF8563_Address_Control_Status_1         (uint8_t)0x00  //控制/状态寄存器1
#define PCF8563_Address_Control_Status_2         (uint8_t)0x01  //控制/状态寄存器2


/******************************************************************************
                               参数屏蔽宏定义
******************************************************************************/

#define PCF8563_Shield_Control_Status_1          (uint8_t)0xa8
#define PCF8563_Shield_Control_Status_2          (uint8_t)0x1f

#define PCF8563_Shield_Seconds                   (uint8_t)0x7f
#define PCF8563_Shield_Minutes                   (uint8_t)0x7f
#define PCF8563_Shield_Hours                     (uint8_t)0x3f

#define PCF8563_Shield_Days                      (uint8_t)0x3f
#define PCF8563_Shield_WeekDays                  (uint8_t)0x07
#define PCF8563_Shield_Months_Century            (uint8_t)0x1f
#define PCF8563_Shield_Years                     (uint8_t)0xff

/******************************************************************************
                                 参数宏定义
******************************************************************************/
#define PCF8563_Address_Years                    (uint8_t)0x08  //年
#define PCF8563_Address_Months                   (uint8_t)0x07  //月
#define PCF8563_Address_Days                     (uint8_t)0x05  //日
#define PCF8563_Address_WeekDays                 (uint8_t)0x06  //星期
#define PCF8563_Address_Hours                    (uint8_t)0x04  //小时
#define PCF8563_Address_Minutes                  (uint8_t)0x03  //分钟
#define PCF8563_Address_Seconds                  (uint8_t)0x02  //秒
//控制/状态寄存器1 --> 0x00
//
#define PCF_Control_Status_NormalMode            (uint8_t)(~(1<<7))  //普通模式
#define PCF_Control_Status_EXT_CLKMode           (uint8_t)(1<<7)     //EXT_CLK测试模式
#define PCF_Control_ChipRuns                     (uint8_t)(~(1<<5))  //芯片运行
#define PCF_Control_ChipStop                     (uint8_t)(1<<5)     //芯片停止运行，所有芯片分频器异步置逻辑0
#define PCF_Control_TestcClose                   (uint8_t)(~(1<<3))  //电源复位功能失效（普通模式时置逻辑0）
#define PCF_Control_TestcOpen                    (uint8_t)(1<<3)     //电源复位功能有效
//秒寄存器 --> 0x02
//
#define PCF_Accuracy_ClockNo                     (uint8_t)(1<<7)     //不保证准确的时钟/日历数据
#define PCF_Accuracy_ClockYes                    (uint8_t)(~(1<<7))  //保证准确的时钟/日历数据



#define PCF_Mode_EXT_CLK                         (unsigned char)0x06
 
#define PCF_Mode_INT_Alarm                       (unsigned char)0x07
#define PCF_Mode_INT_Timer                       (unsigned char)0x08
 
/******************************************************************************
                             参数寄存器地址宏定义
******************************************************************************/
 

 
#define PCF8563_Address_CLKOUT                   (unsigned char)0x0d  //CLKOUT频率寄存器
#define PCF8563_Address_Timer                    (unsigned char)0x0e  //定时器控制寄存器
#define PCF8563_Address_Timer_VAL                (unsigned char)0x0f  //定时器倒计数寄存器
 

 

 
/******************************************************************************
                               参数屏蔽宏定义
******************************************************************************/
 

 
#define PCF8563_Shield_Minute_Alarm              (unsigned char)0x7f
#define PCF8563_Shield_Hour_Alarm                (unsigned char)0x3f
#define PCF8563_Shield_Day_Alarm                 (unsigned char)0x3f
#define PCF8563_Shield_WeekDays_Alarm            (unsigned char)0x07
 
#define PCF8563_Shield_CLKOUT_Frequency          (unsigned char)0x03
#define PCF8563_Shield_Timer_Control             (unsigned char)0x03
#define PCF8563_Shield_Timer_Countdown_Value     (unsigned char)0xff




typedef struct PCF8563_TIME_STR
{
    uint8_t year;
    uint8_t month;
    uint8_t data;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t week;
}TIME_STR;

typedef struct PCF8563_CLASS_STR
{
    IIC_STR  *  iic;
    TIME_STR *  time;
    
    void    (*str_init)(struct PCF8563_CLASS_STR*);
    void    (*init)(struct PCF8563_CLASS_STR*);
    uint8_t (*writeByte)(struct PCF8563_CLASS_STR*,uint8_t,uint8_t);
    uint8_t (*readByte)(struct PCF8563_CLASS_STR*,uint8_t);
    void    (*set_time)(struct PCF8563_CLASS_STR*);
    void    (*read_time)(struct PCF8563_CLASS_STR*);
}PCF8563_STR;

void PCF8563_STR_INIT(PCF8563_STR *pcf8563_str);

#define PCF8563_CLASS_CREATE(_iic,_time)\
{\
    .iic      = _iic,            \
    .time     = _time,           \
    .str_init = PCF8563_STR_INIT,\
}


#define PCF8563_STK_SIZE    120
#define PCF8653_TASK_PRIO   10

void PCF8563_Task_Create(void);

#define PCF8563_SCL_PORT    GPIOB
#define PCF8653_SCL_PIN     GPIO_Pin_10

#define PCF8563_SDA_PORT    GPIOB
#define PCF8653_SDA_PIN     GPIO_Pin_11




#endif


