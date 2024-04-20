#!/bin/bash
# Assumes you already have https://cosmo.zip installed 
# Run the main program
set -euo pipefail
echo "== BUILD AND RUN psv =="
export CC=x86_64-unknown-cosmo-cc
export CXX=x86_64-unknown-cosmo-c++
./configure --prefix=/opt/cosmos/x86_64
make -j
./psv --version
if [ 0 -eq 0 ]; then
    echo "build completed."
else
    echo "build failed."
    exit 1
fi
