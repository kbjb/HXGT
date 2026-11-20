#include "ads1110_head.h"

void ADS1110_INIT(ADS1110_STR *ads1110_str)
{
    ads1110_str->iic_str->str_init(ads1110_str->iic_str);
    
}


void ADS1110_READ(ADS1110_STR *ads1110_str)
{
    uint8_t data1,data2,data3;
    
    ads1110_str->iic_str->start(ads1110_str->iic_str);
    ads1110_str->iic_str->send_byte(ads1110_str->iic_str,ADS1110_READ_REG);
    if(ads1110_str->iic_str->wait_ack(ads1110_str->iic_str) == 1)
    {
        ads1110_str->iic_str->stop(ads1110_str->iic_str);
        return;
    }
    data1 = ads1110_str->iic_str->read_byte(ads1110_str->iic_str);
    ads1110_str->iic_str->ack(ads1110_str->iic_str);
    data2 = ads1110_str->iic_str->read_byte(ads1110_str->iic_str);
    ads1110_str->iic_str->ack(ads1110_str->iic_str);
    data3 = ads1110_str->iic_str->read_byte(ads1110_str->iic_str);
    ads1110_str->iic_str->ack(ads1110_str->iic_str);
    ads1110_str->iic_str->stop(ads1110_str->iic_str);
    
    ads1110_str->ad_data = (data1<<8)|data2;
    ads1110_str->config  = data3;
}

void ADS1110_WRITE(ADS1110_STR *ads1110_str,uint8_t data)
{
    ads1110_str->iic_str->start(ads1110_str->iic_str);
    ads1110_str->iic_str->send_byte(ads1110_str->iic_str,ADS1110_WRITE_REG);
    if(ads1110_str->iic_str->wait_ack(ads1110_str->iic_str) == 1)
    {
        ads1110_str->iic_str->stop(ads1110_str->iic_str);
        return;
    }
    ads1110_str->iic_str->send_byte(ads1110_str->iic_str,data);
    ads1110_str->iic_str->wait_ack(ads1110_str->iic_str);
    ads1110_str->iic_str->stop(ads1110_str->iic_str);
}


uint8_t ADS1110_CHECK(ADS1110_STR *ads1110_str)
{
    ads1110_str->write(ads1110_str,ADS1110_CONFIG_REG_DATA);
    ads1110_str->read(ads1110_str);
    if((ads1110_str->config&0x0F) == (ADS1110_CONFIG_REG_DATA&0x0F))
        return 0;
    return 1;
}

void ADS1110_STR_INIT(ADS1110_STR *ads1110_str)
{
    ads1110_str->init = ADS1110_INIT;
    ads1110_str->check = ADS1110_CHECK;
    ads1110_str->read = ADS1110_READ;
    ads1110_str->write = ADS1110_WRITE;
    ads1110_str->init(ads1110_str);
}







