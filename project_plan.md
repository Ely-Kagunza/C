# Project Plan: C Mastery Through a Real Database System

## Purpose

This project follows the pinned learning guide in [c_guide.md](c_guide.md) and turns it into a concrete build plan. The goal is to learn C by building a real system, not by writing isolated exercises.

## Project Root

The new project will live inside the [DB-Proj](DB-Proj) folder. That folder is the working root for the application, and everything related to the project should be organized under it.

## Working Rules

- Prefer the best real implementation, not a toy shortcut.
- Build one milestone at a time.
- Always keep the project usable at each stage.
- Use Python comparisons as mental bridges when learning concepts.
- Treat error handling, memory management, and testing as part of the lesson, not as optional extras.

## Milestone Policy

This project is now run as a production-grade build, not as a stream of small phase edits.

- Each milestone must be complete end to end before the next one begins.
- No scope drift or small in-between additions unless they are required to finish the current milestone.
- Documentation, code, and tests should all agree on the same milestone boundary.
- If a feature is part of the milestone, we finish it fully instead of leaving a partial version behind.

## Project Direction

We are building a **Simple Database System in C** that grows from a command-line database into a real networked service. The plan starts with core C fundamentals and ends with an event-driven networked server.

To make the project feel complete, we will also include a **browser frontend** that talks to the C backend over HTTP. The frontend is part of the project, not a separate add-on.

The strongest path for this project is:

- Real data structures instead of placeholder logic
- Real persistence instead of in-memory-only demos
- Real query handling instead of hardcoded outputs
- Real socket networking instead of simulated communication
- Real async request handling instead of blocking server code

## Final Outcome

By the time this project is complete, the system should be able to:

- Store records in memory
- Save and load data reliably
- Search, sort, update, and delete records
- Use indexes for faster lookup
- Expose data through a network server
- Accept client requests and return responses
- Handle concurrent clients with event-driven I/O
- Present a usable frontend in the browser for interacting with the backend

## Frontend Scope

The frontend will be kept practical and decent, not overengineered. It should:

- Provide a clean page for entering data and running queries
- Show database records in tables or cards
- Display success and error messages clearly
- Show server status and request results
- Send requests to the C backend with JavaScript `fetch()`

The frontend will use:

- HTML for structure
- CSS for layout and presentation
- Vanilla JavaScript for interaction

This keeps the UI lightweight while still making the project feel like a real application.

## Milestone Roadmap

We are following the guide through **Phase 14** and stopping there for this project, but we will deliver it as complete milestones rather than scattered micro-steps.

### Phase 1: Basic Data Structures
- Build record structs
- Store records in arrays
- Load and save simple text data
- Learn struct access and initialization

### Phase 2: Pointers and Linked Lists
- Use pointers correctly
- Build dynamic record storage
- Create and free linked structures
- Understand memory ownership

### Phase 3: Binary File I/O
- Save and load binary records
- Use `fread`, `fwrite`, `fseek`, and `ftell`
- Learn structured persistence

### Phase 4: Searching and Sorting
- Implement linear search
- Implement binary search
- Sort records with comparison functions
- Measure algorithm behavior

### Phase 5: Dynamic Memory and Growth
- Use `malloc`, `realloc`, and `free`
- Expand storage dynamically
- Track capacity and count correctly
- Check for leaks and allocation failures

### Phase 6: Modular Architecture
- Split code into headers and implementation files
- Define clean APIs
- Separate database logic from I/O logic

### Phase 7: Hash Tables and Indexes
- Build fast lookup structures
- Create indexes on fields like ID and name
- Improve query performance

### Phase 8: Command-Line Interface
- Parse arguments
- Build an interactive CLI
- Validate input and display results clearly

### Phase 9: Advanced I/O and Optimization
- Add caching and batching ideas
- Improve file access patterns
- Measure performance instead of guessing

### Phase 10: SQL Implementation
- Parse SQL-like input
- Execute queries through a simple engine
- Add filtering, ordering, and aggregation

### Phase 11: Advanced C Patterns
- Use callbacks
- Use function pointers
- Build reusable, idiomatic C systems

### Phase 12: Systems Programming
- Use system calls and OS-level features
- Learn signals, processes, memory mapping, and IPC
- Understand how C interacts with the operating system

### Phase 13: Network Programming
- Build TCP client-server communication
- Design a wire protocol
- Handle multiple network clients
- Learn replication and distributed basics

### Phase 14: Event-Driven Async Server
- Build a non-blocking server
- Use an event loop and state machines
- Accept many concurrent clients
- Handle partial reads and writes
- Return responses over HTTP-style requests

## Explicitly Excluded From This Project

We are not continuing into:

- Phase 15: Performance Optimization & Vectorization
- Phase 16: HTTP APIs & Serialization
- Phase 17: Production Systems

Those are useful later, but they are outside the scope of this project plan.

## Project Structure

The project should evolve in layers:

- Core data layer: records, storage, validation
- Query layer: search, sort, filter, update, delete
- Persistence layer: file I/O and binary save/load
- Indexing layer: hash tables and faster lookup
- Interface layer: CLI and later network requests
- Server layer: async request handling and response generation
- Frontend layer: browser UI that communicates with the backend

## Production-Grade Layout

We will arrange the project like a production codebase, not a single-file exercise. A clean layout for [DB-Proj](DB-Proj) should look like this:

```text
DB-Proj/
├── README.md
├── Makefile
├── .gitignore
├── docs/
│   ├── architecture.md
│   ├── api.md
│   └── schema.md
├── include/
│   ├── database.h
│   ├── query.h
│   ├── storage.h
│   ├── index.h
│   ├── cli.h
│   ├── server.h
│   └── frontend.h
├── src/
│   ├── core/
│   │   ├── database.c
│   │   ├── query.c
│   │   └── storage.c
│   ├── indexes/
│   │   └── index.c
│   ├── cli/
│   │   └── cli.c
│   ├── server/
│   │   └── server.c
│   ├── frontend/
│   │   └── frontend.c
│   └── main.c
├── web/
│   ├── index.html
│   ├── styles.css
│   ├── app.js
│   └── assets/
├── data/
│   ├── database.db
│   ├── seeds/
│   └── exports/
├── tests/
│   ├── test_database.c
│   ├── test_query.c
│   ├── test_storage.c
│   └── test_server.c
└── tools/
	└── scripts/
```

This layout keeps responsibilities separated:

- `include/` for public headers
- `src/core/` for database logic
- `src/indexes/` for lookup structures
- `src/cli/` for command-line interaction
- `src/server/` for HTTP and socket handling
- `src/frontend/` for backend support code used by the browser-facing layer
- `web/` for the browser frontend
- `data/` for stored records and exports
- `tests/` for validation and regression tests
- `docs/` for architecture and API notes
- `tools/` for helper scripts

This structure is meant to look and feel like a production project from the beginning, while still staying simple enough to grow phase by phase.

This is the style we want because it scales cleanly as the project grows.

## Implementation Style

- Write one working step at a time
- Test after each major change
- Keep code readable and well-commented where needed
- Prefer correctness first, then performance
- Use the pinned guide as the learning contract

## Completion Definition

This project is complete when Phase 14 is finished and the system can:

- accept real client requests
- process records correctly
- return responses without blocking
- remain understandable as a learning project

The documentation and implementation should stay aligned with this completion boundary instead of adding unrelated side tasks mid-stream.

## Next Step

The next step is to begin implementing the project phase by phase, starting from the earliest missing phase in the repository and working forward in order.
