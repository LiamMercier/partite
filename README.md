# Partite

Partite is a mathematics library, currently focused on graphs.

This project is a work in progress, some parts of the API may change.

## Development

The following documents useful information for developers.

### Toolchain Requirements

- CMake with at least version 3.25
- C++23 compatible compiler and standard library
    - C++23 usage is for typical features, you might be able to compile with a partially implemented library
    
### Compiling Tests

Tests are locked behind the following flags:

| Flag | Purpose |
|------|---------|
| GRAPH_BUILD_TESTS | Enable graph related correctness tests |
| GRAPH_EXTENSIVE_TESTS | Enable extensive graph tests, including correctness tests for every possible graph of 4 vertices |

To compile all tests, go to the project root and run:

```
cmake -S . -B builds -DGRAPH_BUILD_TESTS=ON -DGRAPH_EXTENSIVE_TESTS=ON -DCMAKE_BUILD_TYPE=Release && cmake --build builds
```

Optionally, compile with multiple threads:

```
cmake -S . -B builds -DGRAPH_BUILD_TESTS=ON -DGRAPH_EXTENSIVE_TESTS=ON -DCMAKE_BUILD_TYPE=Release && cmake --build builds -j
```

Tests can be run with the following:

```
./builds/tests/unit-tests
```

## Backlog

This is mostly a personal consideration list for me to track different features that I think would be interesting to see in the library.

### Graphs

- PropertyGraph wrapper with built in safety checks
- unique_neighbors(v) that does filtering for consumers who cannot have duplicates
- noexcept everything that is noexcept compatible
- Representation class template file for users who want to make their own representation
- Evaluate changing EdgeID and VertexID types outside of compilation (i.e template)
- A* search, DAG paths / topological sort, Bidirectional search
- Implicit graphs
- Consider graph algorithms with GPU dispatch (long term idea to look into)

### Groups & Abstract Algebra

- Investigate different schemes for group / ring representation
