#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/../.." && pwd)
OUT=$(mktemp /tmp/b2_master_test_XXXXXX)
trap 'rm -f "$OUT"' EXIT
cc -std=c99 -O1 -g -Wall -Wextra -Wno-unused-variable \
  -fsanitize=undefined -fstack-protector-all -fno-omit-frame-pointer \
  -I "$HERE" -I "$ROOT/STM32L151/Software/inc" \
  "$ROOT/STM32L151/Software/src/usart_code.c" \
  "$ROOT/STM32L151/Software/src/agreement_code.c" \
  "$HERE/test_main.c" -o "$OUT"
UBSAN_OPTIONS=halt_on_error=1 "$OUT"
