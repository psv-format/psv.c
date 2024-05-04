#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "config.h"

#include "cJSON.h"

#include "psv.h"
#include "psv_json.h"

static const char* progname;

static char *getDefaultTableID(char *defaultTableID, size_t maxLen, unsigned int tablePosition) {
    snprintf(defaultTableID, PSV_TABLE_ID_MAX, "table%d", tablePosition);
    return defaultTableID;
}

static void parse_table_to_json_from_stream(FILE* input_stream, FILE* output_stream, unsigned int *tallyCount, int pos_selector, char *id_selector, bool compact_mode) {
    char defaultTableID[PSV_TABLE_ID_MAX];
    PsvTable *table = NULL;
    cJSON *table_json = NULL;

    while ((table = psv_parse_table(input_stream, getDefaultTableID(defaultTableID, PSV_TABLE_ID_MAX, *tallyCount + 1))) != NULL) {

        // Keep track of parsed tables position which is required for table positional selector to function correctly
        *tallyCount = *tallyCount + 1;

        if ((pos_selector > 0) && (pos_selector != *tallyCount)) {
            // Select By Table Position mode was enabled, check if table position was reached
            psv_free_table(&table);
            continue;
        } else if ((id_selector != NULL) && (strcmp(table->id, id_selector) != 0)) {
            // Select By String ID mode was enabled, check if table ID matches
            psv_free_table(&table);
            continue;
        }

        // Table Found, print it to output stream
        cJSON *table_json = compact_mode ? psv_json_create_table_rows(table) : psv_json_create_table_json(table);
        char *json_string = cJSON_PrintUnformatted(table_json);
        fprintf(output_stream, "%s\n", json_string);
        free(json_string);
        cJSON_Delete(table_json);

        // Release table memory
        psv_free_table(&table);

        // Check if in single table search mode
        if ((pos_selector > 0) || (id_selector != NULL)) {
            break;
        }

    }

}

static void parse_singular_table_streaming_rows_to_json_from_stream(FILE* input_stream, FILE* output_stream, unsigned int *tallyCount, int pos_selector, char *id_selector, bool compact_mode) {

    if ((pos_selector == 0) && (id_selector == NULL)) {
        // Expecting to be in singular table search mode
        return;
    }

    PsvTable *table = NULL;
    char defaultTableID[PSV_TABLE_ID_MAX];
    while ((table = psv_parse_table_header(input_stream, getDefaultTableID(defaultTableID, PSV_TABLE_ID_MAX, *tallyCount + 1))) != NULL) {

        // Keep track of parsed tables position which is required for table positional selector to function correctly
        *tallyCount = *tallyCount + 1;

        // Check if we found the table we are looking for
        if ((pos_selector > 0) && (pos_selector != *tallyCount)) {
            // Select By Table Position mode was enabled, check if table position was reached
            while (psv_parse_skip_table_row(input_stream, table)) {/* Skip Rows */};
            psv_free_table(&table);
            continue;
        } else if ((id_selector != NULL) && (strcmp(table->id, id_selector) != 0)) {
            // Select By String ID mode was enabled, check if table ID matches
            while (psv_parse_skip_table_row(input_stream, table)) {/* Skip Rows */};
            psv_free_table(&table);
            continue;
        }

        // Table found, start streaming out the rows
        PsvDataRow data_row = NULL;
        while ((data_row = psv_parse_table_row(input_stream, table)) != NULL) {
            // Row Found, print it to output stream
            cJSON *table_json = psv_json_create_table_single_row(table, data_row);
            char *json_string = cJSON_PrintUnformatted(table_json);
            fprintf(output_stream, "%s\n", json_string);
            free(json_string);
            cJSON_Delete(table_json);

            // Release row memory
            psv_parse_table_free_row(table, &data_row);
        }

        // Release table memory
        psv_free_table(&table);
        break;
    }

    return;
}

static unsigned int parse_table_from_stream(FILE* input_stream, FILE* output_stream, unsigned int *tallyCount, int pos_selector, char *id_selector, bool compact_mode) {
    if (compact_mode && ((pos_selector > 0) || (id_selector != NULL))) {
        // When in compact row only mode and singular table mode, you don't need to wrap the rows with a json array
        // Also it gives us an opportunity to operate in streaming mode to process very very large PSV tables
        parse_singular_table_streaming_rows_to_json_from_stream(input_stream, output_stream, tallyCount, pos_selector, id_selector, compact_mode);
    } else {
        // This is normal table by table streaming. Minimum optimisation for this mode as we don't know the number of tables etc...
        parse_table_to_json_from_stream(input_stream, output_stream, tallyCount, pos_selector, id_selector, compact_mode);
    }
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

            // No input files provided, read from stdin
            parse_table_from_stream(input_file, output_stream, &tallyCount, pos_selector, id_selector, compact_mode);

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
        parse_table_from_stream(stdin, output_stream, &tallyCount, pos_selector, id_selector, compact_mode);
    }

    if (output_file) {
        fclose(output_stream);
    }

    return 0;
}