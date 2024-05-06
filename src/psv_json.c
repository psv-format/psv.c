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
            if (header_metadata->data_annotation_tag_size > 0) {
                // Data Annotation Available
                PsvDataAnnotationType base_type = header_metadata->data_annotation_tags[0].type;
                if (base_type == PSV_DATA_ANNOTATION_INTEGER) {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNumber(atoi(data)));
                    header_metadata->data_annotation_tags[0].processed = true;
                } else if (base_type == PSV_DATA_ANNOTATION_FLOAT) {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNumber(atof(data)));
                    header_metadata->data_annotation_tags[0].processed = true;
                } else if (base_type == PSV_DATA_ANNOTATION_BOOL) {
                    const bool isTrue = strcmp(data, "true") == 0 || strcmp(data, "yes") == 0 || strcmp(data, "active") == 0;
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateBool(isTrue));
                    header_metadata->data_annotation_tags[0].processed = true;
                } else if (base_type == PSV_DATA_ANNOTATION_TEXT) {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
                    header_metadata->data_annotation_tags[0].processed = true;
                } else {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
                }
            } else {
                // Data Annotation Not Used. Default to string handling
                cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
            }
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

    cJSON *data_annotation_json = cJSON_CreateArray();
    for (int i = 0; i < table->num_headers; i++) {
        PsvHeaderMetadataField *header_metadata = &table->header_metadata[i];

        cJSON *data_annotation_entry_json = cJSON_CreateArray();
        for (int j = 0; j < header_metadata->data_annotation_tag_size; j++) {
            if (!header_metadata->data_annotation_tags[j].processed) {
                cJSON_AddItemToArray(data_annotation_entry_json, cJSON_CreateString(header_metadata->data_annotation_tags[j].raw));
            }
        }

        cJSON_AddItemToArray(data_annotation_json, data_annotation_entry_json);
    }
    cJSON_AddItemToObject(table_json, "data_annotation", data_annotation_json);

    cJSON *rows_json = psv_json_create_table_rows(table);
    cJSON_AddItemToObject(table_json, "rows", rows_json);
    return table_json;
}
