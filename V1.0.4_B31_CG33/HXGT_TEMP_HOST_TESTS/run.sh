#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT
${CC:-cc} -std=c99 -Wall -Wextra -Werror -O2 -fsanitize=undefined \
    -I "$here/../QINGJIAOBAN/CLASS/INC" "$here/test_lm95071.c" -lm -o "$out/test"
UBSAN_OPTIONS=halt_on_error=1 "$out/test"
