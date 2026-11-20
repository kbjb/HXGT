#include "iic_head.h"


 void IIC_DELAY(uint32_t cnt)
 {
     while(cnt--);
 }

 void IIC_START(IIC_STR *iic_str)
 {
    iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_Out_PP);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_SET);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_RESET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
 }

 void IIC_STOP(IIC_STR *iic_str)
 {
    iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_Out_PP);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_RESET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_SET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
 }
 
 uint8_t IIC_WAIT_ACK(IIC_STR *iic_str)
 {
    iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_IN_FLOATING);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_SET);  //ÊÍ·ÅSDA 
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET);
    if(iic_str->sda_gpio->read(iic_str->sda_gpio) == Bit_SET)
        return 1;
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
        return 0;
 }
 
 void IIC_ACK(IIC_STR *iic_str)
 {
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
    iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_Out_PP);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_RESET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET); 
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
 }
 
 void IIC_NACK(IIC_STR *iic_str)
 {
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
    iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_Out_PP);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_SET);
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET); 
    if(iic_str->delay!=NULL) iic_str->delay(iic_str);
    else IIC_DELAY(iic_str->delay_cnt);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
 }
 
 void IIC_SEND_BYTE(IIC_STR *iic_str,uint8_t byte)
 {
     iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_Out_PP);
     iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
     for(uint8_t i=0;i<8;i++)
     {
        if(byte&0x80)
            iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_SET);
        else 
            iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_RESET);
        byte<<=1;
        iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET);
        if(iic_str->delay!=NULL) iic_str->delay(iic_str);
        else IIC_DELAY(iic_str->delay_cnt);
        iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET); 
        if(iic_str->delay!=NULL) iic_str->delay(iic_str);
        else IIC_DELAY(iic_str->delay_cnt);
     }
 }
 
 uint8_t IIC_READ_BYTE(IIC_STR *iic_str)
 {
     uint8_t res=0;
     iic_str->sda_gpio->mode_set(iic_str->sda_gpio,GPIO_Mode_IN_FLOATING);
     for(uint8_t i=0;i<8;i++)
     {
         iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_RESET);
         if(iic_str->delay!=NULL) iic_str->delay(iic_str);
         else IIC_DELAY(iic_str->delay_cnt);
         iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET);
         res<<=1;
         if(iic_str->sda_gpio->read(iic_str->sda_gpio) == Bit_SET)
             res++;
         if(iic_str->delay!=NULL) iic_str->delay(iic_str);
         else IIC_DELAY(iic_str->delay_cnt);;
     }
     return res;
 }
 
void IIC_INIT(IIC_STR *iic_str)
{
    iic_str->sclk_gpio->str_init(iic_str->sclk_gpio);
    iic_str->sda_gpio->str_init(iic_str->sda_gpio);
    iic_str->sclk_gpio->write(iic_str->sclk_gpio,Bit_SET);
    iic_str->sda_gpio->write(iic_str->sda_gpio,Bit_SET);
}



void IIC_STR_INIT(IIC_STR *iic_str)
{
    iic_str->init      = IIC_INIT;
    iic_str->start     = IIC_START;
    iic_str->stop      = IIC_STOP;
    iic_str->wait_ack  = IIC_WAIT_ACK;
    iic_str->ack       = IIC_ACK;
    iic_str->nack      = IIC_NACK;
    iic_str->send_byte = IIC_SEND_BYTE;
    iic_str->read_byte = IIC_READ_BYTE;
    iic_str->init(iic_str);
}










