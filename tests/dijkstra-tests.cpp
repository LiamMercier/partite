#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "search-graph-helpers.hpp"
#include <libproject/graphs/dijkstra.hpp>

using namespace libproject::graphs;

// Test that checks the default PQ generally works.
//
// In theory, this should be more exhaustive instead of pretending
// that the graph is a line, but we will put this PQ to use in
// our dijkstra implementation, so it will be tested in the future.
TEST(DijkstraPrimitives, DefaultPQTests)
{
    // Test that the heap works correctly.
    size_t largest_heap = 256;

    for (size_t n = 1; n < largest_heap; n++)
    {
        // Create the vertices array and distances array.
        std::vector<VertexID> vertices(n);
        std::vector<size_t> distances(n);

        for (size_t i = 0; i < n; i++)
        {
            vertices[i] = i;
            distances[i] = std::numeric_limits<size_t>::max();
        }

        size_t fake_dist = 0;

        size_t source = 0;
        distances[source] = 0;

        // Build the priority queue.
        DijkstraDefaultMinPQ<size_t> pq;
        pq.build(vertices, distances, source);

        // Create a vector of nodes for checks.
        std::vector<VertexID> popped;

        // Continuously extract the minimum and
        // set the distance to our fake incrementing distance.
        while (pq.size() > 0)
        {
            EXPECT_EQ(pq.heap_distances[0], fake_dist);

            VertexID min = pq.extract_min();
            popped.push_back(min);

            if (pq.size() == 0)
            {
                break;
            }

            fake_dist += 1;
            pq.decrease_key(min.value + 1, fake_dist);
        }

        EXPECT_EQ(popped.size(), n);

        for (size_t i = 0; i < popped.size(); i++)
        {
            EXPECT_EQ(popped[i], i);
        }
    }
}

TYPED_TEST_SUITE(SearchGraphFixture, TestStringGraphs);

TYPED_TEST(SearchGraphFixture, DijkstraStandard_PBT)
{
    size_t h = 4;
    auto graph = TestFixture::perfect_binary_tree(h);

    DijkstraWorkspace<size_t> workspace;
    StandardDijkstraVisitor visitor(workspace);

    // TODO: call dijkstra when it exists
}
