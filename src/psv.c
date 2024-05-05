/**
 * @file psv.c
 * @brief Processes And Parses PSV Content
 *
 * Copyright (C) 2024-2024 Brian Khuu <contact@briankhuu.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 */

#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

#include "psv.h"
#include "log.h"

#ifdef NDEBUG
    #define assert(expression) ((void)0)
#endif

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

/**
 * @brief Tokenizes a string using an escaped delimiter.
 * 
 * This function tokenizes a string using a specified delimiter, 
 * while handling escaped delimiters within the string.
 * 
 * @param str The string to be tokenized.
 * @param delim The delimiter character used for tokenization.
 * @param tokenization_state A pointer to the tokenization state. 
 *                           Pass NULL to start tokenization from the beginning of the string, 
 *                           and pass the previous tokenization state for subsequent calls 
 *                           to continue tokenization from where it left off.
 * 
 * @return A pointer to the next token in the string, or NULL if no more tokens are found.
 * 
 * @remarks This function tokenizes the input string by searching for the specified delimiter.
 *          It handles escaped delimiters within the string, allowing them to be treated as 
 *          regular characters rather than delimiters. 
 *          The tokenization_state parameter is used to maintain the state of tokenization 
 *          across multiple calls to the function. Pass NULL to start tokenization from the 
 *          beginning of the string, and pass the previous tokenization state returned by 
 *          the function for subsequent calls to continue tokenization from where it left off.
 *          The function modifies the input string by replacing delimiters with null terminators 
 *          to create separate tokens. It also updates the tokenization_state to indicate 
 *          the position in the string where tokenization stopped.
 *          If no more tokens are found in the string, the function returns NULL.
 * 
 * @note The input string is modified by this function. Make sure to pass a writable string 
 *       or create a copy of the original string if needed.
 */
static char *tokenize_escaped_delim(char *str, char delim, char **tokenization_state) {
    if (str == NULL)
        return NULL;

    char *token_start = (*tokenization_state == NULL) ? str : *tokenization_state + 1;
    char *token_end = token_start;

    while (*token_end != '\0') {
        if (*token_end == '\\') {
            if (*(token_end + 1) == '\\' || *(token_end + 1) == delim || ispunct(*(token_end + 1))) {
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

static void capture_data_annotations(const char *header_buffer, PsvDataAnnotationField **data_annotation_array, size_t *num_data_annotation_tags) {
    const char *token_start = header_buffer; 
    const char *token_end = header_buffer;

    while (*token_end != '\0') {
        // Find the start of a data annotation tag
        token_start = strchr(token_end, '[');
        if (token_start == NULL) {
            break; // No more data annotation tags found
        }

        // Skip '['
        token_start++;

        // Find the end of the data annotation tag
        token_end = strchr(token_start, ']');
        if (token_end == NULL) {
            break; // Invalid data annotation tag format
        }

        // Check if we are actually looking at a data annotation token or a markdown link `[example](http:example.com)`
        // If we are looking at a markdown link, then let skip it and search for the next potential data annotation
        if (*(token_end + 1) != '\0' && *(token_end + 1) == '(')
            continue;

        // Check if tag length is non zero
        const size_t tag_length = token_end - token_start;
        if (tag_length == 0)
            continue;

        // Resize Annotation Field Array
        *data_annotation_array = (PsvDataAnnotationField*) realloc(*data_annotation_array, (*num_data_annotation_tags + 1) * sizeof(PsvDataAnnotationField*));
        assert(*data_annotation_array != NULL);

        // Add Annotation Field String To Array
        (*data_annotation_array)[*num_data_annotation_tags] = malloc((tag_length + 1) * sizeof(char));
        memmove((*data_annotation_array)[*num_data_annotation_tags], token_start, tag_length);
        (*data_annotation_array)[*num_data_annotation_tags][tag_length] = '\0';

        *num_data_annotation_tags = *num_data_annotation_tags + 1;
    }
}

/**
 * @brief Trims leading and trailing whitespace from a string.
 * 
 * This function trims leading and trailing whitespace characters (spaces, tabs, newlines, etc.) 
 * from a given string, modifying the string in-place.
 * 
 * @param str The string to be trimmed.
 * 
 * @return A pointer to the trimmed string.
 * 
 * @note The input string is modified by this function. Make sure to pass a writable string 
 *       or create a copy of the original string if needed.
 */
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

/**
 * @brief Trims leading and trailing '|' characters from a Markdown table row.
 *
 * This function removes leading and trailing '|' characters from a Markdown table row.
 *
 * @param line A pointer to a character array representing a Markdown table row.
 * @param line_size The size of the character array.
 * 
 * @return A pointer to the beginning of the first token in the row after trimming.
 *         Returns NULL if the row is empty or doesn't start with '|'.
 */
static char *trim_md_table_row(char *line, ssize_t line_size) {

    // Check if we got a reasonable psv row
    if (line_size <= 2 && line[0] != '|')
        return NULL;

    // Trim '|' on right hand side
    for (int i = line_size - 1; i > 0; i--) {
        if (line[i] == '|') {
            line[i] = '\0';
            break;
        }
    }

    // Return token 
    return line + 1;
}

/**
 * @brief Parses the table ID from the consistent attribute syntax within a table block.
 * 
 * This function extracts the table ID from the consistent attribute syntax within a table block,
 * allowing the table to be identified later. The table ID is typically specified using the
 * `{#id}` syntax at the beginning of the table block.
 * 
 * @param stringBuffer A string buffer containing the internal content of the table block.
 *                     The buffer should already have the outer `{}` removed.
 * @param id A character array where the extracted ID will be stored.
 * @param idSize The size of the character array `id`.
 * 
 * @remarks This function expects the consistent attribute syntax to be present within the table block.
 *          It searches for the table ID specified using the `{#id}` syntax at the beginning of the block.
 *          The function skips all characters until an opening curly brace or the end of the buffer is encountered.
 *          If the ID is found and fits within the provided `id` buffer, it is copied into the buffer.
 * 
 * @note The input buffer should contain the internal content of the table block without the outer `{}`.
 *       The `id` parameter should point to a character array where the extracted ID will be stored.
 *       The `idSize` parameter specifies the maximum size of the `id` buffer to prevent buffer overflow.
 *       This function does not handle scenarios where the table ID is placed later in the block.
 *       It prioritizes simplicity over robustness and may miss the ID if it is not located at the beginning.
 * 
 * @return `true` if the ID is successfully parsed and fits within the provided buffer size, `false` otherwise.
 */
static bool parse_consistent_attribute_syntax_id(const char *stringBuffer, char *id, size_t idSize) {

    // Skip all characters until an opening curly brace or end of stringBuffer
    while (*stringBuffer != '\0' && *stringBuffer != '{') {
        stringBuffer++;
    }

    // Check if an opening curly brace was found
    if (*stringBuffer != '{') {
        return false; // No opening curly brace found
    }

    // Move past the opening curly brace
    stringBuffer++;

    // Skip leading spaces inside the curly braces
    while (*stringBuffer == ' ') {
        stringBuffer++;
    }

    // Check if we found the # id signifier
    if (*stringBuffer != '#') {
        return false; // No id field found
    }

    // Move past the '#' character
    stringBuffer++;

    // Copy characters until a space, '}', or end of stringBuffer is encountered
    size_t i = 0;
    while (i < idSize - 1 && *stringBuffer != '\0' && *stringBuffer != ' ' && *stringBuffer != '}') {
        id[i++] = *stringBuffer++;
    }

    id[i] = '\0'; // Null-terminate the ID

    return true;
}

/**
 * @brief Frees memory allocated for a PsvTable structure.
 * 
 * This function deallocates memory associated with the headers, JSON keys,
 * and data rows of a PsvTable structure, and zeroes out the table's state
 * and variables.
 * 
 * @param table A pointer to the PsvTable structure to be cleared.
 */
static void psv_clear_table(PsvTable *table) {

    // Free Header Metadata
    if (table->header_metadata) {
        for (int i = 0; i < table->num_headers; i++) {
            PsvHeaderMetadataField *header_metadata = &table->header_metadata[i];

            // Free all dynamic strings
            free(header_metadata->raw_header);

            // Free Data Annotation
            if (header_metadata->data_annotation_tags) {
                for (int j = 0; j < header_metadata->data_annotation_tag_size; j++) {
                    PsvDataAnnotationField data_annotation_field = header_metadata->data_annotation_tags[j];
                    free(data_annotation_field);
                    data_annotation_field = NULL;
                }
                free(header_metadata->data_annotation_tags);
                header_metadata->data_annotation_tags = NULL;
            }
        }
        free(table->header_metadata);
        table->header_metadata = NULL;
    }

    // Free Tabular Data
    if (table->data_rows) {
        for (int i = 0; i < table->num_data_rows; i++) {
            PsvDataRow data_row = table->data_rows[i];
            for (int j = 0; j < table->num_headers; j++) {
                PsvDataField data_field = data_row[j];
                free(data_field);
                data_field = NULL;
            }
            free(data_row);
            data_row = NULL;
        }
        free(table->data_rows);
        table->data_rows = NULL;
    }

    // Zero out all state and variables
    *table = (PsvTable){0};
}

/**
 * @brief Frees memory allocated for a PsvTable structure and sets the pointer to NULL.
 * 
 * This function deallocates memory associated with the headers, JSON keys,
 * and data rows of a PsvTable structure, zeroes out the table's state and variables,
 * and then frees the memory allocated for the table structure itself. Additionally,
 * it sets the pointer to NULL in the caller's scope, indicating that the table is
 * no longer valid and preventing further access to its contents.
 * 
 * @param tablePtr A pointer to a pointer to the PsvTable structure to be freed.
 *                 After freeing the memory, this function sets the pointer to NULL
 *                 to indicate that the table is empty.
 */
void psv_free_table(PsvTable **tablePtr) {

    // Free and Clear internal content of the table
    psv_clear_table(*tablePtr);

    // Finally, free the memory allocated for the table structure itself
    free(*tablePtr);

    // Set the pointer to NULL in the caller's scope so other parts of the problem knows it's empty
    *tablePtr = NULL;
}

/**
 * @brief Parses a table header from a file stream and constructs a PsvTable structure.
 * 
 * This function reads lines from the input file stream and parses a table header in 
 * Markdown format. It constructs a PsvTable structure containing the table headers 
 * and their corresponding JSON keys. The function supports parsing consistent attribute 
 * syntax for table IDs and handles various edge cases to determine the parsing state.
 * 
 * @param input The file stream from which to read the table header.
 * @param defaultTableID The default ID to assign to the table if no ID is specified.
 * @return A pointer to the PsvTable structure containing the parsed table header,
 *         or NULL if no valid table header is found or an error occurs.
 */
PsvTable * psv_parse_table_header(FILE *input, char *defaultTableID) {

    // Allocate memory for the table structure
    PsvTable *table = malloc(sizeof(PsvTable));
    *table = (PsvTable){0};

    // Variables for reading lines from input stream
    ssize_t read;
    char *line = NULL;
    size_t len = 0;

    // Loop through lines in the input stream
    while ((read = getline(&line, &len, input)) != -1) {

        // Check if the last character is a newline
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';  // Replace newline with null terminator
        }

        log_trace("processing line '%s'", line);

        // Determine parsing state based on table content
        switch (table->parsing_state) {
        case PSV_TABLE_PARSING_SCANNING:
            if (line[0] == '{') {
                // Parse Consistent attribute syntax https://talk.commonmark.org/t/consistent-attribute-syntax/272

                // Check for expected closing `}`
                // Dev Note: In the future maybe we can support multiline attributes? For now, no.
                bool closing_bracket = false;
                for (int i = read - 1; i > 0; i--) {
                    if (line[i] == '}') {
                        closing_bracket = true;
                        break;
                    }
                }

                if (!closing_bracket) {
                    continue;
                }

                if (parse_consistent_attribute_syntax_id(line, table->id, PSV_TABLE_ID_MAX)) {
                    log_debug("Table ID: %s", table->id);
                }
            } else if (line[0] == '|') {
                table->num_headers = 0;
                table->num_data_rows = 0;

                char *trimmed_psv_row = trim_md_table_row(line, read);
                if (trimmed_psv_row == NULL) {
                    psv_clear_table(table);
                    continue;
                }

                assert(table->header_metadata == NULL);

                // Split and Cache header
                char *tokenization_state = NULL;
                char *token;
                while ((token = tokenize_escaped_delim(trimmed_psv_row, '|', &tokenization_state)) != NULL) {
                    // Got Header Column
                    const char * full_header_buffer = trim_whitespace(token);
                    const size_t full_header_buffer_size = strlen(full_header_buffer) + 1;

                    table->header_metadata = realloc(table->header_metadata, (table->num_headers + 1) * sizeof(PsvHeaderMetadataField));
                    assert(table->header_metadata != NULL);

                    table->header_metadata[table->num_headers] = (PsvHeaderMetadataField){0};

                    PsvHeaderMetadataField *header_metadata = &table->header_metadata[table->num_headers];
                    *header_metadata = (PsvHeaderMetadataField){0};

                    // Raw Headers
                    header_metadata->raw_header = malloc(full_header_buffer_size * sizeof(char));
                    assert(header_metadata->raw_header != NULL);
                    memcpy(header_metadata->raw_header, full_header_buffer, full_header_buffer_size);

                    // Json Keys
                    if (!parse_consistent_attribute_syntax_id(full_header_buffer, header_metadata->id, (PSV_HEADER_ID_MAX) * sizeof(char))) {
                        // Consistent Attribute Syntax not found or does not contain id override
                        // Use heuristics to generate a reasonable json key
                        generateJSONKey(full_header_buffer, header_metadata->id, PSV_HEADER_ID_MAX);
                    }

#if 1 // SEGFAULTS
                    // Data Annotations
                    capture_data_annotations(full_header_buffer, &header_metadata->data_annotation_tags, &header_metadata->data_annotation_tag_size);
#endif

                    table->num_headers++;
                }

                // Diagnostics Printout of Header Content
                log_debug("Found Header Count: %d", table->num_headers);
                for (int i = 0; i < table->num_headers; i++) {
                    log_debug("Found Header ID: %s", table->header_metadata[i].id);
                    log_debug("Found Header Raw: %s", table->header_metadata[i].raw_header);
                    if (table->header_metadata[i].data_annotation_tag_size > 0) {
                        log_debug("Data Annotation Count %d", table->header_metadata[i].data_annotation_tag_size);
                        for (int j = 0; j < table->header_metadata[i].data_annotation_tag_size; j++) {
                            log_debug("Data Annotation %d: %s", j, table->header_metadata[i].data_annotation_tags[j]);
                        }
                    }
                }

                // Check if at least one header field is detected
                if (table->num_headers > 0) {
                    table->parsing_state = PSV_TABLE_PARSING_POTENTIAL_HEADER;
                    if (table->id[0] == '\0') {
                        strcpy(table->id, defaultTableID);
                    }
                } else {
                    psv_clear_table(table);
                }
        } else {
          // Not a table header

          // Clear table in case we found Consistent Attribute Syntax but no
          // table Since it might be for a different block instead
          psv_clear_table(table);
        }
            break;
        
        case PSV_TABLE_PARSING_POTENTIAL_HEADER:
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
                    table->parsing_state = PSV_TABLE_PARSING_DATA_ROW;
                } else {
                    // Mismatch with headers, free the allocated memory
                    table->parsing_state = PSV_TABLE_PARSING_SCANNING;
                    psv_clear_table(table);
                }
            }
            break;
        case PSV_TABLE_PARSING_DATA_ROW:
            break;
        case PSV_TABLE_PARSING_END:
            break;
        }

        // Release memory allocated by getline
        free(line);
        line = NULL;
        len = 0;

        // Exit loop if parsing state is PSV_TABLE_PARSING_DATA_ROW
        if (table->parsing_state == PSV_TABLE_PARSING_DATA_ROW) {
            break;
        }
    }

    // Free memory allocated by getline
    free(line);

    // Check parsing state and free table memory if necessary
    if (table->parsing_state != PSV_TABLE_PARSING_DATA_ROW) {
        psv_free_table(&table);
    }

    // Return the parsed table
    return table;
}

/**
 * @brief Parses a single data row from a file stream and constructs a PsvDataRow.
 * 
 * This function reads a line from the input file stream and parses it as a single data row 
 * of a table in Markdown format. It constructs a PsvDataRow containing the parsed data cells, 
 * corresponding to the table headers. The function assumes that the table is in the data row 
 * parsing state and expects the input file stream to contain valid data row lines.
 * 
 * @param input The file stream from which to read the data row.
 * @param table Pointer to the PsvTable structure representing the table.
 * @return A PsvDataRow containing the parsed data cells of the row, or NULL if the end of the 
 *         table is reached or an error occurs.
 */
PsvDataRow psv_parse_table_row(FILE *input, PsvTable *table) {

    // Cannot return row if not in data row parsing state
    if (table->parsing_state != PSV_TABLE_PARSING_DATA_ROW)
        return NULL;

    // Initialize variables
    PsvDataRow data_row = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    // Read a line from the input stream
    if ((read = getline(&line, &len, input)) != -1) {
        if (line[0] == '|') {
            // Trim '|' on the right-hand side
            for (int i = read - 1; i > 0; i--) {
                if (line[i] == '|') {
                    line[i] = '\0';
                    break;
                }
            }

            // Allocate memory for the data row and initialize with NULL
            data_row = malloc(table->num_headers * sizeof(char *));
            for (int i = 0 ; i < table->num_headers ; i++) {
                data_row[i] = NULL;
            }

            // Split and record each cell
            char *tokenization_state = NULL;
            for (int i = 0 ; i < table->num_headers ; i++) {
                char *token = tokenize_escaped_delim(line + 1, '|', &tokenization_state);
                if (token == NULL)
                    break;

                // Allocate memory for each data cell and copy the trimmed token
                data_row[i] = malloc((strlen(token) + 1) * sizeof(char));
                strcpy(data_row[i], trim_whitespace(token));
            }
        } else {
            // End of Table detected
            table->parsing_state = PSV_TABLE_PARSING_END;
        }
    }
    
    // Release getline's buffer
    free(line);
    return data_row;
}

/**
 * @brief Frees memory allocated for a single data row of a PsvTable.
 * 
 * This function deallocates memory previously allocated for a single data row of a PsvTable,
 * including the memory for each data cell within the row. It sets the pointers to NULL after
 * freeing the memory to prevent dangling references.
 * 
 * @param table Pointer to the PsvTable structure representing the table.
 * @param dataRowPtr Pointer to the PsvDataRow to be freed.
 */
void psv_parse_table_free_row(PsvTable *table, PsvDataRow *dataRowPtr) {
    PsvDataRow data_row = *dataRowPtr;
    // Free Rows
    for (int j = 0; j < table->num_headers; j++) {
        free(data_row[j]);
        data_row[j] = NULL;
    }
    // Free the data row array itself
    free(*dataRowPtr);
    // Set the pointer to NULL to avoid dangling references
    *dataRowPtr = NULL;
}

/**
 * @brief Skips the current row while parsing a PsvTable.
 * 
 * This function reads a line from the input file stream and determines whether it represents a data row
 * of the table. If the line begins with a '|', it is considered a data row, and the function returns true,
 * indicating that the row was found and skipped. If the line does not begin with a '|', it signifies the
 * end of the table, and the function sets the parsing state of the table to indicate the end.
 * 
 * @param input Pointer to the input file stream.
 * @param table Pointer to the PsvTable structure representing the table being parsed.
 * @return True if a data row was found and skipped, false otherwise.
 */
bool psv_parse_skip_table_row(FILE *input, PsvTable *table) {

    // Cannot return row if not in data row parsing state
    if (table->parsing_state != PSV_TABLE_PARSING_DATA_ROW)
        return false;

    bool row_found = false;
    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, input) != -1) {
        if (line[0] == '|') {
            // Is a data row
            row_found = true;
        } else {
            // End of Table detected
            table->parsing_state = PSV_TABLE_PARSING_END;
        }
    }

    // Release getline's buffer
    free(line);
    return row_found;
}

/**
 * @brief Parses a table from a stream input.
 * 
 * This function parses a table from the specified input file stream. It starts by parsing the table header
 * to extract metadata and column names. Then, it reads and parses each data row of the table until the end
 * of the table is reached. Each parsed row is stored in the PsvTable structure.
 * 
 * @param input Pointer to the input file stream.
 * @param defaultTableID Default ID to assign to the table if no ID is specified in the table header.
 * @return A pointer to the parsed PsvTable structure, or NULL if an error occurred during parsing.
 */
PsvTable *psv_parse_table(FILE *input, char *defaultTableID) {
    // Parse the table header to extract metadata and column names

    PsvTable *table = psv_parse_table_header(input, defaultTableID);

    // If table parsing failed, return NULL
    if (table == NULL) {
        return NULL;
    }

    char **data_row = NULL;
    
    // Parse each data row of the table until the end of the table is reached
    while ((data_row = psv_parse_table_row(input, table)) != NULL) {
        // Reallocate memory for data rows and store the parsed row
        table->data_rows = realloc(table->data_rows, (table->num_data_rows + 1) * sizeof(char **));
        table->data_rows[table->num_data_rows] = data_row;
        table->num_data_rows++;
    }

    return table;
}
