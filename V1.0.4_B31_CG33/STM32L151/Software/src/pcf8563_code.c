#include "pcf8563_head.h"

/* [CG39] Calendar bounds are shared by editor and RTC access checks. */
uint8_t PCF8563_DaysInMonth(uint8_t year,uint8_t month,uint8_t century)
{
    static const uint8_t days[12]={31,28,31,30,31,30,31,31,30,31,30,31};
    uint16_t full_year=(uint16_t)(2000U+100U*century+year);
    if (month<1U || month>12U || year>99U || century>1U) return 0U;
    if (month==2U && full_year%4U==0U &&
        (full_year%100U!=0U || full_year%400U==0U)) return 29U;
    return days[month-1U];
}

uint8_t PCF8563_TimeIsValid(const PCF8563_TimeTypeDef *time)
{
    uint8_t days;
    if (time==0) return 0U;
    days=PCF8563_DaysInMonth(time->year,time->month,time->century);
    return (days!=0U && time->day>=1U && time->day<=days &&
            time->hour<=23U && time->minute<=59U && time->second<=59U &&
            time->weekday<=6U) ? 1U : 0U;
}

uint8_t PCF8563_Weekday(uint8_t year,uint8_t month,uint8_t day)
{
    uint32_t days=0U;
    uint8_t y,m;
    if (year>99U || month<1U || month>12U || day<1U ||
        day>PCF8563_DaysInMonth(year,month,0U)) return 0U;
    for (y=0U;y<year;++y) days+=(y%4U==0U) ? 366U : 365U;
    for (m=1U;m<month;++m) days+=PCF8563_DaysInMonth(year,m,0U);
    return (uint8_t)((6U+days+day-1U)%7U); /* 2000-01-01 was Saturday. */
}

void PCF8563_NormalizeEdit(uint8_t fields[6])
{
    uint8_t days;
    if (fields==0) return;
    if (fields[0]>99U) fields[0]=0U;
    if (fields[1]<1U || fields[1]>12U) fields[1]=1U;
    days=PCF8563_DaysInMonth(fields[0],fields[1],0U);
    if (fields[2]<1U) fields[2]=1U;
    if (fields[2]>days) fields[2]=days;
    if (fields[3]>23U) fields[3]=0U;
    if (fields[4]>59U) fields[4]=0U;
    if (fields[5]>59U) fields[5]=0U;
}

static uint8_t PCF8563_BCDIsValid(uint8_t bcd)
{
    return ((bcd&0x0FU)<=9U && (bcd>>4)<=9U) ? 1U : 0U;
}




/**
  * @brief  十进制转BCD码
  */
uint8_t DEC_to_BCD(uint8_t dec)
{
    return ((dec / 10) << 4) | (dec % 10);
}

/**
  * @brief  BCD码转十进制
  */
uint8_t BCD_to_DEC(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
  * @brief  初始化PCF8563
  * @param  hi2c: I2C句柄指针
  * @retval HAL状态
  */
uint8_t PCF8563_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t data[2];
    HAL_StatusTypeDef status;
    
    /* 检查设备是否就绪 */
    if (HAL_I2C_IsDeviceReady(hi2c, PCF8563_ADDR_WRITE, 3, 100) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    /* 启动时钟（清除停止位） */
    data[0] = PCF8563_REG_ST1;
    data[1] = 0x00;  // 确保时钟运行
    
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    if (status != HAL_OK)
    {
        return status;
    }
    
    /* 配置时钟输出为关闭 */
    data[0] = PCF8563_REG_CLKOUT;
    data[1] = 0x00;  // 关闭CLKOUT输出
    
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    
    return status;
}

/**
  * @brief  设置时间
  * @param  hi2c: I2C句柄指针
  * @param  time: 时间结构体指针
  * @retval HAL状态
  */
uint8_t PCF8563_SetTime(I2C_HandleTypeDef *hi2c, PCF8563_TimeTypeDef *time)
{
    uint8_t data[8];
    HAL_StatusTypeDef status;
    
    if (hi2c==0 || !PCF8563_TimeIsValid(time)) return HAL_ERROR;

    /* 准备时间数据 */
    data[0] = PCF8563_REG_SC;  // 起始寄存器地址
    
    /* 秒：清除VL位确保时钟运行 */
    data[1] = DEC_to_BCD(time->second) & 0x7F;
    
    /* 分 */
    data[2] = DEC_to_BCD(time->minute) & 0x7F;
    
    /* 时 */
    data[3] = DEC_to_BCD(time->hour) & 0x3F;
    
    /* 日 */
    data[4] = DEC_to_BCD(time->day) & 0x3F;
    
    /* 星期 */
    data[5] = time->weekday & 0x07;
    
    /* 月：设置世纪标志 */
    data[6] = DEC_to_BCD(time->month) & 0x1F;
    if (time->century)
    {
        data[6] |= PCF8563_MO_C;  // 设置世纪标志
    }
    
    /* 年 */
    data[7] = DEC_to_BCD(time->year);
    
    /* 写入时间数据 */
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 8, 100);
    
    return status;
}

/**
  * @brief  读取时间
  * @param  hi2c: I2C句柄指针
  * @param  time: 时间结构体指针
  * @retval HAL状态
  */
uint8_t PCF8563_GetTime(I2C_HandleTypeDef *hi2c, PCF8563_TimeTypeDef *time)
{
    uint8_t data[7],i;
    static const uint8_t masks[7]={0x7F,0x7F,0x3F,0x3F,0x07,0x1F,0xFF};
    uint8_t reg_addr=PCF8563_REG_SC;
    HAL_StatusTypeDef status;
    PCF8563_TimeTypeDef decoded;
    if (hi2c==0 || time==0) return HAL_ERROR;
    status=HAL_I2C_Master_Transmit(hi2c,PCF8563_ADDR_WRITE,&reg_addr,1,5U);
    if (status!=HAL_OK) return status;
    /* NXP: seconds through years must be read in one access. */
    status=HAL_I2C_Master_Receive(hi2c,PCF8563_ADDR_READ,data,7,5U);
    if (status!=HAL_OK) return status;
    if ((data[0]&PCF8563_SC_VL)!=0U) return HAL_ERROR;
    for (i=0U;i<7U;++i)
        if (i!=4U && !PCF8563_BCDIsValid(data[i]&masks[i])) return HAL_ERROR;
    decoded.second=BCD_to_DEC(data[0]&0x7F);
    decoded.minute=BCD_to_DEC(data[1]&0x7F);
    decoded.hour=BCD_to_DEC(data[2]&0x3F);
    decoded.day=BCD_to_DEC(data[3]&0x3F);
    decoded.weekday=data[4]&0x07;
    decoded.month=BCD_to_DEC(data[5]&0x1F);
    decoded.century=(data[5]&PCF8563_MO_C)!=0U ? 1U : 0U;
    decoded.year=BCD_to_DEC(data[6]);
    if (!PCF8563_TimeIsValid(&decoded)) return HAL_ERROR;
    *time=decoded; /* Caller data is untouched on any error. */
    return HAL_OK;
}

/**
  * @brief  设置闹钟
  * @param  hi2c: I2C句柄指针
  * @param  minute: 分钟 (0-59), 0xFF禁用
  * @param  hour: 小时 (0-23), 0xFF禁用
  * @param  day: 日 (1-31), 0xFF禁用
  * @param  weekday: 星期 (0-6), 0xFF禁用
  * @retval HAL状态
  */
uint8_t PCF8563_SetAlarm(I2C_HandleTypeDef *hi2c, uint8_t minute, uint8_t hour, uint8_t day, uint8_t weekday)
{
    uint8_t data[5];
    HAL_StatusTypeDef status;
    
    data[0] = PCF8563_REG_AMN;  // 闹钟起始寄存器
    
    /* 分钟闹钟 */
    data[1] = (minute == 0xFF) ? 0x80 : (DEC_to_BCD(minute) & 0x7F);
    
    /* 小时闹钟 */
    data[2] = (hour == 0xFF) ? 0x80 : (DEC_to_BCD(hour) & 0x3F);
    
    /* 日报警 */
    data[3] = (day == 0xFF) ? 0x80 : (DEC_to_BCD(day) & 0x3F);
    
    /* 星期报警 */
    data[4] = (weekday == 0xFF) ? 0x80 : (weekday & 0x07);
    
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 5, 100);
    
    return status;
}

/**
  * @brief  使能闹钟中断
  * @param  hi2c: I2C句柄指针
  * @retval HAL状态
  */
uint8_t PCF8563_EnableAlarm(I2C_HandleTypeDef *hi2c)
{
    uint8_t data[2];
    HAL_StatusTypeDef status;
    
    /* 读取当前状态寄存器2 */
    data[0] = PCF8563_REG_ST2;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 1, 100);
    if (status != HAL_OK) return status;
    
    status = HAL_I2C_Master_Receive(hi2c, PCF8563_ADDR_READ, data, 1, 100);
    if (status != HAL_OK) return status;
    
    /* 设置AIE位使能闹钟中断，清除AF位 */
    data[1] = (data[0] & ~0x08) | 0x02;  // AF=0, AIE=1
    
    data[0] = PCF8563_REG_ST2;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    
    return status;
}

/**
  * @brief  禁用闹钟
  * @param  hi2c: I2C句柄指针
  * @retval HAL状态
  */
uint8_t PCF8563_DisableAlarm(I2C_HandleTypeDef *hi2c)
{
    uint8_t data[2];
    HAL_StatusTypeDef status;
    
    /* 读取当前状态寄存器2 */
    data[0] = PCF8563_REG_ST2;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 1, 100);
    if (status != HAL_OK) return status;
    
    status = HAL_I2C_Master_Receive(hi2c, PCF8563_ADDR_READ, data, 1, 100);
    if (status != HAL_OK) return status;
    
    /* 清除AIE位禁用闹钟中断 */
    data[1] = data[0] & ~0x02;  // AIE=0
    
    data[0] = PCF8563_REG_ST2;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    
    return status;
}

/**
  * @brief  清除闹钟标志位
  * @param  hi2c: I2C句柄指针
  * @retval HAL状态
  */
uint8_t PCF8563_ClearAlarmFlag(I2C_HandleTypeDef *hi2c)
{
    uint8_t data[2];
    HAL_StatusTypeDef status;
    
    /* 读取当前状态寄存器2 */
    data[0] = PCF8563_REG_ST2;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 1, 100);
    if (status != HAL_OK) return status;
    
    status = HAL_I2C_Master_Receive(hi2c, PCF8563_ADDR_READ, data, 1, 100);
    if (status != HAL_OK) return status;
    
    /* 清除AF位 */
    data[1] = data[0] & ~0x08;  // AF=0
    
    data[0] = PCF8563_REG_ST2;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    
    return status;
}

/**
  * @brief  设置定时器
  * @param  hi2c: I2C句柄指针
  * @param  freq: 定时器频率 (0=4096Hz, 1=64Hz, 2=1Hz, 3=1/60Hz)
  * @param  value: 定时器计数值
  * @retval HAL状态
  */
uint8_t PCF8563_SetTimer(I2C_HandleTypeDef *hi2c, uint8_t freq, uint8_t value)
{
    uint8_t data[3];
    HAL_StatusTypeDef status;
    
    /* 先停止定时器 */
    data[0] = PCF8563_REG_TMRC;
    data[1] = 0x00;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    if (status != HAL_OK) return status;
    
    /* 设置定时器值 */
    data[0] = PCF8563_REG_TMR;
    data[1] = value;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    if (status != HAL_OK) return status;
    
    /* 启动定时器并设置频率 */
    data[0] = PCF8563_REG_TMRC;
    data[1] = 0x80 | (freq & 0x03);  // TE=1, 设置频率
    
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, data, 2, 100);
    
    return status;
}

/**
  * @brief  检查电压状态
  * @param  hi2c: I2C句柄指针
  * @retval 0:电压正常, 1:电压低
  */
uint8_t PCF8563_CheckVoltage(I2C_HandleTypeDef *hi2c)
{
    uint8_t data;
    HAL_StatusTypeDef status;
    
    /* 读取秒寄存器检查VL位 */
    data = PCF8563_REG_SC;
    status = HAL_I2C_Master_Transmit(hi2c, PCF8563_ADDR_WRITE, &data, 1, 100);
    if (status != HAL_OK) return 1;
    
    status = HAL_I2C_Master_Receive(hi2c, PCF8563_ADDR_READ, &data, 1, 100);
    if (status != HAL_OK) return 1;
    
    return (data & PCF8563_SC_VL) ? 1 : 0;
}




