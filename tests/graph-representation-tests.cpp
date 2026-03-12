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
    // V = {0, 1, 2, 3}
    // E = {(0, 1), (1, 2), (1, 3)}
    AdjacencyMatrix<false> A(4);
    A.add_edge(0, 1, 0);
    A.add_edge(1, 2, 1);
    A.add_edge(1, 3, 2);

    // Do some lookup operations
    EXPECT_TRUE(A.has_edge(0, 1));
    EXPECT_TRUE(A.has_edge(1, 2));
    EXPECT_TRUE(A.has_edge(1, 3));

    EXPECT_EQ(A.find_edge(0, 1), 0);
    EXPECT_EQ(A.find_edge(1, 2), 1);
    EXPECT_EQ(A.find_edge(1, 3), 2);

    // Remove an edge
    A.remove_edge(0, 1);

    EXPECT_FALSE(A.has_edge(0, 1));
    EXPECT_EQ(A.find_edge(0, 1), EdgeID::empty);

    // Try adding the edge back, everything should work the same.
    A.add_edge(0, 1, 0);
    EXPECT_TRUE(A.has_edge(0, 1));
    EXPECT_EQ(A.find_edge(0, 1), 0);
}

TEST(GraphRepresentations, AdjacencyMatrixPairOverloads)
{
    // We will use the undirected graph G = (V, E) where
    // V = {0, 1, 2, 3}
    // E = {(0, 1), (1, 2), (1, 3)}
    AdjacencyMatrix<false> A(4);
    A.add_edge({0, 1}, 0);
    A.add_edge({1, 2}, 1);
    A.add_edge({1, 3}, 2);

    // Do some lookup operations
    EXPECT_TRUE(A.has_edge({0, 1}));
    EXPECT_TRUE(A.has_edge({1, 2}));
    EXPECT_TRUE(A.has_edge({1, 3}));

    EXPECT_EQ(A.find_edge({0, 1}), 0);
    EXPECT_EQ(A.find_edge({1, 2}), 1);
    EXPECT_EQ(A.find_edge({1, 3}), 2);

    // Remove an edge
    A.remove_edge({0, 1});

    EXPECT_FALSE(A.has_edge({0, 1}));
    EXPECT_EQ(A.find_edge({0, 1}), EdgeID::empty);

    // Try adding the edge back, everything should work the same.
    A.add_edge({0, 1}, 0);
    EXPECT_TRUE(A.has_edge({0, 1}));
    EXPECT_EQ(A.find_edge({0, 1}), 0);
}

TEST(GraphRepresentations, AdjacencyMatrixNeighbors)
{
    // Ensure we get neighbors back as expected.
    {
        AdjacencyMatrix<false> A(5);

        A.add_edge(2, 0, 0);
        A.add_edge(2, 1, 1);
        A.add_edge(2, 2, 2);
        A.add_edge(2, 3, 3);
        A.add_edge(2, 4, 4);

        auto res = A.neighbors(2);

        EdgeID expected_eid(0);
        for (const auto & e : res)
        {
            EXPECT_NE(e, EdgeID::empty);
            EXPECT_EQ(e, expected_eid);
            expected_eid.value += 1;
        }
    }

    {
        AdjacencyMatrix<true> A(5);

        A.add_edge(2, 0, 0);
        A.add_edge(2, 1, 1);
        A.add_edge(2, 2, 2);
        A.add_edge(2, 3, 3);
        A.add_edge(2, 4, 4);

        auto res = A.neighbors(2);

        EdgeID expected_eid(0);
        for (const auto & e : res)
        {
            EXPECT_NE(e, EdgeID::empty);
            EXPECT_EQ(e, expected_eid);
            expected_eid.value += 1;
        }
    }

    // Ensure we can call neighbors for everything and get
    // an empty edge back.

    {
        AdjacencyMatrix<false> A(5);

        for (int i = 0; i < A.size(); i++)
        {
            auto res = A.neighbors(i);

            for (const auto & e : res)
            {
                EXPECT_EQ(e, EdgeID::empty);
            }
        }
    }

    {
        AdjacencyMatrix<true> A(5);

        for (int i = 0; i < A.size(); i++)
        {
            auto res = A.neighbors(i);

            for (const auto & e : res)
            {
                EXPECT_EQ(e, EdgeID::empty);
            }
        }
    }

    // Ensure we don't return (u, v) for neighbors(v) in a digraph.
    {
        AdjacencyMatrix<true> A(5);

        A.add_edge({0, 4}, 0);
        A.add_edge({4, 1}, 1);
        A.add_edge({4, 2}, 2);
        A.add_edge({3, 4}, 3);
        A.add_edge({4, 4}, 4);

        std::vector<EdgeID> expected {
            EdgeID(1), EdgeID(2), EdgeID(4)
        };

        auto res = A.neighbors(4);
        size_t indx = 0;
        for (const auto & e : res)
        {
            if (indx > 2)
            {
                FAIL() << "A.neighbors(4) returned too many values!";
                break;
            }

            EXPECT_EQ(e, expected[indx]);
            indx += 1;
        }
    }
}

TEST(GraphRepresentations, AdjacencyMatrixDegrees)
{
    {
        // Use the directed graph G = (V, E) where
        // V = {0, 1, 2, 3, 5}
        // E = {(0, 1), (1, 2), (1, 3), (4, 1), (4, 2), (2, 4), (4, 0), (3, 0)}
        //
        // We then expect degrees to be:
        //
        //            | 0 | 1 | 2 | 3 | 4 |
        // --------------------------------
        // in_degree  | 2 | 2 | 2 | 1 | 1 |
        // out_degree | 1 | 2 | 1 | 1 | 3 |
        // degree     | 3 | 4 | 3 | 2 | 4 |

        AdjacencyMatrix<true> A(5);

        // Try to throw off the count by removing empty edges.
        //
        // This should always be guarded against.
        A.remove_edge({0, 1});
        A.remove_edge({1, 2});
        A.remove_edge({1, 3});
        A.remove_edge({4, 1});

        A.add_edge({0, 1}, 0);
        A.add_edge({1, 2}, 1);
        A.add_edge({1, 3}, 2);
        A.add_edge({4, 1}, 3);
        A.add_edge({4, 2}, 4);
        A.add_edge({2, 4}, 5);
        A.add_edge({4, 0}, 6);
        A.add_edge({3, 0}, 7);

        std::vector<size_t> expected_in_deg
            {
                2, 2, 2, 1, 1
            };

        std::vector<size_t> expected_out_deg
            {
                1, 2, 1, 1, 3
            };

        std::vector<size_t> expected_deg
            {
                3, 4, 3, 2, 4
            };

        for (size_t i = 0; i < A.vertex_count(); i++)
        {
            EXPECT_EQ(expected_in_deg[i], A.in_degree(i));
            EXPECT_EQ(expected_out_deg[i], A.out_degree(i));
            EXPECT_EQ(expected_deg[i], A.degree(i));
        }
    }

    {
        // Use the undirected graph G = (V, E) where
        // V = {0, 1, 2, 3, 5}
        // E = {(0, 1), (1, 2), (1, 3), (4, 1), (2, 4), (4, 0), (3, 0)}
        //
        // We then expect degrees to be:
        //
        //            | 0 | 1 | 2 | 3 | 4 |
        // --------------------------------
        // degree     | 3 | 4 | 2 | 2 | 3 |

        AdjacencyMatrix<false> A(5);

        // Try to throw off the count by removing empty edges.
        //
        // This should always be guarded against.
        A.remove_edge({0, 1});
        A.remove_edge({1, 2});
        A.remove_edge({1, 3});
        A.remove_edge({4, 1});

        A.add_edge({0, 1}, 0);
        A.add_edge({1, 2}, 1);
        A.add_edge({1, 3}, 2);
        A.add_edge({4, 1}, 3);
        A.add_edge({2, 4}, 4);
        A.add_edge({4, 0}, 5);
        A.add_edge({3, 0}, 6);

        std::vector<size_t> expected_deg
            {
                3, 4, 2, 2, 3
            };

        for (size_t i = 0; i < A.vertex_count(); i++)
        {
            EXPECT_EQ(expected_deg[i], A.in_degree(i));
            EXPECT_EQ(expected_deg[i], A.out_degree(i));
            EXPECT_EQ(expected_deg[i], A.degree(i));
        }
    }
}
