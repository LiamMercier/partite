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
        uint16_t fake_epoch{0};
        DijkstraDefaultMinPQ<size_t> pq(n, fake_epoch);

        for (size_t i = 0; i < n; i++)
        {
            pq.push(vertices[i], distances[i]);
        }

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

// TODO: parent checks.
template <typename WeightFunction, typename Numeric>
struct TestLoggingVisitor : NoOpDijkstraVisitor
{
    TestLoggingVisitor(std::vector<VertexID> & p,
                         std::vector<Numeric> & d,
                         WeightFunction w_func)
    :w(w_func),
    parent(p),
    distance(d)
    {}

    void edge_relaxed(VertexID u, VertexID v, EdgeID e)
    {
        distance[v.value] = distance[u.value]
                            + w(e);
    }

    WeightFunction w;
    std::vector<VertexID> & parent;
    std::vector<Numeric> & distance;
};

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

        auto w = [&](EdgeID e) -> Numeric
        {
            unsigned int weight = graph.edge_data(e);
            return weight;
        };

        // Test the generic function with the default NoOp visitor.
        {
            DijkstraWorkspace<Numeric> workspace(graph.vertex_count());

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

        // Test the generic function with temporary workspace.
        //
        // We need to provide a visitor, since otherwise the results
        // are impossible to verify.
        {
            std::vector<VertexID> p(graph.vertex_count());
            std::vector<Numeric> d(graph.vertex_count());

            TestLoggingVisitor visitor(p, d, w);

            dijkstra(graph, source, w, visitor);

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

                EXPECT_EQ(visitor.distance[v], dist)
                        << "Incorrect distance for vertex "
                        << v << ", expected dist = "
                        << dist << ", visitor.distance["
                        << v << "] = " << visitor.distance[v];
            }
        }
    }
}

TYPED_TEST(DijkstraFixture, DijkstraSinglePair_PBT)
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
        VertexID target{1};

        auto w = [&](EdgeID e) -> Numeric
        {
            unsigned int weight = graph.edge_data(e);
            return weight;
        };

        DijkstraWorkspace<Numeric> workspace(graph.vertex_count());

        // Test the generic function with the search visitor.
        {
            DijkstraSearchVisitor visitor(
                [target](VertexID v){ return v == target; }
            );

            dijkstra(graph, source, w, workspace, visitor);

            // Now that we have our distances, check they are correct.
            //
            // This fixture is designed so (u, v) has weight v.
            for (size_t v = 0; v < graph.vertex_count(); v++)
            {
                if (v <= target.value)
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
                // Anything on a depth two lower than the target.
                //
                // Some elements at a depth one lower can have their distance
                // set, since we might explore the same depth and not
                // find the target yet.
                else if (v >= 7)
                {
                    size_t dist = std::numeric_limits<size_t>::max();

                    EXPECT_EQ(workspace.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", workspace.distance["
                              << v << "] = " << workspace.distance[v];
                }
            }

            EXPECT_EQ(visitor.target_found, target);
        }

        // Do the same with the helper functions
        {
            workspace.next();

            dijkstra_single_pair(graph, source, target, w, workspace);

            // Now that we have our distances, check they are correct.
            //
            // This fixture is designed so (u, v) has weight v.
            for (size_t v = 0; v < graph.vertex_count(); v++)
            {
                if (v <= target.value)
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
                // Anything on a depth two lower than the target.
                //
                // Some elements at a depth one lower can have their distance
                // set, since we might explore the same depth and not
                // find the target yet.
                else if (v >= 7)
                {
                    size_t dist = std::numeric_limits<size_t>::max();

                    EXPECT_EQ(workspace.distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", workspace.distance["
                              << v << "] = " << workspace.distance[v];
                }
            }
        }

        {
            auto res = dijkstra_single_pair(graph, source, target, w);
            auto & distance = res.first;
            auto & parent = res.second;

            // Now that we have our distances, check they are correct.
            //
            // This fixture is designed so (u, v) has weight v.
            for (size_t v = 0; v < graph.vertex_count(); v++)
            {
                if (v <= target.value)
                {
                    size_t dist = 0;
                    size_t i = v;

                    while (i > 0)
                    {
                        dist += i;

                        // Parent in a perfect binary tree (i.e same as heap).
                        i = ((i - 1) / 2);
                    }

                    EXPECT_EQ(distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", distance["
                              << v << "] = " << distance[v];
                }
                // Anything on a depth two lower than the target.
                //
                // Some elements at a depth one lower can have their distance
                // set, since we might explore the same depth and not
                // find the target yet.
                else if (v >= 7)
                {
                    size_t dist = std::numeric_limits<size_t>::max();

                    EXPECT_EQ(distance[v], dist)
                              << "Incorrect distance for vertex "
                              << v << ", expected dist = "
                              << dist << ", distance["
                              << v << "] = " << distance[v];
                }
            }
        }
    }
}
