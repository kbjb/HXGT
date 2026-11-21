#include "ads1110_head.h"

#define ADS1110_NOT_NEW  0x80U
#define ADS1110_CFG_MASK 0x7FU

static uint8_t ADS1110_BusIdle(IIC_STR *bus)
{
    return bus->sclk_gpio->read(bus->sclk_gpio) == Bit_SET &&
           bus->sda_gpio->read(bus->sda_gpio) == Bit_SET;
}

static uint8_t ADS1110_Configure(IIC_STR *bus)
{
    uint8_t ok = 0U;
    if (!ADS1110_BusIdle(bus)) return 0U;
    bus->start(bus);
    bus->send_byte(bus, ADS1110_WRITE_REG);
    if (!bus->wait_ack(bus))
    {
        bus->send_byte(bus, ADS1110_CONFIG_REG_DATA);
        ok = !bus->wait_ack(bus);
    }
    bus->stop(bus);
    return ok;
}

void ADS1110_Init(ADS1110_STR *adc, uint32_t now_ms)
{
    adc->iic_str->str_init(adc->iic_str);
    adc->last_poll_ms = now_ms - ADS1110_POLL_MS; /* first poll immediately */
    adc->last_valid_ms = now_ms;
    adc->valid = 0U;
    adc->config = 0U;
    adc->filtered = 0U;
    adc->status = ADS1110_Configure(adc->iic_str) ? ADS1110_WAITING : ADS1110_BUS_ERROR;
}

void ADS1110_Poll(ADS1110_STR *adc, uint32_t now_ms)
{
    IIC_STR *bus = adc->iic_str;
    uint8_t high, low, config;
    uint16_t raw;

    if ((uint32_t)(now_ms - adc->last_poll_ms) < ADS1110_POLL_MS) return;
    adc->last_poll_ms = now_ms;
    if (adc->valid && (uint32_t)(now_ms - adc->last_valid_ms) >= ADS1110_STALE_MS)
    {
        adc->valid = 0U;
        adc->status = ADS1110_STALE;
    }
    if (!ADS1110_BusIdle(bus))
    {
        adc->valid = 0U;
        adc->status = ADS1110_BUS_ERROR;
        return;
    }
    bus->start(bus);
    bus->send_byte(bus, ADS1110_READ_REG);
    if (bus->wait_ack(bus))
    {
        bus->stop(bus);
        adc->valid = 0U;
        adc->status = ADS1110_BUS_ERROR;
        return;
    }
    high = bus->read_byte(bus);
    bus->ack(bus);
    low = bus->read_byte(bus);
    bus->ack(bus);
    config = bus->read_byte(bus);
    bus->nack(bus);
    bus->stop(bus);

    adc->config = config;
    if ((config & ADS1110_CFG_MASK) != ADS1110_CONFIG_REG_DATA)
    {
        adc->valid = 0U;
        adc->status = ADS1110_Configure(bus) ? ADS1110_CONFIG_ERROR : ADS1110_BUS_ERROR;
        return;
    }
    if (config & ADS1110_NOT_NEW) return; /* never filter the same sample twice */

    raw = (uint16_t)(((uint16_t)high << 8) | low);
    if (raw > 0x7FFFU)
    {
        /* The nonnegative displacement path cannot use a negative
         * differential reading. Do not make it a plausible zero. */
        adc->valid = 0U;
        adc->status = ADS1110_NEGATIVE;
        return;
    }
    if (adc->valid)
        adc->filtered = (uint16_t)(((uint32_t)adc->filtered + raw) / 2U);
    else
        adc->filtered = raw; /* prime on first/recovered genuine sample */
    adc->last_valid_ms = now_ms;
    adc->valid = 1U;
    adc->status = ADS1110_VALID;
}

uint16_t ADS1110_WireValue(const ADS1110_STR *adc, uint32_t now_ms)
{
    return (adc->valid && (uint32_t)(now_ms - adc->last_valid_ms) < ADS1110_STALE_MS) ?
           adc->filtered : ADS1110_GAUGE_INVALID;
}
