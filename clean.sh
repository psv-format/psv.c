#!/bin/bash
# Clean out everything as much as possible
set -euo pipefail
echo "== CLEANING =="
make clean
make maintainer-clean
# Extra cleaning is possible if inside a git repo
if git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
  echo "Using .gitignore in an active git repo to clean out autotool files and directories"
  git clean -Xdf
fi
