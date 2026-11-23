#!/bin/sh
# B3 test runner is retained for historical convenience. The previous
# legacy-import tests apply to B3 ONLY, not this production B3.1 build.
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec sh "$HERE/../B31_HOST_TESTS/run_b31_tests.sh"
