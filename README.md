# sqlite-from-scratch

A small SQLite clone written in C, built by following the ["Let's Build a
Simple Database"](https://cstack.github.io/db_tutorial/) tutorial. It
implements a REPL that accepts a minimal SQL subset, persists rows to disk
via a paged file layout, and is in the process of growing into a real
on-disk B-tree.

This is a learning project — currently through **part 14** of the tutorial
(multi-level B-tree with internal node splitting).

## Features so far

- REPL with meta-commands (`.exit`, `.btree`, `.constants`) and SQL-like
  statements (`insert`, `select`)
- Fixed-width row serialization (`id`, `username`, `email`)
- A pager that reads/writes 4KB pages to a backing file, with an in-memory
  page cache
- Cursor abstraction for walking the table
- On-disk leaf node and internal node header/body layout
- Binary search for key lookup within a node, both for leaf nodes
  (`leaf_node_find`) and internal nodes (`internal_node_find_child`)
- Duplicate key rejection on insert
- Leaf node splitting when a node fills up, growing the tree into an
  internal root
- Internal node splitting for multi-level trees, including creating new
  roots as the tree grows taller
- Leaf node sibling pointers so `select` can traverse across multiple
  leaves in order

## Building

```sh
gcc -o db main.c db.c
```

## Running

```sh
./db mydb.db
```

```
db > insert 1 hashim hashim@example.com
Executed
db > select
(1, hashim@example.com, hashim)
db > .exit
```

## Tests

Tests use the vendored [Unity](https://github.com/ThrowTheSwitch/Unity) test
framework.

```sh
gcc -o test test.c db.c Unity/src/unity.c -IUnity/src
./test
```

## Roadmap

- `delete` support
- String/text-typed keys (currently keys are fixed-width `uint32_t` ids)
