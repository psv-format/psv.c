# psv

Quickstart Tip: Just run `./bootstrap.sh && ./build.sh` to build.
Your executable will be located at `./psv`.
Use `./clean.sh\ && ./bootstrap.sh` to reset to a clean slate.

<!-- For now, we don't have test yet here

Quickstart Tip: Just run `./bootstrap.sh && ./build.sh && ./test.sh` to build and test.
Your executable will be located at `./psv`.
Use `./clean.sh\ && ./bootstrap.sh` to reset to a clean slate.

-->

To try this program out use:

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
