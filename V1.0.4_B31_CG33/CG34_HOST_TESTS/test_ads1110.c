#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ads1110_head.h"

static char ops[32];
static unsigned nops;
static uint8_t incoming[3];
static unsigned cursor;
static unsigned polls;
static uint8_t ack_addr_failed, ack_data_failed, write_addr_failed;
static uint8_t is_write, address_sent, payload_sent, scl_stuck, sda_stuck;

static void op(char ch)
{
    assert(nops + 1U < sizeof(ops));
    ops[nops++] = ch;
    ops[nops] = '\0';
}
static void fake_init(IIC_STR *p) { (void)p; }
static void fake_start(IIC_STR *p)
{
    (void)p;
    op('S');
    cursor = 0U;
    address_sent = 0U;
    payload_sent = 0U;
}
static void fake_stop(IIC_STR *p) { (void)p; op('P'); }
static void fake_ack(IIC_STR *p) { (void)p; op('a'); }
static void fake_nack(IIC_STR *p) { (void)p; op('n'); }
static void fake_send(IIC_STR *p, uint8_t x)
{
    (void)p;
    if (!address_sent)
    {
        assert(x == ADS1110_READ_REG || x == ADS1110_WRITE_REG);
        is_write = x == ADS1110_WRITE_REG;
        op(is_write ? 'w' : 'r');
        if (!is_write) ++polls;
        address_sent = 1U;
    }
    else
    {
        assert(is_write && !payload_sent && x == ADS1110_CONFIG_REG_DATA);
        payload_sent = 1U;
        op('c');
    }
}
static uint8_t fake_wait(IIC_STR *p)
{
    (void)p; op('k');
    if (!is_write) return ack_addr_failed;
    if (payload_sent) return ack_data_failed;
    return write_addr_failed;
}
static uint8_t fake_read(IIC_STR *p)
{
    (void)p;
    assert(!is_write && cursor < sizeof(incoming));
    op('d');
    return incoming[cursor++];
}
static BitAction fake_scl_idle(GPIO_STR *g) { (void)g; return scl_stuck ? Bit_RESET : Bit_SET; }
static BitAction fake_sda_idle(GPIO_STR *g) { (void)g; return sda_stuck ? Bit_RESET : Bit_SET; }

static void sample(uint16_t value, uint8_t config)
{
    nops = 0U;
    incoming[0] = (uint8_t)(value >> 8);
    incoming[1] = (uint8_t)value;
    incoming[2] = config;
}

static void test_ads_acquisition(void)
{
    GPIO_STR scl = {.read = fake_scl_idle}, sda = {.read = fake_sda_idle};
    IIC_STR iic = {
        .sclk_gpio = &scl, .sda_gpio = &sda, .str_init = fake_init, .start = fake_start,
        .stop = fake_stop, .wait_ack = fake_wait, .ack = fake_ack, .nack = fake_nack,
        .send_byte = fake_send, .read_byte = fake_read
    };
    ADS1110_STR adc = ADS1110_CLASS_STR(&iic);
    uint32_t now = UINT32_MAX-80U;
    unsigned previous_polls;

    ack_data_failed = 1U; /* startup config address acknowledged, payload rejected */
    ADS1110_Init(&adc, now);
    assert(!strcmp(ops, "SwkckP") && adc.status == ADS1110_BUS_ERROR);
    assert(ADS1110_WireValue(&adc, now) == ADS1110_GAUGE_INVALID);
    ack_data_failed = 0U;
    sample(12345U, 0x8CU);
    ADS1110_Poll(&adc, now);
    assert(!strcmp(ops, "SrkdadadnP"));
    assert(ADS1110_WireValue(&adc, now) == ADS1110_GAUGE_INVALID);

    now += ADS1110_POLL_MS;
    sample(12345U, 0x0CU);
    ADS1110_Poll(&adc, now);
    assert(adc.status == ADS1110_VALID && ADS1110_WireValue(&adc, now) == 12345U);
    previous_polls = polls;
    ADS1110_Poll(&adc, now + ADS1110_POLL_MS - 1U);
    assert(polls == previous_polls);

    now += ADS1110_POLL_MS;
    sample(22346U, 0x0CU);
    ADS1110_Poll(&adc, now);
    assert(ADS1110_WireValue(&adc, now) == 17345U); /* preserve truncating 0.5 IIR */

    sample(30000U, 0x8CU);
    ADS1110_Poll(&adc, now + ADS1110_STALE_MS - 1U);
    assert(ADS1110_WireValue(&adc, now + ADS1110_STALE_MS - 1U) == 17345U);
    assert(ADS1110_WireValue(&adc, now + ADS1110_STALE_MS) == ADS1110_GAUGE_INVALID);
    now += ADS1110_STALE_MS + ADS1110_POLL_MS; /* wrap occurred during stale wait */
    nops = 0U;
    ADS1110_Poll(&adc, now);
    assert(adc.status == ADS1110_STALE);

    sample(23456U, 0x0CU);
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(ADS1110_WireValue(&adc, now) == 23456U);

    sample(1000U, 0x9CU); /* wrong mode although gain and DR match */
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(!strcmp(ops, "SrkdadadnPSwkckP"));
    assert(adc.status == ADS1110_CONFIG_ERROR && ADS1110_WireValue(&adc, now) == ADS1110_GAUGE_INVALID);

    sample(30001U, 0x0CU);
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(ADS1110_WireValue(&adc, now) == 30001U);

    sample(0xFFFFU, 0x0CU);
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(adc.status == ADS1110_NEGATIVE && ADS1110_WireValue(&adc, now) == ADS1110_GAUGE_INVALID);

    ack_addr_failed = 1U; /* previous result must not survive an addr NACK */
    sample(0x7FFFU, 0x0CU);
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(!strcmp(ops, "SrkP"));
    assert(adc.status == ADS1110_BUS_ERROR && ADS1110_WireValue(&adc, now) == ADS1110_GAUGE_INVALID);

    ack_addr_failed = 0U;
    sda_stuck = 1U;
    previous_polls = polls;
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(polls == previous_polls && adc.status == ADS1110_BUS_ERROR);
    sda_stuck = 0U;
    scl_stuck = 1U;
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(polls == previous_polls && adc.status == ADS1110_BUS_ERROR);

    scl_stuck = 0U;
    sample(0x7FFFU, 0x0CU);
    now += ADS1110_POLL_MS;
    ADS1110_Poll(&adc, now);
    assert(ADS1110_WireValue(&adc, now) == 0x7FFFU);
}

static unsigned delay_nr, delay_at_last_edge, rising, captures;
static unsigned sampled_level;
static BitAction scl_level, sda_latch;
static GPIOMode_TypeDef sda_mode;
static uint8_t outgoing;
static int check_wave;

static void pin_init(GPIO_STR *g) { (void)g; }
static void logic_delay(IIC_STR *bus) { (void)bus; ++delay_nr; }
static void scl_write(GPIO_STR *g, BitAction b)
{
    (void)g;
    if (check_wave && b != scl_level)
    {
        assert(delay_nr > delay_at_last_edge);
        if (b == Bit_SET)
        {
            ++rising;
            outgoing <<= 1;
            if (sda_mode == GPIO_Mode_IN_FLOATING || sda_latch == Bit_SET) outgoing |= 1U;
        }
        delay_at_last_edge = delay_nr;
    }
    scl_level = b;
}
static void sda_write(GPIO_STR *g, BitAction b)
{
    (void)g;
    sda_latch = b;
}
static void sda_mode_set(GPIO_STR *g, GPIOMode_TypeDef m)
{
    (void)g;
    assert(m == GPIO_Mode_IN_FLOATING || m == GPIO_Mode_Out_OD);
    sda_mode = m;
}
static BitAction sda_read(GPIO_STR *g)
{
    (void)g;
    assert(sda_mode == GPIO_Mode_IN_FLOATING && scl_level == Bit_SET);
    assert(delay_nr > delay_at_last_edge); /* SDA sampled after SCL settle */
    ++captures;
    return (sampled_level >> ((8U - captures) % 8U)) & 1U ? Bit_SET : Bit_RESET;
}
static void test_waveform_order(void)
{
    GPIO_STR scl = {.str_init = pin_init, .write = scl_write};
    GPIO_STR sda = {.str_init = pin_init, .write = sda_write, .mode_set = sda_mode_set,
                    .read = sda_read, .initdef = {.GPIO_Mode = GPIO_Mode_Out_OD}};
    IIC_STR bus = IIC_CLASS_CREATE(&scl, &sda, 0U, logic_delay);
    bus.str_init(&bus);
    assert(scl_level == Bit_SET && sda_latch == Bit_SET);
    bus.start(&bus);
    assert(scl_level == Bit_RESET && sda_latch == Bit_RESET);

    check_wave = 1;
    delay_at_last_edge = delay_nr;
    bus.send_byte(&bus, 0x91U);
    assert(rising == 8U && outgoing == 0x91U && scl_level == Bit_RESET);
    assert(bus.wait_ack(&bus) == 0U && scl_level == Bit_RESET);
    sampled_level = 0xFFU;
    assert(bus.wait_ack(&bus) == 1U && scl_level == Bit_RESET);
    captures = 0U;
    sampled_level = 0xA5U;
    assert(bus.read_byte(&bus) == 0xA5U && scl_level == Bit_RESET);
    bus.ack(&bus);
    assert((outgoing & 1U) == 0U && scl_level == Bit_RESET);
    bus.nack(&bus);
    assert((outgoing & 1U) == 1U && scl_level == Bit_RESET);
    bus.stop(&bus);
    assert(scl_level == Bit_SET && sda_latch == Bit_SET);
}

int main(void)
{
    test_ads_acquisition();
    test_waveform_order();
    puts("ADS1110/I2C: PASS; address and data NACK, DRDY, mask, stale/wrap, signed rejection, stuck bus, waveform ordering and recovery");
    return 0;
}
