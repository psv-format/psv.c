#!/bin/bash
# Run the main program
set -euo pipefail
echo "== BUILD AND RUN psv =="
make
./psv
if [ 0 -eq 0 ]; then
    echo "build completed."
else
    echo "build failed."
    exit 1
fi
