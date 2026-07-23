#include <unistd.h>
#include <sys/wait.h>
#include "Unity/src/unity.h"
#include "db.h"

void setUp(void) {};
void tearDown(void) {};


static char** capture_stdout_lines(void (*action)(void*), void* arg, int* out_line_count) {
    char temp_path[] = "/tmp/db_test_stdout_XXXXXX";
    int temp_fd = mkstemp(temp_path);
    TEST_ASSERT_TRUE(temp_fd != -1);

    fflush(stdout);
    int saved_stdout_fd = dup(STDOUT_FILENO);
    TEST_ASSERT_TRUE(saved_stdout_fd != -1);

    TEST_ASSERT_TRUE(dup2(temp_fd, STDOUT_FILENO) != -1);
    close(temp_fd);

    action(arg);

    fflush(stdout);

    // Restore stdout so Unity's own output (and later tests) keep working.
    dup2(saved_stdout_fd, STDOUT_FILENO);
    close(saved_stdout_fd);

    FILE* capture_fp = fopen(temp_path, "r");
    TEST_ASSERT_NOT_NULL(capture_fp);

    char** lines = NULL;
    int line_count = 0;
    char line_buf[1024];

    while (fgets(line_buf, sizeof(line_buf), capture_fp) != NULL) {
        size_t len = strlen(line_buf);
        if (len > 0 && line_buf[len - 1] == '\n') {
            line_buf[len - 1] = '\0';
        }

        lines = (char**)realloc(lines, sizeof(char*) * (line_count + 1));
        lines[line_count] = strdup(line_buf);
        line_count++;
    }

    fclose(capture_fp);
    remove(temp_path);

    *out_line_count = line_count;
    return lines;
};

static void free_captured_lines(char** lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
};

// Like capture_stdout_lines, but runs `action` in a forked child process.
// Needed when `action` may call exit() itself (e.g. hitting an
// unimplemented code path), which would otherwise abort the whole
// test binary instead of just that one scenario.
static char** capture_stdout_lines_forked(void (*action)(void*), void* arg, int* out_line_count) {
    char temp_path[] = "/tmp/db_test_stdout_XXXXXX";
    int temp_fd = mkstemp(temp_path);
    TEST_ASSERT_TRUE(temp_fd != -1);

    fflush(stdout);
    pid_t pid = fork();
    TEST_ASSERT_TRUE(pid != -1);

    if (pid == 0) {
        dup2(temp_fd, STDOUT_FILENO);
        close(temp_fd);
        action(arg);
        fflush(stdout);
        _exit(0);
    }

    close(temp_fd);
    int status;
    waitpid(pid, &status, 0);

    FILE* capture_fp = fopen(temp_path, "r");
    TEST_ASSERT_NOT_NULL(capture_fp);

    char** lines = NULL;
    int line_count = 0;
    char line_buf[1024];

    while (fgets(line_buf, sizeof(line_buf), capture_fp) != NULL) {
        size_t len = strlen(line_buf);
        if (len > 0 && line_buf[len - 1] == '\n') {
            line_buf[len - 1] = '\0';
        }

        lines = (char**)realloc(lines, sizeof(char*) * (line_count + 1));
        lines[line_count] = strdup(line_buf);
        line_count++;
    }

    fclose(capture_fp);
    remove(temp_path);

    *out_line_count = line_count;
    return lines;
};

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
    char db_path[] = "/tmp/db_test_table_full_XXXXXX";
    int fd = mkstemp(db_path);
    TEST_ASSERT_TRUE(fd != -1);
    close(fd);

    Table* table = db_open(db_path);

    Statement statement;
    statement.type = STATEMENT_INSERT;

    ExecuteResult result = EXECUTE_SUCCESS;

    for (int i = 0; i < LEAF_NODE_MAX_CELLS + 1; i++) {
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
        EXECUTE_SUCCESS,
        result
    );

    db_close(table);
    remove(db_path);
};

static void run_constants_command(void* arg) {
    (void)arg;
    printf("Constants:\n");
    print_constants();
};

void test_prints_constants(void) {
    int line_count = 0;
    char** lines = capture_stdout_lines(run_constants_command, NULL, &line_count);

    const char* expected[] = {
        "Constants:",
        "ROW_SIZE: 293",
        "COMMON_NODE_HEADER_SIZE: 6",
        "LEAF_NODE_HEADER_SIZE: 10",
        "LEAF_NODE_CELL_SIZE: 297",
        "LEAF_NODE_SPACE_FOR_CELLS: 4086",
        "LEAF_NODE_MAX_CELLS: 13",
    };
    int expected_count = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT_EQUAL(expected_count, line_count);
    for (int i = 0; i < expected_count && i < line_count; i++) {
        TEST_ASSERT_EQUAL_STRING(expected[i], lines[i]);
    }

    free_captured_lines(lines, line_count);
};

void test_duplicate_key(void) {
    char db_path[] = "/tmp/db_test_duplicate_key_XXXXXX";
    int fd = mkstemp(db_path);
    TEST_ASSERT_TRUE(fd != -1);
    close(fd);

    Table* table = db_open(db_path);

    Statement statement;
    statement.type = STATEMENT_INSERT;
    statement.row_to_insert.id = 1;
    strcpy(statement.row_to_insert.username, "user1");
    strcpy(statement.row_to_insert.email, "person1@example.com");

    ExecuteResult first_result = execute_insert(&statement, table);
    ExecuteResult second_result = execute_insert(&statement, table);

    TEST_ASSERT_EQUAL(EXECUTE_SUCCESS, first_result);
    TEST_ASSERT_EQUAL(EXECUTE_DUPLICATE_KEY, second_result);

    db_close(table);
    remove(db_path);
};

static void run_btree_command(void* arg) {
    Table* table = (Table*)arg;

    int ids[] = {3, 1, 2};
    Statement statement;
    statement.type = STATEMENT_INSERT;

    for (int i = 0; i < 3; i++) {
        statement.row_to_insert.id = ids[i];

        snprintf(
            statement.row_to_insert.username,
            sizeof(statement.row_to_insert.username),
            "user%d",
            ids[i]
        );

        snprintf(
            statement.row_to_insert.email,
            sizeof(statement.row_to_insert.email),
            "person%d@example.com",
            ids[i]
        );

        execute_insert(&statement, table);
    }

    printf("Tree:\n");
    print_tree(table->pager, 0, 0);
};

void test_prints_one_node_btree_structure(void) {
    char db_path[] = "/tmp/db_test_btree_XXXXXX";
    int fd = mkstemp(db_path);
    TEST_ASSERT_TRUE(fd != -1);
    close(fd);

    Table* table = db_open(db_path);

    int line_count = 0;
    char** lines = capture_stdout_lines(run_btree_command, table, &line_count);

    const char* expected[] = {
        "Tree:",
        "- leaf (size 3)",
        "  - 1",
        "  - 2",
        "  - 3",
    };
    int expected_count = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT_EQUAL(expected_count, line_count);
    for (int i = 0; i < expected_count && i < line_count; i++) {
        TEST_ASSERT_EQUAL_STRING(expected[i], lines[i]);
    }

    free_captured_lines(lines, line_count);
    db_close(table);
    remove(db_path);
};

static void run_three_leaf_node_btree_command(void* arg) {
    char* db_path = (char*)arg;
    Table* table = db_open(db_path);

    Statement statement;
    statement.type = STATEMENT_INSERT;

    for (int i = 1; i <= 14; i++) {
        statement.row_to_insert.id = i;

        snprintf(
            statement.row_to_insert.username,
            sizeof(statement.row_to_insert.username),
            "user%d",
            i
        );

        snprintf(
            statement.row_to_insert.email,
            sizeof(statement.row_to_insert.email),
            "person%d@example.com",
            i
        );

        execute_insert(&statement, table);
    }

    printf("Tree:\n");
    print_tree(table->pager, 0, 0);

    statement.row_to_insert.id = 15;
    snprintf(statement.row_to_insert.username, sizeof(statement.row_to_insert.username), "user15");
    snprintf(statement.row_to_insert.email, sizeof(statement.row_to_insert.email), "person15@example.com");
    execute_insert(&statement, table);

    db_close(table);
};

void test_prints_three_leaf_node_btree_structure(void) {
    char db_path[] = "/tmp/db_test_btree3_XXXXXX";
    int fd = mkstemp(db_path);
    TEST_ASSERT_TRUE(fd != -1);
    close(fd);

    int line_count = 0;
    char** lines = capture_stdout_lines_forked(run_three_leaf_node_btree_command, db_path, &line_count);

    const char* expected[] = {
        "Tree:",
        "- internal (size 1)",
        "  - leaf (size 7)",
        "    - 1",
        "    - 2",
        "    - 3",
        "    - 4",
        "    - 5",
        "    - 6",
        "    - 7",
        "  - key 7",
        "  - leaf (size 7)",
        "    - 8",
        "    - 9",
        "    - 10",
        "    - 11",
        "    - 12",
        "    - 13",
        "    - 14",
        "Need to implement searching an internal node",
    };
    int expected_count = sizeof(expected) / sizeof(expected[0]);

    TEST_ASSERT_EQUAL(expected_count, line_count);
    for (int i = 0; i < expected_count && i < line_count; i++) {
        TEST_ASSERT_EQUAL_STRING(expected[i], lines[i]);
    }

    free_captured_lines(lines, line_count);
    remove(db_path);
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
    RUN_TEST(test_duplicate_key);
    RUN_TEST(test_prints_constants);
    RUN_TEST(test_prints_one_node_btree_structure);
    RUN_TEST(test_prints_three_leaf_node_btree_structure);
    return UNITY_END();
};