#!/bin/bash
# Run the unit test program
set -euo pipefail
echo "== BUILD AND RUN UNIT TEST =="
make check
./unit_test
if [ 0 -eq 0 ]; then
    echo "Unit tests passed successfully."
else
    echo "Unit tests failed."
    exit 1
fi
