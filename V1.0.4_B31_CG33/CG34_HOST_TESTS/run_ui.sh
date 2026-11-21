#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
SRC="$HERE/../STM32L151/Software"
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
# Keil targets Windows; account for the stock mixed-case include on Linux.
printf '#include "algorithm_head.h"\n' > "$T/Algorithm_head.h"
cc -std=c99 -O1 -finput-charset=GBK -fexec-charset=GBK \
   -Werror=implicit-function-declaration -fsanitize=undefined \
   -ffunction-sections -fdata-sections -I "$HERE/ui" -I "$T" -I "$SRC/inc" \
   "$HERE/ui/test_ui.c" "$SRC/src/App_code.c" "$SRC/src/App_key.c" \
   -Wl,--gc-sections -o "$T/test_ui"
UBSAN_OPTIONS=halt_on_error=1 "$T/test_ui"
