#include <gtest/gtest.h>

#include "test-utils.hpp"
#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>
#include <libproject/graphs/adjacency-list.hpp>

#include <unordered_set>

using namespace libproject::graphs;

//
// Typed test for different graph types on neighbor functions.
//

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

template <typename Config>
class PropertyGraphTest : public ::testing::Test
{
public:
    using Graph = typename Config::Graph;
    using EdgeData = typename Config::EdgeData;
    using VertexData = typename Config::VertexData;

    // static so we can access it from derived test
    static std::vector<EdgeInput<EdgeData>> make_edges()
    {
        if constexpr (std::is_same_v<EdgeData, std::monostate>)
        {
            return std::vector<EdgeInput<EdgeData>>
                   {
                        {0, 1}, {1, 2}, {1, 3},
                        {1, 6}, {2, 3}, {2, 4},
                        {3, 5}, {3, 6}
                   };
        }
        else
        {
            return std::vector<EdgeInput<EdgeData>>
                   {
                        {0, 1, EdgeData()}, {1, 2, EdgeData()},
                        {1, 3, EdgeData()}, {1, 6, EdgeData()},
                        {2, 3, EdgeData()}, {2, 4, EdgeData()},
                        {3, 5, EdgeData()}, {3, 6, EdgeData()}
                   };
        }
    }

    static std::vector<EdgeInput<EdgeData>> self_looping_graph()
    {
        if constexpr (std::is_same_v<EdgeData, std::monostate>)
        {
            return std::vector<EdgeInput<EdgeData>>
                   {
                        {0, 0}, {0, 1}, {1, 2}, {1, 4},
                        {2, 2}, {2, 3}, {3, 1}, {3, 5},
                        {4, 0}, {4, 4}, {4, 5}, {5, 6}
                   };
        }
        else
        {
            return std::vector<EdgeInput<EdgeData>>
                   {
                        {0, 0, EdgeData()}, {0, 1, EdgeData()},
                        {1, 2, EdgeData()}, {1, 4, EdgeData()},
                        {2, 2, EdgeData()}, {2, 3, EdgeData()},
                        {3, 1, EdgeData()}, {3, 5, EdgeData()},
                        {4, 0, EdgeData()}, {4, 4, EdgeData()},
                        {4, 5, EdgeData()}, {5, 6, EdgeData()}
                   };
        }
    }
};

TYPED_TEST_SUITE(PropertyGraphTest, TestGraphs);

TYPED_TEST(PropertyGraphTest, Neighbors)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    std::vector<std::unordered_set<size_t>> neighboring(7);

    // Construct neighboring based on directed
    for (const auto & e : edges)
    {
        if (Graph::directed)
        {
            neighboring[e.from.value].insert(e.to.value);
        }
        else
        {
            neighboring[e.from.value].insert(e.to.value);
            neighboring[e.to.value].insert(e.from.value);
        }
    }

    // For each vertex, check its neighbors.
    //
    // We ensure each vertex is as expected and is not a duplicate.
    for (size_t i = 0; i < graph.vertex_count(); i++)
    {
        std::unordered_set<size_t> seen;

        for (const auto & v : graph.neighbors(i))
        {
            bool match = neighboring[i].contains(v.value);

            if (!match)
            {
                FAIL() << "Unexpected vertex "
                       << v.value
                       << " in graph.neighbors("
                       << i
                       << ")";
            }

            bool duplicate = seen.contains(v.value);

            if (duplicate)
            {
                FAIL() << "Duplicate vertex "
                       << v.value
                       << " in graph.neighbors("
                       << i
                       << ")";
            }

            seen.insert(v.value);
        }

        if (seen.size() != neighboring[i].size())
        {
            FAIL() << "Missing at least one vertex in graph.neighbors("
                    << i
                    << ")\n";
        }
    }

}

TYPED_TEST(PropertyGraphTest, OutNeighbors)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::directed)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<std::unordered_set<size_t>> neighboring(7);

        // For each (u, v) do (neighboring[u] UNION v)
        for (const auto & e : edges)
        {
            neighboring[e.from.value].insert(e.to.value);
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each vertex is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<size_t> seen;

            for (const auto & v : graph.out_neighbors(i))
            {
                bool match = neighboring[i].contains(v.value);

                if (!match)
                {
                    FAIL() << "Unexpected vertex "
                           << v.value
                           << " in graph.out_neighbors("
                           << i
                           << ")";
                }

                bool duplicate = seen.contains(v.value);

                if (duplicate)
                {
                    FAIL() << "Duplicate vertex "
                           << v.value
                           << " in graph.out_neighbors("
                           << i
                           << ")";
                }

                seen.insert(v.value);
            }

            if (seen.size() != neighboring[i].size())
            {
                FAIL() << "Missing at least one vertex in graph.out_neighbors("
                        << i
                        << ")\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, InNeighbors)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::directed)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<std::unordered_set<size_t>> in_neighboring(7);

        // For each (u, v) do (neighboring[v] UNION u)
        for (const auto & e : edges)
        {
            in_neighboring[e.to.value].insert(e.from.value);
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each vertex is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<size_t> seen;

            for (const auto & v : graph.in_neighbors(i))
            {
                bool match = in_neighboring[i].contains(v.value);

                if (!match)
                {
                    FAIL() << "Unexpected vertex "
                        << v.value
                        << " in graph.in_neighbors("
                        << i
                        << ")";
                }

                bool duplicate = seen.contains(v.value);

                if (duplicate)
                {
                    FAIL() << "Duplicate vertex "
                        << v.value
                        << " in graph.in_neighbors("
                        << i
                        << ")";
                }

                seen.insert(v.value);
            }

            if (seen.size() != in_neighboring[i].size())
            {
                FAIL() << "Missing at least one vertex in graph.in_neighbors("
                        << i
                        << ")\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, NeighboringEdgeIDs)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    std::vector<std::unordered_set<size_t>> incident(7);

    // For each (u, v) with EdgeID indx, do (incident[u] UNION indx)
    //
    // For undirected edges, do this with incident[v] as well.
    size_t indx = 0;
    for (const auto & e : edges)
    {
        if (Graph::directed)
        {
            incident[e.from.value].insert(indx);
            indx++;
        }
        else
        {
            incident[e.from.value].insert(indx);
            incident[e.to.value].insert(indx);
            indx++;
        }
    }

    // For each vertex, check its outgoing neighbors.
    //
    // We ensure each vertex is as expected and is not a duplicate.
    for (size_t i = 0; i < graph.vertex_count(); i++)
    {
        std::unordered_set<size_t> seen;

        for (const auto & e : graph.incident_edge_ids(i))
        {
            bool match = incident[i].contains(e.value);

            if (!match)
            {
                FAIL() << "Unexpected edge "
                        << e.value
                        << " in graph.incident_edge_ids("
                        << i
                        << ")";
            }

            bool duplicate = seen.contains(e.value);

            if (duplicate)
            {
                FAIL() << "Duplicate edge "
                        << e.value
                        << " in graph.incident_edge_ids("
                        << i
                        << ")";
            }

            seen.insert(e.value);
        }

        if (seen.size() != incident[i].size())
        {
            FAIL() << "Missing at least one edge in graph.incident_edge_ids("
                    << i
                    << ")\n";
        }
    }
}

TYPED_TEST(PropertyGraphTest, OutEdgeIDs)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::directed)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<std::unordered_set<size_t>> out_edges(7);

        // For each (u, v) with EdgeID indx, do (out_edges[u] UNION indx)
        size_t indx = 0;
        for (const auto & e : edges)
        {
            out_edges[e.from.value].insert(indx);
            indx++;
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each edge is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<size_t> seen;

            for (const auto & e : graph.out_edge_ids(i))
            {
                bool match = out_edges[i].contains(e.value);

                if (!match)
                {
                    FAIL() << "Unexpected edge "
                        << e.value
                        << " in graph.out_edge_ids(i)";
                }

                bool duplicate = seen.contains(e.value);

                if (duplicate)
                {
                    FAIL() << "Duplicate edge "
                        << e.value
                        << " in graph.out_edge_ids(i)";
                }

                seen.insert(e.value);
            }

            if (seen.size() != out_edges[i].size())
            {
                FAIL() << "Missing at least one edge in graph.out_edge_ids("
                       << i
                       << ")\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, InEdgeIDs)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::directed)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<std::unordered_set<size_t>> in_edges(7);

        // For each (u, v) with EdgeID indx, do (in_edges[v] UNION indx)
        size_t indx = 0;
        for (const auto & e : edges)
        {
            in_edges[e.to.value].insert(indx);
            indx++;
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each edge is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<size_t> seen;

            for (const auto & e : graph.in_edge_ids(i))
            {
                bool match = in_edges[i].contains(e.value);

                if (!match)
                {
                    FAIL() << "Unexpected edge "
                        << e.value
                        << " in graph.in_edge_ids(i)";
                }

                bool duplicate = seen.contains(e.value);

                if (duplicate)
                {
                    FAIL() << "Duplicate edge "
                        << e.value
                        << " in graph.in_edge_ids(i)";
                }

                seen.insert(e.value);
            }

            if (seen.size() != in_edges[i].size())
            {
                FAIL() << "Missing at least one edge in graph.in_edge_ids("
                       << i
                       << ")\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, IncidentEdges)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    std::vector<
        std::unordered_set<
            std::pair<size_t, size_t>,
            PairHash>> incident_descriptors(7);

    // For each (u, v) with EdgeID indx, do (incident[u] UNION indx)
    //
    // For undirected edges, do this with incident[v] as well.
    size_t indx = 0;
    for (const auto & e : edges)
    {
        if (Graph::directed)
        {
            incident_descriptors[e.from.value].insert({e.to.value, indx});
            indx++;
        }
        else
        {
            incident_descriptors[e.from.value].insert({e.to.value, indx});
            incident_descriptors[e.to.value].insert({e.from.value, indx});
            indx++;
        }
    }

    // For each vertex, check its outgoing neighbors.
    //
    // We ensure each edge view is as expected and is not a duplicate.
    for (size_t i = 0; i < graph.vertex_count(); i++)
    {
        std::unordered_set<std::pair<size_t, size_t>, PairHash> seen;

        for (const auto & ie : graph.incident_edges(i))
        {
            bool match_pairs = incident_descriptors[i].contains(
                                        {ie.neighbor.value,
                                         ie.id.value});

            if (!match_pairs)
            {
                FAIL() << "Unexpected incident edge {"
                        << ie.neighbor.value
                        << " "
                        << ie.id.value
                        << "} in graph.incident_edges("
                        << i
                        << ")";
            }

            bool duplicate_pairs = seen.contains(
                                        {ie.neighbor.value,
                                         ie.id.value});

            if (duplicate_pairs)
            {
                FAIL() << "Duplicate edge {"
                        << ie.neighbor.value
                        << " "
                        << ie.id.value
                        << "} in graph.incident_edges("
                        << i
                        << ")";
            }

            seen.insert({ie.neighbor.value,
                         ie.id.value});
        }

        if (seen.size() != incident_descriptors[i].size())
        {
            FAIL() << "Missing at least one edge in graph.incident_edges("
                    << i
                    << ")\n";
        }
    }
}

TYPED_TEST(PropertyGraphTest, OutEdgeViews)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::directed)
    {

    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<
            std::unordered_set<
                std::pair<size_t, size_t>,
                PairHash>> incident_descriptors(7);

        // For each (u, v) with EdgeID indx, do (incident[u] UNION indx)
        size_t indx = 0;
        for (const auto & e : edges)
        {
            incident_descriptors[e.from.value].insert({e.to.value, indx});
            indx++;
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each edge view is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<std::pair<size_t, size_t>, PairHash> seen;

            for (const auto & ie : graph.out_edges(i))
            {
                bool match_pairs = incident_descriptors[i].contains(
                                            {ie.neighbor.value,
                                             ie.id.value});

                if (!match_pairs)
                {
                    FAIL() << "Unexpected incident edge {"
                            << ie.neighbor.value
                            << " "
                            << ie.id.value
                            << "} in graph.out_edges("
                            << i
                            << ")";
                }

                bool duplicate = seen.contains({ie.neighbor.value,
                                                ie.id.value});

                if (duplicate)
                {
                    FAIL() << "Duplicate edge {"
                            << ie.neighbor.value
                            << " "
                            << ie.id.value
                            << "} in graph.out_edges("
                            << i
                            << ")";
                }

                seen.insert({ie.neighbor.value, ie.id.value});
            }

            if (seen.size() != incident_descriptors[i].size())
            {
                FAIL() << "Missing at least one edge in graph.out_edges("
                        << i
                        << ")\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, InEdgeViews)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::directed)
    {

    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<
            std::unordered_set<
                std::pair<size_t, size_t>,
                PairHash>> incident_descriptors(7);

        // For each (u, v) with EdgeID indx, do (incident[v] UNION indx)
        size_t indx = 0;
        for (const auto & e : edges)
        {
            incident_descriptors[e.to.value].insert({e.from.value, indx});
            indx++;
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each edge view is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<std::pair<size_t, size_t>, PairHash> seen;

            for (const auto & ie : graph.in_edges(i))
            {
                bool match_pairs = incident_descriptors[i].contains(
                                        {ie.neighbor.value,
                                         ie.id.value});

                if (!match_pairs)
                {
                    FAIL() << "Unexpected incident edge {"
                            << ie.neighbor.value
                            << " "
                            << ie.id.value
                            << "} in graph.in_edges("
                            << i
                            << ")";
                }

                bool duplicate = seen.contains({ie.neighbor.value,
                                                ie.id.value});

                if (duplicate)
                {
                    FAIL() << "Duplicate edge {"
                            << ie.neighbor.value
                            << " "
                            << ie.id.value
                            << "} in graph.in_edges("
                            << i
                            << ")";
                }

                seen.insert({ie.neighbor.value,
                             ie.id.value});
            }

            if (seen.size() != incident_descriptors[i].size())
            {
                FAIL() << "Missing at least one edge in graph.in_edges("
                        << i
                        << ")\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, GlobalEdgeIDs)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    std::unordered_set<size_t> incident(8);

    size_t indx = 0;
    for (const auto & e : edges)
    {
        incident.insert(indx);
        indx++;
    }

    // Iterate over the entire graph's edge IDs.
    std::unordered_set<size_t> seen;

    for (const auto & e : graph.edge_ids())
    {
        bool match = incident.contains(e.value);

        if (!match)
        {
            FAIL() << "Unexpected edge ID "
                    << e.value
                    << " in graph.edge_ids()\n";
        }

        bool duplicate = seen.contains(e.value);

        if (duplicate)
        {
            FAIL() << "Duplicate edge ID "
                    << e.value
                    << " in graph.edge_ids()\n";
        }

        seen.insert(e.value);
    }

    if (seen.size() != incident.size()
        || seen.size() != 8)
    {
        FAIL() << "Missing at least one edge ID in graph.edge_ids()\n";
    }
}

TYPED_TEST(PropertyGraphTest, GlobalEdges)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    std::unordered_set<std::pair<size_t, size_t>, PairHash> edge_pairs(8);
    std::unordered_set<size_t> edge_ids(8);

    size_t indx = 0;
    for (const auto & e : edges)
    {
        edge_pairs.insert({e.from.value, e.to.value});
        edge_ids.insert(indx);
        indx++;
    }

    // Iterate over the entire graph's edges.
    std::unordered_set<std::pair<size_t, size_t>, PairHash> seen_edges;
    std::unordered_set<size_t> seen_ids;

    for (const auto & e : graph.edges())
    {
        bool match = edge_pairs.contains({e.from.value, e.to.value});

        if (!match)
        {
            FAIL() << "Unexpected edge ("
                   << e.from.value
                   << " "
                   << e.to.value
                   << ") in graph.edges()\n";
        }

        bool duplicate = seen_edges.contains({e.from.value, e.to.value});

        if (duplicate)
        {
            FAIL() << "Duplicate edge ("
                   << e.from.value
                   << " "
                   << e.to.value
                   << ") in graph.edges()\n";
        }

        bool match_id = edge_ids.contains(e.id.value);

        if (!match_id)
        {
            FAIL() << "Unexpected edge ID "
                   << e.id.value
                   << " in graph.edges()\n";
        }

        bool duplicate_id = seen_ids.contains(e.id.value);

        if (duplicate_id)
        {
            FAIL() << "Duplicate edge ID "
                   << e.id.value
                   << " in graph.edges()\n";
        }

        seen_edges.insert({e.from.value, e.to.value});
        seen_ids.insert(e.id.value);
    }

    if (seen_edges.size() != edge_pairs.size()
        || seen_edges.size() != edges.size()
        || seen_ids.size() != edges.size())
    {
        FAIL() << "Missing at least one edge in graph.edges()\n";
    }
}

TYPED_TEST(PropertyGraphTest, VertexDegrees)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    std::vector<size_t> degrees(8);

    for (const auto & e : edges)
    {
        degrees[e.from.value] += 1;
        degrees[e.to.value] += 1;
    }

    // Check degree(v) for all vertices.

    for (size_t v = 0; v < graph.vertex_count(); v++)
    {
        bool eq = (degrees[v] == graph.degree(v));

        if (!eq)
        {
            FAIL() << "degree(v) = "
                   << graph.degree(v)
                   << " does not equal expected value "
                   << degrees[v]
                   << "\n";
        }
    }
}

TYPED_TEST(PropertyGraphTest, InOutDegrees)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    // Test is only defined for digraphs.
    if constexpr (!Graph::directed)
    {
        return;
    }
    else
    {

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        EXPECT_EQ(graph.edge_count(), 8);
        EXPECT_EQ(graph.vertex_count(), 7);

        std::vector<size_t> in_degrees(8);
        std::vector<size_t> out_degrees(8);

        for (const auto & e : edges)
        {
            in_degrees[e.to.value] += 1;
            out_degrees[e.from.value] += 1;
        }

        // Check in_degree(v) and out_degree(v) for all vertices.
        for (size_t v = 0; v < graph.vertex_count(); v++)
        {
            bool in_eq = (in_degrees[v] == graph.in_degree(v));

            if (!in_eq)
            {
                FAIL() << "in_degree(v) = "
                    << graph.in_degree(v)
                    << " does not equal expected value "
                    << in_degrees[v]
                    << "\n";
            }

            bool out_eq = (out_degrees[v] == graph.out_degree(v));

            if (!out_eq)
            {
                FAIL() << "out_degree(v) = "
                    << graph.out_degree(v)
                    << " does not equal expected value "
                    << out_degrees[v]
                    << "\n";
            }
        }
    }
}

TYPED_TEST(PropertyGraphTest, SelfLoopless)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    EXPECT_EQ(graph.edge_count(), 8);
    EXPECT_EQ(graph.vertex_count(), 7);

    if (graph.has_self_loops())
    {
        FAIL() << "has_self_loops() returned true "
               << "for a graph with no self loops\n";
    }

    // There are no loops, check this is true for all vertices.
    for (size_t v = 0; v < graph.vertex_count(); v++)
    {
        if (graph.has_self_loop(v))
        {
            FAIL() << "has_self_loop(v) returned true "
                    << "for a vertex with no self loops\n";
        }
    }
}

TYPED_TEST(PropertyGraphTest, SelfLoopGraph)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::self_looping_graph();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    ASSERT_EQ(graph.edge_count(), 12);
    ASSERT_EQ(graph.vertex_count(), 7);

    std::vector<bool> self_looped(graph.vertex_count());

    for (const auto & e : edges)
    {
        if (e.from == e.to)
        {
            self_looped[e.from.value] = true;
        }
    }

    if (!graph.has_self_loops())
    {
        FAIL() << "has_self_loops() returned false "
               << "for a graph with self loops\n";
    }

    // There are no loops, check this is true for all vertices.
    for (size_t v = 0; v < graph.vertex_count(); v++)
    {
        bool eq = (graph.has_self_loop(v) == self_looped[v]);
        if (!eq)
        {
            FAIL() << "has_self_loop(v) returned "
                   << graph.has_self_loop(v)
                   << " for a vertex with "
                   << (self_looped[v] ? "self loops\n" : "no self loops");
        }
    }
}

TYPED_TEST(PropertyGraphTest, VerticesRange)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    ASSERT_EQ(graph.edge_count(), 8);
    ASSERT_EQ(graph.vertex_count(), 7);

    for (const auto v : graph.vertices())
    {
        EXPECT_TRUE(v < 7);
    }
}

TYPED_TEST(PropertyGraphTest, Clear)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    ASSERT_EQ(graph.edge_count(), 8);
    ASSERT_EQ(graph.vertex_count(), 7);

    graph.clear();

    for (const auto & e : edges)
    {
        if (!graph.valid_vertex(e.from) || !graph.valid_vertex(e.to))
        {
            continue;
        }

        bool exists = graph.has_edge(e.from, e.to);

        if (exists)
        {
            FAIL() << "Unexpected edge ("
                   << e.from.value
                   << " "
                   << e.to.value
                   << ") in graph that should be cleared!";
        }
    }

    ASSERT_EQ(graph.edge_count(), 0);
    ASSERT_EQ(graph.vertex_count(), 0);
    ASSERT_FALSE(graph.valid_vertex(0));
}

TYPED_TEST(PropertyGraphTest, VertexModification)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    if constexpr (!Graph::vertices_mutable)
    {
        return;
    }
    else
    {

        ASSERT_EQ(graph.edge_count(), 8);
        ASSERT_EQ(graph.vertex_count(), 7);

        // Add some vertices
        if constexpr (!std::is_same_v<VertexData, std::monostate>)
        {
            // We have vertices with IDs 0 through 9, and
            // ID 7 maps to int 7, ID 8 maps to 8, ID 9 maps to 9.
            graph.add_vertex(7);
            graph.add_vertex(8);
            graph.add_vertex(9);
        }
        else
        {
            graph.add_vertex();
            graph.add_vertex();
            graph.add_vertex();
        }

        ASSERT_EQ(graph.vertex_count(), 10);

        graph.remove_vertex(7);

        ASSERT_EQ(graph.vertex_count(), 9);

        // Skip undirected graphs.
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            int value = 0;
            graph.extract_vertex(8, value);

            ASSERT_EQ(value, 8);
            ASSERT_EQ(graph.vertex_count(), 8);
        }
        else
        {
            graph.remove_vertex(8);
            ASSERT_EQ(graph.vertex_count(), 8);
        }

        // Count how many times we see each VertexData value.
        //
        // We expect to have {0, 0, 0, 0, 0, 0, 0, X, X, 9}
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::unordered_map<size_t, size_t> counts;

            for (const auto & v : graph.vertices())
            {
                const VertexData & data = graph.vertex_data(v);
                counts[data] += 1;
            }

            ASSERT_EQ(counts[0], 7);
            ASSERT_EQ(counts[9], 1);

            ASSERT_EQ(counts[7], 0);
            ASSERT_EQ(counts[8], 0);

            // Directly assert each element is correct as well.
            for (size_t i = 0; i < 7; i++)
            {
                ASSERT_EQ(graph.vertex_data(i), 0);
            }

            ASSERT_EQ(graph.vertex_data(9), 9);
        }

    }
}

TYPED_TEST(PropertyGraphTest, EdgeExtractErase)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    if constexpr (!Graph::edges_mutable || !Graph::has_edge_data)
    {
        return;
    }
    else
    {
        ASSERT_EQ(graph.edge_count(), 8);
        ASSERT_EQ(graph.vertex_count(), 7);

        // add some edges
        graph.add_edge(0, 2, 8);

        graph.add_edge(0, 3, 9);
        graph.add_edge(0, 4, 10);
        graph.add_edge(0, 5, 11);

        ASSERT_EQ(graph.edge_count(), 12);

        // Extract a real edge, we expect to get int(8)
        {
            std::optional<EdgeData> edata = graph.extract_edge(0, 2);
            ASSERT_TRUE(edata);
            ASSERT_EQ(*edata, 8);
        }

        // Now try to extract again, we should not get an edge back.
        {
            std::optional<EdgeData> edata = graph.extract_edge(0, 2);
            ASSERT_FALSE(edata);

            EdgeData e_data;
            bool success = graph.extract_edge(0, 2, e_data);
            ASSERT_FALSE(success);
        }

        // Extract another real edge using the ID
        {
            EdgeData e_data;
            bool success = graph.extract_edge(EdgeID{9}, e_data);
            ASSERT_TRUE(success);
        }

        // Try erasing
        {
            bool success = graph.erase_edge(0, 4);
            ASSERT_TRUE(success);

            success = graph.erase_edge(EdgeID{11});
            ASSERT_TRUE(success);
        }

        // Test that all of these edges do not exist.
        {
            for (size_t i = 2; i < 5; i++)
            {
                ASSERT_FALSE(graph.has_edge(0, i));
            }
        }

    }
}

TYPED_TEST(PropertyGraphTest, Accessors)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    const Graph cgraph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    ASSERT_EQ(graph.edge_count(), 8);
    ASSERT_EQ(graph.vertex_count(), 7);

    int N = 100;

    // non-const graphs
    {
        for (const auto & v : graph.vertices())
        {
            if constexpr (Graph::has_vertex_data)
            {
                VertexData & data = graph[v];
                ASSERT_EQ(data, 0);
                data = N;

                const VertexData & cdata = graph[v];
                ASSERT_EQ(cdata, N);
            }
        }

        for (const auto & e : graph.edge_ids())
        {
            if constexpr (Graph::has_edge_data)
            {
                EdgeData & data = graph[e];
                ASSERT_EQ(data, 0);
                data = N;

                const EdgeData & cdata = graph[e];
                ASSERT_EQ(cdata, N);
            }
        }
    }

    // const graphs
    {
        for (const auto & v : cgraph.vertices())
        {
            if constexpr (Graph::has_vertex_data)
            {
                const VertexData & cdata = cgraph[v];
                ASSERT_EQ(cdata, 0);
            }
        }

        for (const auto & e : cgraph.edge_ids())
        {
            if constexpr (Graph::has_edge_data)
            {
                const EdgeData & cdata = cgraph[e];
                ASSERT_EQ(cdata, 0);
            }
        }
    }

}

TYPED_TEST(PropertyGraphTest, Compress)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    if constexpr (!Graph::vertices_mutable)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        ASSERT_EQ(graph.edge_count(), 8);
        ASSERT_EQ(graph.vertex_count(), 7);

        // Remove some vertices and edges.

        // Right now, our arrays look like
        //
        //      -----------------
        // from |0 1 1 1 2 2 3 3|
        // to   |1 2 3 6 3 4 5 6|
        //      -----------------
        //
        // Removing vertex 6 will remove edges (1, 6) and (3, 6)
        // resulting in the arrays
        //
        //      -----------------
        // from |0 1 1 X 2 2 3 X|
        // to   |1 2 3 X 3 4 5 X|
        //      -----------------
        graph.remove_vertex(6);

        // This removal will change the arrays to be
        //
        //      -----------------
        // from |0 1 1 X 2 X 3 X|
        // to   |1 2 3 X 3 X 5 X|
        //      -----------------
        graph.remove_vertex(4);

        // Finally, after removing this edge, we get
        //
        //      -----------------
        // from |X 1 1 X 2 X 3 X|
        // to   |X 2 3 X 3 X 5 X|
        //      -----------------
        graph.remove_edge(0, 1);

        // Now, lets call compress to reduce the array size.
        //
        // We expect the following vertex mapping
        // i -> i for i < 3
        // 5 -> 4
        // empty otherwise
        std::vector<VertexID> good_vertex_map =
        {
            0, 1, 2,
            3, VertexID::empty, 4,
            VertexID::empty
        };

        // We expect the following edge mapping
        //
        // 6 -> 0
        // 1 -> 1
        // 2 -> 2
        // 4 -> 3
        // empty otherwise.
        std::vector<EdgeID> good_edge_map =
        {
            EdgeID::empty, 1, 2,
            EdgeID::empty, 3, EdgeID::empty,
            0, EdgeID::empty
        };

        auto res = graph.compress();

        // Check vertex mapping lines up.
        for (size_t i = 0; i <= 6; i++)
        {
            ASSERT_EQ(res.old_to_new_vertex[i],
                      good_vertex_map[i]);
        }

        // Check edge mapping lines up.
        for (size_t i = 0; i <= 7; i++)
        {
            ASSERT_EQ(res.old_to_new_edge[i],
                      good_edge_map[i]);
        }

        // Check each edge exists.
        std::vector<std::pair<VertexID, VertexID>> good_edges =
        {
            {1, 2}, {1, 3}, {2, 3}, {3, 5}
        };

        for (const auto & e : good_edges)
        {
            ASSERT_TRUE(graph.has_edge(
                res.old_to_new_vertex[e.first.value],
                res.old_to_new_vertex[e.second.value]
            ));
        }

        ASSERT_EQ(graph.vertex_count(), 5);
        ASSERT_EQ(graph.edge_count(), 4);
    }
}

TYPED_TEST(PropertyGraphTest, CompressVertices)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    if constexpr (!Graph::vertices_mutable)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        ASSERT_EQ(graph.edge_count(), 8);
        ASSERT_EQ(graph.vertex_count(), 7);

        // Remove some vertices and edges.

        // Right now, our arrays look like
        //
        //      -----------------
        // from |0 1 1 1 2 2 3 3|
        // to   |1 2 3 6 3 4 5 6|
        //      -----------------
        //
        // Removing vertex 6 will remove edges (1, 6) and (3, 6)
        // resulting in the arrays
        //
        //      -----------------
        // from |0 1 1 X 2 2 3 X|
        // to   |1 2 3 X 3 4 5 X|
        //      -----------------
        graph.remove_vertex(6);

        // This removal will change the arrays to be
        //
        //      -----------------
        // from |0 1 1 X 2 X 3 X|
        // to   |1 2 3 X 3 X 5 X|
        //      -----------------
        graph.remove_vertex(4);

        // Now, lets call compress to reduce the array size.
        //
        // We expect the following vertex mapping
        // i -> i for i < 3
        // 5 -> 4
        // empty otherwise
        std::vector<VertexID> good_vertex_map =
        {
            0, 1, 2,
            3, VertexID::empty, 4,
            VertexID::empty
        };

        auto res = graph.compress_vertices();

        // Check vertex mapping lines up.
        for (size_t i = 0; i <= 6; i++)
        {
            ASSERT_EQ(res.old_to_new_vertex[i],
                      good_vertex_map[i]);
        }

        // Check each edge exists.
        std::vector<std::pair<VertexID, VertexID>> good_edges =
        {
            {0, 1}, {1, 2}, {1, 3}, {2, 3}, {3, 5}
        };

        for (const auto & e : good_edges)
        {
            ASSERT_TRUE(graph.has_edge(
                res.old_to_new_vertex[e.first.value],
                res.old_to_new_vertex[e.second.value]
            ));
        }

        ASSERT_EQ(graph.vertex_count(), 5);
        ASSERT_EQ(graph.edge_count(), 5);
    }
}

TYPED_TEST(PropertyGraphTest, CompressEdges)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    if constexpr (!Graph::vertices_mutable)
    {
        return;
    }
    else
    {
        auto edges = TestFixture::make_edges();

        Graph graph = [&]() -> Graph
        {
            if constexpr(!std::is_same_v<VertexData, std::monostate>)
            {
                std::vector<VertexData> vertices(7);
                return Graph(vertices, edges);
            }
            else
            {
                return Graph(7, edges);
            }
        }();

        ASSERT_EQ(graph.edge_count(), 8);
        ASSERT_EQ(graph.vertex_count(), 7);

        // Remove some vertices and edges.

        // Right now, our arrays look like
        //
        //      -----------------
        // from |0 1 1 1 2 2 3 3|
        // to   |1 2 3 6 3 4 5 6|
        //      -----------------
        //
        // Removing vertex 6 will remove edges (1, 6) and (3, 6)
        // resulting in the arrays
        //
        //      -----------------
        // from |0 1 1 X 2 2 3 X|
        // to   |1 2 3 X 3 4 5 X|
        //      -----------------
        graph.remove_vertex(6);

        // This removal will change the arrays to be
        //
        //      -----------------
        // from |0 1 1 X 2 X 3 X|
        // to   |1 2 3 X 3 X 5 X|
        //      -----------------
        graph.remove_vertex(4);

        // Finally, after removing this edge, we get
        //
        //      -----------------
        // from |X 1 1 X 2 X 3 X|
        // to   |X 2 3 X 3 X 5 X|
        //      -----------------
        graph.remove_edge(0, 1);

        // We expect the following edge mapping
        //
        // 6 -> 0
        // 1 -> 1
        // 2 -> 2
        // 4 -> 3
        // empty otherwise.
        std::vector<EdgeID> good_edge_map =
        {
            EdgeID::empty, 1, 2,
            EdgeID::empty, 3, EdgeID::empty,
            0, EdgeID::empty
        };

        auto res = graph.compress_edges();

        // Check edge mapping lines up.
        for (size_t i = 0; i <= 7; i++)
        {
            ASSERT_EQ(res.old_to_new_edge[i],
                      good_edge_map[i]);
        }

        // Check each edge exists.
        std::vector<std::pair<VertexID, VertexID>> good_edges =
        {
            {1, 2}, {1, 3}, {2, 3}, {3, 5}
        };

        for (const auto & e : good_edges)
        {
            ASSERT_TRUE(graph.has_edge(e.first, e.second));
        }

        ASSERT_EQ(graph.vertex_count(), 5);
        ASSERT_EQ(graph.edge_count(), 4);
    }
}

TYPED_TEST(PropertyGraphTest, Transpose)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    auto edges = TestFixture::make_edges();

    Graph graph = [&]() -> Graph
    {
        if constexpr(!std::is_same_v<VertexData, std::monostate>)
        {
            std::vector<VertexData> vertices(7);
            return Graph(vertices, edges);
        }
        else
        {
            return Graph(7, edges);
        }
    }();

    ASSERT_EQ(graph.edge_count(), 8);
    ASSERT_EQ(graph.vertex_count(), 7);

    for (const auto & e : edges)
    {
        ASSERT_TRUE(graph.has_edge(e.from, e.to));
    }

    auto GT = graph.compute_transpose();

    for (const auto & e : edges)
    {
        ASSERT_TRUE(GT.has_edge(e.to, e.from));
    }

    // Assert degrees are equal.
    for (const auto v : GT.vertices())
    {
        ASSERT_EQ(GT.degree(v), graph.degree(v));

        if constexpr (graph.directed)
        {
            ASSERT_EQ(GT.in_degree(v), graph.out_degree(v));
            ASSERT_EQ(GT.out_degree(v), graph.in_degree(v));
        }
    }

    ASSERT_EQ(GT.edge_count(), graph.edge_count());
    ASSERT_EQ(GT.vertex_count(), graph.vertex_count());
}
