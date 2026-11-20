#include "spi_head.h"


void SPI_DELAY(uint32_t n)
{
    while(n--);
}

void SPI_INIT(SPI_STR *spi_str)
{
    spi_str->clk_gpio->str_init(spi_str->clk_gpio);
    if(spi_str->miso_gpio != NULL)spi_str->miso_gpio->str_init(spi_str->miso_gpio);
    if(spi_str->mosi_gpio != NULL)spi_str->mosi_gpio->str_init(spi_str->mosi_gpio);
    spi_str->cs_gpio->str_init(spi_str->cs_gpio);
}

uint8_t SPI_RW_MODE0(SPI_STR *spi_str,uint8_t data)
{
    uint8_t i;
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_RESET);
    for(i=0;i<8;i++)
    {
        if(data&0x80)
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_SET);
        else 
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_RESET);
        data<<=1;
        SPI_DELAY(20);
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_SET);
        if(spi_str->miso_gpio != NULL)
        {
            if(spi_str->miso_gpio->read(spi_str->miso_gpio))
                data++;
        }
        SPI_DELAY(20);
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_RESET);
    }
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_SET);
    return data;
} 

uint8_t SPI_RW_MODE1(SPI_STR *spi_str,uint8_t data)
{
    uint8_t i;
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_RESET);
    for(i=0;i<8;i++)
    {   
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_SET);
        if(data&0x80)
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_SET);
        else 
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_RESET);
        data<<=1;
        SPI_DELAY(20);
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_RESET);
        if(spi_str->miso_gpio!=NULL)
        {
            if(spi_str->miso_gpio->read(spi_str->miso_gpio))
                data++;
        }
        SPI_DELAY(20);
    }
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_SET);
    return data;
}

uint8_t SPI_RW_MODE2(SPI_STR *spi_str,uint8_t data)
{
    uint8_t i;
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_RESET);
    for(i=0;i<8;i++)
    {
        if(data&0x80)
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_SET);
        else 
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_RESET);
        data<<=1;
        SPI_DELAY(20);
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_RESET);
        if(spi_str->miso_gpio != NULL)
        {
            if(spi_str->miso_gpio->read(spi_str->miso_gpio))
                data++;
        }
        SPI_DELAY(20);
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_SET);
    }
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_SET);
    return data;
}

uint8_t SPI_RW_MODE3(SPI_STR *spi_str,uint8_t data)
{
    uint8_t i;
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_RESET);
    for(i=0;i<8;i++)
    {        
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_RESET);
        if(data&0x80)
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_SET);
        else 
            spi_str->mosi_gpio->write(spi_str->mosi_gpio,Bit_RESET);
        data<<=1;
        SPI_DELAY(20);
        spi_str->clk_gpio->write(spi_str->clk_gpio,Bit_SET);
        if(spi_str->miso_gpio != NULL)
        {
            if(spi_str->miso_gpio->read(spi_str->miso_gpio))
                data++;
        }
        SPI_DELAY(20);
    }
    spi_str->cs_gpio->write(spi_str->cs_gpio,Bit_SET);
    return data;
}

void SPI_STR_INIT(SPI_STR *spi_str)
{
    spi_str->init = SPI_INIT;
    switch(spi_str->mode)
    {
        case mode0:
            spi_str->readWrite = SPI_RW_MODE0;
            break;
        case mode1: 
            spi_str->readWrite = SPI_RW_MODE1;
            break;
        case mode2:
            spi_str->readWrite = SPI_RW_MODE2;
            break;
        case mode3:
            spi_str->readWrite = SPI_RW_MODE3;
            break;
    }
    spi_str->init(spi_str);
}


