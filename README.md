# libproject

Draft of a mathematics library

## API decisions (subject to change)

TODO: make this proper documentation

### Each GraphRepresentation compatible class exposes 
- is_directed as a bool for template consumers
- vertex_count() as a way to query the number of vertices
- neighbors(v) as a range to iterate over
