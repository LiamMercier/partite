#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>

using namespace libproject::graphs;

TEST(GraphRepresentations, AdjacencyMatrixConstruction)
{
    {
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

    {
        AdjacencyMatrix<false> A(5);

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

}

// Test that we can properly represent an example graph.
TEST(GraphRepresentations, AdjacencyMatrixExample)
{
    // We will use the undirected graph G = (V, E) where
    // V = {1, 2, 3, 4}
    // E = {(1, 2), (2, 3), (2, 4)}
    AdjacencyMatrix<false> A(4);
    A.add_edge(1, 2, 0);
    A.add_edge(2, 3, 1);
    A.add_edge(2, 4, 2);

    // Do some lookup operations
    EXPECT_TRUE(A.has_edge(1, 2));
    EXPECT_TRUE(A.has_edge(2, 3));
    EXPECT_TRUE(A.has_edge(2, 4));

    EXPECT_EQ(A.find_edge(1, 2), 0);
    EXPECT_EQ(A.find_edge(2, 3), 1);
    EXPECT_EQ(A.find_edge(2, 4), 2);

    // Remove an edge
    A.remove_edge(1, 2);

    EXPECT_FALSE(A.has_edge(1, 2));
    EXPECT_EQ(A.find_edge(1, 2), EdgeID::empty);

    // Try adding the edge back, everything should work the same.
    A.add_edge(1, 2, 0);
    EXPECT_TRUE(A.has_edge(1, 2));
    EXPECT_EQ(A.find_edge(1, 2), 0);
}

TEST(GraphRepresentations, AdjacencyMatrixPairOverloads)
{
    // We will use the undirected graph G = (V, E) where
    // V = {1, 2, 3, 4}
    // E = {(1, 2), (2, 3), (2, 4)}
    AdjacencyMatrix<false> A(4);
    A.add_edge({1, 2}, 0);
    A.add_edge({2, 3}, 1);
    A.add_edge({2, 4}, 2);

    // Do some lookup operations
    EXPECT_TRUE(A.has_edge({1, 2}));
    EXPECT_TRUE(A.has_edge({2, 3}));
    EXPECT_TRUE(A.has_edge({2, 4}));

    EXPECT_EQ(A.find_edge({1, 2}), 0);
    EXPECT_EQ(A.find_edge({2, 3}), 1);
    EXPECT_EQ(A.find_edge({2, 4}), 2);

    // Remove an edge
    A.remove_edge({1, 2});

    EXPECT_FALSE(A.has_edge({1, 2}));
    EXPECT_EQ(A.find_edge({1, 2}), EdgeID::empty);

    // Try adding the edge back, everything should work the same.
    A.add_edge({1, 2}, 0);
    EXPECT_TRUE(A.has_edge({1, 2}));
    EXPECT_EQ(A.find_edge({1, 2}), 0);
}

TEST(GraphRepresentations, AdjacencyMatrixNeighbors)
{
    AdjacencyMatrix<false> A(5);

    A.add_edge(2, 0, 0);
    A.add_edge(2, 1, 1);
    A.add_edge(2, 2, 2);
    A.add_edge(2, 3, 3);
    A.add_edge(2, 4, 4);

    auto res = A.neighbors(2);

    for (const auto & x : res)
    {
        std::cout << x << " ";
    }
}
