#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include "config.h"
#include "psv.h"

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

void print_table_rows_json(PsvTable *table, FILE *output, unsigned int tableCount) {
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

void print_table_json(PsvTable *table, FILE *output, unsigned int tableCount) {
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
        "  -t, --table <pos>       specify the position of a single table to output (must be a positive integer)\n"
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

            PsvTable *table = NULL;
            while ((table = psv_parse_table(input_file, &tableCount)) != NULL && !foundSingleTable) {
                if (single_table) {
                    if (table_num_sel > 0) {
                        // Select By Table Position
                        if (table_num_sel == tableCount) {
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
                psv_free_table(table);
                free(table);
            }

            fclose(input_file);
        }
        if (!single_table) {
            fprintf(output_stream, "]\n");
        }
    } else {
        // No input files provided, read from stdin
        PsvTable *table = NULL;
        if (!single_table) {
            fprintf(output_stream, "[\n");
        }
        while ((table = psv_parse_table(stdin, &tableCount)) != NULL && !foundSingleTable) {
            if (single_table) {
                if (table_num_sel > 0) {
                    // Select By Table Position
                    if (table_num_sel == tableCount) {
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
            psv_free_table(table);
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