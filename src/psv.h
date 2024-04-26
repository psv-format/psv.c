#ifndef PSV_H
#define PSV_H
#include <stdio.h>

#define PSV_TABLE_ID_MAX 255

typedef struct {
    char id[PSV_TABLE_ID_MAX + 1];
    int num_headers;
    char **headers;
    int num_data_rows;
    char ***data_rows;
} PsvTable;


void psv_free_table(PsvTable *table);
PsvTable *psv_parse_table(FILE *input, char *defaultTableID);

#endif
