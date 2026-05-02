#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "dijkstra-helpers.hpp"
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

TYPED_TEST_SUITE(DijkstraFixture, TestGraphs);

TYPED_TEST(DijkstraFixture, DijkstraStandard_PBT)
{
    using EdgeData = typename TestFixture::EdgeData;

    if constexpr (!HasData<EdgeData>)
    {
        return;
    }
    else
    {
        using Numeric = size_t;

        size_t h = 4;
        auto graph = TestFixture::perfect_binary_tree(h);

        VertexID source{0};
        DijkstraWorkspace<size_t> workspace(graph.vertex_count());

        auto w = [&](EdgeID e) -> Numeric
        {
            unsigned int weight = graph.edge_data(e);
            return weight;
        };

        dijkstra(graph, source, w, workspace);

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
                i = ((i - 1) / 2);
            }

            EXPECT_EQ(workspace.distance[v], dist)
                     << "Incorrect distance for vertex "
                     << v << ", expected dist = "
                     << dist << ", workspace.distance["
                     << v << "] = " << workspace.distance[v];
        }
    }
}
