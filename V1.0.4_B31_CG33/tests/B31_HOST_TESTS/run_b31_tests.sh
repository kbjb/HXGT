#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
BIN=$(mktemp /tmp/b31_eeprom_XXXXXX)
trap 'rm -f "$BIN"' EXIT
cc -std=c99 -O1 -g -Wall -Wextra -Werror \
  -fsanitize=undefined -fstack-protector-all -fno-omit-frame-pointer \
  -I "$HERE" -I "$ROOT/STM32L151/Software/inc" \
  "$ROOT/STM32L151/Software/src/at21c512_code.c" \
  "$ROOT/STM32L151/Software/src/App_Eep.c" \
  "$HERE/test_b31.c" -o "$BIN"
UBSAN_OPTIONS=halt_on_error=1 "$BIN"
