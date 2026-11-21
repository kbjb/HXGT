#!/bin/sh
set -eu
D=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
Q=$(CDPATH= cd -- "$D/../QINGJIAOBAN" && pwd)
T=$(mktemp -d)
trap 'rm -rf "$T"' EXIT
"${CC:-cc}" -std=c99 -O2 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=undefined \
    -DUSE_STDPERIPH_DRIVER -DSTM32F10X_MD \
    -I "$Q/CORE/ST" -I "$Q/MCU/INC" -I "$Q/USER" -I "$Q/CLASS/INC" \
    "$D/test_ads1110.c" "$Q/CLASS/SCR/ads1110_code.c" "$Q/CLASS/SCR/iic_code.c" -o "$T/test_ads"
"$T/test_ads"
