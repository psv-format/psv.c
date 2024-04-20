# psv

A minimal Autotools-based C project.

Quickstart Tip: Just run `./bootstrap.sh && ./build.sh && ./test.sh` to build and test.
Your executable will be located at `./psv`.
Use `./clean.sh\ && ./bootstrap.sh` to reset to a clean slate.



```
./psv << HEREDOC
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  | 
| Charlie | 19  | London       |
HEREDOC
```
