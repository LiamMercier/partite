#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>

#include <unordered_set>
#include <tuple>

struct PairHash
{
    std::size_t operator()(const std::pair<size_t, size_t> & pair) const
    {
        return std::hash<size_t>()(pair.first)
               ^ std::hash<size_t>()(pair.second);
    }
};

// TODO: redo these tests to use typed test suite
using namespace libproject::graphs;

TEST(PropertyGraphs, AdjacencyMatrixConstruction)
{
    // Empty construction
    {
        PropertyGraph<AdjacencyMatrix<true>> graph(5);
    }

    // Vector of tuples construction
    {
        std::vector<std::tuple<VertexID, VertexID, int>> edges
        {
            {0, 1, 5}, {1, 4, 2}, {2, 4, -5}, {1, 3, 12}, {4, 3, -18}
        };

        PropertyGraph<AdjacencyMatrix<true>, int>
            graph(5, edges);

        // Check that the graph has the correct edges.
        for (const auto & edge : edges)
        {
            bool has_edge = true;
            has_edge = graph.has_edge(std::get<0>(edge), std::get<1>(edge));

            EXPECT_TRUE(has_edge);

            if (!has_edge)
            {
                break;
            }
        }
    }

    // Vector of tuples construction
    {
        std::vector<std::tuple<VertexID, VertexID, std::pair<int, int>>> edges
        {
            {0, 1, {5, 8}}, {1, 4, {2, -12}}, {2, 4, {-5, 78}}, {1, 3, {12, -52}}, {4, 3, {-18, 1}}
        };

        PropertyGraph<AdjacencyMatrix<true>, std::pair<int, int>>
            graph(5, std::move(edges));

        // Check that the graph has the correct edges.
        for (const auto & edge : edges)
        {
            bool has_edge = true;
            has_edge = graph.has_edge(std::get<0>(edge), std::get<1>(edge));

            EXPECT_TRUE(has_edge);

            if (!has_edge)
            {
                break;
            }
        }
    }

    // Vector of pairs construction
    {
        std::vector<std::pair<VertexID, VertexID>> edges
        {
            {0, 1}, {1, 4}, {2, 4}, {1, 3}, {4, 3}
        };

        PropertyGraph<AdjacencyMatrix<false>>
            graph(5, edges);

        // Check that the graph has the correct edges.
        for (const auto & edge : edges)
        {
            // Ensure we can still flip these elements during construction.
            bool has_edge = true;
            has_edge = graph.has_edge(edge.first, edge.second);

            EXPECT_TRUE(has_edge);

            if (!has_edge)
            {
                break;
            }
        }
    }

    // Example storing names on nodes, distance on edges
    {
        std::vector<std::string> vertices
        {
            "a", "b", "c", "d", "e"
        };

        std::vector<EdgeInput<int>> edges
        {
            {{0, 1}, 5}, {{0, 2}, -1}, {{0, 3}, 6},
            {{1, 4}, 3}, {{2, 3}, 8}, {{3, 4}, 4}
        };

        // We use the graph
        //
        // V = [{0, "a"}, {1, "b"}, {2, "c"}, {3, "d"}, {4, "e"}]
        // E = [{{0, 1}, 5}, {{0, 2}, -1}, {{0, 3}, 6},
        //      {{1, 4}, 3}, {{2, 3}, 8}, {{3, 4}, 4}]
        PropertyGraph<AdjacencyMatrix<false>,
                      int,
                      std::string> graph(vertices, edges);

        size_t expected_id = 0;

        // Ensure everything exists.
        for (const auto & e : edges)
        {
            EXPECT_TRUE(graph.has_edge(e.from, e.to));

            EdgeID id = graph.find_edge(e.from, e.to);
            EXPECT_EQ(id, expected_id);

            int & data_ref = graph.get_edge_data(id);
            int data = graph.get_edge_data(id);

            EXPECT_EQ(data_ref, e.data);
            EXPECT_EQ(data, e.data);

            std::string & fdata_ref = graph.get_vertex_data(e.from);
            std::string fdata = graph.get_vertex_data(e.from);

            std::string & tdata_ref = graph.get_vertex_data(e.to);
            std::string tdata = graph.get_vertex_data(e.to);

            EXPECT_EQ(fdata_ref, vertices[e.from.value]);
            EXPECT_EQ(fdata, vertices[e.from.value]);
            EXPECT_EQ(tdata_ref, vertices[e.to.value]);
            EXPECT_EQ(tdata, vertices[e.to.value]);

            expected_id++;
        }
    }
}

TEST(PropertyGraphs, AdjacencyMatrixEdgeOperations)
{
    // No edge or vertex data example.
    {
        std::vector<EdgeInput<>> edges
        {
            {0, 2}, {2, 1}, {1, 5}, {1, 3},
            {1, 4}, {3, 4}, {3, 6}, {4, 6}
        };

        PropertyGraph<AdjacencyMatrix<true>> graph(7, edges);

        auto e = edges.back();
        edges.pop_back();

        // Replace (4, 6) with (4, 2)
        graph.remove_edge(e.from, e.to);

        EXPECT_EQ(graph.edge_count(), 7);
        EXPECT_EQ(graph.vertex_count(), 7);

        graph.add_edge(4, 2);

        // Now check if each edge is still valid.
        edges.push_back({4, 2});

        size_t expected_id = 0;
        for (const auto & e : edges)
        {
            EXPECT_TRUE(graph.has_edge(e.from, e.to));
            EXPECT_EQ(graph.find_edge(e.from, e.to), expected_id);

            expected_id++;
        }

        EXPECT_FALSE(graph.has_edge(4, 6));
        EXPECT_EQ(graph.edge_count(), 8);
    }

    // Traversal example
    {
        std::vector<int> vertices
        {
            0, 0, 0, 0, 0
        };

        std::vector<EdgeInput<std::string>> edges
        {
            {{0, 1}, "a"}, {{0, 2}, "b"}, {{0, 3}, "c"},
            {{1, 4}, "d"}, {{2, 3}, "e"}, {{3, 4}, "f"}
        };

        PropertyGraph<AdjacencyMatrix<false>,
                      std::string,
                      int> graph(vertices, edges);

        // basic traversal algorithm
        std::string concat;
        VertexID id{0};
        std::vector<bool> seen(vertices.size(), false);
        seen[id.value] = true;

        // This isn't really an algorithm, just a poor DFS style test to
        // make sure we can properly use the graph.
        while (id != VertexID::empty)
        {
            bool done = false;

            for (size_t i = 0; i < vertices.size(); i++)
            {
                if (!seen[i] && graph.has_edge(id, i))
                {
                    // Find the edge data
                    EdgeID this_id = graph.find_edge(id, i);
                    std::string & this_data = graph.get_edge_data(this_id);

                    // Append this character to the edges we have used
                    concat.append(this_data);

                    // Increase the distance of i to be one more than id
                    int & id_int = graph.get_vertex_data(id);
                    int & i_int = graph.get_vertex_data(i);

                    i_int = id_int + 1;

                    // Mark as seen.
                    seen[i] = true;

                    // go to the next iteration
                    id = i;
                    done = true;
                    break;
                }
            }

            // If we get here, break.
            if (!done)
            {
                id = VertexID::empty;
            }
        }

        // Expected traversal:
        //
        // concat: "adfe"
        // vertex data: {0, 1, 4, 3, 2}
        std::vector<int> expected_dists {0, 1, 4, 3, 2};

        for (size_t i = 0; i < vertices.size(); i++)
        {
            int dist = graph.get_vertex_data(i);
            EXPECT_EQ(dist, expected_dists[i]);
        }

        EXPECT_EQ(concat, "adfe");

    }
}

TEST(PropertyGraphs, AdjacencyMatrixTranspose)
{
    // Property graph with no edge or vertex data.
    {
        PropertyGraph<AdjacencyMatrix<true>> graph(5);

        // Setup the digraph
        //
        // V = {0, 1, 2, 3}
        // E = {(0, 1), (1, 0), (0, 2), (1, 2), (3, 2)}
        graph.add_edge({0, 1});
        graph.add_edge({1, 0});
        graph.add_edge({0, 2});
        graph.add_edge({1, 2});
        graph.add_edge({3, 2});

        // The transpose will have edges
        // E = {(1, 0), (0, 1), (2, 0), (2, 1), (2, 3)}
        auto Gt = graph.compute_transpose();

        std::vector<std::pair<VertexID, VertexID>> expected_edges
        {
            {1, 0}, {0, 1}, {2, 0}, {2, 1}, {2, 3}
        };

        std::unordered_set<std::pair<size_t, size_t>,
                           PairHash> expected_edges_set;

        for (const auto & e : expected_edges)
        {
            EXPECT_TRUE(Gt.has_edge(e));
            expected_edges_set.insert({e.first.value, e.second.value});
        }

        // Check that our edge IDs map to the transposed values.
        for (const auto e_id : Gt.edge_ids())
        {
            auto [u, v] = Gt.get_edge_pair(e_id);

            bool match = expected_edges_set.contains({u.value, v.value});

            if (!match)
            {
                FAIL() << "EdgeID mapped vertices ("
                       << u.value
                       << " "
                       << v.value
                       << ") but edge does not exist!";
            }
        }
    }
}

//
// Typed test for different graph types on neighbor functions.
//

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

    // EdgeData but no VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, int>,
                int,
                std::monostate>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, int>,
                int,
                std::monostate>,

    // EdgeData and VertexData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, int, int>,
                int,
                int>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, int, int>,
                int,
                int>,

    // VertexData but no EdgeData
    GraphConfig<PropertyGraph<AdjacencyMatrix<true>, std::monostate, int>,
                std::monostate,
                int>,
    GraphConfig<PropertyGraph<AdjacencyMatrix<false>, std::monostate, int>,
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

    // Construct neighboring based on is_directed
    for (const auto & e : edges)
    {
        if (Graph::is_directed)
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
    if constexpr(!Graph::is_directed)
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
    if constexpr(!Graph::is_directed)
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

TYPED_TEST(PropertyGraphTest, NeighboringEdges)
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
        if (Graph::is_directed)
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

        for (const auto & e : graph.edge_ids(i))
        {
            bool match = incident[i].contains(e.value);

            if (!match)
            {
                FAIL() << "Unexpected edge "
                        << e.value
                        << " in graph.edge_ids("
                        << i
                        << ")";
            }

            bool duplicate = seen.contains(e.value);

            if (duplicate)
            {
                FAIL() << "Duplicate edge "
                        << e.value
                        << " in graph.edge_ids("
                        << i
                        << ")";
            }

            seen.insert(e.value);
        }

        if (seen.size() != incident[i].size())
        {
            FAIL() << "Missing at least one edge in graph.edge_ids("
                    << i
                    << ")\n";
        }
    }
}

TYPED_TEST(PropertyGraphTest, OutEdges)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::is_directed)
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

TYPED_TEST(PropertyGraphTest, InEdges)
{
    using Graph = typename TestFixture::Graph;
    using EdgeData = typename TestFixture::EdgeData;
    using VertexData = typename TestFixture::VertexData;

    // Skip undirected graphs.
    if constexpr(!Graph::is_directed)
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

TYPED_TEST(PropertyGraphTest, EdgeViews)
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

    std::vector<std::unordered_set<std::pair<size_t, size_t>, PairHash>> edge_pairs(7);
    std::vector<std::unordered_set<size_t>> edge_ids(7);

    // For each (u, v) with EdgeID indx, do (incident[u] UNION indx)
    //
    // For undirected edges, do this with incident[v] as well.
    size_t indx = 0;
    for (const auto & e : edges)
    {
        if (Graph::is_directed)
        {
            edge_pairs[e.from.value].insert({e.from.value, e.to.value});
            edge_ids[e.from.value].insert(indx);
            indx++;
        }
        else
        {
            edge_pairs[e.from.value].insert({e.from.value, e.to.value});
            edge_ids[e.from.value].insert(indx);

            edge_pairs[e.to.value].insert({e.from.value, e.to.value});
            edge_ids[e.to.value].insert(indx);
            indx++;
        }
    }

    // For each vertex, check its outgoing neighbors.
    //
    // We ensure each edge view is as expected and is not a duplicate.
    for (size_t i = 0; i < graph.vertex_count(); i++)
    {
        std::unordered_set<std::pair<size_t, size_t>, PairHash> seen_pairs;
        std::unordered_set<size_t> seen_ids;

        for (const auto & ev : graph.edges(i))
        {
            bool match_pairs = edge_pairs[i].contains({ev.from.value,
                                                       ev.to.value});

            bool match_id = edge_ids[i].contains({ev.id.value});

            if (!match_pairs || !match_id)
            {
                FAIL() << "Unexpected edge view {"
                        << ev.from.value
                        << " "
                        << ev.to.value
                        << " "
                        << ev.id.value
                        << "} in graph.edges("
                        << i
                        << ")";
            }

            bool duplicate_pairs = seen_pairs.contains({ev.from.value,
                                                        ev.to.value});

            if (duplicate_pairs)
            {
                FAIL() << "Duplicate edge {"
                        << ev.from.value
                        << " "
                        << ev.to.value
                        << "} in graph.edges("
                        << i
                        << ")";
            }

            bool duplicate_id = seen_ids.contains(ev.id.value);

            if (duplicate_id)
            {
                FAIL() << "Duplicate EdgeID "
                        << ev.id.value
                        << " in graph.edges("
                        << i
                        << ")";
            }

            seen_pairs.insert({ev.from.value,
                               ev.to.value});

            seen_ids.insert(ev.id.value);
        }

        if (seen_pairs.size() != edge_pairs[i].size())
        {
            FAIL() << "Missing at least one edge in graph.edges("
                    << i
                    << ")\n";
        }

        if (seen_ids.size() != edge_ids[i].size())
        {
            FAIL() << "Missing at least one EdgeID in graph.edges("
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
    if constexpr(!Graph::is_directed)
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

        std::vector<std::unordered_set<std::pair<size_t, size_t>, PairHash>> edge_pairs(7);
        std::vector<std::unordered_set<size_t>> edge_ids(7);

        // For each (u, v) with EdgeID indx, do (incident[u] UNION indx)
        size_t indx = 0;
        for (const auto & e : edges)
        {
            edge_pairs[e.from.value].insert({e.from.value, e.to.value});
            edge_ids[e.from.value].insert(indx);
            indx++;
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each edge view is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<std::pair<size_t, size_t>, PairHash> seen_pairs;
            std::unordered_set<size_t> seen_ids;

            for (const auto & ev : graph.out_edges(i))
            {
                bool match_pairs = edge_pairs[i].contains({ev.from.value,
                                                           ev.to.value});

                bool match_id = edge_ids[i].contains({ev.id.value});

                if (!match_pairs || !match_id)
                {
                    FAIL() << "Unexpected edge view {"
                            << ev.from.value
                            << " "
                            << ev.to.value
                            << " "
                            << ev.id.value
                            << "} in graph.out_edges("
                            << i
                            << ")";
                }

                bool duplicate_pairs = seen_pairs.contains({ev.from.value,
                                                            ev.to.value});

                if (duplicate_pairs)
                {
                    FAIL() << "Duplicate edge {"
                            << ev.from.value
                            << " "
                            << ev.to.value
                            << "} in graph.out_edges("
                            << i
                            << ")";
                }

                bool duplicate_id = seen_ids.contains(ev.id.value);

                if (duplicate_id)
                {
                    FAIL() << "Duplicate EdgeID "
                            << ev.id.value
                            << " in graph.out_edges("
                            << i
                            << ")";
                }

                seen_pairs.insert({ev.from.value,
                                ev.to.value});

                seen_ids.insert(ev.id.value);
            }

            if (seen_pairs.size() != edge_pairs[i].size())
            {
                FAIL() << "Missing at least one edge in graph.out_edges("
                        << i
                        << ")\n";
            }

            if (seen_ids.size() != edge_ids[i].size())
            {
                FAIL() << "Missing at least one EdgeID in graph.out_edges("
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
    if constexpr(!Graph::is_directed)
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

        std::vector<std::unordered_set<std::pair<size_t, size_t>, PairHash>> edge_pairs(7);
        std::vector<std::unordered_set<size_t>> edge_ids(7);

        // For each (u, v) with EdgeID indx, do (incident[v] UNION indx)
        size_t indx = 0;
        for (const auto & e : edges)
        {
            edge_pairs[e.to.value].insert({e.from.value, e.to.value});
            edge_ids[e.to.value].insert(indx);
            indx++;
        }

        // For each vertex, check its outgoing neighbors.
        //
        // We ensure each edge view is as expected and is not a duplicate.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            std::unordered_set<std::pair<size_t, size_t>, PairHash> seen_pairs;
            std::unordered_set<size_t> seen_ids;

            for (const auto & ev : graph.in_edges(i))
            {
                bool match_pairs = edge_pairs[i].contains({ev.from.value,
                                                           ev.to.value});

                bool match_id = edge_ids[i].contains({ev.id.value});

                if (!match_pairs || !match_id)
                {
                    FAIL() << "Unexpected edge view {"
                            << ev.from.value
                            << " "
                            << ev.to.value
                            << " "
                            << ev.id.value
                            << "} in graph.in_edges("
                            << i
                            << ")";
                }

                bool duplicate_pairs = seen_pairs.contains({ev.from.value,
                                                            ev.to.value});

                if (duplicate_pairs)
                {
                    FAIL() << "Duplicate edge {"
                            << ev.from.value
                            << " "
                            << ev.to.value
                            << "} in graph.in_edges("
                            << i
                            << ")";
                }

                bool duplicate_id = seen_ids.contains(ev.id.value);

                if (duplicate_id)
                {
                    FAIL() << "Duplicate EdgeID "
                            << ev.id.value
                            << " in graph.in_edges("
                            << i
                            << ")";
                }

                seen_pairs.insert({ev.from.value,
                                ev.to.value});

                seen_ids.insert(ev.id.value);
            }

            if (seen_pairs.size() != edge_pairs[i].size())
            {
                FAIL() << "Missing at least one edge in graph.in_edges("
                        << i
                        << ")\n";
            }

            if (seen_ids.size() != edge_ids[i].size())
            {
                FAIL() << "Missing at least one EdgeID in graph.in_edges("
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
    if constexpr (!Graph::is_directed)
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
}
