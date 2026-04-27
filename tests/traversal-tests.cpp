#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "search-graph-helpers.hpp"
#include <libproject/graphs/bfs.hpp>

#include <bit>

using namespace libproject::graphs;

TYPED_TEST_SUITE(SearchGraphFixture, TestStringGraphs);

TYPED_TEST(SearchGraphFixture, BFSTraversal)
{
    auto graph = TestFixture::perfect_binary_tree(4);

    BFSWorkspace workspace(graph.vertex_count());
    workspace.queue.reserve(graph.vertex_count());
    workspace.next();

    {
        bfs(graph, VertexID{0}, workspace, NoOpBFSVisitor{});

        // In a binary tree, we expect every node to be seen from the root.
        for (size_t i = 0; i < graph.vertex_count(); i++)
        {
            EXPECT_EQ(workspace.seen[i], workspace.visit_id);
        }
    }
}

TYPED_TEST(SearchGraphFixture, BFSDistances)
{
    auto graph = TestFixture::perfect_binary_tree(4);

    BFSWorkspace workspace(graph.vertex_count());
    workspace.queue.reserve(graph.vertex_count());
    workspace.next();

    std::vector<VertexID::underlying_type> distances(graph.vertex_count());
    bfs(graph, VertexID{0}, workspace, DistanceBFSVisitor{distances});

    // Lets also test our bfs_distances functions.
    auto d2 = bfs_distances(graph, 0);

    workspace.next();
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
