#ifndef CG_TIMING_WATCH_H
#define CG_TIMING_WATCH_H

#include <stdint.h>

/* Foreground is the only writer. Zero-initialize at power-up. These counters
 * are Keil Watch aids, not a validity/accuracy test or a timing guarantee. */
typedef struct
{
    uint32_t last_tick_ms;
    uint32_t last_gap_ms;
    uint32_t max_gap_ms;
    uint8_t seen;
} CG_TIMING_WATCH;

static void CG_RecordGap(volatile CG_TIMING_WATCH *watch, uint32_t now)
{
    if (watch->seen)
    {
        uint32_t gap = now - watch->last_tick_ms;
        watch->last_gap_ms = gap;
        if (gap > watch->max_gap_ms) watch->max_gap_ms = gap;
    }
    watch->last_tick_ms = now;
    watch->seen = 1U;
}

#endif
