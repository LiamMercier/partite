#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "search-graph-helpers.hpp"
#include <libproject/graphs/dijkstra.hpp>

using namespace libproject::graphs;

TYPED_TEST_SUITE(SearchGraphFixture, TestStringGraphs);

TYPED_TEST(SearchGraphFixture, DijkstraStandard_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    DijkstraWorkspace<size_t> workspace;
    StandardDijkstraVisitor visitor(workspace);

    // TODO: call dijkstra when it exists
}
