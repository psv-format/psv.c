/**
 * @file psv.h
 * @brief Processes And Parses PSV Content
 *
 * Copyright (C) 2024-2024 Brian Khuu <contact@briankhuu.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef PSV_H
#define PSV_H
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include "cbor_constants.h"

#define PSV_TABLE_ID_MAX 255
#define PSV_HEADER_ID_MAX 255

typedef enum {
    // This enum defines the most base types that all PSV parsers are expected to handle (except for lite versions) as standard.
    // By default, if the datatype of a cell is unknown, it is safer to fall back to string representation.

    PSV_DATA_ANNOTATION_UNKNOWN,

    // Basic JSON compatible atomic value
    PSV_DATA_ANNOTATION_TEXT,        ///< [string][str] Used to store text strings, which can represent various types of textual data. Similar to SQLite TEXT.
    PSV_DATA_ANNOTATION_INTEGER,     ///< [integer][int] Used to store integer numbers, suitable for representing whole numbers without decimal points. Similar to SQLite INTEGER.
    PSV_DATA_ANNOTATION_FLOAT,       ///< [float] Used to store floating-point numbers, allowing for decimal numbers with both integer and fractional parts. Similar to SQLite REAL.
    PSV_DATA_ANNOTATION_BOOL,        ///< [bool] Used to store boolean values, represented as integers (0 for false, 1 for true). Similar to SQLite INTEGER.

    // Basic CBOR Binary compatible atomic value
    PSV_DATA_ANNOTATION_HEX,         ///< [hex] Used to store binary data in hexadecimal format. Similar to SQLite BLOB.
    PSV_DATA_ANNOTATION_BASE64,      ///< [base64] Used to store binary data in Base64 encoding. Similar to SQLite BLOB.
    PSV_DATA_ANNOTATION_DATA_URI,    ///< [dataURI] Used to store binary data encoded as data URIs. Similar to SQLite BLOB.

    // Semantics
    PSV_DATA_ANNOTATION_DATETIME,    ///< [datetime] Standard date/time string https://en.wikipedia.org/wiki/ISO_8601
    PSV_DATA_ANNOTATION_UUID,        ///< [uuid] Universally unique identifier https://en.wikipedia.org/wiki/Universally_unique_identifier

    PSV_DATA_ANNOTATION_MAX
} PsvDataAnnotationType;

// Note: UUIDs, DATETIMEs, and other semantic types are represented using CBOR semantic tags whose numeric id
// will essentially be synced to CBOR tags (https://www.iana.org/assignments/cbor-tags/cbor-tags.xhtml)
// A lookup table may be provided for mapping common semantic tags to CBOR tag numbers.
// If not found then 0xFFFFFFFFFFFFFFFF is set

// PSV Line Parsing State
typedef enum {
    PSV_TABLE_PARSING_SCANNING = 0,
    PSV_TABLE_PARSING_POTENTIAL_HEADER,
    PSV_TABLE_PARSING_DATA_ROW,
    PSV_TABLE_PARSING_END,
} PsvTableParsingState;

typedef struct {
    char* raw;
    PsvDataAnnotationType type;
    cbor_tag_t tag;

    // Use this flag to track if a tag has already been processed downstream
    // e.g. string converted to float when '[float]'
    bool processed;
} PsvDataAnnotationField;

typedef struct {
    char id[PSV_HEADER_ID_MAX];
    char *raw_header;

    // These are data annotation tags that are derived from `[<data annotation tag>]` list
    // in the header fields. These data annotation tags stacks as you may have a situation
    // such as a "0xD823456789ABCD" string that should be decoded first in `[hex]` then 
    // decoded as a `[cbor]` payload. Keeping with general english conventions all tags
    // are interpreted from left to right conventions.
    size_t data_annotation_tag_size;
    PsvDataAnnotationField *data_annotation_tags;

    // TODO: Later on we may want to also capture the consistent attribute syntax here as well

} PsvHeaderMetadataField;
typedef PsvHeaderMetadataField* PsvHeaderMetadataColumns;

// Data Rows Typedefs
typedef char* PsvDataField;
typedef PsvDataField* PsvDataRow;
typedef PsvDataRow* PsvDataRows;

// Table Structs
typedef struct {
    PsvTableParsingState parsing_state;
    char id[PSV_TABLE_ID_MAX];

    int num_headers;
    PsvHeaderMetadataField *header_metadata;

    int num_data_rows;
    PsvDataRow *data_rows;
} PsvTable;

void psv_free_table(PsvTable **tablePtr);

PsvTable * psv_parse_table_header(FILE *input, char *defaultTableID);

PsvDataRow psv_parse_table_row(FILE *input, PsvTable *table);
void psv_parse_table_free_row(PsvTable *table, PsvDataRow *dataRowPtr);
bool psv_parse_skip_table_row(FILE *input, PsvTable *table);

PsvTable *psv_parse_table(FILE *input, char *defaultTableID);

#endif
