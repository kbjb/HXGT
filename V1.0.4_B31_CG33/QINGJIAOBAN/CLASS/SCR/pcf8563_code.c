#include "pcf8563_head.h"



uint8_t RTC_BinToBcd2(uint8_t BINValue)
{
    uint8_t bcdhigh = 0;
    while (BINValue >= 10)
    {
        bcdhigh++;
        BINValue -= 10;
    }
    return ((uint8_t)(bcdhigh << 4) | BINValue);
}


uint8_t RTC_Bcd2ToBin(uint8_t BCDValue)
{
    uint8_t tmp = 0;
    tmp = ((uint8_t)(BCDValue & (uint8_t)0xF0) >> (uint8_t)0x04) * 10;
    return (tmp + (BCDValue & (uint8_t)0x0F));
}

uint8_t PCF8563_Write_Byte(PCF8563_STR *pcf8563_str,uint8_t addr,uint8_t data)
{
    pcf8563_str->iic->start(pcf8563_str->iic);
    pcf8563_str->iic->send_byte(pcf8563_str->iic,PCF8563_Write);//选择写寄存器
    if(pcf8563_str->iic->wait_ack(pcf8563_str->iic))
    {
        pcf8563_str->iic->stop(pcf8563_str->iic);
        return 1;
    }
    pcf8563_str->iic->send_byte(pcf8563_str->iic,addr);//写入寄存器地址
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    
    pcf8563_str->iic->send_byte(pcf8563_str->iic,data);//写数据
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    
    pcf8563_str->iic->stop(pcf8563_str->iic);
    return 0;
}

uint8_t PCF8563_Read_Byte(PCF8563_STR *pcf8563_str,uint8_t addr)
{
    uint8_t data;
    pcf8563_str->iic->start(pcf8563_str->iic);
    pcf8563_str->iic->send_byte(pcf8563_str->iic,PCF8563_Write);
    if(pcf8563_str->iic->wait_ack(pcf8563_str->iic))
    {
        pcf8563_str->iic->stop(pcf8563_str->iic);
        return 0;
    }
    pcf8563_str->iic->send_byte(pcf8563_str->iic,addr);//写入操作地址
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    
    pcf8563_str->iic->stop(pcf8563_str->iic); 
    pcf8563_str->iic->start(pcf8563_str->iic);//重启总线
    
    pcf8563_str->iic->send_byte(pcf8563_str->iic,PCF8563_Read);//发送读命令
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    data = pcf8563_str->iic->read_byte(pcf8563_str->iic); //读数据
    pcf8563_str->iic->nack(pcf8563_str->iic);
    pcf8563_str->iic->stop(pcf8563_str->iic);
    return data;
}

void PCF8563_WRITE_N_Byte(PCF8563_STR *pcf8563_str,uint8_t addr,uint8_t len,uint8_t *datap)
{
    pcf8563_str->iic->start(pcf8563_str->iic);
    pcf8563_str->iic->send_byte(pcf8563_str->iic,PCF8563_Write);
    if(pcf8563_str->iic->wait_ack(pcf8563_str->iic))
    {
        pcf8563_str->iic->stop(pcf8563_str->iic);
        return ;
    }
    pcf8563_str->iic->send_byte(pcf8563_str->iic,addr);//写入操作地址
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    for(uint8_t i=0;i<len;i++)
    {
        pcf8563_str->iic->send_byte(pcf8563_str->iic,datap[i]);
        pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    }
    pcf8563_str->iic->stop(pcf8563_str->iic);
}

void PCF8563_READ_N_Byte(PCF8563_STR *pcf8563_str,uint8_t addr,uint8_t len,uint8_t *datap)
{
    pcf8563_str->iic->start(pcf8563_str->iic);//启动IIC
    pcf8563_str->iic->send_byte(pcf8563_str->iic,PCF8563_Write);//选择写设备
    if(pcf8563_str->iic->wait_ack(pcf8563_str->iic))//等待应答
    {
        pcf8563_str->iic->stop(pcf8563_str->iic);
        return ;
    }
    pcf8563_str->iic->send_byte(pcf8563_str->iic,addr);//发送要读的地址，并接受应答
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    
    pcf8563_str->iic->start(pcf8563_str->iic);
    pcf8563_str->iic->send_byte(pcf8563_str->iic,PCF8563_Read);
    pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    
    for(uint8_t i=0;i<len;i++)
    {
        datap[i] = pcf8563_str->iic->read_byte(pcf8563_str->iic);
        pcf8563_str->iic->wait_ack(pcf8563_str->iic);
    }
    pcf8563_str->iic->stop(pcf8563_str->iic);
}

void PCF8563_START(PCF8563_STR *pcf8563_str)
{
    uint8_t temp = 0;
    temp = pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Control_Status_1);//读取控制/状态寄存器
    if(temp & PCF_Control_ChipStop)
        temp &= PCF_Control_ChipRuns;
    if((temp& (1<<7)) == 0)//普通模式
        temp &= PCF_Control_TestcClose;//电源复位模式失效
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Control_Status_1,temp);
}




void PCF8563_SET_TIME(PCF8563_STR *pcf8563_str)
{
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Seconds,RTC_BinToBcd2(pcf8563_str->time->sec)&0x7F);
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Minutes,RTC_BinToBcd2(pcf8563_str->time->min));
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Hours,RTC_BinToBcd2(pcf8563_str->time->hour));
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Days,RTC_BinToBcd2(pcf8563_str->time->data));
//    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_WeekDays,RTC_BinToBcd2(pcf8563_str->time->week));
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Months,RTC_BinToBcd2(pcf8563_str->time->month &= ~PCF_Century_SetBitC));
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Years,RTC_BinToBcd2(pcf8563_str->time->year));
}

void PCF8563_READ_TIME(PCF8563_STR *pcf8563_str)
{
    pcf8563_str->time->sec   =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Seconds)&PCF8563_Shield_Seconds);
    pcf8563_str->time->min   =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Minutes)&PCF8563_Shield_Minutes);
    pcf8563_str->time->hour  =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Hours)&PCF8563_Shield_Hours);
    pcf8563_str->time->data  =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Days)&PCF8563_Shield_Days);
//    pcf8563_str->time->week  =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_WeekDays));
    pcf8563_str->time->month =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Months)&PCF8563_Shield_Months_Century);
    pcf8563_str->time->year  =  RTC_Bcd2ToBin(pcf8563_str->readByte(pcf8563_str,PCF8563_Address_Years)&PCF8563_Shield_Years);
}

void PCF8563_INIT(PCF8563_STR *pcf8563_str)
{
    pcf8563_str->iic->str_init(pcf8563_str->iic);
    
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Control_Status_1,0);
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_Control_Status_2,0);
    pcf8563_str->writeByte(pcf8563_str,PCF8563_Address_CLKOUT,0);
    
}

void PCF8563_STR_INIT(PCF8563_STR *pcf8563_str)
{
    pcf8563_str->init = PCF8563_INIT;
    pcf8563_str->writeByte = PCF8563_Write_Byte;
    pcf8563_str->readByte  = PCF8563_Read_Byte;
    pcf8563_str->set_time  = PCF8563_SET_TIME;
    pcf8563_str->read_time = PCF8563_READ_TIME;
    pcf8563_str->init(pcf8563_str);
}




GPIO_STR pcf8563_scl = GPIO_CLASS_CREATE(PCF8563_SCL_PORT,PCF8653_SCL_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
GPIO_STR pcf8563_sda = GPIO_CLASS_CREATE(PCF8563_SDA_PORT,PCF8653_SDA_PIN,GPIO_Mode_Out_PP,GPIO_Speed_50MHz);
IIC_STR  pcf8563_iic = IIC_CLASS_CREATE(&pcf8563_scl,&pcf8563_sda,100,NULL);
TIME_STR real_time = {25,12,12,12,12,12,6};


PCF8563_STR pcf8563_str = PCF8563_CLASS_CREATE(&pcf8563_iic,&real_time);



void PCF8563_task(void *pvParameters)
{   
    pcf8563_str.str_init(&pcf8563_str);
    pcf8563_str.set_time(&pcf8563_str);
    while(1)
    {
        pcf8563_str.read_time(&pcf8563_str);
        vTaskDelay(1000);
    }
}

 
void PCF8563_Task_Create(void)
{
    //任务句柄
    TaskHandle_t PCF8563Task_Handler;
        //创建LED1任务
    xTaskCreate((TaskFunction_t )PCF8563_task,
                (const char*    )"PCF8563_task",
                (uint16_t       )PCF8563_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )PCF8653_TASK_PRIO,
                (TaskHandle_t*  )&PCF8563Task_Handler);
}



