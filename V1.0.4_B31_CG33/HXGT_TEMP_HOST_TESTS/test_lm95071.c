#include "lm95071_sample.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
    uint32_t i;
    assert(isnan(LM95071_Decode(0x0000)));
    assert(LM95071_Decode(0x0003) == 0.0f);
    assert(LM95071_Decode(0x0C83) == 25.0f);
    assert(LM95071_Decode(0xEC03) == -40.0f);
    assert(LM95071_Decode(0x0007) == 0.03125f);
    assert(LM95071_Decode(0xFFFF) == -0.03125f);
    for (i = 0U; i <= UINT16_MAX; ++i) {
        float t = LM95071_Decode((uint16_t)i);
        if ((i % 4U) != 3U) assert(isnan(t));
        else {
            /* Independent signed wire value, without shifting negatives. */
            int32_t signed_bits = (i < 32768U) ? (int32_t)i : (int32_t)i - 65536;
            assert((int32_t)(t * 128.0f) + 3 == signed_bits);
        }
    }
    assert(!LM95071_ReadDue(0U, 0U));
    assert(!LM95071_ReadDue(249U, 0U));
    assert(LM95071_ReadDue(250U, 0U));
    assert(LM95071_ReadDue(10000U, 250U)); /* delayed foreground */
    assert(!LM95071_ReadDue(10000U, 10000U));
    assert(!LM95071_ReadDue(148U, UINT32_MAX-100U));
    assert(LM95071_ReadDue(149U, UINT32_MAX-100U));
    puts("LM95071: PASS (all 65536 raw words; startup, delayed polls and wrap)");
    return 0;
}
