# CG33 / 2026-11-16: Remove the +/-240 mm numeric display suppression

Baseline: CG32 (mainboard STM32L151 plus inclination board STM32F103).

## Requested changes

1. The output deadband lives at `QINGJIAOBAN/USER/cg_angle_filter.h:11`:
   `#define CG_ANGLE_DEADBAND_AD 3L`. This release leaves it at **3 AD**.
2. The old five-sample stable accumulator is **no longer used** by the CG32
   sampling path in `QINGJIAOBAN/USER/main.c`. The filter now uses a separate
   30-sample trimmed rolling average; `AngleParam.tempSumValue = 0` is still
   assigned every iteration for compatibility with existing Keil Watch setup.
3. The two `+/-240 mm` checks inside `Get_CG_Value()` have been removed.
   `CG_Result_Status` remains `CG_RESULT_OK` for any numeric value computed
   with acceptable calibration and temperature, including values above 240 mm.
4. The `CG RANGE` OLED branch, obsolete error-status constant, and obsolete
   `CG_MAX_MEASURE_MM` define are removed. `+3000.00` can now be displayed if
   the actual numeric result reaches that value.

## Intentionally unchanged

- Calibration span/gap validation, temperature bounds, sensor-frame freshness,
  EEPROM error indication, and normal display refresh logic.
- AT24C512 B3.1 storage layout, A/B snapshots, CRC, initialization, and
  both disabled legacy migration switches (0).
- The QINGJIAOBAN sampling filter, its 3 AD deadband, the UART packet format,
  and the ADC161 signed conversion.

## Important

Removing the numeric limit is **not** a calibration correction. A displayed
`3000 mm` does not imply a physically valid superelevation. During bench tests,
record `CG_Ad`, `CG_Value_t`, `CG_Value`, `CG_Param`, and `CG_Result_Status`
when the number spikes. Keil rebuild and real-device acceptance remain pending.
