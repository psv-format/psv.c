#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "psv.h"

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
void psv_free_table(PsvTable *table) {

    // Free Tabular Header
    if (table->headers) {
        for (int i = 0; i < table->num_headers; i++) {
            free(table->headers[i]);
            table->headers[i] = NULL;
        }
        free(table->headers);
        table->headers = NULL;
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

    // Reset Metadata
    table->num_data_rows = 0;
    table->num_headers = 0;

    free(table);
}

// Grab A Table From A Stream Input
PsvTable *psv_parse_table(FILE *input, char *defaultTableID) {
    typedef enum {
        SCANNING,
        POTENTIAL_HEADER,
        DATA
    } State;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    State state = SCANNING;
    PsvTable *table = malloc(sizeof(PsvTable));
    *table = (PsvTable){0};

    while ((read = getline(&line, &len, input)) != -1) {
        if (state == SCANNING) {
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

                // Parse table ID anchor
                // TODO: Capture `.class`, `key=value` and `key="value"`
                char *hashPos = line + 1;
                while (*hashPos != '\0' && *hashPos != '#') {
                    if (*hashPos == ' ') {
                        hashPos++;
                    } else {
                        break;
                    }
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
                    table->headers[table->num_headers] = malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(table->headers[table->num_headers], trim_whitespace(token));
                    table->num_headers++;
                }

                // Check if at least one header field is detected
                if (table->num_headers > 0) {
                    state = POTENTIAL_HEADER;
                    if (table->id[0] == '\0') {
                        strcpy(table->id, defaultTableID);
                    }
                } else {
                    psv_free_table(table);
                }
            } else {
                // Not a table header

                // Free table in case we found Consistent Attribute Syntax but no table
                // Since it might be for a different block instead
                psv_free_table(table);
            }
        } else if (state == POTENTIAL_HEADER) {
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
                    state = DATA;
                } else {
                    // Mismatch with headers, free the allocated memory
                    state = SCANNING;
                    psv_free_table(table);
                }
            }
        } else if (state == DATA) {
            if (line[0] == '|') {
                // Trim '|' on right hand side
                for (int i = read - 1; i > 0; i--) {
                    if (line[i] == '|') {
                        line[i] = '\0';
                        break;
                    }
                }

                // Add a new row
                table->data_rows = realloc(table->data_rows, (table->num_data_rows + 1) * sizeof(char **));
                table->data_rows[table->num_data_rows] = malloc(table->num_headers * sizeof(char *));
                for (int i = 0 ; i < table->num_headers ; i++) {
                    // Set every header data entry in a row to NULL
                    // (This is to tell valgrind that we intentionally want each entry as empty in case of missing entries)
                    table->data_rows[table->num_data_rows][i] = NULL;
                }

                // Split and Cache data row
                char *tokenization_state = NULL;
                int num_data_col = 0;
                char *token;
                while ((token = tokenize_escaped_delim(line + 1, '|', &tokenization_state)) != NULL) {
                    table->data_rows[table->num_data_rows][num_data_col] = malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(table->data_rows[table->num_data_rows][num_data_col], trim_whitespace(token));
                    num_data_col++;
                }

                // Keep track of data row
                table->num_data_rows++;
            } else {
                // End of Table detected
                break;
            }
        }
    }

    // Release getline's buffer
    free(line);

    // Check if we have a table to return to caller
    if (state != DATA)
    {
        psv_free_table(table);
        return NULL;
    }

    return table;
}
