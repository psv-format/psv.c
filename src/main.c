#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "config.h"
#include "psv.h"
#include "cJSON.h"

static const char* progname;

// Create JSON object of a single tabular row
cJSON *create_table_single_row_json(PsvTable *table, char **data_row_entry) {
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
cJSON *create_table_rows_json(PsvTable *table) {
    cJSON *rows_json = cJSON_CreateArray();
    for (int i = 0; i < table->num_data_rows; i++) {
        cJSON_AddItemToArray(rows_json, create_table_single_row_json(table, table->data_rows[i]));
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

static char *getDefaultTableID(char *defaultTableID, size_t maxLen, unsigned int tablePosition) {
    snprintf(defaultTableID, PSV_TABLE_ID_MAX, "table%d", tablePosition);
    return defaultTableID;
}

unsigned int parse_table_to_json_from_stream(FILE* input_stream, FILE* output_stream, unsigned int tallyCount, int pos_selector, char *id_selector, bool compact_mode) {
    char defaultTableID[PSV_TABLE_ID_MAX];
    unsigned int tableCount = 0;
    PsvTable *table = NULL;
    cJSON *table_json = NULL;

    while ((table = psv_parse_table(input_stream, getDefaultTableID(defaultTableID, PSV_TABLE_ID_MAX, tallyCount + 1))) != NULL) {

        if ((pos_selector > 0) && (pos_selector != (tallyCount + 1))){
            // Select By Table Position mode was enabled, check if table position was reached
            psv_free_table(&table);
            continue;
        } else if ((id_selector != NULL) && (strcmp(table->id, id_selector) != 0)) {
            // Select By String ID mode was enabled, check if table ID matches
            psv_free_table(&table);
            continue;
        }

        cJSON *table_json = compact_mode ? create_table_rows_json(table) : create_table_json(table);
        char *json_string = cJSON_PrintUnformatted(table_json);
        fprintf(output_stream, "%s\n", json_string);
        free(json_string);
        cJSON_Delete(table_json);

        psv_free_table(&table);

        tallyCount = tallyCount + 1;
        tableCount++;

        // Check if in single table search mode
        if ((pos_selector > 0) || (id_selector != NULL)) {
            break;
        }
    }

    return tableCount;
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

    bool compact_mode = false;

    int opt;
    int pos_selector = 0;
    char* id_selector = NULL;
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
                // Set output file
                output_file = optarg;
                break;
            case 'i':
                // ID based single table mode
                id_selector = optarg;
                break;
            case 't':
                // Table Position Single Table
                pos_selector = atoi(optarg);
                if (pos_selector <= 0) {
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
    unsigned int tallyCount = 0;
    unsigned int tablePosition = 0;
    if (optind < argc) {
        for (int i = optind; i < argc; i++) {
            FILE* input_file = fopen(argv[i], "r");
            if (!input_file) {
                fprintf(stderr, "Error: Cannot open file '%s' for reading.\n", argv[i]);
                exit(1);
            }

            tallyCount += parse_table_to_json_from_stream(input_file, output_stream, tallyCount, pos_selector, id_selector, compact_mode);

            // Table Found?
            if (tallyCount > 0) {
                // Check if in single table search mode
                if ((pos_selector > 0) || (id_selector != NULL)) {
                    break;
                }
            }

            fclose(input_file);
        }
    } else {
        // No input files provided, read from stdin
        parse_table_to_json_from_stream(stdin, output_stream, tallyCount, pos_selector, id_selector, compact_mode);
    }

    if (output_file) {
        fclose(output_stream);
    }

    return 0;
}