#include <gtest/gtest.h>

#include "test-utils.hpp"
#include "bellman-ford-helpers.hpp"
#include <libproject/graphs/bellman-ford.hpp>

using namespace libproject::graphs;

TYPED_TEST_SUITE(BellmanFordFixture, TestGraphs);

TYPED_TEST(BellmanFordFixture, BellmanFordStandard_PBT)
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

                bellman_ford(graph, source, w, workspace);

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
        }
    }
}
