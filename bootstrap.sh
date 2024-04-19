#!/bin/bash
# Setup Autotools Enviroment
set -euo pipefail
echo "== BOOTSTRAPPING =="

# Run autoreconf to generate configure script and Makefile.in files
echo "Running autoreconf..."
[ -e configure ] || autoreconf -vim

# Configure and build the project
echo "Configuring and building the project..."
[ -e Makefile ]  || ( ./configure && make )
