# Shade

> *"In the kingdom of data, nothing ever truly dies—it just fades to ghost."*

A database where deleted data never truly dies—it becomes **ghost data** that lingers, influences queries, and can be resurrected. Built for developers who want persistence with a memory.

---

## The Concept

In most databases, `DELETE` means gone forever. In **Shade**, `DELETE` means transition to a ghost state:

- **Living Data**: Active, queryable data
- **Ghost Data**: Deleted but still present, decaying over time
- **Exorcised Data**: Permanently removed after ghost decay

---

## Why Shade?

Sometimes data shouldn't disappear immediately. Sometimes you need a safety net. Sometimes the past should influence the present.

---

## Features

- **Ghost Persistence**: Deleted data lingers as ghosts
- **Resurrection**: Bring deleted records back to life
- **Ghost Analytics**: Monitor ghost activity and decay
- **Configurable Decay**: Control how quickly ghosts fade
- **Flexible Queries**: Query living data only or include ghosts
- **Type Safety**: Support for `INT`, `FLOAT`, `BOOL`, `STRING` data types

---

## Quick Start

```sql
-- Create a table
CREATE TABLE users (id INT, name STRING, age INT)

-- Insert data
INSERT INTO users VALUES (1, "Alice", 30)

-- Query living data
SELECT * FROM users

-- Delete a record (becomes a ghost)
DELETE FROM users WHERE id = 1

-- Query including ghosts
SELECT * FROM users WITH GHOSTS

-- Resurrect a deleted record
RESURRECT users 1

-- Monitor ghost activity
GHOST STATS

-- Weaken ghosts over time
DECAY GHOSTS 0.5
```

---

## Command Reference

```text
CREATE TABLE <name> (<col1 type>, ...)             - Create a new table
DROP TABLE <name>                                  - Remove a table
DEBUG INFO                                         - Show memory usage info
INSERT INTO <table> VALUES (<val1>, ...)           - Insert a new record
SELECT * FROM <table>                              - Query all living data
SELECT * FROM <table> WITH GHOSTS                  - Query including ghosts
DELETE FROM <table> WHERE id = <id>                - Delete record (create ghost)
RESURRECT <table> <id>                             - Bring ghost back to life
GHOST STATS                                        - Show ghost analytics
DECAY GHOSTS <amount>                              - Weaken all ghosts
HELP                                               - Show help message
EXIT                                               - Exit Shade DB
```

---

## Data Types

| Type | Description |
|------|-------------|
| `INT` | Integer numbers |
| `FLOAT` | Floating-point numbers |
| `BOOL` | Boolean values (true/false) |
| `STRING` | Text data |

---

## Ghost System

### Ghost Lifecycle

1. **Living**: Active, queryable data
2. **Ghost**: Deleted but lingering, decaying over time
3. **Exorcised**: Permanently removed after sufficient decay

### Ghost Operations

- **`DECAY GHOSTS`**: Reduce ghost strength (0.0-1.0)
- **`RESURRECT`**: Restore ghost to living state
- **`GHOST STATS`**: Analytics on ghost population and decay

## Architecture

Shade uses an embedded database architecture with:

- In-memory storage with ghost tracking
- Planned persistence layer
- Type-safe data handling
- Ghost decay management system

---

**Shade: Because data deserves an afterlife.**
