#ifndef __KEY_HEAD_H
#define __KEY_HEAD_H

void Beep_Low_Battery(void);

#include "main.h"

#include "App_oled.h"
#include "param_head.h"






// 状态定义
#define STATE_RELEASED      0
#define STATE_DEBOUNCE_DOWN 1
#define STATE_PRESSED       2
#define STATE_DEBOUNCE_UP   3

// 时间阈值定义 (单位: ms)
#define DEBOUNCE_TIME       15  // 消抖时间
#define LONG_PRESS_TIME     800 // 长按时间
#define HOLD_REPEAT_TIME    200 // 长按保持后，连续触发间隔



void KEY_Init(void) ;

void KEY_Parser_Scan(void) ;


uint8_t Get_Power_Status(void);

void On_Key_Check_Scn(void);
void Change_Key_Check_Scn(void);
void Parser_Key_Check_Scn(void);
void Storage_Key_Check_Scn(void);

void Beep_Reflash(void);
void Beep_Ctrl(uint16_t time,uint16_t cnt);
#endif

