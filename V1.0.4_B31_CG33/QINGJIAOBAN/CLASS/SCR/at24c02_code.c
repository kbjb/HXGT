#include "at24c02_head.h"


void AT24C02_INIT(AT24C02_STR * at24c02_str)
{
    at24c02_str->iic->str_init(at24c02_str->iic);
    at24c02_str->used_memory = at24c02_str->readbyte(at24c02_str,0);//读取at24c02内存使用情况
}


uint8_t AT24C02_READ_ONE_BYTE(AT24C02_STR * at24c02_str,uint8_t addr)
{
    uint8_t data;
    at24c02_str->iic->start(at24c02_str->iic);
    at24c02_str->iic->send_byte(at24c02_str->iic,0xA0);//写数据设备地址
    at24c02_str->iic->wait_ack(at24c02_str->iic);
    at24c02_str->iic->send_byte(at24c02_str->iic,addr);
    at24c02_str->iic->wait_ack(at24c02_str->iic);
    
    at24c02_str->iic->start(at24c02_str->iic);
    at24c02_str->iic->send_byte(at24c02_str->iic,0xA1);//读数据设备地址
    at24c02_str->iic->wait_ack(at24c02_str->iic);
    data = at24c02_str->iic->read_byte(at24c02_str->iic);
    at24c02_str->iic->stop(at24c02_str->iic);
    return data;
}
uint8_t AT24C02_WRITE_ONE_BYTE(AT24C02_STR * at24c02_str,uint8_t addr,uint8_t data)
{
    uint8_t res;
    at24c02_str->iic->start(at24c02_str->iic);
    at24c02_str->iic->send_byte(at24c02_str->iic,0xA0);//写数据设备地址
    res = at24c02_str->iic->wait_ack(at24c02_str->iic);
    at24c02_str->iic->send_byte(at24c02_str->iic,addr);//发送地址
    res = at24c02_str->iic->wait_ack(at24c02_str->iic);
    at24c02_str->iic->send_byte(at24c02_str->iic,data);
    res = at24c02_str->iic->wait_ack(at24c02_str->iic);
    at24c02_str->iic->stop(at24c02_str->iic);
    return res;
}
uint8_t AT24C02_CHECK(AT24C02_STR * at24c02_str)
{
    uint8_t temp = at24c02_str->readbyte(at24c02_str,AT24C02_CHECK_ADDR);
    if(temp == AT24C02_CHECK_VALUE)
    {
        at24c02_str->used_memory = at24c02_str->readbyte(at24c02_str,AT24C02_INFO_ADDR);//获取当前内存使用情况
        return 0;
    }
    else
    {
        //第一次使用
        at24c02_str->writebyte(at24c02_str,AT24C02_CHECK_ADDR,AT24C02_CHECK_VALUE);
        temp = at24c02_str->readbyte(at24c02_str,AT24C02_CHECK_ADDR);
        if(temp == AT24C02_CHECK_VALUE)
        {
            at24c02_str->writebyte(at24c02_str,AT24C02_INFO_ADDR,0);
            at24c02_str->used_memory = 0;
            return 0;
        }
    }
    return 1;
}

uint8_t AT24C02_READ(AT24C02_STR * at24c02_str,uint8_t addr,uint8_t len,uint8_t*datap)
{
    uint8_t res;
    if(addr<2)
        res = 1;
    if((addr+len)>255)
        return res = 1;
    for(uint8_t i=0;i<len;i++)
        datap[i] = at24c02_str->readbyte(at24c02_str,addr+i);
    res = 0;
    return res;
}

uint8_t AT24C02_WRITE(AT24C02_STR * at24c02_str,uint8_t addr,uint8_t len,uint8_t*datap)
{
    if(addr<2)
        return 1;
    if((addr+len)>255)
        return 1;
    for(uint8_t i=0;i<len;i++)
        if(at24c02_str->writebyte(at24c02_str,addr+i,datap[i]) == 1)return 1;
    return 0;
}

void AT24C02_STR_INIT(AT24C02_STR * at24c02_str)
{
    at24c02_str->init = AT24C02_INIT;
    at24c02_str->check = AT24C02_CHECK;
    at24c02_str->readbyte = AT24C02_READ_ONE_BYTE;
    at24c02_str->writebyte = AT24C02_WRITE_ONE_BYTE;
    at24c02_str->read = AT24C02_READ;
    at24c02_str->write = AT24C02_WRITE;
    at24c02_str->init(at24c02_str);
}



/*************************************************************OS*****************************************************/


GPIO_STR at24c02_scl_str   = GPIO_CLASS_CREATE(AT24C02_IIC_SCL_PORT,AT24C02_IIC_SCL_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz); 
GPIO_STR at24c02_sda_str   = GPIO_CLASS_CREATE(AT24C02_IIC_SDA_PORT,AT24C02_IIC_SDA_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);

IIC_STR  at24c02_iic_str   = IIC_CLASS_CREATE(&at24c02_scl_str,&at24c02_sda_str,1000,NULL);

AT24C02_STR at24c02_str    = AT24C02_CLASS_CREATE(&at24c02_iic_str,254,AT24C02_ADDR);
 uint16_t t;
void AT24C02_task(void *pvParameters)
{   
    at24c02_str.str_init(&at24c02_str);
    at24c02_str.check(&at24c02_str);
    while(1)
    {
         vTaskDelay(1000);
    }
}

 
void AT24C02_Task_Create(void)
{
    //任务句柄
    TaskHandle_t AT24C02_Handler;
        //创建LED1任务
    xTaskCreate((TaskFunction_t )AT24C02_task,
                (const char*    )"AT24C02_task",
                (uint16_t       )AT24C02_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )AT24C02_TASK_PRO,
                (TaskHandle_t*  )&AT24C02_Handler);
}




