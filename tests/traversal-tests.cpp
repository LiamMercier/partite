#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "search-graph-helpers.hpp"
#include <libproject/graphs/bfs.hpp>

using namespace libproject::graphs;

TYPED_TEST_SUITE(SearchGraphFixture, TestStringGraphs);

TYPED_TEST(SearchGraphFixture, BFS)
{
    auto graph = TestFixture::perfect_binary_tree(4);

    BFSWorkspace workspace(graph.vertex_count());
    workspace.queue.reserve(graph.vertex_count());

    bfs(graph, VertexID{0}, workspace);

    for (size_t i = 0; i < workspace.seen.size(); i++)
    {
        EXPECT_EQ(workspace.seen[i], workspace.visit_id);
    }
}
