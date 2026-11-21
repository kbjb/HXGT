#ifndef __MAIN_HEAD_H
#define __MAIN_HEAD_H


#include "stm32f10x.h"


#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "math.h"

#include "data_head.h"

#define VERSION "1.0.2" /* [B2] UART protocol reliability update. */
#define SENSOR_SEND_PERIOD_MS 100U /* 10 Hz reports, independent of ACK. Not the ADC sample period. */





//任务优先级
#define START_TASK_PRIO     1
//任务堆栈大小
#define START_STK_SIZE      128
//任务函数
void start_task(void *pvParameters);





void user_task_init(void);




#define LOGO_PAGE_ID                0
#define MAIN_PAGE_ID                1



#define SHORTCUT_MENU_PAGE_ID       2
#define SHORTCUT_MENU_NUM           2

#define CHECKUP_PAGE_ID             3

#define MUBIAO_PAGE_ID              4







#endif


