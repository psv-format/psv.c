#ifndef PSV_H
#define PSV_H
#include <stdio.h>

#define PSV_TABLE_ID_MAX 255

// PSV Line Parsing State
typedef enum {
    PSV_PARSING_SCANNING = 0,
    PSV_PARSING_POTENTIAL_HEADER,
    PSV_PARSING_DATA_ROW,
    PSV_PARSING_END,
} PsvParsingState;


typedef struct {
    PsvParsingState parsing_state;
    char id[PSV_TABLE_ID_MAX + 1];
    int num_headers;
    char **headers;
    int num_data_rows;
    char ***data_rows;
} PsvTable;

void psv_free_table(PsvTable **tablePtr);

PsvTable * psv_parse_table_header(FILE *input, char *defaultTableID);

char **psv_parse_table_row(FILE *input, PsvTable *table);

PsvTable *psv_parse_table(FILE *input, char *defaultTableID);

#endif
