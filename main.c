#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "db.h"

int main(int argc, char* argv[]) {
    InputBuffer* input_buffer = new_input_buffer();

    if (argc < 2) {
        printf("Must supply a database filename\n");
        exit(EXIT_FAILURE);
    };

    char* filename = argv[1];
    Table* table = db_open(filename);

    while (true) {
        print_prompt();
        read_input(input_buffer);

        // META commands (like .exit)
        if (input_buffer->buffer[0] == '.') {
            switch (do_meta_command(input_buffer, table)) {
                case META_COMMAND_SUCCESS:
                    continue;
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    printf("Unrecognized command '%s'\n", input_buffer->buffer);
                    continue;
            };
        };

        // parse statement
        Statement statement;
        switch (prepare_statement(input_buffer, &statement)) {
            case PREPARE_SUCCESS:
                break;

            case PREPARE_SYNTAX_ERROR:
                printf("Syntax error\n");
                continue;

            case PREPARE_STRING_TOO_LONG:
                printf("String too long\n");
                continue;
            
            case PREPARE_NEGATIVE_ID:
                printf("ID must be positive.\n");
                continue;

            case PREPARE_UNRECOGNIZED_STATEMENT:
                printf("Unrecognized statement\n");
                continue;
        };

        // execute
        switch (execute_statement(&statement, table)) {
            case EXECUTE_SUCCESS:
                printf("Executed\n");
                break;

            case EXECUTE_TABLE_FULL:
                printf("Error: Table full\n");
                break;
        };
    };
};