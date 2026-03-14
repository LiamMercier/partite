#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>

using namespace libproject::graphs;

TEST(PropertyGraphs, Construction)
{
    PropertyGraph<AdjacencyMatrix<true>> graph(5);
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

        size_t expected_eid = 0;

        for (const auto & e : expected_edges)
        {
            EXPECT_TRUE(Gt.has_edge(e));
            //EXPECT_EQ(Gt.find_edge(e).value, expected_eid);
            expected_eid++;
        }
    }
}
