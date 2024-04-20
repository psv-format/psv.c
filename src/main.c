#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

typedef enum {
    SCANNING,
    POTENTIAL_HEADER,
    DATA
} State;

typedef struct {
    int num_headers;
    char **headers;
    int num_data_rows;
    char ***data_rows;
} Table;

char *trim_whitespace(char *str) {
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

void free_table(Table *table) {
    // Free Tabular Header
    for (int i = 0; i < table->num_headers; i++) {
        free(table->headers[i]);
    }
    free(table->headers);
    table->headers = NULL;

    // Free Tabular Data
    for (int i = 0; i < table->num_data_rows; i++) {
        for (int j = 0; j < table->num_headers; j++) {
            free(table->data_rows[i][j]);
        }
        free(table->data_rows[i]);
    }
    free(table->data_rows);
    table->data_rows = NULL;

    // Reset Metadata
    table->num_data_rows = 0;
    table->num_headers = 0;
}

// Function to check if a string represents a valid JSON number
bool is_number(const char *value) {
    char *endptr;
    strtod(value, &endptr);
    return *endptr == '\0';
}

// Function to check if a string represents a valid JSON boolean
bool is_boolean(const char *value) {
    return strcmp(value, "true") == 0 || strcmp(value, "false") == 0;
}

Table *parse_table(FILE *input) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    State state = SCANNING;
    Table *table = malloc(sizeof(Table));
    table->num_headers = 0;
    table->headers = NULL;
    table->num_data_rows = 0;
    table->data_rows = NULL;

    while ((read = getline(&line, &len, input)) != -1) {
        if (state == SCANNING) {
            if (line[0] == '|') {
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
                char *token = strtok(line + 1, "|");
                while (token != NULL) {
                    table->headers = realloc(table->headers, (table->num_headers + 1) * sizeof(char *));
                    table->headers[table->num_headers] = malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(table->headers[table->num_headers], trim_whitespace(token));
                    table->num_headers++;
                    token = strtok(NULL, "|");
                }

                // Check if at least one header field is detected
                if (table->num_headers > 0) {
                    state = POTENTIAL_HEADER;
                } else {
                    free_table(table);
                }
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
                int num_header_separators = 0;
                char *token = strtok(line + 1, "|");
                while (token != NULL) {
                    if (strstr(token, "---")) {
                        num_header_separators++;
                    }
                    token = strtok(NULL, "|");
                }

                // Check if possible table signature is valid
                if (table->num_headers == num_header_separators) {
                    // It's most likely a markdown table
                    // We can now safely start parsing the data rows
                    state = DATA;
                } else {
                    // Mismatch with headers, free the allocated memory
                    state = SCANNING;
                    free_table(table);
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

                // Split and Cache data row
                int num_data_col = 0;
                char *token = strtok(line + 1, "|");
                while (token != NULL) {
                    table->data_rows[table->num_data_rows][num_data_col] = malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(table->data_rows[table->num_data_rows][num_data_col], trim_whitespace(token));
                    num_data_col++;
                    token = strtok(NULL, "|");
                }
                // Keep track of data row
                table->num_data_rows++;
            } else {
                // End of Table detected
                break;
            }
        }
    }

    free(line);
    return table;
}

void print_json(Table *table) {
    printf("[\n");
    for (int i = 0; i < table->num_data_rows; i++) {
        printf("{");
        for (int j = 0; j < table->num_headers; j++) {
            const char *key=table->headers[j];
            const char *data=table->data_rows[i][j];
            if (!data) {
                // Omitting field if data is missing was chosen over using 'null' 
                // because it keeps the JSON output cleaner and more concise.
                // This makes it clearer that the data is missing without adding unnecessary clutter to the JSON structure.
                continue;
            }

            // Add comma to separate key:data fields
            if (j != 0) {
                printf(", ");
            }

            // Check if the data is a valid JSON number or boolean
            if (is_number(data) || is_boolean(data)) {
                // If the data is a number or boolean, print it without quotes
                printf("\"%s\": %s", key, data);
            } else {
                // Otherwise, print it with quotes
                printf("\"%s\": \"%s\"", key, data);
            }
        }
        printf("}");
        if (i < table->num_data_rows - 1) {
            printf(",\n");
        } else {
            printf("\n");
        }
    }
    printf("]\n");
}

int main() {
    Table *table = parse_table(stdin);
    if (table->num_headers > 0) {
        print_json(table);
    }
    free_table(table);
    free(table);
    return 0;
}