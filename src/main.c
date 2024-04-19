#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    SCANNING,
    POTENTIAL_HEADER,
    DATA
} State;

int num_headers = 0;
char **headers = NULL;
char **data_rows = NULL;
int num_data_rows = 0;

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

void free_memory() {
    for (int i = 0; i < num_headers; i++) {
        free(headers[i]);
    }
    free(headers);

    for (int i = 0; i < num_data_rows; i++) {
        free(data_rows[i]);
    }
    free(data_rows);
}

void parse_table(FILE *input) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    State state = SCANNING;

    while ((read = getline(&line, &len, input)) != -1) {
        if (state == SCANNING) {
            if (line[0] == '|') {
                num_headers = 0;
                num_data_rows = 0;
                // Trim '|' on right hand side
                for (int i = read - 1; i > 0 ; i--)
                {
                    if (line[i] == '|')
                    {
                        line[i] = '\0';
                        break;
                    }
                }
                // Split and Cache header
                char *token = strtok(line + 1, "|");
                while (token != NULL) {
                    headers = realloc(headers, (num_headers + 1) * sizeof(char *));
                    headers[num_headers] = malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(headers[num_headers], trim_whitespace(token));
                    //printf("|%s|\n", headers[num_headers]);
                    num_headers++;
                    token = strtok(NULL, "|");
                }
                // Check 
                if (num_headers > 0) {
                    state = POTENTIAL_HEADER;
                } else {
                    state = SCANNING;
                    for (int i = 0; i < num_headers; i++) {
                        free(headers[i]);
                    }
                    free(headers);
                }
            }
        } else if (state == POTENTIAL_HEADER) {
            // Check if actual header by checking if there is enough '|---|'
            if (line[0] == '|') {
                // Trim '|' on right hand side
                for (int i = read - 1; i > 0 ; i--)
                {
                    if (line[i] == '|')
                    {
                        line[i] = '\0';
                        break;
                    }
                }
                // Split and Cache header
                int num_header_separators = 0;
                char *token = strtok(line + 1, "|");
                while (token != NULL) {
                    if (strstr(token, "---")){
                        num_header_separators++;
                    }
                    token = strtok(NULL, "|");
                }
                if (num_headers == num_header_separators) {
                    // It's most likely a markdown table
                    // we can now safely start parsing the data rows
                    state = DATA;
                } else {
                    // Mismatch with 
                    state = SCANNING;
                    for (int i = 0; i < num_headers; i++) {
                        free(headers[i]);
                    }
                    free(headers);
                }
            }
        } else if (state == DATA) {
            if (line[0] == '|') {
                // Trim '|' on right hand side
                for (int i = read - 1; i > 0 ; i--)
                {
                    if (line[i] == '|')
                    {
                        line[i] = '\0';
                        break;
                    }
                }

                // Add a new row
                data_rows = realloc(data_rows, ((num_data_rows + 1)  * num_headers) * sizeof(char *));

                // Split and Cache header
                int num_data_col = 0;
                char *token = strtok(line + 1, "|");
                while (token != NULL) {
                    data_rows[num_data_rows * num_headers + num_data_col] = malloc((strlen(token) + 1) * sizeof(char));
                    strcpy(data_rows[num_data_rows * num_headers + num_data_col], trim_whitespace(token));
                    num_data_col++;
                    token = strtok(NULL, "|");
                }
                // Keep track of data row
                num_data_rows++;
            } else {
                break;
            }
        }
    }

    free(line);
}

void print_json() {
    printf("[\n");
    for (int i = 0; i < num_data_rows; i++) {
        printf("{");
        for (int j = 0; j < num_headers; j++) {
            printf("\"%s\": \"%s\"", headers[j], data_rows[i * num_headers + j]);
            if (j < num_headers - 1) {
                printf(",");
            }
        }
        printf("}");
        if (i < num_data_rows - 1) {
            printf(",\n");
        } else {
            printf("\n");
        }
    }
    printf("]\n");
}

int main() {
    parse_table(stdin);
    print_json();
    free_memory();
    return 0;
}