#include "iic_head.h"

static void IIC_Delay(IIC_STR *bus)
{
    if (bus->delay != NULL)
        bus->delay(bus);
    else
    {
        /* Preserve GPIO setup/hold across optimized builds. The actual
         * clock and rise times still need scope verification on the PCB. */
        volatile uint32_t remaining = bus->delay_cnt;
        while (remaining != 0U) --remaining;
    }
}

static void IIC_SCL(IIC_STR *bus, BitAction value)
{
    bus->sclk_gpio->write(bus->sclk_gpio, value);
}

static void IIC_SDA(IIC_STR *bus, BitAction value)
{
    bus->sda_gpio->write(bus->sda_gpio, value);
}

static void IIC_SDA_Output(IIC_STR *bus, BitAction initial)
{
    IIC_SDA(bus, initial); /* set ODR before switching input -> output */
    bus->sda_gpio->mode_set(bus->sda_gpio, bus->sda_output_mode);
}

static void IIC_SDA_Release(IIC_STR *bus)
{
    bus->sda_gpio->mode_set(bus->sda_gpio, GPIO_Mode_IN_FLOATING);
}

static void IIC_START(IIC_STR *bus)
{
    IIC_SDA_Output(bus, Bit_SET);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_SET);
    IIC_Delay(bus);
    IIC_SDA(bus, Bit_RESET);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_RESET);
}

static void IIC_STOP(IIC_STR *bus)
{
    IIC_SCL(bus, Bit_RESET);
    IIC_SDA_Output(bus, Bit_RESET);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_SET);
    IIC_Delay(bus);
    IIC_SDA(bus, Bit_SET);
    IIC_Delay(bus);
}

static uint8_t IIC_WAIT_ACK(IIC_STR *bus)
{
    uint8_t missing;
    IIC_SCL(bus, Bit_RESET);
    IIC_SDA_Release(bus);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_SET);
    IIC_Delay(bus);
    missing = bus->sda_gpio->read(bus->sda_gpio) == Bit_SET;
    IIC_SCL(bus, Bit_RESET);
    return missing;
}

static void IIC_ACK(IIC_STR *bus)
{
    IIC_SCL(bus, Bit_RESET);
    IIC_SDA_Output(bus, Bit_RESET);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_SET);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_RESET);
}

static void IIC_NACK(IIC_STR *bus)
{
    IIC_SCL(bus, Bit_RESET);
    IIC_SDA_Release(bus); /* pull-up supplies HIGH without contention */
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_SET);
    IIC_Delay(bus);
    IIC_SCL(bus, Bit_RESET);
}

static void IIC_SEND_BYTE(IIC_STR *bus, uint8_t byte)
{
    uint8_t i;
    IIC_SCL(bus, Bit_RESET);
    IIC_SDA_Output(bus, (byte & 0x80U) ? Bit_SET : Bit_RESET);
    for (i = 0U; i < 8U; ++i)
    {
        IIC_SDA(bus, (byte & 0x80U) ? Bit_SET : Bit_RESET);
        byte <<= 1;
        IIC_Delay(bus);
        IIC_SCL(bus, Bit_SET);
        IIC_Delay(bus);
        IIC_SCL(bus, Bit_RESET);
    }
}

static uint8_t IIC_READ_BYTE(IIC_STR *bus)
{
    uint8_t res = 0U, i;
    IIC_SCL(bus, Bit_RESET);
    IIC_SDA_Release(bus);
    for (i = 0U; i < 8U; ++i)
    {
        IIC_Delay(bus);
        IIC_SCL(bus, Bit_SET);
        IIC_Delay(bus);
        res = (uint8_t)((res << 1) | (bus->sda_gpio->read(bus->sda_gpio) == Bit_SET));
        IIC_SCL(bus, Bit_RESET);
    }
    return res;
}

static void IIC_INIT(IIC_STR *bus)
{
    bus->sda_output_mode = (bus->sda_gpio->initdef.GPIO_Mode == GPIO_Mode_Out_OD) ?
                           GPIO_Mode_Out_OD : GPIO_Mode_Out_PP;
    bus->sclk_gpio->str_init(bus->sclk_gpio);
    bus->sda_gpio->str_init(bus->sda_gpio);
    IIC_SDA_Output(bus, Bit_SET);
    IIC_SCL(bus, Bit_SET);
    IIC_Delay(bus);
}

void IIC_STR_INIT(IIC_STR *bus)
{
    bus->init      = IIC_INIT;
    bus->start     = IIC_START;
    bus->stop      = IIC_STOP;
    bus->wait_ack  = IIC_WAIT_ACK;
    bus->ack       = IIC_ACK;
    bus->nack      = IIC_NACK;
    bus->send_byte = IIC_SEND_BYTE;
    bus->read_byte = IIC_READ_BYTE;
    bus->init(bus);
}
