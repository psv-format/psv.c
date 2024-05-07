#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "psv_json.h"

static PsvDataAnnotationType get_basic_type(PsvTable *table, size_t header_column) {
    const PsvHeaderMetadataField *header_metadata = &table->header_metadata[header_column];
    if (header_metadata->data_annotation_tag_size > 0) {
        for (int i = 0; i < header_metadata->data_annotation_tag_size; i++) {    
            const PsvDataAnnotationType base_type = header_metadata->data_annotation_tags[i].type;
            if (base_type == PSV_DATA_ANNOTATION_INTEGER)
                return base_type;

            if (base_type == PSV_DATA_ANNOTATION_FLOAT)
                return base_type;
            
            if (base_type == PSV_DATA_ANNOTATION_BOOL)
                return base_type;
            
            if (base_type == PSV_DATA_ANNOTATION_TEXT)
                return base_type;
        }
    }

    return PSV_DATA_ANNOTATION_TEXT;
}

// Create JSON object of a single tabular row
cJSON *psv_json_create_table_single_row(PsvTable *table, char **data_row_entry) {
    cJSON *single_row_json = cJSON_CreateObject();
    for (int i = 0; i < table->num_headers; i++) {
        const PsvHeaderMetadataField *header_metadata = &table->header_metadata[i];
        const char *key = header_metadata->id;
        const char *data = data_row_entry[i];
        if (data) {
            const PsvDataAnnotationType basic_type = get_basic_type(table, i);
            switch (basic_type) {
                case PSV_DATA_ANNOTATION_INTEGER: {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNumber(atoi(data)));
                } break;
                case PSV_DATA_ANNOTATION_FLOAT: {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNumber(atof(data)));
                } break;
                case PSV_DATA_ANNOTATION_BOOL: {
                    const bool isTrue = strcmp(data, "true") == 0 || strcmp(data, "yes") == 0 || strcmp(data, "active") == 0 || strcmp(data, "y") == 0;
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateBool(isTrue));
                } break;
                case PSV_DATA_ANNOTATION_TEXT: {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
                } break;
                default: {
                    cJSON_AddItemToObject(single_row_json, key, cJSON_CreateString(data));
                }
            }
        } else {
            cJSON_AddItemToObject(single_row_json, key, cJSON_CreateNull());
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

        // Add data annotation
        cJSON *data_annotation_entry_json = cJSON_CreateArray();
        for (int j = 0; j < header_metadata->data_annotation_tag_size; j++) {
            cJSON_AddItemToArray(data_annotation_entry_json, cJSON_CreateString(header_metadata->data_annotation_tags[j].raw));
        }

        cJSON_AddItemToArray(data_annotation_json, data_annotation_entry_json);
    }
    cJSON_AddItemToObject(table_json, "data_annotation", data_annotation_json);

    cJSON *rows_json = psv_json_create_table_rows(table);
    cJSON_AddItemToObject(table_json, "rows", rows_json);
    return table_json;
}
