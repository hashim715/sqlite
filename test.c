#include "Unity/src/unity.h"
#include "db.h"

void setUp(void) {};
void tearDown(void) {};

void test_row_id(void) {
    Row row = {1, "hashim", "test@example.com"};
    TEST_ASSERT_EQUAL(1, row.id);
};

void test_prepare_insert(void) {
    InputBuffer input;
    Statement statement;

    char command[] =
        "insert 1 user1 person1@example.com";

    input.buffer = command;

    PrepareResult result = prepare_statement(&input, &statement);

    TEST_ASSERT_EQUAL(PREPARE_SUCCESS, result);
    TEST_ASSERT_EQUAL(1, statement.row_to_insert.id);
    TEST_ASSERT_EQUAL_STRING("user1", statement.row_to_insert.username);
    TEST_ASSERT_EQUAL_STRING("person1@example.com",
                             statement.row_to_insert.email);
};

void test_insert_max_length_strings(void) {
    InputBuffer input;
    Statement statement;

    char command[400];

    char username[33];
    memset(username, 'a', 32);
    username[32] = '\0';

    char email[256];
    memset(email, 'a', 255);
    email[255] = '\0';

    snprintf(
        command,
        sizeof(command),
        "insert 1 %s %s",
        username,
        email
    );

    input.buffer = command;

    PrepareResult result =
        prepare_statement(&input, &statement);

    TEST_ASSERT_EQUAL(PREPARE_SUCCESS, result);

    TEST_ASSERT_EQUAL_STRING(
        username,
        statement.row_to_insert.username
    );

    TEST_ASSERT_EQUAL_STRING(
        email,
        statement.row_to_insert.email
    );
};

void test_username_too_long(void) {
    InputBuffer input;
    Statement statement;

    char username[34];
    memset(username, 'a', 33);
    username[33] = '\0';

    char command[400];

    snprintf(
        command,
        sizeof(command),
        "insert 1 %s test@example.com",
        username
    );

    input.buffer = command;

    PrepareResult result =
        prepare_statement(&input, &statement);

    TEST_ASSERT_EQUAL(
        PREPARE_STRING_TOO_LONG,
        result
    );
};

void test_email_too_long(void) {
    InputBuffer input;
    Statement statement;

    char email[257];
    memset(email, 'a', 256);
    email[256] = '\0';

    char command[500];

    snprintf(
        command,
        sizeof(command),
        "insert 1 user %s",
        email
    );

    input.buffer = command;

    PrepareResult result =
        prepare_statement(&input, &statement);

    TEST_ASSERT_EQUAL(
        PREPARE_STRING_TOO_LONG,
        result
    );
};

void test_negative_id(void) {
    InputBuffer input;
    Statement statement;

    char command[500];

    int id = -1;

    snprintf(
        command,
        sizeof(command),
        "insert %d user user@gmail.com",
        id
    );

    input.buffer = command;

    PrepareResult result =
        prepare_statement(&input, &statement);

    TEST_ASSERT_EQUAL(PREPARE_NEGATIVE_ID, result);
};

void test_table_full(void) {
    Table* table = new_table();

    Statement statement;
    statement.type = STATEMENT_INSERT;

    ExecuteResult result = EXECUTE_SUCCESS;

    for (int i = 0; i < TABLE_MAX_ROWS + 1; i++) {
        statement.row_to_insert.id = i;

        strcpy(
            statement.row_to_insert.username,
            "user"
        );

        strcpy(
            statement.row_to_insert.email,
            "user@example.com"
        );

        result =
            execute_insert(&statement, table);
    }

    TEST_ASSERT_EQUAL(
        EXECUTE_TABLE_FULL,
        result
    );

    free_table(table);
};

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_row_id);
    RUN_TEST(test_prepare_insert);
    RUN_TEST(test_insert_max_length_strings);
    RUN_TEST(test_username_too_long);
    RUN_TEST(test_email_too_long);
    RUN_TEST(test_table_full);
    RUN_TEST(test_negative_id);
    return UNITY_END();
};