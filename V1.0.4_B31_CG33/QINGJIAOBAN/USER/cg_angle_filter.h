#ifndef CG_ANGLE_FILTER_H
#define CG_ANGLE_FILTER_H

#include <stdint.h>

/* [CG32-01] 40-sample trimmed moving average at 20 ms/sample (discard one min and max).
 * A 2-count output hysteresis is about 0.029 mm at factory CG calibration.
 * This is a provisional engineering value, NOT an accuracy qualification.
 * C99 and armcc5 compatible; no dynamic allocation or float processing. */
#define CG_ANGLE_WINDOW_SIZE    40U
#define CG_ANGLE_DEADBAND_AD     2L

typedef struct
{
    int16_t samples[CG_ANGLE_WINDOW_SIZE];
    uint8_t write_index;
    uint8_t count;
    uint8_t initialized;
    int16_t raw_value;
    int16_t filtered_value;
    int16_t output_value;
} CG_ANGLE_FILTER;

static void CG_AngleFilter_Init(CG_ANGLE_FILTER *f)
{
    uint8_t i;
    if (f == 0) return;
    f->write_index = 0U;
    f->count = 0U;
    f->initialized = 0U;
    f->raw_value = 0;
    f->filtered_value = 0;
    f->output_value = 0;
    for (i = 0U; i < CG_ANGLE_WINDOW_SIZE; ++i) f->samples[i] = 0;
}

static int16_t CG_AngleFilter_Push(CG_ANGLE_FILTER *f, int16_t sample)
{
    int32_t sum = 0;
    int16_t highest = INT16_MIN;
    int16_t lowest  = INT16_MAX;
    uint8_t i;
    uint8_t n;
    int32_t delta;
    if (f == 0) return 0;
    f->raw_value = sample;
    f->samples[f->write_index] = sample;
    f->write_index++;
    if (f->write_index >= CG_ANGLE_WINDOW_SIZE) f->write_index = 0U;
    if (f->count < CG_ANGLE_WINDOW_SIZE) f->count++;
    n = f->count;
    for (i = 0U; i < n; ++i)
    {
        int16_t value = f->samples[i];
        sum += (int32_t)value;
        if (value > highest) highest = value;
        if (value < lowest) lowest = value;
    }
    if (n >= 3U)
    {
        sum -= (int32_t)highest + (int32_t)lowest;
        f->filtered_value = (int16_t)(sum / (int32_t)(n - 2U));
    }
    else f->filtered_value = (int16_t)(sum / (int32_t)n);

    delta = (int32_t)f->filtered_value - (int32_t)f->output_value;
    if (!f->initialized || delta >= CG_ANGLE_DEADBAND_AD ||
        delta <= -CG_ANGLE_DEADBAND_AD)
    {
        f->output_value = f->filtered_value;
        f->initialized = 1U;
    }
    return f->output_value;
}

#endif /* CG_ANGLE_FILTER_H */
