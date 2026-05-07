#pragma once

#include <tuple>
#include <unordered_set>

#include <libproject/graphs/graph-constants.hpp>
#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>
#include <libproject/graphs/adjacency-list.hpp>

using namespace libproject::graphs;

template <typename GraphT, typename EdgeDataT, typename VertexDataT>
struct GraphConfig
{
    using Graph = GraphT;
    using EdgeData = EdgeDataT;
    using VertexData = VertexDataT;
};

// Tests for a graph type combination.
using TestGraphs = ::testing::Types
<
    // Simple graph (no VertexData or EdgeData)
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<true>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<false>>,
                std::monostate,
                std::monostate>,

    // EdgeData but no VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, int>,
                int,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, int>,
                int,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, int>,
                int,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, int>,
                int,
                std::monostate>,

    // EdgeData and VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, int, int>,
                int,
                int>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, int, int>,
                int,
                int>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, int, int>,
                int,
                int>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, int, int>,
                int,
                int>,

    // VertexData but no EdgeData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, std::monostate, int>,
                std::monostate,
                int>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, std::monostate, int>,
                std::monostate,
                int>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, std::monostate, int>,
                std::monostate,
                int>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, std::monostate, int>,
                std::monostate,
                int>
>;

using TestStringGraphs = ::testing::Types
<
    // Simple graph (no VertexData or EdgeData)
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<true>>,
                std::monostate,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<false>>,
                std::monostate,
                std::monostate>,

    // EdgeData but no VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, std::string>,
                std::string,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, std::string>,
                std::string,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, std::string>,
                std::string,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, std::string>,
                std::string,
                std::monostate>,

    // EdgeData and VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, std::string, std::string>,
                std::string,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, std::string, std::string>,
                std::string,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<true>, std::string, std::string>,
                std::string,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<false>, std::string, std::string>,
                std::string,
                std::string>,

    // VertexData but no EdgeData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<true>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>,
    GraphConfig<PropertyGraph<AdjacencyList<false>,
                              std::monostate,
                              std::string>,
                std::monostate,
                std::string>
>;

struct PairHash
{
    std::size_t operator()(const std::pair<size_t, size_t> & pair) const
    {
        // boost hash combine constants to prevent collisions in new use case.
        auto h1 = std::hash<size_t>()(pair.first);
        auto h2 = std::hash<size_t>()(pair.second);
        return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
    }
};

struct VertexIDHash
{
    std::size_t operator()(const VertexID & v) const
    {
        return std::hash<size_t>()(v.value);
    }
};

struct EdgeIDHash
{
    std::size_t operator()(const EdgeID & e) const
    {
        return std::hash<size_t>()(e.value);
    }
};

struct IncidentEdgeHash
{
    std::size_t operator()(const IncidentEdge & edge) const
    {
        return std::hash<size_t>()(edge.neighbor.value)
               ^ std::hash<size_t>()(edge.id.value);
    }
};

struct IncidentEdgeEqual
{
    bool operator()(const IncidentEdge & a, const IncidentEdge & b) const
    {
        return (a.neighbor.value == b.neighbor.value)
               && (a.id == b.id);
    }
};

struct EdgeViewHash
{
    std::size_t operator()(const EdgeView & ev) const
    {
        return std::hash<size_t>()(ev.from.value)
               ^ std::hash<size_t>()(ev.to.value)
               ^ std::hash<size_t>()(ev.id.value);
    }
};

struct EdgeViewEqual
{
    bool operator()(const EdgeView & a, const EdgeView & b) const
    {
        return (a.from == b.from)
               && (a.to == b.to)
               && (a.id == b.id);
    }
};

using VertexIDSet = std::unordered_set<VertexID, VertexIDHash>;
using EdgeIDSet = std::unordered_set<EdgeID, EdgeIDHash>;
using IncidentEdgeSet = std::unordered_set<IncidentEdge,
                                           IncidentEdgeHash,
                                           IncidentEdgeEqual>;
using EdgeViewSet = std::unordered_set<EdgeView, EdgeViewHash, EdgeViewEqual>;

template <typename Graph, typename EdgeDataFunction>
Graph make_perfect_binary_tree(size_t h, EdgeDataFunction make_data)
{
    using EdgeData = typename Graph::EdgeDataType;

    std::vector<EdgeInput<EdgeData>> edges;

    if (h == 0)
    {
        return Graph(0, edges);
    }

    using id_type = VertexID::underlying_type;

    id_type n = (1ULL << (h+1)) - 1;

    for (id_type i = 0; i < n; i++)
    {
        id_type left = 2*i + 1;
        id_type right = 2*i + 2;

        if (left < n)
        {
            if constexpr (HasData<EdgeData>)
            {
                edges.push_back({VertexID{i},
                                VertexID{left},
                                make_data(i, left)});
            }
            else
            {
                edges.push_back({VertexID{i}, VertexID{left}});
            }
        }

        if (right < n)
        {
            if constexpr (HasData<EdgeData>)
            {
                edges.push_back({VertexID{i},
                                VertexID{right},
                                make_data(i, right)});
            }
            else
            {
                edges.push_back({VertexID{i}, VertexID{right}});
            }
        }
    }

    return Graph{n, edges};
}

template <typename Graph, typename EdgeDataFunction>
Graph make_grid_graph(size_t length, EdgeDataFunction make_data)
{
    using EdgeData = typename Graph::EdgeDataType;

    std::vector<EdgeInput<EdgeData>> edges;

    if (length == 0)
    {
        return Graph(0, edges);
    }

    using id_type = VertexID::underlying_type;

    // Create a grid of length * length nodes.
    id_type v_count = length * length;

    // For each row
    for (id_type row = 0; row < length; row++)
    {
        // For each column
        for (id_type col = 0; col < length; col++)
        {
            // Compute this vertex
            id_type v = row * length + col;

            // Ignore the last column, it has no right neighbor.
            if ((v % length) != (length - 1))
            {
                // Connect this vertex to its right neighbor
                id_type r = v + 1;

                if constexpr (HasData<EdgeData>)
                {
                    edges.push_back({VertexID{v},
                                    VertexID{r},
                                    make_data(v, r)});
                }
                else
                {
                    edges.push_back({VertexID{v}, VertexID{r}});
                }
            }

            // Ignore the last row, it has no lower neighbor.
            if ((v / length) != (length - 1))
            {
                // Connect this vertex to its lower neighbor
                id_type lower = v + length;

                if constexpr (HasData<EdgeData>)
                {
                    edges.push_back({VertexID{v},
                                    VertexID{lower},
                                    make_data(v, lower)});
                }
                else
                {
                    edges.push_back({VertexID{v}, VertexID{lower}});
                }
            }

        }
    }

    return Graph{v_count, edges};
}

// Create a cluster graph (k disjoint clusters of N vertices).
template <typename Graph, typename EdgeDataFunction>
Graph make_cluster_graph(size_t N,
                         size_t k,
                         EdgeDataFunction make_data)
{
    using EdgeData = typename Graph::EdgeDataType;

    std::vector<EdgeInput<EdgeData>> edges;

    // We need at least 1 vertex for each cluster.
    if (N < k)
    {
        return Graph(0, edges);
    }

    using id_type = VertexID::underlying_type;

    // Each cluster except the last gets N / k vertices
    size_t cluster_size = N / k;

    // We further have N % k vertices to distribute, starting
    // from the first cluster.
    size_t cluster_extra = N % k;

    // Now, distribute.
    id_type curr_vertex_id = 0;

    for (size_t i = 0; i < k; i++)
    {
        id_type start = curr_vertex_id;

        curr_vertex_id += (cluster_size - 1);

        if (cluster_extra > 0)
        {
            curr_vertex_id++;
            cluster_extra--;
        }

        id_type end = curr_vertex_id;

        // Setup for next iteration.
        curr_vertex_id++;

        // Create an edge for each vertex between these nodes.
        for (id_type from = start; from <= end; from++)
        {
            for (id_type to = start; to <= end; to++)
            {
                if constexpr (HasData<EdgeData>)
                {
                    edges.push_back({VertexID{from},
                                     VertexID{to},
                                     make_data(from, to)});
                }
                else
                {
                    edges.push_back({VertexID{from}, VertexID{to}});
                }
            }
        }
    }

    return Graph{N, edges};
}
