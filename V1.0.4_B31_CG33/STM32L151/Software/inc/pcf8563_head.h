#ifndef __PCF8563_HEAD_H
#define __PCF8563_HEAD_H

#include "main.h"

/* PCF8563设备地址 */
#define PCF8563_ADDR_WRITE    0xA2
#define PCF8563_ADDR_READ     0xA3

/* PCF8563寄存器地址 */
#define PCF8563_REG_ST1      0x00  // 状态寄存器1
#define PCF8563_REG_ST2      0x01  // 状态寄存器2
#define PCF8563_REG_SC       0x02  // 秒
#define PCF8563_REG_MN       0x03  // 分
#define PCF8563_REG_HR       0x04  // 时
#define PCF8563_REG_DM       0x05  // 日
#define PCF8563_REG_DW       0x06  // 星期
#define PCF8563_REG_MO       0x07  // 月
#define PCF8563_REG_YR       0x08  // 年
#define PCF8563_REG_AMN      0x09  // 分钟报警
#define PCF8563_REG_AHR      0x0A  // 小时报警
#define PCF8563_REG_ADM      0x0B  // 日报警
#define PCF8563_REG_ADW      0x0C  // 星期报警
#define PCF8563_REG_CLKOUT   0x0D  // 时钟输出
#define PCF8563_REG_TMRC     0x0E  // 定时器控制
#define PCF8563_REG_TMR      0x0F  // 定时器

/* 状态位定义 */
#define PCF8563_SC_VL        0x80  // 电压低标志
#define PCF8563_MO_C         0x80  // 世纪标志

/* 时间结构体 */
typedef struct {
    uint8_t year;    // 年 (00-99)
    uint8_t month;   // 月 (1-12)
    uint8_t day;     // 日 (1-31)
    uint8_t weekday; // 星期 (0-6, 0=星期日)
    uint8_t hour;    // 时 (0-23)
    uint8_t minute; // 分 (0-59)
    uint8_t second; // 秒 (0-59)
    uint8_t century;// 世纪 (0=20xx, 1=21xx)
} PCF8563_TimeTypeDef;




/* 函数声明 */
/* [CG39] Application century convention: 0=2000..2099, 1=2100..2199.
 * Editing on the six-field page is explicitly restricted to 20xx. */
uint8_t PCF8563_DaysInMonth(uint8_t year,uint8_t month,uint8_t century);
uint8_t PCF8563_TimeIsValid(const PCF8563_TimeTypeDef *time);
uint8_t PCF8563_Weekday(uint8_t year,uint8_t month,uint8_t day);
void PCF8563_NormalizeEdit(uint8_t fields[6]);

uint8_t PCF8563_Init(I2C_HandleTypeDef *hi2c);
uint8_t PCF8563_SetTime(I2C_HandleTypeDef *hi2c, PCF8563_TimeTypeDef *time);
uint8_t PCF8563_GetTime(I2C_HandleTypeDef *hi2c, PCF8563_TimeTypeDef *time);
uint8_t PCF8563_SetAlarm(I2C_HandleTypeDef *hi2c, uint8_t minute, uint8_t hour, uint8_t day, uint8_t weekday);
uint8_t PCF8563_EnableAlarm(I2C_HandleTypeDef *hi2c);
uint8_t PCF8563_DisableAlarm(I2C_HandleTypeDef *hi2c);
uint8_t PCF8563_ClearAlarmFlag(I2C_HandleTypeDef *hi2c);
uint8_t PCF8563_SetTimer(I2C_HandleTypeDef *hi2c, uint8_t freq, uint8_t value);
uint8_t PCF8563_EnableTimer(I2C_HandleTypeDef *hi2c);
uint8_t PCF8563_CheckVoltage(I2C_HandleTypeDef *hi2c);

/* 工具函数 */
uint8_t DEC_to_BCD(uint8_t dec);
uint8_t BCD_to_DEC(uint8_t bcd);



#endif

