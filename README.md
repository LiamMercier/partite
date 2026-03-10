# libproject

Draft of a mathematics library

## API decisions (subject to change)

TODO: make this proper documentation

- Each GraphRepresentation compatible class exposes "is_directed" as a bool for consumers
- Each GraphRepresentation provides add_edge(from, to, edge_id) to add edges
- Each GraphRepresentation provides find_edge(from, to) and has_edge(from, to)
