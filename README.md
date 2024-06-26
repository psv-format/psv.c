# psv

## Overview

This is a reference implementation of a Markdown to JSON converter, designed specifically for parsing Markdown tables into JSON objects. It allows for easy conversion of Markdown documents containing tables into structured JSON data.

Ultimately the aim is to inform on what the draft psv standard should look like in <https://github.com/psv-format/psv-format.github.io>

## Quickstart

To build the program, simply run:

```bash
./bootstrap.sh && ./build.sh
```

Your executable will be located at `./psv`.

```
$./psv -h
psv - command-line Markdown to JSON converter
Usage:
        ./psv [options] [--id <id>] [file...]

psv reads Markdown documents from the input files or stdin and converts them to JSON format.

Options:
  -o, --output <file>     output JSON to the specified file
  -i, --id <id>           specify the ID of a single table to output
  -t, --table <pos>       specify the position of a single table to output (must be a positive integer)
  -c, --compact           output only the rows
  -h, --help              display this help message and exit
  -v, --version           output version information and exit

For more information, use './psv --help'.
```

### Quick Install From Source

To install from source to usual location in linux

```bash
# (Typical Debian) Install typical packages required for autotools and building a C project
sudo apt install autoconf automake libtool build-essential

# Setup Build System And Compile
./bootstrap.sh
./configure --prefix=/usr/local
make -j8
./psv --version # check it's locally working

# Install Into System (So it's accessible anywhere)
sudo make install
which psv # psv now installed to /usr/local/bin/psv
psv --version # check it's globally working
```

If you don't want psv anymore, then you can also run

```bash
# Remove psv
sudo make uninstall
```

Alternatively you can use this to define your own install path

```bash
sudo make install PREFIX=/usr/local
```


### Usage

To parse Markdown tables from standard input:

```bash
make && ./psv << 'HEREDOC'
| Name    | Age [float] | City Name |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  |
| Charlie | 19  | London       |

{#test2}
| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
HEREDOC
```

The above command would give a response that may look like below

```json
{"id":"table1","headers":["Name","Age [float]","City Name"],"keys":["name","age","city_name"],"data_annotation":[[],["float"],[]],"rows":[{"name":"Alice","age":25,"city_name":"New York"},{"name":"Bob","age":32.4,"city_name":"San Francisco"},{"name":"Bob","age":32,"city_name":null},{"name":"Charlie","age":19,"city_name":"London"}]}
{"id":"test2","headers":["Name","Age [int]","City [str]","Want Candy (jellybeans) [bool]"],"keys":["name","age","city","want_candy"],"data_annotation":[[],["int"],["str"],["bool"]],"rows":[{"name":"Alice","age":25,"city":"New York","want_candy":true},{"name":"Bob","age":32,"city":"San Francisco","want_candy":false},{"name":"Bob","age":32,"city":"Melbourne","want_candy":true},{"name":"Charlie","age":19,"city":"London","want_candy":true}]}
```

There is also a compact mode


```bash
make && ./psv -c << 'HEREDOC'
| Name    | Age [float] | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  |
| Charlie | 19  | London       |

| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
HEREDOC
```

which has a more compact representation

```json
[{"name":"Alice","age":25,"city":"New York"},{"name":"Bob","age":32.4,"city":"San Francisco"},{"name":"Bob","age":32,"city":null},{"name":"Charlie","age":19,"city":"London"}]
[{"name":"Alice","age":25,"city":"New York","want_candy":true},{"name":"Bob","age":32,"city":"San Francisco","want_candy":false},{"name":"Bob","age":32,"city":"Melbourne","want_candy":true},{"name":"Charlie","age":19,"city":"London","want_candy":true}]
```

Finally you can select table by ID

```bash
make && ./psv --id dog -c << 'HEREDOC'
{#cat}
| Name    | Age [float] | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  |
| Charlie | 19  | London       |

{#dog}
| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
HEREDOC
```

Which would output just the table marked as dog. Note that because we are in single table and compact row mode, this program will switch to row streaming mode. This allows for streaming very very large files with lots and lots of rows.

```json
{"name":"Alice","age":25,"city":"New York","want_candy":true}
{"name":"Bob","age":32,"city":"San Francisco","want_candy":false}
{"name":"Bob","age":32,"city":"Melbourne","want_candy":true}
{"name":"Charlie","age":19,"city":"London","want_candy":true}
```

To specify an output file:

```bash
make && ./psv -o test.json << 'HEREDOC'
...
HEREDOC
```

To parse Markdown tables from a file:

```bash
make && ./psv -o test.json testdoc.md
```


### Using with jq

You can pipe results from psv into jq

```bash
make && ./psv --id dog << 'HEREDOC' | jq
{#dog}
| Name    | Age [int] | City [str]   | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------ | ------------------------------ |
| Alice   | 25        | New York     | yes                            |
| Bob     | 32        | San Francisco| no                             |
| Bob     | 32        | Melbourne    | yes                            |
| Charlie | 19        | London       | yes                            |
HEREDOC
```

or


```bash
make && ./psv -t 1 << 'HEREDOC' | jq
{#dog}
| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
HEREDOC
```

Which would result in a pretty printed output by jq

```json
{
  "id": "dog",
  "headers": [
    "Name",
    "Age [int]",
    "City [str]",
    "Want Candy (jellybeans) [bool]"
  ],
  "keys": [
    "name",
    "age",
    "city",
    "want_candy"
  ],
  "data_annotation": [
    [],
    [
      "int"
    ],
    [
      "str"
    ],
    [
      "bool"
    ]
  ],
  "rows": [
    {
      "name": "Alice",
      "age": 25,
      "city": "New York",
      "want_candy": true
    },
    {
      "name": "Bob",
      "age": 32,
      "city": "San Francisco",
      "want_candy": false
    },
    {
      "name": "Bob",
      "age": 32,
      "city": "Melbourne",
      "want_candy": true
    },
    {
      "name": "Charlie",
      "age": 19,
      "city": "London",
      "want_candy": true
    }
  ]
}
```

You can also output csv by piping though jq

```bash
make && ./psv --id dog -c << 'HEREDOC' | jq -r 'map(.) | @csv'
{#dog}
| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
HEREDOC
```

which would result in this output

```csv
"Alice",25,"New York",true
"Bob",32,"San Francisco",false
"Bob",32,"Melbourne",true
"Charlie",19,"London",true
```

## Table Format

The .psv standard is currently in flux as we figure out what will work best, but this is a writeup about the current state and limitation of this psv.c convertor. In general the format looks like below and is based on [Github Flavored Markdown Tables (extension)](https://github.github.com/gfm/#tables-extension-) but with the addition of an optional [Consistent attribute syntax](https://talk.commonmark.org/t/consistent-attribute-syntax/272) header which is currently just for the table anchor ID for now.


```markdown
{#id}
| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
```

* How to deal with vertical bars in string: Let's just keep [Github Table example 200](https://github.github.com/gfm/#example-200
) approach of using `\|` to escape vertical bars (`\\` to escape the backslash itself). 
* Quote strings not supported in Github Flavored Markdown and for simplicity sake this behavior is carried over to psv
* Spaces between pipes and cell content are trimmed. 
  - DEV: I don't think we should be supporting `"` quoting, as I cannot imagine why anyone would want to have space in front or back of a string in the context of a markdown table (You would usually want quoted string if you are trying to store ascii art etc... but if that's the case then you really should be choosing a more flexible format... as supporting this would make the psv standard more ugly and complex to deal with). This would free `"` for normal usage without having to worry about escaping it.
* The header row must match the delimiter row in the number of cells. If not, a table will not be recognized [Example 202](https://github.github.com/gfm/#example-202)
* Excess table cell is ignored like in github markdown tables [Example 204](https://github.github.com/gfm/#example-204). This keeps parsing simpler.

We do diverge from Github Flavored Markdown in that a vertical bar is always expected in each line, while [ Example 199 ](https://github.github.com/gfm/#example-199) showed they can handle partially corrupted tables data rows. We don't to keep parsing simpler.

Also regarding conversion to json:

* All cells are assumed to be string by default and the user is expected to do the conversion themselves. This conservative data typing keeps the conversion process more consistent with less magic to go wrong.
  - Later on, we can figure out how to do semantic tagging and datatype annotation to more explicitly recommend a datatype and interpretation

* We use data annotations `[<annotation tag>]` to indicate how each cell should be interpreted. Below are the supported basic tags and how it is converted to json.

| data annotation | json value        | Notes                                    |
| --------------- | ----------------- | ---------------------------------------- |
| string          | string            | stores normal byte string                |
| str             | string            | stores normal byte string                |
| integer         | number            | round integer value                      |
| int             | number            | round integer value                      |
| float           | number            | floating point value                     |
| bool            | "true" or "false" | yes/no, y/n, true/false, active/inactive |


## Dev Tips:

### Memory Leak Detection (using Valgrind)

```bash
make && valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./psv -t 1 << 'HEREDOC'
| Name    | Age [int] | City [str]    | Want Candy (jellybeans) [bool] |
| ------- | --------- | ------------- | ------------------------------ |
| Alice   | 25        | New York      | yes                            |
| Bob     | 32        | San Francisco | no                             |
| Bob     | 32        | Melbourne     | yes                            |
| Charlie | 19        | London        | yes                            |
HEREDOC
```

```bash
make && valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./psv -t 1 test.psv
```

### GDB Testing

```bash
make && gdb --args ./psv -t 1 test.psv
```

### Doxygen

```bash
./bootstrap.sh
cd docs
doxygen

# Open HTML
xdg-open ./html/index.html

# Generate PDF and Open
make -C latex
xdg-open ./latex/refman.pdf
```

## Design Considerations

- **Integration with Existing Tools**: psv is intentionally designed to be simple and focused, serving as a specialized tool for Markdown table conversion. While jq offers extensive JSON manipulation capabilities, psv complements it by providing a straightforward solution specifically tailored for Markdown tables. This is by outputting json which jq can more easily parse.

- **Output Format**: The output JSON format is designed to mirror the structure of Markdown tables, with each table represented as a JSON object containing an ID, headers, and rows. This format aims to provide a clear and intuitive mapping from Markdown to JSON, making it easy to work with the converted data programmatically.

- **Partial Parsing of Consistent Attribute Syntax**: The decision to focus on parsing the `#id` attribute only, rather than the entire consistent attribute syntax, was made to simplify the implementation and streamline the conversion process for this MVP. Later on we can add extra feature.

