#!/bin/sh
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/.." && pwd)
sh "$root/CG32_HOST_TESTS/run_cg32_tests.sh"
sh "$root/HXGT_TEMP_HOST_TESTS/run.sh"
sh "$here/run_ads1110_tests.sh"
sh "$here/run_gj.sh"
python3 "$here/test_display.py"
python3 "$here/test_telemetry.py"
sh "$here/run_ui.sh"
python3 "$here/check_projects.py"
