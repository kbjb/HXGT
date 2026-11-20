#ifndef __DATA_HEAD_H
#define __DATA_HEAD_H

#include "main_head.h"


#ifndef   GLOBAL_PARAM_DEFINE

#define   GLOBAL_PARAM_DEFINE
    extern float    temperature;
    extern float    angle;
    extern float    ruler; 
    extern volatile uint8_t  send_en;
    extern volatile uint16_t send_cnt;
#endif

#define POOLS_PAGE_ID_ADDR      0
#define POOLS_PAGE_ID_LEN       2
#define JU_DATA_ZHENG_ADDR      POOLS_PAGE_ID_ADDR+POOLS_PAGE_ID_LEN
#define JU_DATA_ZHENG_LEN       1
#define JU_DATA_FU_ADDR         JU_DATA_ZHENG_ADDR+JU_DATA_ZHENG_LEN
#define JU_DATA_FU_LEN          1
#define PING_DATA_ZHENG_ADDR    JU_DATA_FU_ADDR+JU_DATA_FU_LEN
#define PING_DATA_ZHENG_LEN     1
#define PING_DATA_FU_ADDR       PING_DATA_ZHENG_ADDR+PING_DATA_ZHENG_LEN
#define PING_DATA_FU_LEN        1
#define POWER_CHARGE_ADDR       PING_DATA_FU_ADDR+PING_DATA_FU_LEN
#define POWER_CHARGE_LEN        1

#define SHORTCUT_MENU_ID_ADDR   POWER_CHARGE_ADDR+POWER_CHARGE_LEN
#define SHORTCUT_MENU_ID_LEN    1

#define MUBIAO_ONE_ADDR         SHORTCUT_MENU_ID_ADDR+SHORTCUT_MENU_ID_LEN
#define MUBIAO_ONE_LEN          1
#define MUBIAO_TWO_ADDR         MUBIAO_ONE_ADDR+MUBIAO_ONE_LEN
#define MUBIAO_TWO_LEN          1
#define MUBIAO_THR_ADDR         MUBIAO_TWO_ADDR+MUBIAO_TWO_LEN
#define MUBIAO_THR_LEN          1
#define MUBIAO_FOU_ADDR         MUBIAO_THR_ADDR+MUBIAO_THR_LEN
#define MUBIAO_FOU_LEN          1
#define MUBIAO_FIV_ADDR         MUBIAO_FOU_ADDR+MUBIAO_FOU_LEN
#define MUBIAO_FIV_LEN          1
#define MUBIAO_SIX_ADDR         MUBIAO_FIV_ADDR+MUBIAO_FIV_LEN
#define MUBIAO_SIX_LEN          1
#define MUBIAO_SEV_ADDR         MUBIAO_SIX_ADDR+MUBIAO_SIX_LEN
#define MUBIAO_SEV_LEN          1
#define MUBIAO_EIG_ADDR         MUBIAO_SEV_ADDR+MUBIAO_SEV_LEN
#define MUBIAO_EIG_LEN          1
#define MUBIAO_NIN_ADDR         MUBIAO_EIG_ADDR+MUBIAO_EIG_LEN
#define MUBIAO_NIN_LEN          1
#define MUBIAO_SELECT_ADDR      MUBIAO_NIN_ADDR+MUBIAO_NIN_LEN
#define MUBIAO_SELECT_LEN       1


#define GUIJUCHI_DATA_ADDR      0
#define GUIJUCHI_DATA_LEN       2


void data_pools_write(uint8_t addr,uint8_t len,void *data);

void data_pools_read(uint8_t addr,uint8_t len,void *data);



// IWDG配置参数
#define LSI_FREQ                        40000   // LSI典型频率40kHz（实际需校准）
#define PRESCALER                       IWDG_Prescaler_256
#define RELOAD_VALUE                    (1000 * LSI_FREQ) / (256 * 1000)  // 计算1秒超时



#endif

