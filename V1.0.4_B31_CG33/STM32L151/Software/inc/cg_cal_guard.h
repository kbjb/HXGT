#ifndef CG_CAL_GUARD_H
#define CG_CAL_GUARD_H

#include "param_head.h"

/* [CG32-03] Both 180 mm calibration spans should be substantial.
 * The factory spans are approximately 12446 / 12463 AD. 4000 AD is a
 * conservative guard against slopes that could show thousands of mm.
 * These values are provisional pending real sensor characterization. */
#define CG_MIN_180MM_SPAN_AD  4000L
#define CG_MAX_ZERO_GAP_AD   1500L

static uint8_t CG_Calibration_Sane(const CG_Calibration_Param_Str *p)
{
    int32_t left_span, right_span, zero_gap;
    if (p == 0) return 0U;
    left_span = (int32_t)p->CG_Zero_Left_Ad - (int32_t)p->CG_Max_Left_Ad;
    right_span = (int32_t)p->CG_Max_Right_Ad - (int32_t)p->CG_Zero_Right_Ad;
    zero_gap = (int32_t)p->CG_Zero_Left_Ad - (int32_t)p->CG_Zero_Right_Ad;
    if (zero_gap < 0) zero_gap = -zero_gap;
    return (left_span >= CG_MIN_180MM_SPAN_AD &&
            right_span >= CG_MIN_180MM_SPAN_AD &&
            zero_gap <= CG_MAX_ZERO_GAP_AD &&
            p->temp >= -40.0f && p->temp <= 100.0f) ? 1U : 0U;
}

#endif /* CG_CAL_GUARD_H */
