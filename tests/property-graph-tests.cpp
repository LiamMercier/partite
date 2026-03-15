#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>

using namespace libproject::graphs;

TEST(PropertyGraphs, AdjacencyMatrixConstruction)
{
    // Empty construction
    {
        PropertyGraph<AdjacencyMatrix<true>> graph(5);
    }

    // Vector of tuples construction
    {
        std::vector<std::tuple<VertexID, VertexID, int>> edges
        {
            {0, 1, 5}, {1, 4, 2}, {2, 4, -5}, {1, 3, 12}, {4, 3, -18}
        };

        PropertyGraph<AdjacencyMatrix<true>, int>
            graph(5, edges);

        // Check that the graph has the correct edges.
        for (const auto & edge : edges)
        {
            bool has_edge = true;
            has_edge = graph.has_edge(std::get<0>(edge), std::get<1>(edge));

            EXPECT_TRUE(has_edge);

            if (!has_edge)
            {
                break;
            }
        }
    }

    // Vector of tuples construction
    {
        std::vector<std::tuple<VertexID, VertexID, std::pair<int, int>>> edges
        {
            {0, 1, {5, 8}}, {1, 4, {2, -12}}, {2, 4, {-5, 78}}, {1, 3, {12, -52}}, {4, 3, {-18, 1}}
        };

        PropertyGraph<AdjacencyMatrix<true>, std::pair<int, int>>
            graph(5, std::move(edges));

        // Check that the graph has the correct edges.
        for (const auto & edge : edges)
        {
            bool has_edge = true;
            has_edge = graph.has_edge(std::get<0>(edge), std::get<1>(edge));

            EXPECT_TRUE(has_edge);

            if (!has_edge)
            {
                break;
            }
        }
    }

    // Vector of pairs construction
    {
        std::vector<std::pair<VertexID, VertexID>> edges
        {
            {0, 1}, {1, 4}, {2, 4}, {1, 3}, {4, 3}
        };

        PropertyGraph<AdjacencyMatrix<false>>
            graph(5, edges);

        // Check that the graph has the correct edges.
        for (const auto & edge : edges)
        {
            // Ensure we can still flip these elements during construction.
            bool has_edge = true;
            has_edge = graph.has_edge(edge.first, edge.second);

            EXPECT_TRUE(has_edge);

            if (!has_edge)
            {
                break;
            }
        }
    }
}

TEST(PropertyGraphs, AdjacencyMatrixEdgeOperations)
{
    // No edge or vertex data example.
    {
        PropertyGraph<AdjacencyMatrix<true>> graph(5);

        // Use the example digraph
        //
        //

    }
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

        for (const auto & e : expected_edges)
        {
            EXPECT_TRUE(Gt.has_edge(e));
        }
    }
}
