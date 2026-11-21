#ifndef HXGT_LM95071_SAMPLE_H
#define HXGT_LM95071_SAMPLE_H

#include <stdint.h>
#include <math.h>

/* TI SNIS137G, sec. 6.5 note 4: allow 228 ms after power-up and more
 * than 228 ms between reads. Use 250 one-ms ticks including tick phase
 * margin. The board clock starts only after the sensor GPIOs are ready. */
#define LM95071_READ_INTERVAL_MS 250U

static inline uint8_t LM95071_ReadDue(uint32_t now, uint32_t last)
{
    return (uint8_t)((uint32_t)(now - last) >= LM95071_READ_INTERVAL_MS);
}

static inline float LM95071_Decode(uint16_t word)
{
    int32_t counts;
    /* TI SNIS137G, sec. 8.6.1.2: D1:D0 are both one. A malformed
     * word is never a temperature. In particular 0x0000 is NOT 0 C;
     * 0xFFFF, however, is a valid -1 LSB (cannot detect stuck high). */
    if ((word & 3U) != 3U) return NAN;
    counts = (int32_t)(word >> 2);
    if ((word & 0x8000U) != 0U) counts -= 16384;
    return (float)counts * 0.03125f;
}

#endif
