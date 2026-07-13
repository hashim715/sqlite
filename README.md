# sqlite-from-scratch

A small SQLite clone written in C, built by following the ["Let's Build a
Simple Database"](https://cstack.github.io/db_tutorial/) tutorial. It
implements a REPL that accepts a minimal SQL subset, persists rows to disk
via a paged file layout, and is in the process of growing into a real
on-disk B-tree.

This is a learning project — currently through **part 7** of the tutorial
(leaf-node B-tree layout, before internal nodes and node splitting).

## Features so far

- REPL with meta-commands (`.exit`) and SQL-like statements (`insert`,
  `select`)
- Fixed-width row serialization (`id`, `username`, `email`)
- A pager that reads/writes 4KB pages to a backing file, with an in-memory
  page cache
- Cursor abstraction for walking the table
- On-disk leaf node header/body layout, laying the groundwork for a proper
  B-tree

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

- Internal B-tree nodes and node splitting
- Multi-node trees / tree traversal for `select`
- `delete` support
- Cursor-based B-tree search instead of linear scan
