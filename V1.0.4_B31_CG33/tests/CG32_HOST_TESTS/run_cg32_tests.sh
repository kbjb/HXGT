#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
TESTS=$(CDPATH= cd -- "$HERE/.." && pwd)
ROOT=$(CDPATH= cd -- "$TESTS/.." && pwd)
T=$(mktemp -d /tmp/cg32_XXXXXX)
trap 'rm -rf "$T"' EXIT
cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=undefined -fno-omit-frame-pointer -I "$ROOT/QINGJIAOBAN/USER" "$HERE/test_filter.c" -o "$T/test_filter"
UBSAN_OPTIONS=halt_on_error=1 "$T/test_filter"
python3 "$HERE/test_spi_decode.py"
python3 "$TESTS/CG34_HOST_TESTS/test_sampling.py"
python3 "$TESTS/CG_FIX_HOST_TESTS/test_cg.py"
python3 "$TESTS/CG40_HOST_TESTS/test_temperature.py"
python3 "$TESTS/CG38_HOST_TESTS/test_ch.py"
python3 "$TESTS/CG39_HOST_TESTS/test_ui.py"
sh "$TESTS/B31_HOST_TESTS/run_b31_tests.sh"
sh "$TESTS/B2_HOST_TESTS/run_master_tests.sh"
python3 "$TESTS/B2_HOST_TESTS/test_qing.py"
