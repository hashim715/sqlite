#ifndef DB_H
#define DB_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 255
#define TABLE_MAX_PAGES 100

#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

#define ID_SIZE size_of_attribute(Row, id)
#define USERNAME_SIZE size_of_attribute(Row, username)
#define EMAIL_SIZE size_of_attribute(Row, email)

#define ID_OFFSET 0
#define USERNAME_OFFSET (ID_OFFSET + ID_SIZE)
#define EMAIL_OFFSET (USERNAME_OFFSET + USERNAME_SIZE)
#define ROW_SIZE (ID_SIZE + USERNAME_SIZE + EMAIL_SIZE)

#define PAGE_SIZE 4096

/*
 * Common Node Header Layout
*/

#define NODE_TYPE_SIZE         sizeof(uint8_t)
#define NODE_TYPE_OFFSET       0
#define IS_ROOT_SIZE           sizeof(uint8_t)
#define IS_ROOT_OFFSET         NODE_TYPE_SIZE
#define PARENT_POINTER_SIZE    sizeof(uint32_t)
#define PARENT_POINTER_OFFSET  (IS_ROOT_OFFSET + IS_ROOT_SIZE)
#define COMMON_NODE_HEADER_SIZE (NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE)

/*
* Leaf Node Header Layout
*/
#define LEAF_NODE_NUM_CELLS_SIZE   sizeof(uint32_t)
#define LEAF_NODE_NUM_CELLS_OFFSET COMMON_NODE_HEADER_SIZE
#define LEAF_NODE_NEXT_LEAF_SIZE  sizeof(uint32_t)
#define LEAF_NODE_NEXT_LEAF_OFFSET \
    (LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE)
#define LEAF_NODE_HEADER_SIZE  (COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_NEXT_LEAF_SIZE)

/*
* Leaf Node Body Layout
*/
#define LEAF_NODE_KEY_SIZE     sizeof(uint32_t)
#define LEAF_NODE_KEY_OFFSET   0
#define LEAF_NODE_VALUE_SIZE   ROW_SIZE
#define LEAF_NODE_VALUE_OFFSET (LEAF_NODE_KEY_OFFSET + LEAF_NODE_KEY_SIZE)
#define LEAF_NODE_CELL_SIZE    (LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE)
#define LEAF_NODE_SPACE_FOR_CELLS (PAGE_SIZE - LEAF_NODE_HEADER_SIZE)
#define LEAF_NODE_MAX_CELLS (LEAF_NODE_SPACE_FOR_CELLS / LEAF_NODE_CELL_SIZE)


/*
Leaf Node Sizes
*/
#define LEAF_NODE_RIGHT_SPLIT_COUNT ((LEAF_NODE_MAX_CELLS + 1) / 2)
#define LEAF_NODE_LEFT_SPLIT_COUNT \
    ((LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT)


/*
Internal Node Header Layout
*/
#define INTERNAL_NODE_NUM_KEYS_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_NUM_KEYS_OFFSET COMMON_NODE_HEADER_SIZE
#define INTERNAL_NODE_RIGHT_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_RIGHT_CHILD_OFFSET (INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE)
#define INTERNAL_NODE_HEADER_SIZE  (COMMON_NODE_HEADER_SIZE + INTERNAL_NODE_NUM_KEYS_SIZE + INTERNAL_NODE_RIGHT_CHILD_SIZE)

/*
Internal Node Body Layout
*/
#define INTERNAL_NODE_KEY_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CHILD_SIZE sizeof(uint32_t)
#define INTERNAL_NODE_CELL_SIZE (INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE)
#define INTERNAL_NODE_SPACE_FOR_CELLS (PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE)
/* Keep this small for testing */
#define INTERNAL_NODE_MAX_CELLS 3


typedef struct {
    char* buffer;
    size_t buffer_length;
    ssize_t input_length;
} InputBuffer;

typedef enum {
    NODE_INTERNAL,
    NODE_LEAF
} NodeType;

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    uint32_t num_pages;
    void* pages[TABLE_MAX_PAGES];
} Pager;

typedef struct {
    uint32_t root_page_num;
    Pager* pager;
} Table;

typedef struct {
    Table* table;
    uint32_t page_num;
    uint32_t cell_num;
    bool end_of_table; // Indicates a position one past the last element
} Cursor;

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE + 1];
    char email[COLUMN_EMAIL_SIZE + 1];
} Row;

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
} MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_NEGATIVE_ID,
    PREPARE_SYNTAX_ERROR,
    PREPARE_STRING_TOO_LONG
} PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
} StatementType;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_TABLE_FULL,
    EXECUTE_DUPLICATE_KEY
} ExecuteResult;

typedef struct {
    StatementType type;
    Row row_to_insert;
} Statement;

/* function declarations */
InputBuffer* new_input_buffer();

void print_prompt();

void read_input(InputBuffer* input_buffer);

MetaCommandResult do_meta_command(InputBuffer* input_buffer, Table* table);

PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement);

ExecuteResult execute_statement(Statement* statement, Table* table);

ExecuteResult execute_insert(Statement* statement, Table* table);

ExecuteResult execute_select(Statement* statement, Table* table);

Table* db_open(const char* filename);

Cursor* table_start(Table* table);

Cursor* table_end(Table* table);

Cursor* table_find(Table* table, uint32_t key);

Cursor* internal_node_find(Table* table, uint32_t root_page_num, uint32_t key);

void* cursor_value(Cursor* cursor);

void cursor_advance(Cursor* cursor);

void db_close(Table* table);

NodeType get_node_type(void* node);

void set_node_type(void* node, NodeType type);

Cursor* leaf_node_find(Table* table, uint32_t page_num, uint32_t key);

void pager_flush(Pager* pager, uint32_t page_num);

void print_row(Row* row);

Pager* page_open(const char* filename);

void* get_page(Pager* pager, uint32_t page_num);

uint32_t* leaf_node_num_cells(void* node);

void* leaf_node_cell(void* node, uint32_t cell_num);

uint32_t* leaf_node_key(void* node, uint32_t cell_num);

uint32_t* internal_node_num_keys(void* node);

uint32_t* internal_node_right_child(void* node);

uint32_t* internal_node_cell(void* node, uint32_t cell_num);

uint32_t* internal_node_child(void* node, uint32_t child_num);

uint32_t* internal_node_key(void* node, uint32_t key_num);

uint32_t get_node_max_key(void* node);

uint32_t* node_parent(void* node);

void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);

uint32_t* leaf_node_next_leaf(void* node);

void initialize_internal_node(void* node);

bool is_node_root(void* node);

void set_node_root(void* node, bool is_root);

void create_new_root(Table* table, uint32_t right_child_page_num);

void internal_node_insert(Table* table, uint32_t parent_page_num, uint32_t child_page_num);

void* leaf_node_value(void* node, uint32_t cell_num);

void initialize_leaf_node(void* node);

void leaf_node_split_and_insert(Cursor* cursor, uint32_t key, Row* value);

uint32_t get_unused_page_num(Pager* pager);

void leaf_node_insert(Cursor* cursor, uint32_t key, Row* value);

void print_constants();

void indent(uint32_t level);

void print_tree(Pager* pager, uint32_t page_num, uint32_t indentation_level);

#endif