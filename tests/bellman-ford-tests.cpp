#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "bellman-ford-helpers.hpp"
#include <partite/graphs/bellman-ford.hpp>

using namespace partite::graphs;

template <typename WeightFunction, typename Numeric>
struct TestLoggingVisitor : NoOpBellmanFordVisitor
{
    TestLoggingVisitor(std::vector<VertexID> & p,
                       std::vector<Numeric> & d,
                       WeightFunction w_func)
    :w(w_func),
    parent(p),
    distance(d)
    {}

    void source_vertex(VertexID u)
    {
        distance[u.value] = 0;
    }

    void edge_relaxed(VertexID u, VertexID v, EdgeID e)
    {
        distance[v.value] = distance[u.value]
                            + w(e);

        parent[v.value] = u;
    }

    WeightFunction w;
    std::vector<VertexID> & parent;
    std::vector<Numeric> & distance;
};

TYPED_TEST_SUITE(BellmanFordFixture, TestGraphs);

TYPED_TEST(BellmanFordFixture, BellmanFord_PBT)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>)
    {
        return;
    }
    else
    {
        using Numeric = size_t;
        auto INF = BellmanFordWorkspace<Numeric>::INF;

        size_t MAX_LENGTH = 10;

        for (size_t h : std::ranges::iota_view{size_t{4}, MAX_LENGTH + 1})
        {
            auto graph = TestFixture::perfect_binary_tree(h);
            VertexID source{0};

            auto w = [&](EdgeID e) -> Numeric
            {
                unsigned int weight = graph.edge_data(e);
                return weight;
            };

            // Test the generic function with the default NoOp visitor.
            {
                BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

                auto res = bellman_ford(graph, source, w, workspace);

                EXPECT_EQ(res, BellmanFordResult::Good);

                // Now that we have our distances, check they are correct.
                //
                // This fixture is designed so (u, v) has weight v.
                for (size_t v = 0; v < graph.vertex_count(); v++)
                {
                    size_t dist = 0;
                    size_t i = v;

                    while (i > 0)
                    {
                        dist += i;

                        // Parent in a perfect binary tree (i.e same as heap).
                        i = (i - 1) / 2;
                    }

                    EXPECT_EQ(workspace.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", workspace.distance["
                              << v << "] = " << workspace.distance[v];

                    size_t parent = (v - 1) / 2;

                    EXPECT_EQ(workspace.parent[v], parent)
                              << "Incorrect parent for vertex "
                              << v << ", expected parent = "
                              << parent << ", workspace.parent["
                              << v << "] = " << workspace.parent[v].value;
                }
            }

            // Try the temporary workspace helper with our logging visitor.
            {
                std::vector<VertexID> p(graph.vertex_count(), VertexID::empty);
                std::vector<Numeric> d(graph.vertex_count(), INF);

                TestLoggingVisitor visitor(p, d, w);
                auto res_v = bellman_ford(graph, source, w, visitor);

                EXPECT_EQ(res_v, BellmanFordResult::Good);

                for (size_t v = 0; v < graph.vertex_count(); v++)
                {
                    size_t dist = 0;
                    size_t i = v;

                    while (i > 0)
                    {
                        dist += i;

                        i = (i - 1) / 2;
                    }

                    EXPECT_EQ(visitor.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", visitor.distance["
                              << v << "] = " << visitor.distance[v];

                    size_t parent = (v - 1) / 2;

                    EXPECT_EQ(visitor.parent[v], parent)
                              << "Incorrect parent for vertex "
                              << v << ", expected parent = "
                              << parent << ", visitor.parent["
                              << v << "] = " << visitor.parent[v].value;
                }
            }

        }
    }
}

TYPED_TEST(BellmanFordFixture, BellmanFord_Grid)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>)
    {
        return;
    }
    else
    {
        using Numeric = size_t;
        auto INF = BellmanFordWorkspace<Numeric>::INF;
        size_t MAX_LENGTH = 10;

        for (size_t length : std::ranges::iota_view{size_t{3}, MAX_LENGTH + 1})
        {
            auto graph = TestFixture::grid_graph(length);
            VertexID source{0};

            auto w = [&](EdgeID e) -> Numeric
            {
                unsigned int weight = graph.edge_data(e);
                return weight;
            };

            {
                BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

                auto res = bellman_ford(graph, source, w, workspace);

                EXPECT_EQ(res, BellmanFordResult::Good);

                std::vector<VertexID> p(graph.vertex_count(), VertexID::empty);
                std::vector<Numeric> d(graph.vertex_count(), INF);

                TestLoggingVisitor visitor(p, d, w);

                auto res_v = bellman_ford(graph, source, w, visitor);

                EXPECT_EQ(res_v, BellmanFordResult::Good);

                for (size_t v = 0; v < graph.vertex_count(); v++)
                {
                    size_t dist = 0;

                    // The shortest path in our fixture is obtained
                    // by taking the rightmost path, then going down.
                    size_t row = v / length;
                    size_t col = v % length;
                    size_t curr = 0;

                    // Move from vertex 0 to vertex col
                    while (curr < col)
                    {
                        curr += 1;
                        dist += curr;
                    }

                    // Move from vertex col to vertex v
                    while (curr < v)
                    {
                        curr += length;
                        dist += curr;
                    }

                    // Ensure no issue with test traversal.
                    ASSERT_EQ(curr, v);

                    EXPECT_EQ(workspace.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", workspace.distance["
                              << v << "] = " << workspace.distance[v];

                    EXPECT_EQ(visitor.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", visitor.distance["
                              << v << "] = " << visitor.distance[v];

                    size_t parent;

                    // If already at the top of the grid, go left
                    if (row == 0)
                    {
                        parent = v - 1;
                    }
                    // Otherwise, move up.
                    else
                    {
                        parent = v - length;
                    }

                    EXPECT_EQ(workspace.parent[v], parent)
                              << "Incorrect parent for vertex "
                              << v << ", expected parent = "
                              << parent << ", workspace.parent["
                              << v << "] = " << workspace.parent[v].value;

                    EXPECT_EQ(visitor.parent[v], parent)
                              << "Incorrect parent for vertex "
                              << v << ", expected parent = "
                              << parent << ", visitor.parent["
                              << v << "] = " << visitor.parent[v].value;
                }
            }
        }
    }
}

TYPED_TEST(BellmanFordFixture, BellmanFord_Clusters)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>)
    {
        return;
    }
    else
    {
        using Numeric = size_t;
        using id_type = VertexID::underlying_type;

        auto INF = BellmanFordWorkspace<Numeric>::INF;

        size_t MAX_N = 25;
        size_t MAX_k = 5;

        for (size_t k : std::ranges::iota_view{size_t{2}, MAX_k + 1})
        {
            for (size_t N : std::ranges::iota_view{size_t{k}, MAX_N + 1})
            {
                auto graph = TestFixture::cluster_graph(N, k);

                auto w = [&](EdgeID e) -> Numeric
                {
                    unsigned int weight = graph.edge_data(e);
                    return weight;
                };

                BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

                size_t cluster_size = N / k;
                size_t cluster_extra = N % k;
                id_type curr_vertex_id = 0;

                for (size_t c = 0; c < k; c++)
                {
                    VertexID source{curr_vertex_id};

                    curr_vertex_id += (cluster_size - 1);

                    if (cluster_extra > 0)
                    {
                        curr_vertex_id++;
                        cluster_extra--;
                    }

                    curr_vertex_id++;

                    // Default NoOpVisitor setup.
                    auto res = bellman_ford(graph, source, w, workspace);

                    EXPECT_EQ(res, BellmanFordResult::Good);

                    // Custom logging visitor setup.
                    std::vector<VertexID> p(graph.vertex_count(),
                                            VertexID::empty);
                    std::vector<Numeric> d(graph.vertex_count(), INF);
                    TestLoggingVisitor visitor(p, d, w);

                    auto res_v = bellman_ford(graph, source, w, visitor);

                    EXPECT_EQ(res_v, BellmanFordResult::Good);

                    // This fixture is designed so (u, v) has weight u.
                    for (id_type v = 0; v < graph.vertex_count(); v++)
                    {
                        // If source, equals zero.
                        if (v == source.value)
                        {
                            EXPECT_EQ(workspace.distance[v], 0)
                                      << "Incorrect distance for vertex "
                                      << v << ", expected dist = "
                                      << 0 << ", workspace.distance["
                                      << v << "] = " << workspace.distance[v];

                            EXPECT_EQ(workspace.parent[v], VertexID::empty)
                                      << "Incorrect parent for vertex "
                                      << v
                                      << ", expected v_parent = "
                                      << VertexID::empty
                                      << ", workspace.parent["
                                      << v << "] = "
                                      << workspace.parent[v].value;

                            EXPECT_EQ(visitor.distance[v], 0)
                                      << "Incorrect distance for vertex "
                                      << v << ", expected dist = "
                                      << 0 << ", visitor.distance["
                                      << v << "] = " << visitor.distance[v];

                            EXPECT_EQ(visitor.parent[v], VertexID::empty)
                                      << "Incorrect parent for vertex "
                                      << v
                                      << ", expected v_parent = "
                                      << VertexID::empty
                                      << ", visitor.parent["
                                      << v << "] = "
                                      << visitor.parent[v].value;
                        }
                        // If in cluster, it equals the source.
                        else if (v >= source.value && v < curr_vertex_id)
                        {
                            EXPECT_EQ(workspace.distance[v], source.value)
                                      << "Incorrect distance for vertex "
                                      << v << ", expected dist = "
                                      << source.value << ", workspace.distance["
                                      << v << "] = " << workspace.distance[v];

                            EXPECT_EQ(workspace.parent[v], source)
                                      << "Incorrect parent for vertex "
                                      << v
                                      << ", expected v_parent = "
                                      << source.value << ", workspace.parent["
                                      << v << "] = "
                                      << workspace.parent[v].value;

                            EXPECT_EQ(visitor.distance[v], source.value)
                                      << "Incorrect distance for vertex "
                                      << v << ", expected dist = "
                                      << source.value << ", visitor.distance["
                                      << v << "] = " << visitor.distance[v];

                            EXPECT_EQ(visitor.parent[v], source)
                                      << "Incorrect parent for vertex "
                                      << v
                                      << ", expected v_parent = "
                                      << source.value << ", visitor.parent["
                                      << v << "] = "
                                      << visitor.parent[v].value;
                        }
                        else
                        {
                            EXPECT_EQ(workspace.distance[v], INF)
                                      << "Incorrect distance for vertex "
                                      << v << ", expected dist = "
                                      << INF << ", workspace.distance["
                                      << v << "] = " << workspace.distance[v];

                            EXPECT_EQ(workspace.parent[v], VertexID::empty)
                                      << "Incorrect parent for vertex "
                                      << v
                                      << ", expected v_parent = "
                                      << VertexID::empty
                                      << ", workspace.parent["
                                      << v << "] = "
                                      << workspace.parent[v].value;

                            EXPECT_EQ(visitor.distance[v], INF)
                                      << "Incorrect distance for vertex "
                                      << v << ", expected dist = "
                                      << INF << ", visitor.distance["
                                      << v << "] = " << visitor.distance[v];

                            EXPECT_EQ(visitor.parent[v], VertexID::empty)
                                      << "Incorrect parent for vertex "
                                      << v
                                      << ", expected v_parent = "
                                      << VertexID::empty
                                      << ", visitor.parent["
                                      << v << "] = "
                                      << visitor.parent[v].value;
                        }
                    }
                }

            }
        }
    }
}

TYPED_TEST(BellmanFordFixture, BellmanFord_GreedyTrap)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>)
    {
        return;
    }
    else
    {
        using Numeric = size_t;
        auto INF = BellmanFordWorkspace<Numeric>::INF;
        size_t MAX_LENGTH = 100;

        for (size_t p1_hops : std::ranges::iota_view{size_t{4}, MAX_LENGTH + 1})
        {
            auto graph = TestFixture::greedy_trap(p1_hops);
            VertexID source{0};
            VertexID target{static_cast<VertexID::underlying_type>(p1_hops)};

            auto w = [&](EdgeID e) -> Numeric
            {
                unsigned int weight = graph.edge_data(e);
                return weight;
            };

            BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

            auto res = bellman_ford(graph, source, w, workspace);

            EXPECT_EQ(res, BellmanFordResult::Good);

            // Check the target has the correct path length, which
            // is the long path each with weight 1.
            EXPECT_EQ(workspace.distance[target.value], p1_hops)
                      << "Incorrect distance for vertex "
                      << target.value << ", expected dist = "
                      << p1_hops << ", workspace.distance["
                      << target.value << "] = "
                      << workspace.distance[target.value];

            EXPECT_EQ(workspace.parent[target.value], target.value - 1)
                      << "Incorrect parent for vertex "
                      << target.value << ", expected parent = "
                      << target.value - 1 << ", workspace.parent["
                      << target.value << "] = "
                      << workspace.parent[target.value].value;

            std::vector<VertexID> p(graph.vertex_count(), VertexID::empty);
            std::vector<Numeric> d(graph.vertex_count(), INF);

            TestLoggingVisitor visitor(p, d, w);

            auto res_v = bellman_ford(graph, source, w, visitor);

            EXPECT_EQ(res_v, BellmanFordResult::Good);

            EXPECT_EQ(visitor.distance[target.value], p1_hops)
                      << "Incorrect distance for vertex "
                      << target.value << ", expected dist = "
                      << p1_hops << ", visitor.distance["
                      << target.value << "] = "
                      << visitor.distance[target.value];

            EXPECT_EQ(visitor.parent[target.value], target.value - 1)
                      << "Incorrect parent for vertex "
                      << target.value << ", expected parent = "
                      << target.value - 1 << ", visitor.parent["
                      << target.value << "] = "
                      << visitor.parent[target.value].value;
        }
    }
}

TYPED_TEST(BellmanFordFixture, BellmanFord_ZeroCycleGraph)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>)
    {
        return;
    }
    else
    {
        using Numeric = size_t;
        auto INF = BellmanFordWorkspace<Numeric>::INF;
        size_t MAX_LENGTH = 100;

        for (size_t path_hops : std::ranges::iota_view{size_t{1},
                                                       MAX_LENGTH + 1})
        {
            auto graph = TestFixture::zero_cycle_graph(path_hops);
            VertexID source{0};

            auto w = [&](EdgeID e) -> Numeric
            {
                unsigned int weight = graph.edge_data(e);
                return weight;
            };

            BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

            auto res = bellman_ford(graph, source, w, workspace);

            EXPECT_EQ(res, BellmanFordResult::Good);

            size_t curr_vertex_id = path_hops;

            for (size_t v = 0; v <= path_hops; v++)
            {
                curr_vertex_id++;
                size_t cycle_first = curr_vertex_id;

                curr_vertex_id++;
                size_t cycle_second = curr_vertex_id;

                EXPECT_EQ(workspace.distance[v], v)
                          << "Incorrect distance for vertex "
                          << v << ", expected dist = "
                          << v << ", workspace.distance["
                          << v << "] = " << workspace.distance[v];

                EXPECT_EQ(workspace.distance[cycle_first], v)
                          << "Incorrect distance for vertex "
                          << cycle_first << ", expected dist = "
                          << v << ", workspace.distance["
                          << cycle_first << "] = "
                          << workspace.distance[cycle_first];

                EXPECT_EQ(workspace.distance[cycle_second], v)
                          << "Incorrect distance for vertex "
                          << cycle_second << ", expected dist = "
                          << v << ", workspace.distance["
                          << cycle_second << "] = "
                          << workspace.distance[cycle_second];

                if (v != 0)
                {
                    EXPECT_EQ(workspace.parent[v], v - 1)
                              << "Incorrect parent for vertex "
                              << v << ", expected v_parent = "
                              << v - 1
                              << ", workspace.parent["
                              << v << "] = "
                              << workspace.parent[v].value;
                }
                else
                {
                    EXPECT_EQ(workspace.parent[v], VertexID::empty)
                              << "Incorrect parent for vertex "
                              << v << ", expected v_parent = "
                              << VertexID::empty
                              << ", workspace.parent["
                              << v << "] = "
                              << workspace.parent[v].value;
                }

            }

            std::vector<VertexID> p(graph.vertex_count(), VertexID::empty);
            std::vector<Numeric> d(graph.vertex_count(), INF);

            TestLoggingVisitor visitor(p, d, w);

            auto res_v = bellman_ford(graph, source, w, visitor);

            EXPECT_EQ(res_v, BellmanFordResult::Good);

            curr_vertex_id = path_hops;

            for (size_t v = 0; v <= path_hops; v++)
            {
                curr_vertex_id++;
                size_t cycle_first = curr_vertex_id;

                curr_vertex_id++;
                size_t cycle_second = curr_vertex_id;

                EXPECT_EQ(visitor.distance[v], v)
                          << "Incorrect distance for vertex "
                          << v << ", expected dist = "
                          << v << ", visitor.distance["
                          << v << "] = " << visitor.distance[v];

                EXPECT_EQ(visitor.distance[cycle_first], v)
                          << "Incorrect distance for vertex "
                          << cycle_first << ", expected dist = "
                          << v << ", visitor.distance["
                          << cycle_first << "] = "
                          << visitor.distance[cycle_first];

                EXPECT_EQ(visitor.distance[cycle_second], v)
                          << "Incorrect distance for vertex "
                          << cycle_second << ", expected dist = "
                          << v << ", visitor.distance["
                          << cycle_second << "] = "
                          << visitor.distance[cycle_second];

                if (v != 0)
                {
                    EXPECT_EQ(visitor.parent[v], v - 1)
                              << "Incorrect parent for vertex "
                              << v << ", expected v_parent = "
                              << v - 1
                              << ", visitor.parent["
                              << v << "] = "
                              << visitor.parent[v].value;
                }
                else
                {
                    EXPECT_EQ(visitor.parent[v], VertexID::empty)
                              << "Incorrect parent for vertex "
                              << v << ", expected v_parent = "
                              << VertexID::empty
                              << ", visitor.parent["
                              << v << "] = "
                              << visitor.parent[v].value;
                }

            }
        }
    }
}

constexpr bool is_debug_build =
#ifndef NDEBUG
    true;
#else
    false;
#endif

TYPED_TEST(BellmanFordFixture, BellmanFord_NegativeCycleGraph)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData> || is_debug_build)
    {
        return;
    }
    else
    {
        using Numeric = int;
        auto INF = BellmanFordWorkspace<Numeric>::INF;
        size_t MAX_LENGTH = 25;

        for (size_t path_hops : std::ranges::iota_view{size_t{1},
                                                       MAX_LENGTH + 1})
        {
            auto graph = TestFixture::negative_cycle_graph(path_hops);
            VertexID source{0};
            VertexID target{static_cast<VertexID::underlying_type>(path_hops)};

            auto w = [&](EdgeID e) -> Numeric
            {
                int weight = graph.edge_data(e);
                return weight;
            };

            BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

            auto res = bellman_ford<BellmanFordExecutionConfig{true}>
                (graph, source, w, workspace);

            EXPECT_EQ(res, BellmanFordResult::NegativeCycle);
        }
    }
}

TYPED_TEST(BellmanFordFixture, BellmanFord_NegativeDAG)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>
                  || is_debug_build
                  || !TestFixture::Graph::directed)
    {
        return;
    }
    else
    {
        using Numeric = int;
        auto INF = BellmanFordWorkspace<Numeric>::INF;
        using id_type = VertexID::underlying_type;
        size_t MAX_LENGTH = 25;

        for (size_t depth : std::ranges::iota_view{size_t{4},
                                                   MAX_LENGTH + 1})
        {
            auto graph = TestFixture::negative_DAG(depth);
            VertexID source{0};

            auto w = [&](EdgeID e) -> Numeric
            {
                int weight = graph.edge_data(e);
                return weight;
            };

            BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

            auto res = bellman_ford(graph, source, w, workspace);

            EXPECT_EQ(res, BellmanFordResult::Good);

            EXPECT_EQ(workspace.distance[0], 0)
                      << "Incorrect distance for vertex "
                      << 0 << ", expected dist = "
                      << 0 << ", workspace.distance["
                      << 0 << "] = " << workspace.distance[0];

            // Check the first two vertices.
            EXPECT_EQ(workspace.distance[1], -1)
                      << "Incorrect distance for vertex "
                      << 1 << ", expected dist = "
                      << -1 << ", workspace.distance["
                      << 1 << "] = " << workspace.distance[1];

            EXPECT_EQ(workspace.distance[2], -1)
                      << "Incorrect distance for vertex "
                      << 2 << ", expected dist = "
                      << -1 << ", workspace.distance["
                      << 2 << "] = " << workspace.distance[2];

            id_type curr_vertex_id = 2;

            // Check each layer.
            for (size_t layer = 1; layer < depth; layer++)
            {
                id_type layer_min = curr_vertex_id + 1;
                id_type layer_max = layer_min + 3;

                for (id_type v = layer_min; v <= layer_max; v++)
                {
                    Numeric dist = -1 * (layer + 1);

                    EXPECT_EQ(workspace.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", workspace.distance["
                              << v << "] = " << workspace.distance[v];
                }

                curr_vertex_id = layer_max;
            }

            // Check the last two nodes.
            id_type layer_min = curr_vertex_id + 1;
            id_type layer_max = layer_min + 1;

            for (id_type v = layer_min; v <= layer_max; v++)
            {
                Numeric dist = -(depth + 1);

                EXPECT_EQ(workspace.distance[v], dist)
                            << "Incorrect distance for vertex "
                            << v << ", expected dist = "
                            << dist << ", workspace.distance["
                            << v << "] = " << workspace.distance[v];
            }
        }
    }
}

TYPED_TEST(BellmanFordFixture, BellmanFord_OverflowGraph)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData> || is_debug_build)
    {
        return;
    }
    else
    {
        using Numeric = int;
        auto INF = BellmanFordWorkspace<Numeric>::INF;

        {
            auto graph = TestFixture::overflow_graph();
            VertexID source{0};

            auto w = [&](EdgeID e) -> Numeric
            {
                int weight = graph.edge_data(e);
                return weight;
            };

            BellmanFordWorkspace<Numeric> workspace(graph.vertex_count());

            EXPECT_THROW(bellman_ford<BellmanFordExecutionConfig{true}>
                            (graph, source, w, workspace),
                         std::overflow_error);
        }
    }
}
