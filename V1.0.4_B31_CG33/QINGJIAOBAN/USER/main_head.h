#ifndef __MAIN_HEAD_H
#define __MAIN_HEAD_H


#include "stm32f10x.h"


#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "math.h"

#include "data_head.h"

#define VERSION "CG34_TEST"
#define SENSOR_SAMPLE_PERIOD_MS 20U
#define SENSOR_SEND_PERIOD_MS   100U /* 10 Hz, independent of ACK. */
extern volatile uint32_t sensor_tick_ms;





//�������ȼ�
#define START_TASK_PRIO     1
//�����ջ��С
#define START_STK_SIZE      128
//������
void start_task(void *pvParameters);





void user_task_init(void);




#define LOGO_PAGE_ID                0
#define MAIN_PAGE_ID                1



#define SHORTCUT_MENU_PAGE_ID       2
#define SHORTCUT_MENU_NUM           2

#define CHECKUP_PAGE_ID             3

#define MUBIAO_PAGE_ID              4







#endif


