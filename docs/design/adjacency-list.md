# Adjacency List Class Design

## Assumptions

TODO: review this now that the class is multigraph

The AdjacencyList class is designed assuming the graph will be sparse, and mutations will occur. Why?

If the graph will be dense, it makes more sense for the user to use the AdjacencyMatrix class instead, this is what an adjacency matrix is designed for.

If a graph is static, a CSR style structure is more performant for sparse graphs. You don't need any of the performance tradeoffs that come with vertex and edge mutability.

If your graph is both sparse and mutable (most real data is) then you likely want to use an adjacency list.

## Data Storage

We use a vector of vectors, each storing a {VertexID, EdgeID} pair. PropertyGraph expects an EdgeID on lookup, and VertexID is required for the adjacency list structure.

For a directed graph, we store two of these structures, one for incoming vertices and one for outgoing vertices. This allows for faster deletion and iteration of incoming vertices while using a cache friendly data structure.

## Mutation

Adding an edge is trivial, we append a new element to the storage. If the data cannot be trusted to not contain duplicates, add_edge_unique should be used to provide checks before insertion. This is utilized by the PropertyGraph add_edge_unique operation.

Removing an edge (u, v) results in searching the underlying list(s) to do a swap and pop, resulting in Theta(deg(v) + deg(u)) average time.

Adding a vertex is also trivial, we simply expand the main vector if the VertexID is larger than the list size, otherwise we use an old slot that was already cleared when it was deleted.

Removing a vertex is done with the following pseudocode (for digraphs)

```text
for u : out_list(v)
    for a : in_list(u)
        if (a == v)
            remove a

for u : in_list(v)
    for a : out_list(u)
        if (a == v)
            remove a

clear in_list(v) and out_list(v)
```

We must loop over v's outgoing and incoming lists which results in Theta(deg(v)) iterations. Each inner loop requires Theta(deg(u)) iterations to search for v. Thus, the operation takes Theta(deg(v) * average_deg()) time on average.

### Justification

Removing a vertex in Theta(deg(v) * average_deg()) average time sounds bad, the performance can degrade to Theta(E) average time in a dense graph. Why? Because if E = Theta(V^2) (dense graph) then average_deg() = Theta(V) and on average deg(v) = Theta(V) thus we would get Theta(V^2) performance.

However, this assumes the graph is dense, which should not be true if you are using an AdjacencyList over an AdjacencyMatrix. Most real world data is sparse, and the average degree might be very small. For example, even in a social network the average person might only have a few hundred friends.

We have considered the following alternatives along with reasons for why they were not adopted for this class.

#### Keeping sorted lists

If we keep each vector sorted, insertions become Theta(N) on average as we must reorder the list each time.

If we store a binary search tree, the search time for a list will be Theta(log(deg(v))) on average at the cost of cache fragmentation. If deg(v) is small on average as anticipated, the performance will likely be worse.

#### Hashmap based lists

Using a container like std::unordered_map in theory would provide O(1) lookup and deletion, but has the same problems as with storing a binary search tree. The hashing function has overhead, the branching will be less predictable, the memory is fragmented or will have cache misses, and thus if the average degree is small we will experience performance degradation.

#### Storing lookup indices

Instead of std::pair<VertexID, EdgeID> we could additionally store a size_t value to instantly find the associated index in the other list. This does not work well because swap and pop deletion will reorder the indices, and without swap and pop we will have Theta(N) average deletion performance.

#### Lazy deletion

In theory, lazy deletion is a good idea. In practice, lazy deletion introduces branches into every graph iteration, like calls to neighbors(v). Bloating the iteration loop with branching does not make sense under this design.

### improvements (TODO / backlog)

Each inner std::vector is allocated on the heap, causing memory fragmentation. It would be better to allow allocation in a memory pool, but we need to solve vector resizing.

Further, we could convert the data storage to be a structure of arrays style storage. This doesn't make sense when VertexID and EdgeID are both uint32_t, since 8 bytes should fit into registers perfectly fine, but if the user compiles with larger identifiers we would likely gain performance from splitting into a SoA style format like with PropertyGraph instead of the current design.
