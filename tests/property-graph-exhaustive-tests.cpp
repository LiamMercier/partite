#include <gtest/gtest.h>

#include "test-utils.hpp"
#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>
#include <libproject/graphs/adjacency-list.hpp>

using SubtestResultPair = std::pair<std::string, bool>;

template <typename Graph, typename Edges>
SubtestResultPair assert_edges_exist(const Graph & graph,
                                     const Edges & edges)
{
    size_t index = 0;
    for (const auto & edge : edges)
    {
        bool exists = graph.has_edge(edge.from, edge.to);

        if (!exists)
        {
            std::string fail = "missing edge ("
                               + std::to_string(edges[index].from.value)
                               + ", "
                               + std::to_string(edges[index].to.value)
                               + ")";
            return {fail, false};
        }

        index++;
    }

    return {std::string(), true};
}

template <bool directed, typename Edges>
void populate_neighbors(const Edges & edges,
                        std::vector<VertexIDSet> & neighbors,
                        std::vector<VertexIDSet> & out_neighbors,
                        std::vector<VertexIDSet> & in_neighbors)
{
    for (const auto & e : edges)
    {
        if constexpr (directed)
        {
            neighbors[e.from.value].insert(e.to);
            out_neighbors[e.from.value].insert(e.to);
            in_neighbors[e.to.value].insert(e.from);
        }
        else
        {
            neighbors[e.from.value].insert(e.to);
            neighbors[e.to.value].insert(e.from);
        }
    }
}

template <bool directed, typename Edges>
void populate_edge_ids(const Edges & edges,
                       std::vector<EdgeIDSet> & edge_ids,
                       std::vector<EdgeIDSet> & out_edge_ids,
                       std::vector<EdgeIDSet> & in_edge_ids)
{
    size_t index = 0;
    for (const auto & e : edges)
    {
        if constexpr (directed)
        {
            edge_ids[e.from.value].insert(index);
            out_edge_ids[e.from.value].insert(index);
            in_edge_ids[e.to.value].insert(index);
        }
        else
        {
            edge_ids[e.from.value].insert(index);
            edge_ids[e.to.value].insert(index);
        }

        index++;
    }
}

template <bool directed, typename Edges>
void populate_edges(const Edges & edges,
                    std::vector<IncidentEdgeSet> & incident_edges,
                    std::vector<IncidentEdgeSet> & out_edges,
                    std::vector<IncidentEdgeSet> & in_edges)
{
    size_t index = 0;
    for (const auto & e : edges)
    {
        if constexpr (directed)
        {
            incident_edges[e.from.value].insert({e.to.value, index});
            out_edges[e.from.value].insert({e.to.value, index});
            in_edges[e.to.value].insert({e.from.value, index});
        }
        else
        {
            incident_edges[e.from.value].insert({e.to.value, index});
            incident_edges[e.to.value].insert({e.from.value, index});
        }

        index++;
    }
}

template <bool directed, typename Graph>
SubtestResultPair
    assert_neighbors(const Graph & graph,
                     const std::vector<VertexIDSet> & neighbors,
                     const std::vector<VertexIDSet> & out_neighbors,
                     const std::vector<VertexIDSet> & in_neighbors)
{
    for (const auto & v : graph.vertices())
    {
        // Check neighbors(v)
        {
            VertexIDSet seen;

            for (const auto & u : graph.neighbors(v))
            {
                bool match = neighbors[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected vertex "
                                       + std::to_string(u.value)
                                       + " for neighbors("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u.value);

                if (duplicate)
                {
                    std::string fail = "Duplicate vertex "
                                       + std::to_string(u.value)
                                       + " for neighbors("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != neighbors[v.value].size())
            {
                std::string fail = "neighbors("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " neighbors, expected to see "
                                   + std::to_string(
                                       out_neighbors[v.value].size()
                                   );
                return {fail, false};
            }
        }

        // Check out_neighbors(v) if digraph.
        if constexpr (directed)
        {
            VertexIDSet seen;

            for (const auto & u : graph.out_neighbors(v))
            {
                bool match = out_neighbors[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected vertex "
                                       + std::to_string(u.value)
                                       + " for out_neighbors("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate vertex "
                                       + std::to_string(u.value)
                                       + " for out_neighbors("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != out_neighbors[v.value].size())
            {
                std::string fail = "out_neighbors("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " neighbors, expected to see "
                                   + std::to_string(
                                       out_neighbors[v.value].size()
                                   );
                return {fail, false};
            }
        }

        // Check in_neighbors(v) if digraph.
        if constexpr (directed)
        {
            VertexIDSet seen;

            for (const auto & u : graph.in_neighbors(v))
            {
                bool match = in_neighbors[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected vertex "
                                       + std::to_string(u.value)
                                       + " for in_neighbors("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate vertex "
                                       + std::to_string(u.value)
                                       + " for in_neighbors("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != in_neighbors[v.value].size())
            {
                std::string fail = "in_neighbors("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " neighbors, expected to see "
                                   + std::to_string(
                                       in_neighbors[v.value].size()
                                   );
                return {fail, false};
            }
        }
    }

    return {std::string(), true};
}

template <bool directed, typename Graph>
SubtestResultPair
    assert_neighboring_edge_ids(const Graph & graph,
                                const std::vector<EdgeIDSet> & edge_ids,
                                const std::vector<EdgeIDSet> & out_edge_ids,
                                const std::vector<EdgeIDSet> & in_edge_ids)
{
    for (const auto & v : graph.vertices())
    {
        // Check incident_edge_ids(v)
        {
            EdgeIDSet seen;

            for (const auto & u : graph.incident_edge_ids(v))
            {
                bool match = edge_ids[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected EdgeID "
                                       + std::to_string(u.value)
                                       + " for incident_edge_ids("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate EdgeID "
                                       + std::to_string(u.value)
                                       + " for incident_edge_ids("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != edge_ids[v.value].size())
            {
                std::string fail = "incident_edge_ids("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " ids, expected to see "
                                   + std::to_string(
                                       edge_ids[v.value].size()
                                   );
                return {fail, false};
            }
        }

        // Check out_edge_ids(v) if digraph.
        if constexpr (directed)
        {
            EdgeIDSet seen;

            for (const auto & u : graph.out_edge_ids(v))
            {
                bool match = out_edge_ids[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected EdgeID "
                                       + std::to_string(u.value)
                                       + " for out_edge_ids("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate EdgeID "
                                       + std::to_string(u.value)
                                       + " for out_edge_ids("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != out_edge_ids[v.value].size())
            {
                std::string fail = "out_edge_ids("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " ids, expected to see "
                                   + std::to_string(
                                       out_edge_ids[v.value].size()
                                   );
                return {fail, false};
            }
        }

        // Check in_edge_ids(v) if digraph.
        if constexpr (directed)
        {
            EdgeIDSet seen;

            for (const auto & u : graph.in_edge_ids(v))
            {
                bool match = in_edge_ids[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected EdgeID "
                                       + std::to_string(u.value)
                                       + " for in_edge_ids("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate EdgeID "
                                       + std::to_string(u.value)
                                       + " for in_edge_ids("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != in_edge_ids[v.value].size())
            {
                std::string fail = "in_edge_ids("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " ids, expected to see "
                                   + std::to_string(
                                       in_edge_ids[v.value].size()
                                   );
                return {fail, false};
            }
        }
    }

    return {std::string(), true};
}

template <bool directed, typename Graph>
SubtestResultPair
    assert_neighboring_edges(const Graph & graph,
                             const std::vector<IncidentEdgeSet> & incident_edges,
                             const std::vector<IncidentEdgeSet> & out_edges,
                             const std::vector<IncidentEdgeSet> & in_edges)
{
    for (const auto & v : graph.vertices())
    {
        // Check incident_edges(v)
        {
            IncidentEdgeSet seen;

            for (const auto & u : graph.incident_edges(v))
            {
                bool match = incident_edges[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected IncidentEdge (neighbor: "
                                       + std::to_string(u.neighbor.value)
                                       + ", id: "
                                       + std::to_string(u.id.value)
                                       + ") for incident_edges("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate IncidentEdge (neighbor: "
                                       + std::to_string(u.neighbor.value)
                                       + ", id: "
                                       + std::to_string(u.id.value)
                                       + ") for incident_edges("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != incident_edges[v.value].size())
            {
                std::string fail = "incident_edges("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " incident edges, expected to see "
                                   + std::to_string(
                                       incident_edges[v.value].size()
                                   );
                return {fail, false};
            }
        }

        // Check out_edges(v) if digraph.
        if constexpr (directed)
        {
            IncidentEdgeSet seen;

            for (const auto & u : graph.out_edges(v))
            {
                bool match = out_edges[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected IncidentEdge (neighbor: "
                                       + std::to_string(u.neighbor.value)
                                       + ", id: "
                                       + std::to_string(u.id.value)
                                       + ") for out_edges("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate IncidentEdge (neighbor: "
                                       + std::to_string(u.neighbor.value)
                                       + ", id: "
                                       + std::to_string(u.id.value)
                                       + ") for out_edges("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != out_edges[v.value].size())
            {
                std::string fail = "out_edges("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " incident edges, expected to see "
                                   + std::to_string(
                                       out_edges[v.value].size()
                                   );
                return {fail, false};
            }
        }

        // Check in_edges(v) if digraph.
        if constexpr (directed)
        {
            IncidentEdgeSet seen;

            for (const auto & u : graph.in_edges(v))
            {
                bool match = in_edges[v.value].contains(u);

                if (!match)
                {
                    std::string fail = "Unexpected IncidentEdge (neighbor: "
                                       + std::to_string(u.neighbor.value)
                                       + ", id: "
                                       + std::to_string(u.id.value)
                                       + ") for in_edges("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                bool duplicate = seen.contains(u);

                if (duplicate)
                {
                    std::string fail = "Duplicate IncidentEdge (neighbor: "
                                       + std::to_string(u.neighbor.value)
                                       + ", id: "
                                       + std::to_string(u.id.value)
                                       + ") for in_edges("
                                       + std::to_string(v.value)
                                       + ")";
                    return {fail, false};
                }

                seen.insert(u);
            }

            if (seen.size() != in_edges[v.value].size())
            {
                std::string fail = "in_edges("
                                   + std::to_string(v.value)
                                   + ") saw "
                                   + std::to_string(seen.size())
                                   + " incident edges, expected to see "
                                   + std::to_string(
                                       in_edges[v.value].size()
                                   );
                return {fail, false};
            }
        }
    }

    return {std::string(), true};
}

template <typename Graph, typename Edges>
SubtestResultPair
    assert_global_edge_ids(const Graph & graph,
                           const Edges & edges)
{
    EdgeIDSet seen;

    for (const auto & e : graph.edge_ids())
    {
        if (edges.size() == 0)
        {
            std::string fail = "Unexpected EdgeID "
                               + std::to_string(e.value)
                               + " for edge_ids()";
            return {fail, false};
        }

        bool valid = (e.value <= (edges.size() - 1));

        if (!valid)
        {
            std::string fail = "Unexpected EdgeID "
                               + std::to_string(e.value)
                               + " for edge_ids()";
            return {fail, false};
        }

        bool duplicate = seen.contains(e);

        if (duplicate)
        {
            std::string fail = "Duplicate edge ID "
                               + std::to_string(e.value)
                               + " for edge_ids()";
            return {fail, false};
        }

        seen.insert(e);
    }

    if (seen.size() != edges.size())
    {
        std::string fail = "edge_ids() saw "
                           + std::to_string(seen.size())
                           + " incident edges, expected to see "
                           + std::to_string(edges.size());
        return {fail, false};
    }

    return {std::string(), true};
}

template <typename Graph, typename Edges>
SubtestResultPair
    assert_global_edges(const Graph & graph,
                        const Edges & edges)
{
    EdgeViewSet seen;

    for (const auto & e : graph.edges())
    {
        if (edges.size() == 0)
        {
            std::string fail = "Unexpected EdgeView (from: "
                               + std::to_string(e.from.value)
                               + ", to: "
                               + std::to_string(e.to.value)
                               + ", id: "
                               + std::to_string(e.id.value)
                               + ") for edges()";
            return {fail, false};
        }

        bool valid = (e.id.value <= (edges.size() - 1));

        if (!valid)
        {
            std::string fail = "Unexpected EdgeView (from: "
                               + std::to_string(e.from.value)
                               + ", to: "
                               + std::to_string(e.to.value)
                               + ", id: "
                               + std::to_string(e.id.value)
                               + ") for edges()";
            return {fail, false};
        }

        bool duplicate = seen.contains(e);

        if (duplicate)
        {
            std::string fail = "Duplicate EdgeView (from: "
                               + std::to_string(e.from.value)
                               + ", to: "
                               + std::to_string(e.to.value)
                               + ", id: "
                               + std::to_string(e.id.value)
                               + ") for edges()";
            return {fail, false};
        }

        seen.insert(e);
    }

    if (seen.size() != edges.size())
    {
        std::string fail = "edges() saw "
                           + std::to_string(seen.size())
                           + " edge views, expected to see "
                           + std::to_string(edges.size());
        return {fail, false};
    }

    return {std::string(), true};
}

template <typename Graph>
SubtestResultPair
    assert_global_vertices(const Graph & graph, size_t v_count)
{
    VertexIDSet seen;

    for (const auto & v : graph.vertices())
    {
        bool valid = (v.value < v_count);

        if (!valid)
        {
            std::string fail = "Unexpected VertexID "
                               + std::to_string(v.value)
                               + " for vertices()";
            return {fail, false};
        }

        bool duplicate = seen.contains(v);

        if (duplicate)
        {
            std::string fail = "Duplicate VertexID "
                               + std::to_string(v.value)
                               + " for vertices()";
            return {fail, false};
        }

        seen.insert(v);
    }

    if (seen.size() != v_count)
    {
        std::string fail = "vertices() saw "
                           + std::to_string(seen.size())
                           + " vertices, expected to see "
                           + std::to_string(v_count);
        return {fail, false};
    }

    return {std::string(), true};
}

using namespace libproject::graphs;

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

// Test suite for each permutation of a graph of 4 vertices.
template <typename Config>
class PropertyGraphExhaustiveTest : public ::testing::Test
{
public:
    using Graph = typename Config::Graph;
    using EdgeData = typename Config::EdgeData;
    using VertexData = typename Config::VertexData;

    // We need to test the following number of graphs:
    //
    // Assumptions:
    // - We allow self loops
    // - Graphs may be directed or undirected
    // - Count isomorphic graphs, i.e do not skip E = {(1, 2)}
    //   just because we tested E = {(0, 1)}
    //
    // Directed case:
    //
    // We have |V| ways to choose a source, and |V| ways
    // to choose a target, so there are |V| * |V| possible
    // edges in the graph. Since each edge is either
    // included or not, there are 2^(|V| * |V|) graphs.
    //
    // For |V| = 4 we thus have 2^(4 * 4) = 2^16 graphs
    // which we need to test.
    //
    // Undirected case:
    //
    // For distinct vertices, we have (|V| choose 2) ways
    // to create an edge, and then we have |V| self loops. Thus,
    // there are ((|V| choose 2) + |V|) possible edges.
    //
    // Let C be the count of possible edges, then:
    //
    // |V| choose 2 = (|V| * (|V| - 1)) / 2
    //
    // C = (|V| * (|V| - 1)) / 2 + |V|
    // = (|V| * (|V| - 1)) / 2 + (2 * |V| / 2)
    // = (|V| * (|V| - 1) + 2|V|) / 2
    // = (|V|^2 - |V| + 2|V|) / 2
    // = (|V| * (|V| + 1)) / 2
    //
    // And since each edge is either included or not, we have
    // 2^((|V| * (|V| + 1)) / 2) possible graphs.
    //
    // For |V| = 4 we thus have 2^(4 * 5 / 2) = 2^10 graphs
    // which we need to test.
    static constexpr size_t v_count = 4;
    static constexpr size_t directed_graphs = 1 << (v_count * v_count);
    static constexpr size_t undirected_graphs = 1 << ((v_count
                                                       * (v_count + 1)) / 2);

    // Generator for each distinct labeled graph of 4 vertices.
    static std::vector<EdgeInput<EdgeData>> compute_edges(size_t i)
    {
        // For the directed case, we simply map the bit vector
        // of i to each edge. For example, we map (0, 0)
        // to be i[0], (1, 0) to be i[4 + 0], etc.
        //
        // We get the mapping for (u, v) as index = u * |V| + v
        //
        // Since we do not actually have a bit vector, we simply
        // bit shift and compute the mask to check the value.
        if constexpr (Graph::directed)
        {
            std::vector<EdgeInput<EdgeData>> edges;
            edges.reserve(v_count * v_count);

            for (size_t u = 0; u < v_count; u++)
            {
                for (size_t v = 0; v < v_count; v++)
                {
                    size_t index = u * v_count + v;
                    if (i & (size_t{1} << index))
                    {
                        /// Handle data vs no data cases.
                        if constexpr (HasData<EdgeData>)
                        {
                            // Encode the iteration and edge pair.
                            std::string label = std::to_string(i)
                                                + ":"
                                                + std::to_string(u)
                                                + "-"
                                                + std::to_string(v);

                            edges.push_back({u, v, std::move(label)});
                        }
                        else
                        {
                            edges.push_back({u, v});
                        }
                    }
                }
            }

            return edges;
        }
        // For the undirected case, we only need to check (u, v) and not
        // (v, u) when u < v, so we can iterate across each u, and then
        // across each v > u, and insert (u, v).
        //
        // To do this, we keep track of the bit.
        else
        {
            std::vector<EdgeInput<EdgeData>> edges;
            edges.reserve((v_count * (v_count + 1)) / 2);

            size_t index = 0;

            for (size_t u = 0; u < v_count; u++)
            {
                // Start at v = u, since (u, u - N)
                // is unnecessary to check.
                for (size_t v = u; v < v_count; v++)
                {
                    if (i & (size_t{1} << index))
                    {
                        /// Handle data vs no data cases.
                        if constexpr (HasData<EdgeData>)
                        {
                            // Encode the iteration and edge pair.
                            std::string label = std::to_string(i)
                                                + ":"
                                                + std::to_string(u)
                                                + "-"
                                                + std::to_string(v);

                            edges.push_back({u, v, std::move(label)});
                        }
                        else
                        {
                            edges.push_back({u, v});
                        }
                    }

                    index++;
                }
            }

            return edges;
        }
    }

    // Cache all graphs for significantly faster tests.
    static const std::vector<Graph> & all_graphs()
    {
        size_t num_graphs = Graph::directed ? directed_graphs
                                            : undirected_graphs;

        static std::vector<Graph> cache = [num_graphs]{
            std::vector<Graph> res;

            for (size_t i = 0; i < num_graphs; i++)
            {
                auto edges = compute_edges(i);
                res.emplace_back(v_count, edges);
            }

            return res;
        }();

        return cache;
    }

    // We simply generate a list of vertex labels "0, ..., |V| - 1"
    static std::vector<VertexData> compute_vertices()
    {
        std::vector<VertexData> vertices;
        vertices.reserve(v_count);

        for (size_t v = 0; v < v_count; v++)
        {
            vertices.push_back(std::to_string(v));
        }

        return vertices;
    }
};

TYPED_TEST_SUITE(PropertyGraphExhaustiveTest, TestGraphs);

TYPED_TEST(PropertyGraphExhaustiveTest, ValidateGraphs)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    size_t num_graphs = Graph::directed ? TestFixture::directed_graphs
                                        : TestFixture::undirected_graphs;

    size_t v_count = TestFixture::v_count;
    size_t i = 0;

    // Go over each possible graph, and do every test necessary.
    for (const auto & graph : TestFixture::all_graphs())
    {
        auto edges = TestFixture::compute_edges(i);
        i++;

        {
            auto res = assert_edges_exist(graph, edges);
            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_edges_exist failed; "
                                    << res.first;
        }

        // Check neighbors(v), out_neighbors(v), in_neighbors(v)
        std::vector<VertexIDSet> neighbors(v_count);
        std::vector<VertexIDSet> out_neighbors(v_count);
        std::vector<VertexIDSet> in_neighbors(v_count);

        populate_neighbors<Graph::directed>(edges,
                                            neighbors,
                                            out_neighbors,
                                            in_neighbors);

        {
            auto res = assert_neighbors<Graph::directed>(graph,
                                                         neighbors,
                                                         out_neighbors,
                                                         in_neighbors);
            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_neighbors failed; "
                                    << res.first;
        }

        std::vector<EdgeIDSet> edge_ids(v_count);
        std::vector<EdgeIDSet> out_edge_ids(v_count);
        std::vector<EdgeIDSet> in_edge_ids(v_count);

        populate_edge_ids<Graph::directed>(edges,
                                           edge_ids,
                                           out_edge_ids,
                                           in_edge_ids);

        {
            auto res = assert_neighboring_edge_ids
                           <Graph::directed>(graph,
                                             edge_ids,
                                             out_edge_ids,
                                             in_edge_ids);

            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_neighboring_edge_ids failed; "
                                    << res.first;
        }

        std::vector<IncidentEdgeSet> incident_edges(v_count);
        std::vector<IncidentEdgeSet> out_edges(v_count);
        std::vector<IncidentEdgeSet> in_edges(v_count);

        populate_edges<Graph::directed>(edges,
                                        incident_edges,
                                        out_edges,
                                        in_edges);

        {
            auto res = assert_neighboring_edges
                           <Graph::directed>(graph,
                                             incident_edges,
                                             out_edges,
                                             in_edges);

            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_neighboring_edges failed; "
                                    << res.first;
        }

        {
            auto res = assert_global_edge_ids(graph, edges);

            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_global_edge_ids failed; "
                                    << res.first;
        }

        {
            auto res = assert_global_edges(graph, edges);

            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_global_edges failed; "
                                    << res.first;
        }

        {
            auto res = assert_global_vertices(graph, v_count);

            ASSERT_TRUE(res.second) << "Graph "
                                    << i
                                    << ": assert_global_vertices failed; "
                                    << res.first;
        }

        // TODO: more tests.

        continue;
    }
}
