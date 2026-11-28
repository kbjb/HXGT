#include "adc161_head.h"

/* SPI1 uses PA5=SCK, PA6=MISO and software CS on PA4.
 * Mode 3 samples on the rising edge after the ADC changes DOUT. */
#define ADC161_SPI_TIMEOUT_US 50U
volatile uint32_t ADC161_Spi_Error_Count;

void ADC161_INIT(ADC161_STR *adc161_str)
{
    GPIO_InitTypeDef pins;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_SPI1, ENABLE);

    GPIOA->BSRR = ADC161_CS_PIN;
    adc161_str->cs->str_init(adc161_str->cs);
    adc161_str->out->str_init(adc161_str->out);

    /* 8 MHz PCLK2 / 4 = 2 MHz. Two queued 16-bit words keep SCK continuous. */
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_BR_0 | SPI_CR1_CPOL | SPI_CR1_CPHA |
                SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_DFF;
    SPI1->CR1 |= SPI_CR1_SPE;
    pins.GPIO_Pin = ADC161_CLK_PIN;
    pins.GPIO_Mode = GPIO_Mode_AF_PP;
    pins.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(ADC161_CLK_PORT, &pins);
}

/* [CG32-02] Standard 16-bit two's-complement interpretation.
 * The previous signed shift was undefined when bit 15 became set, and
 * adding 1 to all negative results gave an off-by-one error. */
int16_t uint16_twos_complement_to_int16_original(uint16_t tc)
{
    if ((tc & 0x8000U) != 0U)
        return (int16_t)((int32_t)tc - 65536L);
    return (int16_t)tc;
}

static uint8_t ADC161_Wait(uint16_t flag, uint16_t expected, uint16_t start)
{
    while ((SPI1->SR & flag) != expected)
        if ((uint16_t)(TIM2->CNT - start) >= ADC161_SPI_TIMEOUT_US) return 0U;
    return 1U;
}

uint8_t ADC161_READ(ADC161_STR *adc161_str)
{
    uint16_t first, second, bits;
    uint32_t saved_mask = __get_PRIMASK();
    uint16_t start;
    uint8_t success = 0U;

    __disable_irq();
    start = (uint16_t)TIM2->CNT;
    GPIOA->BRR = ADC161_CS_PIN;
    SPI1->DR = 0U;
    if (ADC161_Wait(SPI_SR_TXE, SPI_SR_TXE, start))
    {
        SPI1->DR = 0U;
        if (ADC161_Wait(SPI_SR_RXNE, SPI_SR_RXNE, start))
        {
            first = SPI1->DR;
            if (ADC161_Wait(SPI_SR_RXNE, SPI_SR_RXNE, start))
            {
                second = SPI1->DR;
                if (ADC161_Wait(SPI_SR_BSY, 0U, start))
                {
                    /* Ignore two leading clocks and 14 trailing acquisition clocks. */
                    bits = (uint16_t)(((uint32_t)first << 2) | (second >> 14));
                    adc161_str->ad_data = uint16_twos_complement_to_int16_original(bits);
                    success = 1U;
                }
            }
        }
    }
    GPIOA->BSRR = ADC161_CS_PIN;
    if (!success)
    {
        SPI1->CR1 &= (uint16_t)~SPI_CR1_SPE;
        (void)SPI1->DR;
        (void)SPI1->SR;
        SPI1->CR1 |= SPI_CR1_SPE;
        ADC161_Spi_Error_Count++;
    }
    __set_PRIMASK(saved_mask);
    return success;
}

void ADC161_STR_INIT(ADC161_STR *adc161_str)
{
    adc161_str->init = ADC161_INIT;
    adc161_str->read = ADC161_READ;
    adc161_str->init(adc161_str);
}

