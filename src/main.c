#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <getopt.h>

#include "config.h"

static const char* progname;

#define TABLE_ID_MAX 255

typedef struct {
    char id[TABLE_ID_MAX];
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

// Parse Consistent Attribute Syntax For ID field
char* parse_consistent_attribute_id(const char *attribute) {
    const char *start = strchr(attribute, '#');
    if (!start) {
        // No ID found
        return NULL;
    }
    start++; // Move past the '#' character
    const char *end = strchr(start, ' '); // ID ends with a space or end of string
    if (!end) {
        end = attribute + strlen(attribute); // If no space found, ID extends till end of string
    }
    int id_length = end - start;
    char *id = (char *)malloc((id_length + 1) * sizeof(char));
    if (!id) {
        // Memory allocation failed
        return NULL;
    }
    strncpy(id, start, id_length);
    id[id_length] = '\0'; // Null-terminate the string
    return id;
}

// Free Memory Allocation For A Table
void free_table(Table *table) {
    // Free Table ID

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
}

void parse_consistent_attribute_syntax_id(const char *buffer, Table *table) {
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
        while ((i < TABLE_ID_MAX) && (*buffer != '\0') && *buffer != ' ' && *buffer != '}') {
            table->id[i] = *buffer++;
            i++;
        }
        table->id[i] = '\0'; // Null-terminate the ID
    }
}

// Grab A Table From A Stream Input
Table *parse_table(FILE *input, unsigned int tableCount) {
    typedef enum {
        SCANNING,
        POTENTIAL_HEADER,
        DATA
    } State;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    State state = SCANNING;
    Table *table = malloc(sizeof(Table));
    *table = (Table){0};

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
                    if (table->id[0] == '\0') {
                        snprintf(table->id, TABLE_ID_MAX, "table%d", tableCount+1);
                    }
                } else {
                    free_table(table);
                }
            } else {
                // Not a table header

                // Free table in case we found Consistent Attribute Syntax but no table
                // Since it might be for a different block instead
                free_table(table);
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
                for (int i = 0 ; i < table->num_headers ; i++) {
                    // Set every header data entry in a row to NULL
                    // (This is to tell valgrind that we intentionally want each entry as empty in case of missing entries)
                    table->data_rows[table->num_data_rows][i] = NULL;
                }

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

    // Release getline's buffer
    free(line);

    // Check if we have a table to return to caller
    if (state != DATA)
    {
        free_table(table);
        free(table);
        return NULL;
    }

    return table;
}

void print_table_rows_json(Table *table, FILE *output, unsigned int tableCount) {
    if (tableCount != 0) {
        fprintf(output, "   ,\n");
    }

    fprintf(output, "   [\n");
    for (int i = 0; i < table->num_data_rows; i++) {
        fprintf(output, "       {");
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
                fprintf(output, ", ");
            }

            // Check if the data is a valid JSON number or boolean
            if (is_number(data) || is_boolean(data)) {
                // If the data is a number or boolean, print it without quotes
                fprintf(output, "\"%s\": %s", key, data);
            } else {
                // Otherwise, print it with quotes
                fprintf(output, "\"%s\": \"%s\"", key, data);
            }
        }
        fprintf(output, "}");
        if (i < table->num_data_rows - 1) {
            fprintf(output, ",\n");
        } else {
            fprintf(output, "\n");
        }
    }
    fprintf(output, "   ]\n");
}

void print_table_json(Table *table, FILE *output, unsigned int tableCount) {
    if (tableCount != 0) {
        fprintf(output, "    ,\n");
    }

    fprintf(output, "    {\n");
    fprintf(output, "        \"id\": \"%s\",\n", table->id);
    fprintf(output, "        \"headers\": [");

    // Print headers
    for (int j = 0; j < table->num_headers; j++) {
        fprintf(output, "\"%s\"", table->headers[j]);
        if (j < table->num_headers - 1) {
            fprintf(output, ", ");
        }
    }
    fprintf(output, "],\n");

    fprintf(output, "        \"rows\": [\n");

    // Print rows
    for (int i = 0; i < table->num_data_rows; i++) {
        fprintf(output, "            {");
        for (int j = 0; j < table->num_headers; j++) {
            const char *key = table->headers[j];
            const char *data = table->data_rows[i][j];
            if (!data) {
                continue;
            }
            if (j != 0) {
                fprintf(output, ", ");
            }
            if (is_number(data) || is_boolean(data)) {
                fprintf(output, "\"%s\": %s", key, data);
            } else {
                fprintf(output, "\"%s\": \"%s\"", key, data);
            }
        }
        fprintf(output, "}");
        if (i < table->num_data_rows - 1) {
            fprintf(output, ",\n");
        } else {
            fprintf(output, "\n");
        }
    }

    fprintf(output, "        ]\n");
    fprintf(output, "    }\n");
}

enum {
    PSV_OK              =  0
};

static void usage(int code) {
    FILE *f = (code == 0) ? stdout : stderr;
    fprintf(f,
        PACKAGE_NAME " - command-line Markdown to JSON converter\n"
        "Usage:\n"
        "\t%s [options] [--id <id>] [file...]\n\n"
        "psv reads Markdown documents from the input files or stdin and converts them to JSON format.\n\n"
        "Options:\n"
        "  -o, --output <file>     output JSON to the specified file\n"
        "  -i, --id <id>           specify the ID of a single table to output\n"
        "  -t, --table <pos>       specify the table position of a single table to output (must be positive integer)\n"
        "  -c, --compact           output only the rows\n"
        "  -h, --help              display this help message and exit\n"
        "  -v, --version           output version information and exit\n\n"
        "For more information, use '%s --help'.\n",
        progname, progname);
    exit(code);
}

int main(int argc, char* argv[]) {
    progname = argv[0];

    bool single_table = false;
    bool compact_mode = false;

    int opt;
    int table_num_sel = 0;
    char* id = NULL;
    char* output_file = NULL;

    static struct option long_options[] = {
        {"output",  required_argument, 0, 'o'},
        {"id",      required_argument, 0, 'i'},
        {"table",   required_argument, 0, 't'},
        {"compact", no_argument,       0, 'c'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "o:i:t:chv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                output_file = optarg;
                break;
            case 'i':
                single_table = true;
                id = optarg;
                break;
            case 't':
                single_table = true;
                table_num_sel = atoi(optarg);
                if (table_num_sel <= 0) {
                    printf("-t must be a positive integer\n");
                    usage(1);
                    break;
                }
                break;
            case 'c':
                // Compact Output Mode
                compact_mode = true;
                break;
            case 'h':
                // Help / Usage
                usage(0);
                break;
            case 'v':
                // Version Print
                printf("%s-%s\n", PACKAGE_NAME, PACKAGE_VERSION);
                exit(0);
            case '?':
                // Unknown Argument
                usage(1);
                break;
            default:
                // No Argument
                usage(1);
        }
    }

    // Prep output stream
    FILE* output_stream = stdout; // Default to stdout
    if (output_file) {
        output_stream = fopen(output_file, "w");
        if (!output_stream) {
            fprintf(stderr, "Error: Cannot open file '%s' for writing.\n", output_file);
            exit(1);
        }
    }

    // Process input files
    unsigned int tableCount = 0;
    bool foundSingleTable = false;
    if (optind < argc) {
        if (!single_table) {
            fprintf(output_stream, "[\n");
        }
        for (int i = optind; i < argc && !foundSingleTable; i++) {
            FILE* input_file = fopen(argv[i], "r");
            if (!input_file) {
                fprintf(stderr, "Error: Cannot open file '%s' for reading.\n", argv[i]);
                exit(1);
            }

            Table *table = NULL;
            while ((table = parse_table(input_file, tableCount)) != NULL && !foundSingleTable) {
                if (single_table) {
                    if (table_num_sel > 0) {
                        // Select By Table Position
                        if (table_num_sel == (tableCount + 1)) {
                            if (compact_mode) {
                                print_table_rows_json(table, output_stream, 0);
                            } else {
                                print_table_json(table, output_stream, 0);
                            }
                            foundSingleTable = true; // Set flag to exit loop
                        }
                    } else {
                        // Select By String ID
                        if (strcmp(table->id, id) == 0) {
                            if (compact_mode) {
                                print_table_rows_json(table, output_stream, 0);
                            } else {
                                print_table_json(table, output_stream, 0);
                            }
                            foundSingleTable = true; // Set flag to exit loop
                        }
                    }
                } else {
                    if (compact_mode) {
                        print_table_rows_json(table, output_stream, tableCount);
                    } else {
                        print_table_json(table, output_stream, tableCount);
                    }
                }
                tableCount++;
                free_table(table);
                free(table);
            }

            fclose(input_file);
        }
        if (!single_table) {
            fprintf(output_stream, "]\n");
        }
    } else {
        // No input files provided, read from stdin
        Table *table = NULL;
        if (!single_table) {
            fprintf(output_stream, "[\n");
        }
        while ((table = parse_table(stdin, tableCount)) != NULL && !foundSingleTable) {
            if (single_table) {
                if (table_num_sel > 0) {
                    // Select By Table Position
                    if (table_num_sel == (tableCount + 1)) {
                        if (compact_mode) {
                            print_table_rows_json(table, output_stream, 0);
                        } else {
                            print_table_json(table, output_stream, 0);
                        }
                        foundSingleTable = true; // Set flag to exit loop
                    }
                } else {
                    // Select By String ID
                    if (strcmp(table->id, id) == 0) {
                        if (compact_mode) {
                            print_table_rows_json(table, output_stream, 0);
                        } else {
                            print_table_json(table, output_stream, 0);
                        }
                        foundSingleTable = true; // Set flag to exit loop
                    }
                }
            } else {
                if (compact_mode) {
                    print_table_rows_json(table, output_stream, tableCount);
                } else {
                    print_table_json(table, output_stream, tableCount);
                }
                tableCount++;
            }
            free_table(table);
            free(table);
        }
        if (!single_table) {
            fprintf(output_stream, "]\n");
        }
    }

    if (output_file) {
        fclose(output_stream);
    }

    return PSV_OK;
}