#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <getopt.h>

#include "config.h"


#define TABLE_ID_MAX_SIZE 255

static const char* progname;

typedef enum {
    SCANNING,
    POTENTIAL_HEADER,
    DATA
} State;

typedef struct {
    char id[TABLE_ID_MAX_SIZE];
    char *attribute; ///< Consistent attribute syntax https://talk.commonmark.org/t/consistent-attribute-syntax/272
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

// Free Memory Allocation For A Table
void free_table(Table *table) {
    // Free Consistent Attribute Syntax
    if (table->attribute) {
        free(table->attribute);
        table->attribute = NULL;
    }

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

// Grab A Table From A Stream Input
Table *parse_table(FILE *input) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    State state = SCANNING;
    Table *table = malloc(sizeof(Table));
    table->attribute = NULL;
    table->num_headers = 0;
    table->headers = NULL;
    table->num_data_rows = 0;
    table->data_rows = NULL;

    while ((read = getline(&line, &len, input)) != -1) {
        if (state == SCANNING) {
            if (line[0] == '{') {
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

                table->attribute = malloc((str_line + 1) * sizeof(char));
                strcpy(table->attribute, trim_whitespace(line + 1));
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

void print_json(Table *table, FILE *output) {
    if (table->attribute)
    {
        printf("%s\n", table->attribute);
    }

    fprintf(output, "[\n");
    for (int i = 0; i < table->num_data_rows; i++) {
        fprintf(output, "{");
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
    fprintf(output, "]\n");
}

enum {
    PSV_OK              =  0,
    PSV_OK_NULL_KIND    = -1, /* exit 0 if --exit-status is not set*/
    PSV_ERROR_SYSTEM    =  2,
    PSV_ERROR_COMPILE   =  3,
    PSV_OK_NO_OUTPUT    = -4, /* exit 0 if --exit-status is not set*/
    PSV_ERROR_UNKNOWN   =  5,
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
        "  --id <id>               specify the ID of a single table to output\n"
        "  -h, --help              display this help message and exit\n"
        "  -v, --version           output version information and exit\n\n"
        "For more information, use '%s --help'.\n",
        progname, progname);
    exit(code);
}

int main(int argc, char* argv[]) {
    progname = argv[0]; 
        int opt;
    char* id = NULL;
    char* output_file = NULL;

    static struct option long_options[] = {
        {"output", required_argument, 0, 'o'},
        {"id",     required_argument, 0, 'i'},
        {"help",   no_argument,       0, 'h'},
        {"version",no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "o:i:hv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'o':
                output_file = optarg;
                break;
            case 'i':
                id = optarg;
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
    if (optind < argc) {
        for (int i = optind; i < argc; i++) {
            FILE* input_file = fopen(argv[i], "r");
            if (!input_file) {
                fprintf(stderr, "Error: Cannot open file '%s' for reading.\n", argv[i]);
                exit(1);
            }

            printf("::%s::\n", argv[i]);

            Table *table = NULL;
            while ((table = parse_table(input_file)) != NULL) {
                print_json(table, output_stream);
                free_table(table);
                free(table);
            }

            fclose(input_file);
        }
    } else {
        // No input files provided, read from stdin
        Table *table = NULL;
        while ((table = parse_table(stdin)) != NULL) {
            print_json(table, output_stream);
            free_table(table);
            free(table);
        }
    }

    if (output_file) {
        fclose(output_stream);
    }

    return PSV_OK;
}