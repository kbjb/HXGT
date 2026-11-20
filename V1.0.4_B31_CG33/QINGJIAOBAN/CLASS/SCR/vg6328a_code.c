#include "vg6328a_head.h"


void VGA6328_INIT(VG6328A_STR *vga6328_str)
{
    xSemaphoreGive(vga6328_str->usart_str->work_en);
}
uint8_t VGA6328_SEND_SET_CMD(VG6328A_STR *vga6328_str,char *cmd,uint8_t len,uint8_t ack_en)
{
    uint8_t res = 1;
    vga6328_str->usart_str->send_string(vga6328_str->usart_str,(char*)cmd,len);
    if(ack_en == 1)
    {
        if(xSemaphoreTake(vga6328_str->usart_str->rx_ok,500) == pdTRUE)
        {
            vga6328_str->rx_len = vga6328_str->usart_str->rx_len;
            for(uint8_t i=0;i<vga6328_str->rx_len;i++)
                vga6328_str->rx_buffer[i] = vga6328_str->usart_str->rx_buffer[i];
            if(vga6328_str->rx_buffer[0] == 'O'&&
               vga6328_str->rx_buffer[1] == 'K'&&
               vga6328_str->rx_buffer[2] == '\r'&&
               vga6328_str->rx_buffer[3] == '\n')
            {
                res = 0;
            }
        }
    }
    return res;
}

void VGA6328A_STR_INIT(VG6328A_STR *vga6328_str)
{
    vga6328_str->rx_buffer = (uint8_t*)malloc(sizeof(uint8_t)*VG6328A_RX_LEN);
    vga6328_str->tx_buffer = (uint8_t*)malloc(sizeof(uint8_t)*VG6328A_TX_LEN);
    vga6328_str->init = VGA6328_INIT;
    vga6328_str->send_set_cmd = VGA6328_SEND_SET_CMD;
    vga6328_str->init(vga6328_str);
}

extern USATR_STR usart2_str;
VG6328A_STR vga6328_str = VG6283A_CLASS_CREATE(&usart2_str);

void vga6328a_task(void *pvParameters)
{
    VG6328A_MODE last_mode = other;
    vga6328_str.str_init(&vga6328_str);
    while(1)
    {
        if(last_mode != vga6328_str.mode)
        {
            if(vga6328_str.mode == cmd_mode)
                vga6328_str.send_set_cmd(&vga6328_str,AT_ENAT,9,1);
            else 
                vga6328_str.send_set_cmd(&vga6328_str,AT_EXAT,9,1);
            last_mode = vga6328_str.mode;
        }
        if(vga6328_str.mode == data_receive_mode)
        {
            if(xSemaphoreTake(vga6328_str.usart_str->rx_ok,portMAX_DELAY) == pdTRUE)
            {
                vga6328_str.rx_len = vga6328_str.usart_str->rx_len;
                for(uint8_t i=0;i<vga6328_str.rx_len;i++)
                    vga6328_str.rx_buffer[i] = vga6328_str.usart_str->rx_buffer[i];
                vga6328_str.usart_str->rx_len = 0;
            }
        }
        else if(vga6328_str.mode == data_send_mode)
        {
            vga6328_str.usart_str->send_string(vga6328_str.usart_str,(char*)vga6328_str.tx_buffer,vga6328_str.tx_len);
            vTaskDelay(1000);
        }
        else if(vga6328_str.mode == cmd_mode)
        {
            
        }
    }
}

 
void VGA6328a_Task_Create(void)
{
    //任务句柄
    TaskHandle_t VG6328A_Task_Handler;
        //创建LED1任务
    xTaskCreate((TaskFunction_t )vga6328a_task,
                (const char*    )"vga6328a_task",
                (uint16_t       )VGA6328A_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )VGA6328A_TASK_PRIO,
                (TaskHandle_t*  )&VG6328A_Task_Handler);
}

