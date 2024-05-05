#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "psv_json.h"


// Create JSON object of a single tabular row
cJSON *psv_json_create_table_single_row(PsvTable *table, char **data_row_entry) {
    cJSON *single_row_json = cJSON_CreateObject();
    for (int i = 0; i < table->num_headers; i++) {
        PsvHeaderMetadataField *header_metadata = &table->header_metadata[i];
        const char *key = header_metadata->id;
        const char *data = data_row_entry[i];
        if (data) {
#if 0
            // Dev Note: This is horribly inefficient as it is searching each row multiple times, but works!
            PsvBaseEncodingType base_encoding = psv_get_base_encoding_type(table->headers[j]);

            switch (base_encoding) {
            case PSV_BASE_TYPE_INTEGER: {
                cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNumber(atoi(data)));
                } break;

            case PSV_BASE_TYPE_FLOAT: {
                cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNumber(atof(data)));
                } break;

            case PSV_BASE_TYPE_BOOL: {
                bool isTrue = strcmp(data, "true") == 0 || strcmp(data, "yes") == 0 || strcmp(data, "active") == 0;
                cJSON_AddItemToObject(single_row_json, key, cJSON_CreateBool(isTrue));
                } break;

            // By default and for other values that cannot be stored in json cleanly, use string
            case PSV_BASE_TYPE_TEXT:
            case PSV_BASE_TYPE_HEX:
            case PSV_BASE_TYPE_BASE64:
            case PSV_BASE_TYPE_DATA_URI: {
                cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
                } break;
            }
#else
            cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
#endif
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
    for (int i = 0; i < table->num_headers; i++) {
        PsvHeaderMetadataField *header_metadata = &table->header_metadata[i];
        cJSON_AddItemToArray(headers_json, cJSON_CreateString(header_metadata->raw_header));
    }
    cJSON_AddItemToObject(table_json, "headers", headers_json);

    cJSON *keys_json = cJSON_CreateArray();
    for (int i = 0; i < table->num_headers; i++) {
        PsvHeaderMetadataField *header_metadata = &table->header_metadata[i];
        cJSON_AddItemToArray(keys_json, cJSON_CreateString(header_metadata->id));
    }
    cJSON_AddItemToObject(table_json, "keys", keys_json);

    cJSON *rows_json = psv_json_create_table_rows(table);
    cJSON_AddItemToObject(table_json, "rows", rows_json);
    return table_json;
}
