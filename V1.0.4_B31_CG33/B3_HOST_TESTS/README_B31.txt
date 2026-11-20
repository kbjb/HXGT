B3.1 production code no longer supports legacy EEPROM migration.
The historical test_b3.c in this folder documents B3 behavior only.
Run run_b3_tests.sh, which now forwards to B31_HOST_TESTS, or run
B31_HOST_TESTS/run_b31_tests.sh directly. The old test_b3.c must NOT
be compiled against B3.1 firmware: its legacy test expectations differ.
