#include <stdbool.h>
#include "psv_json.h"

// Create JSON object of a single tabular row
cJSON *psv_json_create_table_single_row(PsvTable *table, char **data_row_entry) {
    cJSON *single_row_json = cJSON_CreateObject();
    for (int j = 0; j < table->num_headers; j++) {
        const char *key = table->headers[j];
        const char *data = data_row_entry[j];
        if (data) {
            cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
        }
    }
    return single_row_json;
}

// Create JSON array of table rows
cJSON *psv_json_create_table_rows(PsvTable *table) {
    cJSON *rows_json = cJSON_CreateArray();
    for (int i = 0; i < table->num_data_rows; i++) {
        cJSON_AddItemToArray(rows_json, psv_json_create_table_single_row(table, table->data_rows[i]));
    }
    return rows_json;
}

// Create JSON object representing a table
cJSON *psv_json_create_table_json(PsvTable *table) {
    cJSON *table_json = cJSON_CreateObject();
    cJSON_AddItemToObject(table_json, "id", cJSON_CreateString(table->id));

    cJSON *headers_json = cJSON_CreateArray();
    for (int j = 0; j < table->num_headers; j++) {
        cJSON_AddItemToArray(headers_json, cJSON_CreateString(table->headers[j]));
    }
    cJSON_AddItemToObject(table_json, "headers", headers_json);

    cJSON *rows_json = psv_json_create_table_rows(table);
    cJSON_AddItemToObject(table_json, "rows", rows_json);
    return table_json;
}
