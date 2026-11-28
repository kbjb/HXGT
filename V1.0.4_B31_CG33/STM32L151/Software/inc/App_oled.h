#ifndef __APP_OLED_H
#define __APP_OLED_H

#include "main.h"
#include "oled_head.h"
#include "oledfont.h"
#include "param_head.h"
#include "key_head.h"



typedef struct 
{
    void (*init)(void);
    void (*refresh)(void);
}PAGE_FUN_T;




void Page_init(void);
void Measurement_Power_Session(void);
void Measurement_Battery_Service(void);
void oled_Reflash(void);








#endif

