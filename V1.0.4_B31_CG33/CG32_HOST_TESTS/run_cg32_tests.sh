#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT=$(CDPATH= cd -- "$HERE/.." && pwd)
T=$(mktemp -d /tmp/cg32_XXXXXX)
trap 'rm -rf "$T"' EXIT
cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-omit-frame-pointer -I "$ROOT/QINGJIAOBAN/USER" "$HERE/test_filter.c" -o "$T/test_filter"
UBSAN_OPTIONS=halt_on_error=1 "$T/test_filter"
cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-omit-frame-pointer -I "$HERE" "$ROOT/QINGJIAOBAN/CLASS/SCR/adc161_code.c" "$HERE/test_adc.c" -o "$T/test_adc"
UBSAN_OPTIONS=halt_on_error=1 "$T/test_adc"
python3 "$ROOT/CG_FIX_HOST_TESTS/test_cg.py"
sh "$ROOT/B31_HOST_TESTS/run_b31_tests.sh"
sh "$ROOT/B2_HOST_TESTS/run_master_tests.sh"
python3 "$ROOT/B2_HOST_TESTS/test_qing.py"
