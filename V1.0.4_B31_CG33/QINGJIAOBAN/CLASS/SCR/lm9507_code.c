#include "lm9507_head.h"

void LM9507_DELAY(uint32_t time)
{
    /* [CG40] Volatile loop state and CMSIS NOP retain the wait even
     * when optimized. The argument counts iterations, not microseconds. */
    volatile uint32_t remaining = time;
    while (remaining > 0U)
    {
        __NOP();
        remaining--;
    }
}

void LM9507_INIT(LM9507_STR *lm9507_str)
{
    lm9507_str->clk->str_init(lm9507_str->clk);
    lm9507_str->cs->str_init(lm9507_str->cs);
    lm9507_str->mosi->str_init(lm9507_str->mosi);
    lm9507_str->miso->str_init(lm9507_str->miso);
}

void LM9507_WRITE(LM9507_STR *lm9507_str,uint16_t data)
{
    for(int16_t i=15;i>=0;i--)
    {
        lm9507_str->mosi->write(lm9507_str->mosi,((data & (1<<i)) ? Bit_SET : Bit_RESET));
        lm9507_str->clk->write(lm9507_str->clk,Bit_SET);
        LM9507_DELAY(10);
        lm9507_str->clk->write(lm9507_str->clk,Bit_RESET);
        LM9507_DELAY(10);
    }
}

uint16_t LM9507_READ(LM9507_STR *lm9507_str)
{
    uint16_t data = 0;
    for(uint8_t i=0;i<16;i++)
    {
        lm9507_str->clk->write(lm9507_str->clk,Bit_SET);
        data<<=1;
        if(lm9507_str->miso->read(lm9507_str->miso) == Bit_SET)
            data+=1;
        lm9507_str->clk->write(lm9507_str->clk,Bit_RESET);
        LM9507_DELAY(10);
    }
    return data;
}

// 读取温度（单位：℃）
float LM95071_READTEMP(LM9507_STR *lm9507_str)
{
    uint16_t raw;
    int16_t temp;
    lm9507_str->cs->write(lm9507_str->cs,Bit_RESET);
    raw = lm9507_str->read(lm9507_str);            // 读取温度数据
    lm9507_str->cs->write(lm9507_str->cs,Bit_SET);
    
    
    // 数据处理（14位有符号数）
    raw >>= 2; // 去除低2位固定值
    temp = (int16_t)(raw << 2); // 符号扩展
    temp >>= 2; // 恢复实际值
    
    return temp * 0.03125f;
}

// 进入关断模式
void LM95071_SHOUTDOWN(LM9507_STR *lm9507_str) 
{
    lm9507_str->cs->write(lm9507_str->cs,Bit_RESET);
    lm9507_str->write(lm9507_str,0x00FF);
    lm9507_str->cs->write(lm9507_str->cs,Bit_SET);
}


void LM9507_STR_INIT(LM9507_STR *lm9507_str)
{
    lm9507_str->init = LM9507_INIT;
    lm9507_str->write = LM9507_WRITE;
    lm9507_str->read = LM9507_READ;
    lm9507_str->readTemp = LM95071_READTEMP;
    lm9507_str->shoutDown = LM95071_SHOUTDOWN;
    lm9507_str->init(lm9507_str);
}




//GPIO_STR   lm9507_cs_gpio = GPIO_CLASS_CREATE(LM9507_CS_PORT,LM9507_CS_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
//GPIO_STR   lm9507_clk_gpio = GPIO_CLASS_CREATE(LM9507_CLK_PORT,LM9507_CLK_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
//GPIO_STR   lm9507_mosi_gpio = GPIO_CLASS_CREATE(LM9507_MOSI_PORT,LM9507_MOSI_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
//GPIO_STR   lm9507_miso_gpio = GPIO_CLASS_CREATE(LM9507_MISO_PORT,LM9507_MISO_PIN,GPIO_Mode_IPU,GPIO_Speed_50MHz);

//LM9507_STR lm9531_str = LM9507_CLASS_STR(&lm9507_clk_gpio,&lm9507_cs_gpio,&lm9507_mosi_gpio,&lm9507_miso_gpio);

//void lm9507_task(void *pvParameters)
//{
//    lm9531_str.str_init(&lm9531_str);
//    while(1)
//    {
//        temperature = lm9531_str.readTemp(&lm9531_str);
//        vTaskDelay(10);
//    }
//}

// 
//void LM9531_Task_Create(void)
//{
//    //任务句柄
//    TaskHandle_t Lm9507Task_Handler;
//        //创建LED1任务
//    xTaskCreate((TaskFunction_t )lm9507_task,
//                (const char*    )"lm9507_task",
//                (uint16_t       )LM9531_STK_SIZE,
//                (void*          )NULL,
//                (UBaseType_t    )LM9531_TASK_PRIO,
//                (TaskHandle_t*  )&Lm9507Task_Handler);
//}















