> *Where deleted data gets an afterlife*

---

## What is Shade?

Shade reimagines data deletion. Instead of permanent removal, deleted records become **ghosts** - lingering in memory, queryable when needed, and resurrectable at will. Think of it as version control meets database management, with a supernatural twist.

---

## See It In Action

Watch the demo video to see a quick demo of some basic features without having to download it for yourself

<p align="center">
  <a href="https://hc-cdn.hel1.your-objectstorage.com/s/v3/c96e6c99fb5cd6f1c1bf2fdc5328604ab1b4f9db_shade-demo.mp4">
    <img src="https://dummyimage.com/800x450/000/fff&text=▶+Watch+Shade+Demo" alt="Watch the Shade Demo Video" width="600"/>
  </a>
</p>

--- 

## Key Features

### Ghost Data System
- **Soft Deletion**: Deleted records transition to ghost state instead of permanent removal
- **Resurrection**: Bring any ghost back to life with a single command
- **Decay Mechanics**: Ghost data gradually weakens over time, eventually exorcising itself
- **Ghost Analytics**: Track ghost population, decay rates, and system health

### Core Database Features
- **SQL-like Syntax**: Familiar commands with ghost-powered extensions
- **Type Safety**: Native support for `INT`, `FLOAT`, `BOOL`, and `STRING` types
- **In-Memory Storage**: Fast operations with optional persistence layer
- **Flexible Queries**: Query living data only, or include ghosts when needed

### Developer Tools
- **Debug Info**: Memory usage and performance insights
- **Persistence Control**: Enable/disable data persistence on demand
- **Interactive REPL**: Built-in command-line interface for exploration

---

## Installation

**Download** the shade binary from this releases page if you're on Linux, otherwise build it yourself:

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

## Use Cases

- **Data Safety Net**: Recover from accidental deletions
- **Audit Trails**: Keep track of what was deleted and when
- **Soft Delete Patterns**: Implement logical deletion without schema changes
- **Testing & Development**: Experiment with data without permanent consequences
- **Time-Travel Queries**: Analyze data including deleted records

---
