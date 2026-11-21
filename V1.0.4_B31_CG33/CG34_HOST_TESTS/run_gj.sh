#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT="$HERE/../STM32L151"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
# Keil's Windows build ignores the original mixed-case include.
printf '#include "algorithm_head.h"\n' > "$T/Algorithm_head.h"
${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-int-to-pointer-cast \
    -finput-charset=GBK -fsanitize=undefined -DUSE_HAL_DRIVER -DSTM32L151xBA \
    -I "$T" -I "$ROOT/Core/Inc" -I "$ROOT/Software/inc" \
    -I "$ROOT/Drivers/STM32L1xx_HAL_Driver/Inc" \
    -I "$ROOT/Drivers/CMSIS/Device/ST/STM32L1xx/Include" \
    -I "$ROOT/Drivers/CMSIS/Include" \
    "$HERE/test_gj.c" "$ROOT/Software/src/Algorithm_code.c" -lm -o "$T/test"
UBSAN_OPTIONS=halt_on_error=1 "$T/test"
