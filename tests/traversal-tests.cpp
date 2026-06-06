#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "search-graph-helpers.hpp"
#include <partite/graphs/bfs.hpp>
#include <partite/graphs/dfs.hpp>

#include <bit>

using namespace partite::graphs;

TYPED_TEST_SUITE(SearchGraphFixture, TestStringGraphs);

//
// BFS tests
//

TYPED_TEST(SearchGraphFixture, BFSTraversal_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    BFSWorkspace workspace(graph.vertex_count());
    workspace.queue.reserve(graph.vertex_count());

    {
        bfs(graph, VertexID{0}, workspace, NoOpSearchVisitor{});

        // In a binary tree, we expect every node to be seen from the root.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(workspace.seen[i], workspace.visit_id);
        }
    }
}

TYPED_TEST(SearchGraphFixture, BFSDistances_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    BFSWorkspace workspace(graph.vertex_count());
    workspace.queue.reserve(graph.vertex_count());

    std::vector<VertexID::underlying_type> distances(graph.vertex_count());
    bfs(graph, VertexID{0}, workspace, DistanceBFSVisitor{distances});

    // Lets also test our bfs_distances functions.
    auto d2 = bfs_distances(graph, 0);

    auto d3 = bfs_distances(graph, 0, workspace);

    // A perfect binary tree has 2^d nodes at depth d,
    // and each depth layer has 2^d - 1 nodes above it.
    //
    // So, any vertex at depth d is between [2^d - 1, 2^(d+1) - 2]
    // since our ID values start at zero.
    //
    // We can thus get the depth of each node by its ID
    // using the base 2 log.
    //
    // 2^d - 1 <= v <= 2^(d+1) - 2
    //
    // 2^d <= v + 1 < 2^(d+1)
    //
    // d <= log_2(v + 1) < d + 1
    //
    // Thus d = floor(log_2(v + 1)) which we can compute
    // using bit lengths since we are in base 2.
    for (size_t v = 0; v < graph.vertex_count(); v++)
    {
        size_t depth = std::bit_width(v + 1) - 1;
        EXPECT_EQ(distances[v], depth);
        EXPECT_EQ(d2[v], depth);
        EXPECT_EQ(d3[v], depth);
    }
}

TYPED_TEST(SearchGraphFixture, BFSVertexSearch_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    BFSWorkspace workspace(graph.vertex_count());
    workspace.queue.reserve(graph.vertex_count());

    // Simple (and, functionally unnecessary) predicate that just
    // looks for a VertexID during traversal. We just want to ensure
    // we are able to do early stopping.
    VertexID target = 7;
    VertexSearchVisitor visitor{
        [target](VertexID v){ return v == target; }
    };

    {
        bfs(graph, VertexID{0}, workspace, visitor);

        // Check that we found the correct vertex.
        EXPECT_EQ(visitor.target_found, target);

        // Every node from the previous level should be seen
        // and the target is the first of a next level.
        //
        // It is possible we have seen N > target if N is
        // of the same level, since graphs are not required to
        // return neighbors in any order.
        for (size_t i = 0; i <= target.value; i++)
        {
            EXPECT_EQ(workspace.seen[i], workspace.visit_id);
        }

        VertexID next_level = 15;

        for (size_t i = next_level.value; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(workspace.seen[i], 0);
        }
    }
}

//
// DFS tests
//

TYPED_TEST(SearchGraphFixture, DFSTraversal_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    // Test dfs with a workspace that explicitly tracks color.
    {
        DFSColorWorkspace workspace(graph.vertex_count());
        workspace.stack.reserve(graph.vertex_count());

        dfs(graph, VertexID{0}, workspace, NoOpSearchVisitor{});

        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(workspace.color(i), DFSColorState::Black);
        }
    }

    // Test dfs with a workspace that does not track color, but
    // a visitor that implicitly does.
    {
        DFSWorkspace workspace;
        workspace.stack.reserve(graph.vertex_count());

        std::vector<size_t> d(graph.vertex_count(),
                              TimerDFSVisitor<>::NUMERIC_MAX);
        std::vector<size_t> f(graph.vertex_count(),
                              TimerDFSVisitor<>::NUMERIC_MAX);
        TimerDFSVisitor visitor{d, f};

        dfs(graph, VertexID{0}, workspace, visitor);

        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(visitor.color(i), DFSColorState::Black);
        }

        // We assign 0, ..., 2*|V| - 1 to the nodes, and timer should
        // thus end up equal to 2*|V| after the final increment.
        EXPECT_EQ(visitor.time(), (2 * graph.vertex_count()));
    }

    // Test dfs with erroneous (memory wasted) setup.
    {
        DFSColorWorkspace workspace(graph.vertex_count());
        workspace.stack.reserve(graph.vertex_count());

        std::vector<size_t> d(graph.vertex_count(),
                              TimerDFSVisitor<>::NUMERIC_MAX);
        std::vector<size_t> f(graph.vertex_count(),
                              TimerDFSVisitor<>::NUMERIC_MAX);
        TimerDFSVisitor visitor{d, f};

        dfs(graph, VertexID{0}, workspace, visitor);

        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(visitor.color(i), DFSColorState::Black);
            EXPECT_EQ(visitor.color(i), workspace.color(i));
        }

        EXPECT_EQ(visitor.time(), (2 * graph.vertex_count()));
    }
}

TYPED_TEST(SearchGraphFixture, DFSTimestamping_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    // Test timer dfs with workspace overload.
    {
        std::vector<size_t> d(graph.vertex_count(),
                              TimerDFSVisitor<>::NUMERIC_MAX);
        std::vector<size_t> f(graph.vertex_count(),
                              TimerDFSVisitor<>::NUMERIC_MAX);
        TimerDFSVisitor visitor{d, f};

        dfs(graph, VertexID{0}, visitor);

        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(visitor.color(i), DFSColorState::Black);
        }

        // We assign 0, ..., 2*|V| - 1 to the nodes, and timer should
        // thus end up equal to 2*|V| after the final increment.
        EXPECT_EQ(visitor.time(), (2 * graph.vertex_count()));
    }

    auto NUMERIC_MAX = TimerDFSVisitor<>::NUMERIC_MAX;

    // Test built in dfs timestamp overloads
    {
        DFSWorkspace workspace;
        workspace.stack.reserve(graph.vertex_count());

        auto res = dfs_timestamps(graph, VertexID{0}, workspace);
        auto d = res.first;
        auto f = res.second;

        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_TRUE(d[i] != NUMERIC_MAX
                        && f[i] != NUMERIC_MAX)
                        << "DFSTimestamping:perfect_binary_tree "
                        << "d[" << i << "] = " << d[i]
                        << " and f[" << i << "] = " << f[i];
        }
    }

    {
        auto res = dfs_timestamps(graph, VertexID{0});
        auto d = res.first;
        auto f = res.second;

        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_TRUE(d[i] != NUMERIC_MAX
                        && f[i] != NUMERIC_MAX)
                        << "DFSTimestamping:perfect_binary_tree "
                        << "d[" << i << "] = " << d[i]
                        << " and f[" << i << "] = " << f[i];
        }
    }
}

TYPED_TEST(SearchGraphFixture, DFSVertexSearch_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    DFSColorWorkspace workspace(graph.vertex_count());
    workspace.stack.reserve(graph.vertex_count());

    // Set the target as the first node of the
    // layer at depth 3.
    VertexID target = 7;
    VertexSearchVisitor visitor{
        [target](VertexID v){ return v == target; }
    };

    {
        dfs(graph, VertexID{0}, workspace, visitor);

        // Check that we found the correct vertex.
        EXPECT_EQ(visitor.target_found, target);
    }
}
