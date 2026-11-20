#include "key_head.h"

static uint32_t Beep_Last_Time  = 0;
static uint16_t Beep_Touch_Num  = 0;
static uint16_t Beep_Touch_Time = 0;
static uint16_t Beep_Touch_Cnt  = 0;

void Beep_Reflash(void)
{
    uint32_t Beep_Current_Time = HAL_GetTick();
    if(Beep_Current_Time - Beep_Last_Time < 5)
        return;
    Beep_Last_Time = Beep_Current_Time;
    if(Beep_Touch_Num)
    {
        Beep_Touch_Cnt++;
        if((Beep_Touch_Cnt*5)<Beep_Touch_Time)
            HAL_GPIO_WritePin(Beep_Ctrl_GPIO_Port,Beep_Ctrl_Pin,GPIO_PIN_SET);
        else if((Beep_Touch_Cnt*5)>=Beep_Touch_Time)
        {
            HAL_GPIO_WritePin(Beep_Ctrl_GPIO_Port,Beep_Ctrl_Pin,GPIO_PIN_RESET);
            if((Beep_Touch_Cnt*5)>(Beep_Touch_Time*2))
            {
                Beep_Touch_Cnt = 0;
                Beep_Touch_Num--;
            }
        }
    }
}




void Beep_Ctrl(uint16_t time,uint16_t cnt)
{
    Beep_Touch_Time = time;
    Beep_Touch_Num  = cnt;
}



void Key_Fun_Select(KEY_ID Key_id,KEY_STATUS Key_Status)
{
    PAGE_ID page_id = get_PageId();
    for(uint8_t i=0;i<Key_Funs_Number;i++)
    {
        if(page_id == Key_Funs[i].page_id || Key_Funs[i].page_id == page_size)
        {
            if(Key_id == Key_Funs[i].Key_id && Key_Status == Key_Funs[i].key_status)
            {
                if(Key_Funs[i].Key_Fun!= NULL)
                {
                    Key_Funs[i].Key_Fun();
                }
            }
        }
    }
}




uint32_t On_Key_Cnt = 0;
uint32_t On_Key_Current_Time = 0,On_Key_Last_Time = 0;
uint8_t  On_Key_Long_Flag = 0;
void On_Key_Check_Scn(void)
{
    
    uint32_t On_Key_Current_Time = HAL_GetTick();
    // 10ms扫描一次
    if (On_Key_Current_Time - On_Key_Last_Time < 10) return;
    On_Key_Last_Time = On_Key_Current_Time;
    
    if(HAL_GPIO_ReadPin(On_Check_GPIO_Port,On_Check_Pin) == GPIO_PIN_RESET)
    {
        On_Key_Cnt++;
        if(On_Key_Cnt == 100)
        {
            //长按操作
            On_Key_Long_Flag = 1;
            Key_Fun_Select(KEY_ID_ON,KEY_STATUS_LONG );
            On_Key_Cnt = 0;
        }
    }
    else
    {
        if(On_Key_Cnt>=10 && On_Key_Cnt<100)
        {
            //短按操作
            if(On_Key_Long_Flag == 0)
                Key_Fun_Select(KEY_ID_ON,KEY_STATUS_SHORT );
        }
        On_Key_Long_Flag = 0;
        On_Key_Cnt = 0;
    }
}


uint32_t Change_Key_Cnt = 0;
uint32_t Change_Key_Current_Time = 0,Change_Key_Last_Time = 0;
uint8_t  Change_Key_Long_Flag = 0;
void Change_Key_Check_Scn(void)
{
    
    uint32_t Change_Key_Current_Time = HAL_GetTick();
    // 10ms扫描一次
    if (Change_Key_Current_Time - Change_Key_Last_Time < 10) return;
    Change_Key_Last_Time = Change_Key_Current_Time;
    
    if(HAL_GPIO_ReadPin(LR_Check_GPIO_Port,LR_Check_Pin) == GPIO_PIN_RESET)
    {
        Change_Key_Cnt++;
        if(Change_Key_Cnt == 10)
        {
            Key_Fun_Select(KEY_ID_CHANGE,KEY_STATUS_SHORT );
        }
    }
    else
    {
        Change_Key_Cnt = 0;
    }
}




uint8_t Cancel_Key_Cnt  = 0;
uint8_t Set_Key_Cnt     = 0;
uint8_t Left_Key_Cnt    = 0;
uint8_t Right_Key_Cnt   = 0;








static uint8_t Parser_State = 0;
uint32_t Parser_Key_Current_Time = 0,Parser_Key_Last_Time = 0;
void Parser_Key_Check_Scn(void)
{
    Parser_Key_Current_Time = HAL_GetTick();
    if(Parser_Key_Current_Time-Parser_Key_Last_Time < 2)
        return;
    switch(Parser_State)
    {
        case 0:
            HAL_GPIO_WritePin(Width1_Check_GPIO_Port,Width1_Check_Pin,GPIO_PIN_SET);
            HAL_GPIO_WritePin(Width2_Check_GPIO_Port,Width2_Check_Pin,GPIO_PIN_RESET);
            Parser_State = 1;
            break;
        case 1:
            if(HAL_GPIO_ReadPin(High1_Check_GPIO_Port,High1_Check_Pin) == GPIO_PIN_SET)
            {
                Cancel_Key_Cnt++;
                if(Cancel_Key_Cnt==10)
                    Key_Fun_Select(KEY_ID_CANCEL,KEY_STATUS_SHORT);
            }
            else
            {
                Cancel_Key_Cnt = 0;
            }
            if(HAL_GPIO_ReadPin(High2_check_GPIO_Port,High2_check_Pin) == GPIO_PIN_SET)
            {
                Set_Key_Cnt++;
                if(Set_Key_Cnt==10)
                    Key_Fun_Select(KEY_ID_SET,KEY_STATUS_SHORT);
            }
            else
            {
                Set_Key_Cnt = 0;
            }
            Parser_State = 2;
            break;
        case 2:
            HAL_GPIO_WritePin(Width1_Check_GPIO_Port,Width1_Check_Pin,GPIO_PIN_RESET);
            HAL_GPIO_WritePin(Width2_Check_GPIO_Port,Width2_Check_Pin,GPIO_PIN_SET);
            Parser_State = 3;
            break;
        case 3:
            if(HAL_GPIO_ReadPin(High1_Check_GPIO_Port,High1_Check_Pin) == GPIO_PIN_SET)
            {
                Left_Key_Cnt++;
                if(Left_Key_Cnt==10)
                    Key_Fun_Select(KEY_ID_LEFT,KEY_STATUS_SHORT);
            }
            else
            {
                Left_Key_Cnt = 0;
            }
            if(HAL_GPIO_ReadPin(High2_check_GPIO_Port,High2_check_Pin) == GPIO_PIN_SET)
            {
                Right_Key_Cnt++;
                if(Right_Key_Cnt==10)
                    Key_Fun_Select(KEY_ID_RIGHT,KEY_STATUS_SHORT);
            }
            else
            {
                Right_Key_Cnt = 0;
            }
            Parser_State = 0;
            break;
    }
    Parser_Key_Last_Time = Parser_Key_Current_Time;
}

uint32_t Storage_Key_Cnt = 0;
uint32_t Storage_Key_Current_Time = 0,Storage_Key_Last_Time = 0;
uint8_t  Storage_Key_Long_Flag = 0;
void Storage_Key_Check_Scn(void)
{
    uint32_t Storage_Key_Current_Time = HAL_GetTick();
    // 10ms扫描一次
    if (Storage_Key_Current_Time - Storage_Key_Last_Time < 10) return;
    Storage_Key_Last_Time = Storage_Key_Current_Time;
    
    if(HAL_GPIO_ReadPin(Mem_Check_GPIO_Port,Mem_Check_Pin) == GPIO_PIN_RESET)
    {
        Storage_Key_Cnt++;
        if(Storage_Key_Cnt == 100)
        {
            //长按操作
            Storage_Key_Long_Flag = 1;
            Key_Fun_Select(KEY_ID_STORAGE,KEY_STATUS_LONG );
            Storage_Key_Cnt = 0;
        }
    }
    else
    {
        if(Storage_Key_Cnt>=10 && Storage_Key_Cnt<100)
        {
            //短按操作
            if(Storage_Key_Long_Flag == 0)
                Key_Fun_Select(KEY_ID_STORAGE,KEY_STATUS_SHORT );
        }
        Storage_Key_Long_Flag = 0;
        Storage_Key_Cnt = 0;
    }
}













