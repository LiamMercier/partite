#include <gtest/gtest.h>

#include <libproject/graphs/property-graph.hpp>
#include <libproject/graphs/adjacency-matrix.hpp>
#include <libproject/graphs/adjacency-list.hpp>

#include <unordered_map>
#include <unordered_set>

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

    // Ensure we can call neighbors and get no empty edges.

    {
        AdjacencyMatrix<false> A(5);

        for (int i = 0; i < A.size(); i++)
        {
            auto res = A.neighbors(i);

            for (const auto & e : res)
            {
                FAIL() << "Unexpected edge in empty AdjacencyMatrix!";
                break;
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
                FAIL() << "Unexpected edge in empty AdjacencyMatrix!";
                break;
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
                FAIL() << "A.neighbors() returned too many values!";
                break;
            }

            EXPECT_EQ(e, expected[indx]);
            indx += 1;
        }
    }
}

TEST(GraphRepresentations, AdjacencyMatrixInwardNeighbors)
{
    // Test fully connected node
    {
        AdjacencyMatrix<true> A(5);

        A.add_edge(0, 4, 0);
        A.add_edge(1, 4, 1);
        A.add_edge(2, 4, 2);
        A.add_edge(3, 4, 3);
        A.add_edge(4, 4, 4);

        auto res = A.in_neighbors(4);

        EdgeID expected_eid(0);
        for (const auto & e : res)
        {
            EXPECT_NE(e, EdgeID::empty);
            EXPECT_EQ(e, expected_eid);
            expected_eid.value += 1;
        }
    }

    // Ensure we can call neighbors on empty graph
    {
        AdjacencyMatrix<true> A(5);

        for (int i = 0; i < A.size(); i++)
        {
            auto res = A.neighbors(i);

            for (const auto & e : res)
            {
                FAIL() << "Unexpected edge in empty AdjacencyMatrix!";
                break;
            }
        }
    }

    // Ensure we don't return (v, u) for in_neighbors(v) in a digraph
    // and visa versa for out_neighbors(v)
    {
        AdjacencyMatrix<true> A(5);

        A.add_edge({4, 0}, 0);
        A.add_edge({1, 4}, 1);
        A.add_edge({2, 4}, 2);
        A.add_edge({4, 3}, 3);

        std::unordered_set<size_t> out_eids {0, 3};
        std::unordered_set<size_t> in_eids {1, 2};

        auto res = A.in_neighbors(4);
        size_t indx = 0;
        for (const auto & e : res)
        {
            if (indx > 2)
            {
                FAIL() << "A.in_neighbors(4) returned too many values!";
                break;
            }

            EXPECT_FALSE(out_eids.contains(e.value));
            EXPECT_TRUE(in_eids.contains(e.value));
        }

        auto out_res = A.out_neighbors(4);
        indx = 0;
        for (const auto & e : out_res)
        {
            if (indx > 2)
            {
                FAIL() << "A.out_neighbors(4) returned too many values!";
                break;
            }

            EXPECT_TRUE(out_eids.contains(e.value));
            EXPECT_FALSE(in_eids.contains(e.value));
        }
    }
}

TEST(GraphRepresentations, AdjacencyMatrixDegrees)
{
    {
        // Use the directed graph G = (V, E) where
        // V = {0, 1, 2, 3, 4}
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
        // V = {0, 1, 2, 3, 4}
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
            EXPECT_EQ(expected_deg[i], A.degree(i));
        }
    }
}

TEST(GraphRepresentations, AdjacencyMatrixEdges)
{
    {
        // Use the directed graph G = (V, E) where
        // V = {0, 1, 2, 3, 4, 5, 6}
        // E = {(0, 3), (0, 5), (1, 2), (1, 5), (3, 1),
        //      (3, 2), (3, 4), (3, 6), (4, 6), (5, 4)}

        AdjacencyMatrix<true> A(7);

        A.add_edge({0, 3}, 0);
        A.add_edge({0, 5}, 1);
        A.add_edge({1, 2}, 2);
        A.add_edge({1, 5}, 3);
        A.add_edge({3, 1}, 4);
        A.add_edge({3, 2}, 5);
        A.add_edge({3, 4}, 6);
        A.add_edge({3, 6}, 7);
        A.add_edge({4, 6}, 8);
        A.add_edge({5, 4}, 9);

        auto res = A.edges();
        std::unordered_map<size_t, size_t> edges
        {
            {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0},
            {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0},
            {EdgeID::empty, 0}
        };

        // Ensure we have no unexpected edges.
        for (const auto & edge : res)
        {
            EXPECT_TRUE(edges.contains(size_t(edge.value)));
        }

        // Count edges.
        for (const auto & edge : res)
        {
            size_t val = edges[edge.value];
            edges[edge.value] = val + 1;
        }

        // Ensure edges 0 through 9 occur once.
        for (size_t i = 0; i < 10; i++)
        {
            EXPECT_EQ(edges[i], 1);
        }

        // Ensure we returned no empty edges.
        EXPECT_EQ(edges[EdgeID::empty], 0);
    }

    {
        // Use the undirected graph G = (V, E) where
        // V = {0, 1, 2, 3, 4}
        // E = {(0, 3), (0, 5), (1, 2), (1, 3), (1, 5)
        //      (2, 3), (3, 4), (3, 6), (4, 5), (4, 6)}

        AdjacencyMatrix<false> A(7);

        A.add_edge({0, 3}, 0);
        A.add_edge({0, 5}, 1);
        A.add_edge({1, 2}, 2);
        A.add_edge({1, 5}, 3);
        A.add_edge({3, 1}, 4);
        A.add_edge({3, 2}, 5);
        A.add_edge({3, 4}, 6);
        A.add_edge({3, 6}, 7);
        A.add_edge({4, 6}, 8);
        A.add_edge({5, 4}, 9);

        auto res = A.edges();
        std::unordered_map<size_t, size_t> edges
        {
            {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0},
            {5, 0}, {6, 0}, {7, 0}, {8, 0}, {9, 0},
            {EdgeID::empty, 0}
        };

        // Ensure we have no unexpected edges.
        for (const auto & edge : res)
        {
            EXPECT_TRUE(edges.contains(size_t(edge.value)));
        }

        // Count edges.
        for (const auto & edge : res)
        {
            size_t val = edges[edge.value];
            edges[edge.value] = val + 1;
        }

        // Ensure edges 0 through 9 occur once.
        for (size_t i = 0; i < 10; i++)
        {
            EXPECT_EQ(edges[i], 1);
        }

        // Ensure we returned no empty edges.
        EXPECT_EQ(edges[EdgeID::empty], 0);
    }
}

TEST(GraphRepresentations, AdjacencyMatrixClear)
{
    {
        AdjacencyMatrix<true> A(7);

        A.add_edge({0, 3}, 0);
        A.add_edge({0, 5}, 1);
        A.add_edge({1, 2}, 2);
        A.add_edge({1, 5}, 3);
        A.add_edge({3, 1}, 4);
        A.add_edge({3, 2}, 5);
        A.add_edge({3, 4}, 6);
        A.add_edge({3, 6}, 7);
        A.add_edge({4, 6}, 8);
        A.add_edge({5, 4}, 9);

        A.clear();

        // Ensure all degrees are zero
        for (size_t i = 0; i < A.size(); i++)
        {
            EXPECT_EQ(A.degree(i), 0);
        }

        // Ensure each edge is empty
        for (size_t i = 0; i < A.size(); i++)
        {
            for (size_t j = 0; j < A.size(); j++)
            {
                EXPECT_FALSE(A.has_edge(i, j));
            }
        }
    }

    {
        AdjacencyMatrix<false> A(7);

        A.add_edge({0, 3}, 0);
        A.add_edge({0, 5}, 1);
        A.add_edge({1, 2}, 2);
        A.add_edge({1, 5}, 3);
        A.add_edge({3, 1}, 4);
        A.add_edge({3, 2}, 5);
        A.add_edge({3, 4}, 6);
        A.add_edge({3, 6}, 7);
        A.add_edge({4, 6}, 8);
        A.add_edge({5, 4}, 9);

        A.clear();

        // Ensure all degrees are zero
        for (size_t i = 0; i < A.size(); i++)
        {
            EXPECT_EQ(A.degree(i), 0);
        }

        // Ensure each edge is empty
        for (size_t i = 0; i < A.size(); i++)
        {
            for (size_t j = 0; j < A.size(); j++)
            {
                EXPECT_FALSE(A.has_edge(i, j));
            }
        }
    }
}

TEST(GraphRepresentations, AdjacencyMatrixLoopUtils)
{
    {
        for (size_t i = 0; i < 7; i++)
        {
            AdjacencyMatrix<true> A(7);
            A.add_edge({i, i}, 0);

            EXPECT_TRUE(A.has_self_loops());
            EXPECT_TRUE(A.has_self_loop(i));
        }

        for (size_t i = 0; i < 7; i++)
        {
            AdjacencyMatrix<false> A(7);

            for (size_t j = 0; j < i; j++)
            {
                A.add_edge({i, j}, 0);
            }

            for (size_t j = i + 1; j < 7; j++)
            {
                A.add_edge({i, j}, 0);
            }

            EXPECT_FALSE(A.has_self_loops());

            for (size_t j = 0; j < 7; j++)
            {
                EXPECT_FALSE(A.has_self_loop(j));
            }
        }
    }

    {
        for (size_t i = 0; i < 7; i++)
        {
            AdjacencyMatrix<false> A(7);
            A.add_edge({i, i}, 0);

            EXPECT_TRUE(A.has_self_loops());
            EXPECT_TRUE(A.has_self_loop(i));
        }

        for (size_t i = 0; i < 7; i++)
        {
            AdjacencyMatrix<false> A(7);

            for (size_t j = 0; j < i; j++)
            {
                A.add_edge({i, j}, 0);
            }

            for (size_t j = i + 1; j < 7; j++)
            {
                A.add_edge({i, j}, 0);
            }

            EXPECT_FALSE(A.has_self_loops());

            for (size_t j = 0; j < 7; j++)
            {
                EXPECT_FALSE(A.has_self_loop(j));
            }
        }
    }
}

TEST(GraphRepresentations, AdjacencyMatrixTranspose)
{
    {
        AdjacencyMatrix<true> A(5);

        // Setup the digraph
        //
        // V = {0, 1, 2, 3}
        // E = {(0, 1), (1, 0), (0, 2), (1, 2), (3, 2)}

        A.add_edge({0, 1}, 0);
        A.add_edge({1, 0}, 1);
        A.add_edge({0, 2}, 2);
        A.add_edge({1, 2}, 3);
        A.add_edge({3, 2}, 4);

        // The transpose will have edges
        // E = {(1, 0), (0, 1), (2, 0), (2, 1), (2, 3)}
        auto At = A.compute_transpose();

        std::vector<std::pair<VertexID, VertexID>> expected_edges
        {
            {1, 0}, {0, 1}, {2, 0}, {2, 1}, {2, 3}
        };

        size_t expected_eid = 0;

        for (const auto & e : expected_edges)
        {
            EXPECT_TRUE(At.has_edge(e));
            EXPECT_EQ(At.find_edge(e).value, expected_eid);
            expected_eid++;
        }
    }

    // Test computing the transpose adjacency matrix of
    // an undirected graph.
    //
    // This should just be a copy of the graph.
    {
        AdjacencyMatrix<false> A(5);

        A.add_edge({0, 1}, 0);
        A.add_edge({0, 2}, 1);
        A.add_edge({1, 2}, 2);
        A.add_edge({3, 2}, 3);

        auto At = A.compute_transpose();

        // Some of these are flipped randomly for robustness because
        // in an undirected graph we should be handling (a, b) == (b, a).
        std::vector<std::pair<VertexID, VertexID>> expected_edges
        {
            {1, 0}, {0, 2}, {1, 2}, {2, 3}
        };

        size_t expected_eid = 0;

        for (const auto & e : expected_edges)
        {
            EXPECT_TRUE(At.has_edge(e));
            EXPECT_EQ(At.find_edge(e).value, expected_eid);
            expected_eid++;
        }
    }


}

using TestRepresentations = ::testing::Types
<
    AdjacencyMatrix<true>,
    AdjacencyMatrix<false>,
    AdjacencyList<true>,
    AdjacencyList<false>
>;

template <typename Rep>
class RepresentationCore : public ::testing::Test
{
public:
    using Representation = Rep;

    static std::vector<std::pair<VertexID, VertexID>> get_edges()
    {
        return {
            {0, 1}, {0, 2}, {0, 3},
            {1, 4},
            {2, 1}, {2, 3}, {2, 4},
            {3, 4}
        };
    }
};

TYPED_TEST_SUITE(RepresentationCore, TestRepresentations);

TYPED_TEST(RepresentationCore, EdgeOperations)
{
    using Representation = typename TestFixture::Representation;
    auto edges = TestFixture::get_edges();

    Representation rep = [&]() -> Representation
    {
        return Representation(edges.size());
    }();

    size_t i = 0;
    for (const auto & e : edges)
    {
        rep.add_edge(e.first, e.second, i);
        i++;
    }

    for (const auto & e : edges)
    {
        ASSERT_TRUE(rep.has_edge(e.first, e.second));
    }

    {
        auto edge_pair = edges[0];
        ASSERT_EQ(rep.find_edge(edge_pair.first, edge_pair.second), 0);
    }

    auto edge_pair = edges.back();
    edges.pop_back();
    rep.remove_edge(edge_pair.first, edge_pair.second);

    for (const auto & e : edges)
    {
        ASSERT_TRUE(rep.has_edge(e.first, e.second));
    }

    ASSERT_FALSE(rep.has_edge(edge_pair.first, edge_pair.second));
    ASSERT_EQ(rep.find_edge(edge_pair.first, edge_pair.second), EdgeID::empty);
}
