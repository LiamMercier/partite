# libproject

A work in progress mathematics library.

### Backlog

This is mostly a personal consideration list for me to track different features that I think would be interesting to see in the library.

#### Graphs

- PropertyGraph wrapper with built in safety checks
- unique_neighbors(v) that does filtering for consumers who cannot have duplicates
- noexcept everything that is noexcept compatible
- Representation class template file for users who want to make their own representation
- Evaluate changing EdgeID and VertexID types outside of compilation (i.e template)
- A* search, DAG paths / topological sort, Bidirectional search
- Implicit graphs
- Consider graph algorithms with GPU dispatch (long term idea to look into)

#### Groups

- Investigate different schemes for group representation
