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

```bash
make && ./psv << 'HEREDOC'
{#table1 description="Hello World 1"}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  | 
| Charlie | 19  | London       |

{#table2 description="Hello World 2"}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```


```bash
make && ./psv -o test.json << 'HEREDOC'
{#table1 description="Hello World 1"}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  | 
| Charlie | 19  | London       |

{#table2 description="Hello World 2"}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```

```bash
make && ./psv -o test.json testdoc.md
make && ./psv testdoc.md
```

```bash
make && valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./psv --output test.json testdoc.md testdoc.md
```


```json
[
    {
        "id": "table1",
        "description": "This is the first table",
        "headers": ["Name", "Age", "Country"],
        "rows": [
            {"Name": "Alice", "Age": 25, "City": "New York"},
            {"Name": "Bob", "Age": 32.4, "City": "San Francisco"},
            {"Name": "Bob", "Age": 32},
            {"Name": "Charlie", "Age": 19, "City": "London"}
        ]
    }
]
```


---



