#ifndef PSV_JSON_H
#define PSV_JSON_H

#include "psv.h"
#include "cJSON.h"

cJSON *psv_json_create_table_single_row(PsvTable *table, char **data_row_entry);
cJSON *psv_json_create_table_rows(PsvTable *table);
cJSON *psv_json_create_table_json(PsvTable *table);

#endif /* PSV_JSON_H */