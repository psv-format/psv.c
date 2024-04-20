#!/bin/bash
# Run the main program
set -euo pipefail
echo "== BUILD AND RUN psv =="
make
./psv << HEREDOC
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  | 
| Charlie | 19  | London       |
HEREDOC
if [ 0 -eq 0 ]; then
    echo "build completed."
else
    echo "build failed."
    exit 1
fi
