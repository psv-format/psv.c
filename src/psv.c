#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "psv.h"

/**
 * @brief Generates a JSON key from a header string.
 * 
 * This function generates a JSON key from a given header string, 
 * ensuring that the key is valid and follows certain conventions.
 * 
 * @param header The header string from which to generate the JSON key.
 * @param jsonKeyBuffer A buffer to store the generated JSON key.
 * @param bufferSize The size of the jsonKeyBuffer.
 * 
 * @return A pointer to the generated JSON key string.
 * 
 * @remarks The generated JSON key will be stored in jsonKeyBuffer. 
 *          The function ensures that the generated key is valid and 
 *          follows certain conventions:
 *          - Special characters and spaces are replaced with underscores.
 *          - Only one underscore is allowed between words.
 *          - The generated key does not start or end with an underscore.
 *          - If bufferSize is insufficient to store the entire key, 
 *            the key will be truncated to fit within the buffer size.
 */
static char* generateJSONKey(const char* header, char* jsonKeyBuffer, size_t bufferSize) {
    char* writePtr = jsonKeyBuffer;

    // Iterate through the header
    for (int i = 0; header[i] != '\0' && bufferSize > 1; i++) {
        char c = header[i];
        
        // Break if parentheses, brackets, or braces are found
        if (c == '(' || c == '[' || c == '{')
            break;

        // Convert to lowercase
        c = tolower(c);

        // Replace special characters and spaces with underscores
        if (!isalnum(c) && c != '_')
            c = '_';

        // Ensure only one underscore between words and that we don't start with underscore
        if (c == '_' && ((writePtr == NULL || writePtr[-1] == '_') || (writePtr == jsonKeyBuffer)))
            continue;

        *writePtr++ = c;
        bufferSize--;
    }

    // Remove trailing underscore, if any
    if (writePtr > 0 && *(writePtr - 1) == '_') {
        // Trailing _ found, end the string at the _
        *(writePtr - 1) = '\0';
    } else {
        // No trailing _ found, just end the string normally
        *writePtr = '\0';
    }

    return jsonKeyBuffer;
}

static char *tokenize_escaped_delim(char *str, char delim, char **tokenization_state) {
    if (str == NULL)
        return NULL;

    char *token_start = (*tokenization_state == NULL) ? str : *tokenization_state + 1;
    char *token_end = token_start;

    while (*token_end != '\0') {
        if (*token_end == '\\') {
            if (*(token_end + 1) == '\\'
                || *(token_end + 1) == delim
                || ispunct(*(token_end + 1))) {
                // Handle escaped backslash, delimiter and punctuation
                memmove(token_end, token_end + 1, strlen(token_end + 1) + 1);
                token_end++;
            }
        } else if (*token_end == delim) {
            // Found delimiter
            *token_end = '\0';
            *tokenization_state = token_end;
            return token_start;
        }
        token_end++;
    }

    if (*token_start == '\0')
        return NULL;

    *tokenization_state = token_end - 1;
    return token_start;
}

static char *trim_whitespace(char *str) {
    // Trim leading space
    while (isspace((unsigned char)*str)) {
        str++;
    }

    // Trim trailing space
    if (*str != '\0') {
        char *end = str + strlen(str) - 1;
        while (end > str && isspace((unsigned char)*end)) {
            end--;
        }
        *(end + 1) = '\0';
    }

    return str;
}

static void parse_consistent_attribute_syntax_id(const char *buffer, PsvTable *table) {
    while (*buffer != '\0' && *buffer != '#') {
        if (*buffer == ' ') {
            buffer++;
        } else {
            // ID must be in front of all other class or key:value
            return;
        }
    }
    if (*buffer == '#') {
        // ID found
        buffer++; // Move past the '#' character
        // Copy the ID until a space or '}' is encountered
        int i = 0;
        while ((i < PSV_TABLE_ID_MAX) && (*buffer != '\0') && *buffer != ' ' && *buffer != '}') {
            table->id[i] = *buffer++;
            i++;
        }
        table->id[i] = '\0'; // Null-terminate the ID
    }
}

// Free Memory Allocation For A Table
static void psv_clear_table(PsvTable *table) {

    // Free Tabular Header
    if (table->headers) {
        for (int i = 0; i < table->num_headers; i++) {
            free(table->headers[i]);
            table->headers[i] = NULL;
        }
        free(table->headers);
        table->headers = NULL;
    }

    // Free JSON keys Header
    if (table->json_keys) {
        for (int i = 0; i < table->num_headers; i++) {
            free(table->json_keys[i]);
            table->json_keys[i] = NULL;
        }
        free(table->json_keys);
        table->json_keys = NULL;
    }

    // Free Tabular Data
    if (table->data_rows) {
        for (int i = 0; i < table->num_data_rows; i++) {
            for (int j = 0; j < table->num_headers; j++) {
                free(table->data_rows[i][j]);
                table->data_rows[i][j] = NULL;
            }
            free(table->data_rows[i]);
            table->data_rows[i] = NULL;
        }
        free(table->data_rows);
        table->data_rows = NULL;
    }

    // Zero out all state and variables
    *table = (PsvTable){0};
}

// Free Memory Allocation For A Table
void psv_free_table(PsvTable **tablePtr) {

    // Free and Clear internal content of the table
    psv_clear_table(*tablePtr);

    // Finally, free the memory allocated for the table structure itself
    free(*tablePtr);

    // Set the pointer to NULL in the caller's scope so other parts of the problem knows it's empty
    *tablePtr = NULL;
}

PsvTable * psv_parse_table_header(FILE *input, char *defaultTableID) {
    PsvTable *table = malloc(sizeof(PsvTable));
    *table = (PsvTable){0};

    ssize_t read;
    char *line = NULL;
    size_t len = 0;
    while ((read = getline(&line, &len, input)) != -1) {
        switch (table->parsing_state) {
        case PSV_PARSING_SCANNING:
            if (line[0] == '{') {
                // Parse Consistent attribute syntax https://talk.commonmark.org/t/consistent-attribute-syntax/272

                // Check for metadata
                bool closing_bracket = false;

                // Trim '}' on right hand side
                for (int i = read - 1; i > 0; i--) {
                    if (line[i] == '}') {
                        line[i] = '\0';
                        closing_bracket = true;
                        break;
                    }
                }

                size_t str_line = strlen(line + 1);
                if (!closing_bracket && str_line == 0) {
                    continue;
                }

                parse_consistent_attribute_syntax_id(trim_whitespace(line + 1), table);
            } else if (line[0] == '|') {
                table->num_headers = 0;
                table->num_data_rows = 0;

                // Trim '|' on right hand side
                for (int i = read - 1; i > 0; i--) {
                    if (line[i] == '|') {
                        line[i] = '\0';
                        break;
                    }
                }

                // Split and Cache header
                char *tokenization_state = NULL;
                char *token;
                while ((token = tokenize_escaped_delim(line + 1, '|', &tokenization_state)) != NULL) {
                    table->headers = realloc(table->headers, (table->num_headers + 1) * sizeof(char *));
                    table->json_keys = realloc(table->json_keys, (table->num_headers + 1) * sizeof(char *));

                    const char * full_header = trim_whitespace(token);
                    const unsigned long full_header_buffer_size = strlen(token) + 1;

                    table->headers[table->num_headers] = malloc((full_header_buffer_size) * sizeof(char));
                    strcpy(table->headers[table->num_headers], full_header);

                    table->json_keys[table->num_headers] = malloc((full_header_buffer_size) * sizeof(char));
                    generateJSONKey(full_header, table->json_keys[table->num_headers], full_header_buffer_size);

                    table->num_headers++;
                }

                // Check if at least one header field is detected
                if (table->num_headers > 0) {
                    table->parsing_state = PSV_PARSING_POTENTIAL_HEADER;
                    if (table->id[0] == '\0') {
                        strcpy(table->id, defaultTableID);
                    }
                } else {
                    psv_clear_table(table);
                }
            } else {
                // Not a table header

                // Clear table in case we found Consistent Attribute Syntax but no table
                // Since it might be for a different block instead
                psv_clear_table(table);
            }
            break;
        
        case PSV_PARSING_POTENTIAL_HEADER:
            // Check if actual header by checking if there is enough '|---|'
            if (line[0] == '|') {
                // Trim '|' on right hand side
                for (int i = read - 1; i > 0; i--) {
                    if (line[i] == '|') {
                        line[i] = '\0';
                        break;
                    }
                }

                // Split and check header separators
                char *tokenization_state = NULL;
                int num_header_separators = 0;
                char *token;
                while ((token = tokenize_escaped_delim(line + 1, '|', &tokenization_state)) != NULL) {
                    if (strstr(token, "---")) {
                        num_header_separators++;
                    }
                }

                // Check if possible table signature is valid
                if (table->num_headers == num_header_separators) {
                    // It's most likely a markdown table
                    // We can now safely start parsing the data rows
                    table->parsing_state = PSV_PARSING_DATA_ROW;
                } else {
                    // Mismatch with headers, free the allocated memory
                    table->parsing_state = PSV_PARSING_SCANNING;
                    psv_clear_table(table);
                }
            }
            break;
        case PSV_PARSING_DATA_ROW:
            break;
        case PSV_PARSING_END:
            break;
        }

        // Release getline's buffer
        free(line);
        line = NULL;
        len = 0;

        if (table->parsing_state == PSV_PARSING_DATA_ROW) {
            break;
        }
    }

    if (table->parsing_state != PSV_PARSING_DATA_ROW) {
        psv_free_table(&table);
    }

    // Release getline's buffer
    free(line);
    return table;
}

PsvDataRow psv_parse_table_row(FILE *input, PsvTable *table) {

    // Cannot return row if not in data row parsing state
    if (table->parsing_state != PSV_PARSING_DATA_ROW)
        return NULL;

    PsvDataRow data_row = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    if ((read = getline(&line, &len, input)) != -1) {
        if (line[0] == '|') {
            // Trim '|' on right hand side
            for (int i = read - 1; i > 0; i--) {
                if (line[i] == '|') {
                    line[i] = '\0';
                    break;
                }
            }

            // Add a new row and prefill with NULL
            data_row = malloc(table->num_headers * sizeof(char *));
            for (int i = 0 ; i < table->num_headers ; i++) {
                // Set every header data entry in a row to NULL
                // (This is to tell valgrind that we intentionally want each entry as empty in case of missing entries)
                data_row[i] = NULL;
            }

            // Split and record each cell
            char *tokenization_state = NULL;
            for (int i = 0 ; i < table->num_headers ; i++) {
                char *token = tokenize_escaped_delim(line + 1, '|', &tokenization_state);
                if (token == NULL)
                    break;

                data_row[i] = malloc((strlen(token) + 1) * sizeof(char));
                strcpy(data_row[i], trim_whitespace(token));
            }
        } else {
            // End of Table detected
            table->parsing_state = PSV_PARSING_END;
        }
    }
    
    // Release getline's buffer
    free(line);
    return data_row;
}

void psv_parse_table_free_row(PsvTable *table, PsvDataRow *dataRowPtr) {
    PsvDataRow data_row = *dataRowPtr;
    // Free Rows
    for (int j = 0; j < table->num_headers; j++) {
        free(data_row[j]);
        data_row[j] = NULL;
    }
    free(*dataRowPtr);
    *dataRowPtr = NULL;
}

bool psv_parse_skip_table_row(FILE *input, PsvTable *table) {

    // Cannot return row if not in data row parsing state
    if (table->parsing_state != PSV_PARSING_DATA_ROW)
        return NULL;

    bool row_found = false;
    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, input) != -1) {
        if (line[0] == '|') {
            // Is a data row
            row_found = true;
        } else {
            // End of Table detected
            table->parsing_state = PSV_PARSING_END;
        }
    }

    // Release getline's buffer
    free(line);
    return row_found;
}

// Grab A Table From A Stream Input
PsvTable *psv_parse_table(FILE *input, char *defaultTableID) {
    PsvTable *table = psv_parse_table_header(input, defaultTableID);
    if (table == NULL) {
        return NULL;
    }

    char **data_row = NULL;
    while ((data_row = psv_parse_table_row(input, table)) != NULL) {
        table->data_rows = realloc(table->data_rows, (table->num_data_rows + 1) * sizeof(char **));
        table->data_rows[table->num_data_rows] = data_row;
        table->num_data_rows++;
    }

    return table;
}
