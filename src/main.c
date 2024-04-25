#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "config.h"
#include "psv.h"
#include "cJSON.h"

static const char* progname;

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

// Create JSON array of table rows
cJSON *create_table_rows_json(PsvTable *table) {
    cJSON *rows_json = cJSON_CreateArray();
    for (int i = 0; i < table->num_data_rows; i++) {
        cJSON *row_json = cJSON_CreateObject();
        for (int j = 0; j < table->num_headers; j++) {
            const char *key = table->headers[j];
            const char *data = table->data_rows[i][j];
            if (data) {
#if 0   // Automatic Type Detection
        // Dev Note: Disabled for now until explicit typing is figured out
                if (is_number(data) || is_boolean(data)) {
                    cJSON_AddItemToObject(row_json, key, cJSON_CreateRaw(data));
                } else {
                    cJSON_AddItemToObject(row_json, key, cJSON_CreateString(data));
                }
#else
                cJSON_AddItemToObject(row_json, key, cJSON_CreateString(data));
#endif
            }
        }
        cJSON_AddItemToArray(rows_json, row_json);
    }
    return rows_json;
}

// Create JSON object representing a table
cJSON *create_table_json(PsvTable *table) {
    cJSON *table_json = cJSON_CreateObject();
    cJSON_AddItemToObject(table_json, "id", cJSON_CreateString(table->id));

    cJSON *headers_json = cJSON_CreateArray();
    for (int j = 0; j < table->num_headers; j++) {
        cJSON_AddItemToArray(headers_json, cJSON_CreateString(table->headers[j]));
    }
    cJSON_AddItemToObject(table_json, "headers", headers_json);

    cJSON *rows_json = create_table_rows_json(table);
    cJSON_AddItemToObject(table_json, "rows", rows_json);
    return table_json;
}

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
        "  -t, --table <pos>       specify the position of a single table to output (must be a positive integer)\n"
        "  -c, --compact           output only the rows\n"
        "  -h, --help              display this help message and exit\n"
        "  -v, --version           output version information and exit\n\n"
        "For more information, use '%s --help'.\n",
        progname, progname);
    exit(code);
}

// Print usage information
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
                    fprintf(stderr, "-t must be a positive integer\n");
                    usage(1);
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
        for (int i = optind; i < argc && !foundSingleTable; i++) {
            FILE* input_file = fopen(argv[i], "r");
            if (!input_file) {
                fprintf(stderr, "Error: Cannot open file '%s' for reading.\n", argv[i]);
                exit(1);
            }

            PsvTable *table = NULL;
            cJSON *table_json = NULL;
            while ((table = psv_parse_table(input_file, &tableCount)) != NULL && !foundSingleTable) {
                if (single_table) {
                    if (table_num_sel > 0) {
                        // Select By Table Position
                        if (table_num_sel == tableCount) {
                            table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
                            foundSingleTable = true; // Set flag to exit loop
                        }
                    } else {
                        // Select By String ID
                        if (strcmp(table->id, id) == 0) {
                            table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
                            foundSingleTable = true; // Set flag to exit loop
                        }
                    }
                } else {
                    table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
                }

                if (table_json) {
                    char *json_string = cJSON_PrintUnformatted(table_json);
                    fprintf(output_stream, "%s\n", json_string);
                    free(json_string);
                    cJSON_Delete(table_json);
                }

                psv_free_table(table);
                free(table);
            }

            fclose(input_file);
        }
    } else {
        // No input files provided, read from stdin
        PsvTable *table = NULL;
        cJSON *table_json = NULL;
        while ((table = psv_parse_table(stdin, &tableCount)) != NULL && !foundSingleTable) {
            if (single_table) {
                if (table_num_sel > 0) {
                    // Select By Table Position
                    if (table_num_sel == tableCount) {
                        table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
                        foundSingleTable = true; // Set flag to exit loop
                    }
                } else {
                    // Select By String ID
                    if (strcmp(table->id, id) == 0) {
                        table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
                        foundSingleTable = true; // Set flag to exit loop
                    }
                }
            } else {
                table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
            }

            if (table_json) {
                char *json_string = cJSON_PrintUnformatted(table_json);
                fprintf(output_stream, "%s\n", json_string);
                free(json_string);
                cJSON_Delete(table_json);
            }

            psv_free_table(table);
            free(table);
        }
    }

    if (output_file) {
        fclose(output_stream);
    }

    return 0;
}