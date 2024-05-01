#ifndef PSV_H
#define PSV_H
#include <stdio.h>

#define PSV_TABLE_ID_MAX 255
#define PSV_HEADER_ID_MAX 255

typedef char** PsvDataRow;
typedef char*** PsvDataRows;

// PSV Line Parsing State
typedef enum {
    PSV_PARSING_SCANNING = 0,
    PSV_PARSING_POTENTIAL_HEADER,
    PSV_PARSING_DATA_ROW,
    PSV_PARSING_END,
} PsvParsingState;

typedef enum {
    // This enum defines the most base types that all PSV parsers are expected to handle.
    // By default, if the datatype of a cell is unknown, it is safer to fall back to string representation.

    PSV_BASE_TYPE_TEXT = 0,    ///< [string][str] Used to store text strings, which can represent various types of textual data. Similar to SQLite TEXT.
    PSV_BASE_TYPE_INTEGER,     ///< [integer][int] Used to store integer numbers, suitable for representing whole numbers without decimal points. Similar to SQLite INTEGER.
    PSV_BASE_TYPE_FLOAT,       ///< [float] Used to store floating-point numbers, allowing for decimal numbers with both integer and fractional parts. Similar to SQLite REAL.
    PSV_BASE_TYPE_BOOL,        ///< [bool] Used to store boolean values, represented as integers (0 for false, 1 for true). Similar to SQLite INTEGER.
    PSV_BASE_TYPE_HEX,         ///< [hex] Used to store binary data in hexadecimal format. Similar to SQLite BLOB.
    PSV_BASE_TYPE_BASE64,      ///< [base64] Used to store binary data in Base64 encoding. Similar to SQLite BLOB.
    PSV_BASE_TYPE_DATA_URI,    ///< [dataURI] Used to store binary data encoded as data URIs. Similar to SQLite BLOB.
    PSV_BASE_TYPE_MAX
} PsvBaseEncodingType;

typedef enum {
    // This enum defines optional complex types that can be derived from base types.

    PSV_INTERMEDIATE_TYPE_RAW = 0, ///< No intermediate type required. Cell content is processed as is.
    PSV_INTERMEDIATE_TYPE_JSON,    ///< [json] Process content as JSON. Allows for structured data representation.
    PSV_INTERMEDIATE_TYPE_CBOR,    ///< [cbor] Process content as CBOR. Provides compact binary encoding.
    PSV_INTERMEDIATE_TYPE_LIST,    ///< [list] Process content as single line CSV. Equivalent to JSON array, but easier to write.
    PSV_INTERMEDIATE_TYPE_MAX
} PsvIntermediateType;

// Note: UUIDs, DATETIMEs, and other semantic types are represented using CBOR semantic tags.
// A lookup table may be provided for mapping common semantic tags to CBOR tag numbers.
// The choice of base and intermediate types aims to provide flexibility, efficiency, and compatibility within the PSV parsing ecosystem.
// For now this is not implemented but will be part of the spec

typedef struct {
    PsvParsingState parsing_state;
    char id[PSV_TABLE_ID_MAX];

    int num_headers;
    char **headers;
    char **json_keys;

    int num_data_rows;
    char ***data_rows;
} PsvTable;

void psv_free_table(PsvTable **tablePtr);

PsvTable * psv_parse_table_header(FILE *input, char *defaultTableID);

PsvDataRow psv_parse_table_row(FILE *input, PsvTable *table);
void psv_parse_table_free_row(PsvTable *table, PsvDataRow *dataRowPtr);
bool psv_parse_skip_table_row(FILE *input, PsvTable *table);

PsvTable *psv_parse_table(FILE *input, char *defaultTableID);

#endif
