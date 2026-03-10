#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>

TEST(GraphRepresentations, AdjacencyMatrixConstruction)
{
    using namespace libproject::graphs;

    AdjacencyMatrix<true> A(5);

    for (int i = 0; i < A.size(); i++)
    {
        for (int j = 0; j < A.size(); j++)
        {
            // There should be no valid edges
            EXPECT_FALSE(A.has_edge(i, j));
            EXPECT_EQ(A.find_edge(i, j), EdgeID::empty);
        }

        // Test adding self edges
        A.add_edge(i, i, 0);
    }

    // Test removing edges
    for (int i = 0; i < A.size(); i++)
    {
        // There should be a valid edge
        EXPECT_TRUE(A.has_edge(i, i));
        EXPECT_EQ(A.find_edge(i, i), 0);

        for (int j = 0; j < A.size(); j++)
        {
            A.remove_edge(i, j);
        }

        EXPECT_FALSE(A.has_edge(i, i));
        EXPECT_EQ(A.find_edge(i, i), EdgeID::empty);
    }
}
