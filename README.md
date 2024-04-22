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

### Usage

To parse Markdown tables from standard input:

```bash
make && ./psv << 'HEREDOC'
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  |
| Charlie | 19  | London       |

{#test2}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```

The above command would give a response that may look like below

```json
[
    {
        "id": "table1",
        "headers": ["Name", "Age", "City"],
        "rows": [
            {"Name": "Alice", "Age": 25, "City": "New York"},
            {"Name": "Bob", "Age": 32.4, "City": "San Francisco"},
            {"Name": "Bob", "Age": 32},
            {"Name": "Charlie", "Age": 19, "City": "London"}
        ]
    }
    ,
    {
        "id": "test2",
        "headers": ["Name", "Age", "City"],
        "rows": [
            {"Name": "Alice", "Age": 25, "City": "New York"},
            {"Name": "Bob", "Age": 32, "City": "San Francisco"},
            {"Name": "Bob", "Age": 32, "City": "Melbourne"},
            {"Name": "Charlie", "Age": 19, "City": "London"}
        ]
    }
]
```

There is also a compact mode


```bash
make && ./psv -c << 'HEREDOC'
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  |
| Charlie | 19  | London       |

| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```

which has a more compact representation

```json
[
   [
       {"Name": "Alice", "Age": 25, "City": "New York"},
       {"Name": "Bob", "Age": 32.4, "City": "San Francisco"},
       {"Name": "Bob", "Age": 32},
       {"Name": "Charlie", "Age": 19, "City": "London"}
   ]
   ,
   [
       {"Name": "Alice", "Age": 25, "City": "New York"},
       {"Name": "Bob", "Age": 32, "City": "San Francisco"},
       {"Name": "Bob", "Age": 32, "City": "Melbourne"},
       {"Name": "Charlie", "Age": 19, "City": "London"}
   ]
]
```

Finally you can select table by ID

```bash
make && ./psv --id dog -c << 'HEREDOC'
{#cat}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32.4  | San Francisco|
| Bob     | 32  |
| Charlie | 19  | London       |

{#dog}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```

Which would output just the table marked as dog

```json
   [
       {"Name": "Alice", "Age": 25, "City": "New York"},
       {"Name": "Bob", "Age": 32, "City": "San Francisco"},
       {"Name": "Bob", "Age": 32, "City": "Melbourne"},
       {"Name": "Charlie", "Age": 19, "City": "London"}
   ]
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
./psv -t 1 -c << 'HEREDOC' | jq
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```

Which would result in a pretty printed output by jq

```json
[
  {
    "Name": "Alice",
    "Age": 25,
    "City": "New York"
  },
  {
    "Name": "Bob",
    "Age": 32,
    "City": "San Francisco"
  },
  {
    "Name": "Bob",
    "Age": 32,
    "City": "Melbourne"
  },
  {
    "Name": "Charlie",
    "Age": 19,
    "City": "London"
  }
]
```



You can also output csv by piping though jq

```bash
./psv -t 1 -c << 'HEREDOC' | jq -r '.[] | [.Name, .Age, .City] | @csv'
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
HEREDOC
```

which would result in this output

```csv
"Alice",25,"New York"
"Bob",32,"San Francisco"
"Bob",32,"Melbourne"
"Charlie",19,"London"
```

## Table Format

The .psv standard is currently in flux as we figure out what will work best, but this is a writeup about the current state and limitation of this psv.c convertor. In general the format looks like below

```
{#id}
| Name    | Age | City         |
| ------- | --- | ------------ |
| Alice   | 25  | New York     |
| Bob     | 32  | San Francisco|
| Bob     | 32  | Melbourne    |
| Charlie | 19  | London       |
```

But key thing to note is that there is `{}` which is the consistent attribute syntax which will eventually contain metadata as needed, but for now is just the ID field.

We also have a standard markdown table. Just note that during the conversation to json these things are to be noted:
* Whitespace in front and back is trimmed
  - DEV: I don't think we should be supporting `"` quoting, as I cannot imagine why anyone would want to have space in front or back of a string in the context of a markdown table (You would usually want quoted string if you are trying to store ascii art etc... but if that's the case then you really should be choosing a more flexible format... as supporting this would make the psv standard more ugly and complex to deal with). This would free `"` for normal usage without having to worry about escaping it.
* Content of each cell is always assumed to be a straight up json string, so you need to escape `"`, `\`, `/`, backspace, formfeed, linefeed, carriage, return, tab, etc...
  - TODO: Later on we really need to take advantage of the `|` syntax and minimise the amount of 'escaping' required to just unicode, carriage return, linefeed, formfeed, backspace and `\`. This would allow us to free `"`, `/` for normal usage.
* If a cell has only json compatible numerical or `true` or `false` json booleans, then the current behavior is to copy it to the data field unquoted.
  - DEV: This can be an argument for having `"` quoting implemented if there are cases where `true` is actually a string. However my argument would be that's why I added `{}` consistent attribute support on top. We could use that feature to add a schema to notify that a particular column should be interpreted as a string. The argument you would then need, is to argue if there is ever a case where a colum would have a mix of string and other datatype.

## Dev Tips:

### Memory Leak Detection (using Valgrind)

```bash
make && valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         --log-file=valgrind-out.txt \
         ./psv --output test.json testdoc.md testdoc.md
```

## Design Considerations

- **Integration with Existing Tools**: psv is intentionally designed to be simple and focused, serving as a specialized tool for Markdown table conversion. While jq offers extensive JSON manipulation capabilities, psv complements it by providing a straightforward solution specifically tailored for Markdown tables. This is by outputting json which jq can more easily parse.

- **Output Format**: The output JSON format is designed to mirror the structure of Markdown tables, with each table represented as a JSON object containing an ID, headers, and rows. This format aims to provide a clear and intuitive mapping from Markdown to JSON, making it easy to work with the converted data programmatically.

- **Partial Parsing of Consistent Attribute Syntax**: The decision to focus on parsing the `#id` attribute only, rather than the entire consistent attribute syntax, was made to simplify the implementation and streamline the conversion process for this MVP. Later on we can add extra feature.

