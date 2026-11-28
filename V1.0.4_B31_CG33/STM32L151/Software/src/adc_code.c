#include "adc_head.h"

#define ADC_MAX_CODE                 4095.0f
#define VREFINT_FACTORY_VDDA_V           3.0f
#define VREFINT_FALLBACK_V              1.224f
#define VREFINT_CAL_MIN_COUNTS           1600U
#define VREFINT_CAL_MAX_COUNTS           1720U
#define BATTERY_ADC_TIMEOUT_MS              2U

extern ADC_HandleTypeDef hadc;
extern float POWER_Vol;
volatile float Battery_Voltage;
volatile uint8_t Battery_Vref_Fallback;
volatile uint32_t Battery_ADC_Error_Count;

static uint8_t ADC_Channel_Mean(uint32_t channel, float *mean)
{
    ADC_ChannelConfTypeDef config = {0};
    uint32_t sum = 0U;
    uint32_t i;
    config.Channel = channel;
    config.Rank = ADC_REGULAR_RANK_1;
    config.SamplingTime = ADC_SAMPLETIME_384CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc, &config) != HAL_OK) return 0U;
    for (i = 0U; i <= BATTERY_ADC_SAMPLES; ++i)
    {
        uint32_t value;
        if (HAL_ADC_Start(&hadc) != HAL_OK ||
            HAL_ADC_PollForConversion(&hadc, BATTERY_ADC_TIMEOUT_MS) != HAL_OK)
            return 0U;
        value = HAL_ADC_GetValue(&hadc);
        if (i != 0U) sum += value; /* Discard the first conversion after switching. */
    }
    *mean = (float)sum / (float)BATTERY_ADC_SAMPLES;
    return 1U;
}

static float Battery_VDDA(float reference, uint16_t calibration, uint8_t *fallback)
{
    if (!(reference > 0.0f && reference < ADC_MAX_CODE)) return 0.0f;
    *fallback = calibration < VREFINT_CAL_MIN_COUNTS ||
                calibration > VREFINT_CAL_MAX_COUNTS;
    if (*fallback) return VREFINT_FALLBACK_V * ADC_MAX_CODE / reference;
    return VREFINT_FACTORY_VDDA_V * (float)calibration / reference;
}

enum { ADC_BATT_IDLE, ADC_BATT_HSI, ADC_BATT_REFERENCE };
static uint8_t battery_adc_state;
static uint32_t battery_adc_tick;

void ADC_Battery_Cancel(void)
{
    if (battery_adc_state == ADC_BATT_REFERENCE)
    {
        if (HAL_ADC_Stop(&hadc) != HAL_OK)
        {
            __HAL_RCC_ADC1_FORCE_RESET();
            __HAL_RCC_ADC1_RELEASE_RESET();
            (void)HAL_ADC_Init(&hadc);
        }
        CLEAR_BIT(ADC->CCR, ADC_CCR_TSVREFE);
        __HAL_RCC_ADC1_CLK_DISABLE();
    }
    if (battery_adc_state != ADC_BATT_IDLE) __HAL_RCC_HSI_DISABLE();
    battery_adc_state = ADC_BATT_IDLE;
}

uint8_t ADC_Battery_Step(float *volts)
{
    float reference, battery = 0.0f, vdda = 0.0f;
    uint32_t now = HAL_GetTick();
    uint8_t fallback = 0U;
    if (battery_adc_state == ADC_BATT_IDLE)
    {
        __HAL_RCC_HSI_ENABLE();
        battery_adc_state = ADC_BATT_HSI;
        battery_adc_tick = now;
    }
    if (battery_adc_state == ADC_BATT_HSI)
    {
        if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
        {
            if ((uint32_t)(now - battery_adc_tick) < 3U) return BATTERY_ADC_WAIT;
            ADC_Battery_Cancel();
            Battery_ADC_Error_Count++;
            return BATTERY_ADC_FAILED;
        }
        __HAL_RCC_ADC1_CLK_ENABLE();
        SET_BIT(ADC->CCR, ADC_CCR_TSVREFE);
        battery_adc_tick = now;
        battery_adc_state = ADC_BATT_REFERENCE;
        return BATTERY_ADC_WAIT;
    }
    /* ST specifies up to 3 ms for VREFINT startup. Let the main loop
     * keep servicing UART, keys and OLED while waiting. */
    if ((uint32_t)(now - battery_adc_tick) < 4U) return BATTERY_ADC_WAIT;
    if (ADC_Channel_Mean(ADC_CHANNEL_VREFINT, &reference) &&
        ADC_Channel_Mean(ADC_CHANNEL_9, &battery))
        vdda = Battery_VDDA(reference,
                *(const volatile uint16_t *)VREFINT_CAL_ADDR_CMSIS, &fallback);
    ADC_Battery_Cancel();

    if (!(vdda >= 2.70f && vdda <= 3.60f && battery > 0.0f && battery < ADC_MAX_CODE))
    {
        Battery_ADC_Error_Count++;
        return BATTERY_ADC_FAILED;
    }
    POWER_Vol = vdda;
    Battery_Vref_Fallback = fallback;
    Battery_Voltage = (battery / ADC_MAX_CODE) * vdda *
        ((BATTERY_DIVIDER_TOP_KOHM + BATTERY_DIVIDER_BOTTOM_KOHM) /
         BATTERY_DIVIDER_BOTTOM_KOHM);
    *volts = Battery_Voltage;
    return BATTERY_ADC_VALID;
}

uint8_t Battery_Level_For_Voltage(float volts, uint8_t previous_level)
{
    static const float thresholds[] = {BATTERY_LEVEL_1_V, BATTERY_LEVEL_2_V,
                                       BATTERY_LEVEL_3_V, BATTERY_LEVEL_4_V};
    uint8_t level = previous_level;
    if (level > 4U)
    {
        level = 0U;
        while (level < 4U && volts >= thresholds[level]) level++;
        return level;
    }
    while (level > 0U && volts < thresholds[level - 1U] - BATTERY_LEVEL_HYST_V)
        level--;
    while (level < 4U && volts >= thresholds[level] + BATTERY_LEVEL_HYST_V)
        level++;
    return level;
}
